// Mission planner: executes a YAML mission file one step at a time, like a
// tape machine — each step sends one setpoint to the controller and then
// blocks in a wait loop until the controller reports it's within tolerance.
//
// The planner speaks in the sub's local terms (x/y N metres, z N metres
// depth, yaw N degrees); the controller's set_x/set_y/set_z/set_yaw convert
// these into global setpoints and handle all movement.
//
// Only four step types exist: x, y, z, yaw. No vision, no holds, no
// detections — just setpoints and a wait.
//
// Change the mission by editing the YAML and relaunching — no rebuild needed:
//   ros2 run snappy_cpp planner --ros-args -p mission_file:=/ros2_ws/src/snappy_cpp/missions/pool_test.yaml
// Check a mission file without moving the sub: add -p validate_only:=true
//
// Step types, their fields and completion conditions are documented in
// missions/testing1.yaml. The wire contract with the controller is documented
// at task_callback in controller.cpp.

#include <chrono>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "rclcpp/rclcpp.hpp"
#include "snappy_cpp/msg/controller_status.hpp"
#include "snappy_cpp/msg/task.hpp"

using namespace std::chrono_literals;
using std::placeholders::_1;

namespace {

constexpr double kDegToRad = M_PI / 180.0;

// One mission step: a flat superset of every step type's fields. Per-type
// requirements are enforced in load_mission so a bad file is rejected at
// launch, on land — not mid-mission, underwater.
struct Step {
    std::string name;
    std::string type;              // x | y | z | yaw
    double meters = 0.0;            // metres in the sub's local frame   (x, y, z)
    double yaw_deg = 0.0;           // degrees                          (yaw)
    bool absolute = false;          // true: face heading, false: turn by (yaw)

    // Filled from mission defaults unless the step overrides them
    double timeout = 30.0;
    std::string on_fail = "abort";  // skip | abort
    double xy_tolerance = 0.10;     // metres
    double z_tolerance = 0.10;      // metres
    double yaw_tolerance_deg = 10.0;
};

struct Mission {
    std::string name;
    double max_seconds = 480.0;
    Step defaults;
    std::vector<Step> steps;
};

[[noreturn]] void bad_step(const std::string & step, const std::string & why) {
    throw std::runtime_error("step '" + step + "': " + why);
}

Mission load_mission(const std::string & path) {
    YAML::Node root = YAML::LoadFile(path);
    YAML::Node m = root["mission"];
    if (!m) {
        throw std::runtime_error("missing top-level 'mission' key");
    }

    Mission mission;
    if (m["name"]) mission.name = m["name"].as<std::string>();
    if (m["max_seconds"]) mission.max_seconds = m["max_seconds"].as<double>();

    if (YAML::Node d = m["defaults"]) {
        if (d["timeout"]) mission.defaults.timeout = d["timeout"].as<double>();
        if (d["on_fail"]) mission.defaults.on_fail = d["on_fail"].as<std::string>();
        if (d["xy_tolerance"]) mission.defaults.xy_tolerance = d["xy_tolerance"].as<double>();
        if (d["z_tolerance"]) mission.defaults.z_tolerance = d["z_tolerance"].as<double>();
        if (d["yaw_tolerance_deg"]) mission.defaults.yaw_tolerance_deg = d["yaw_tolerance_deg"].as<double>();
    }

    if (!m["steps"] || !m["steps"].IsSequence() || m["steps"].size() == 0) {
        throw std::runtime_error("mission needs a non-empty 'steps' list");
    }

    for (const YAML::Node & n : m["steps"]) {
        Step s = mission.defaults;
        s.name = n["name"] ? n["name"].as<std::string>()
                           : "step " + std::to_string(mission.steps.size() + 1);
        if (!n["type"]) bad_step(s.name, "missing 'type'");
        s.type = n["type"].as<std::string>();

        if (n["timeout"]) s.timeout = n["timeout"].as<double>();
        if (n["on_fail"]) s.on_fail = n["on_fail"].as<std::string>();
        if (n["xy_tolerance"]) s.xy_tolerance = n["xy_tolerance"].as<double>();
        if (n["z_tolerance"]) s.z_tolerance = n["z_tolerance"].as<double>();
        if (n["yaw_tolerance_deg"]) s.yaw_tolerance_deg = n["yaw_tolerance_deg"].as<double>();
        if (s.on_fail != "skip" && s.on_fail != "abort") {
            bad_step(s.name, "on_fail must be skip or abort (got '" + s.on_fail + "')");
        }

        if (s.type == "x" || s.type == "y" || s.type == "z") {
            if (!n["meters"]) bad_step(s.name, s.type + " step needs 'meters'");
            s.meters = n["meters"].as<double>();
        } else if (s.type == "yaw") {
            if (!n["yaw_deg"]) bad_step(s.name, "yaw step needs 'yaw_deg'");
            s.yaw_deg = n["yaw_deg"].as<double>();
            // Explicit on purpose: 'turn by 90' and 'face 90' are very different moves
            if (!n["absolute"]) {
                bad_step(s.name, "yaw step needs 'absolute' "
                    "(true = face yaw_deg, false = turn by yaw_deg)");
            }
            s.absolute = n["absolute"].as<bool>();
        } else {
            bad_step(s.name, "unknown type '" + s.type + "' (only x, y, z, yaw allowed)");
        }

        mission.steps.push_back(s);
    }

    return mission;
}

}  // namespace

class Planner : public rclcpp::Node {
public:
    Planner() : Node("planner") {
        declare_parameter("mission_file", "");
        declare_parameter("start_delay", 10.0);
        declare_parameter("settle_ticks", 10);
        declare_parameter("validate_only", false);

        start_delay_ = get_parameter("start_delay").as_double();
        settle_ticks_ = static_cast<int>(get_parameter("settle_ticks").as_int());
        validate_only_ = get_parameter("validate_only").as_bool();

        std::string mission_file = get_parameter("mission_file").as_string();
        if (mission_file.empty()) {
            throw std::runtime_error("set the 'mission_file' parameter to a mission YAML path");
        }
        mission_ = load_mission(mission_file);

        RCLCPP_INFO(get_logger(), "Mission '%s': %zu steps, max %.0f s",
            mission_.name.c_str(), mission_.steps.size(), mission_.max_seconds);
        for (size_t i = 0; i < mission_.steps.size(); i++) {
            const Step & s = mission_.steps[i];
            RCLCPP_INFO(get_logger(), "  %2zu. %-20s %-6s timeout %5.1fs on_fail %s",
                i + 1, s.name.c_str(), s.type.c_str(), s.timeout, s.on_fail.c_str());
        }
        if (validate_only_) {
            RCLCPP_INFO(get_logger(), "Mission file OK (validate_only, not executing)");
            return;  // main() exits before spinning
        }

        task_publisher_ = create_publisher<snappy_cpp::msg::Task>("/planner/task", 10);
        status_subscription_ = create_subscription<snappy_cpp::msg::ControllerStatus>(
            "/controller/status", 10, std::bind(&Planner::status_callback, this, _1));

        boot_time_ = now();
        timer_ = create_wall_timer(100ms, std::bind(&Planner::tick, this));
        RCLCPP_INFO(get_logger(), "Planner started; arming in %.0f s", start_delay_);
    }

    bool validate_only() const { return validate_only_; }

private:
    enum class Phase { WAIT_CONTROLLER, RUNNING, DONE };

    void tick() {
        const rclcpp::Time t = now();

        switch (phase_) {
            case Phase::WAIT_CONTROLLER: {
                if ((t - boot_time_).seconds() < start_delay_) return;
                if (!have_status_) {
                    RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 5000,
                        "Waiting for /controller/status before starting the mission");
                    return;
                }
                mission_start_ = t;
                phase_ = Phase::RUNNING;
                RCLCPP_INFO(get_logger(), "Mission '%s' armed", mission_.name.c_str());
                start_step();
                return;
            }
            case Phase::RUNNING: {
                if ((t - mission_start_).seconds() > mission_.max_seconds) {
                    finish("mission clock expired");
                    return;
                }
                const Step & s = mission_.steps[idx_];
                if (step_done(s)) {
                    RCLCPP_INFO(get_logger(), "Step %zu/%zu '%s' done (%.1f s)",
                        idx_ + 1, mission_.steps.size(), s.name.c_str(),
                        (t - step_start_).seconds());
                    advance();
                } else {
                    // Debug heartbeat: which step we're waiting on and how far off we are
                    RCLCPP_INFO_THROTTLE(get_logger(), *get_clock(), 2000,
                        "Waiting on step %zu/%zu '%s' (%s): err_xy=%.2fm err_z=%.2fm "
                        "err_yaw=%.1fdeg ok_ticks=%d/%d (%.1f/%.1f s)",
                        idx_ + 1, mission_.steps.size(), s.name.c_str(), s.type.c_str(),
                        status_.err_xy, status_.err_z, status_.err_yaw / kDegToRad,
                        ok_ticks_, settle_ticks_,
                        (t - step_start_).seconds(), s.timeout);
                    if ((t - step_start_).seconds() > s.timeout) {
                        fail_step(s);
                    }
                }
                return;
            }
            case Phase::DONE:
                // Give the final kill message a moment to flush, then stop this node too.
                if ((t - done_time_).seconds() > 1.0) {
                    RCLCPP_INFO(get_logger(), "Planner exiting");
                    rclcpp::shutdown();
                }
                return;
        }
    }

    void start_step() {
        const Step & s = mission_.steps[idx_];
        step_start_ = now();
        ok_ticks_ = 0;

        if (s.type == "z") {
            RCLCPP_INFO(get_logger(), "Starting step %zu/%zu '%s': z -> %.2f m",
                idx_ + 1, mission_.steps.size(), s.name.c_str(), s.meters);
            send_task("move", "z", s.meters, true);
        } else if (s.type == "yaw") {
            RCLCPP_INFO(get_logger(), "Starting step %zu/%zu '%s': yaw %s %.1f deg",
                idx_ + 1, mission_.steps.size(), s.name.c_str(),
                s.absolute ? "to" : "by", s.yaw_deg);
            send_task("move", "yaw", s.yaw_deg * kDegToRad, s.absolute);
        } else if (s.type == "x" || s.type == "y") {
            RCLCPP_INFO(get_logger(), "Starting step %zu/%zu '%s': %s -> %.2f m",
                idx_ + 1, mission_.steps.size(), s.name.c_str(), s.type.c_str(), s.meters);
            send_task("move", s.type, s.meters, false);
        }
    }

    bool step_done(const Step & s) {
        // Only trust the errors once the controller confirms it applied our
        // latest command (seq echo), then require the error to stay inside
        // tolerance for settle_ticks consecutive ticks — i.e. wait/while-true
        // until we're within 10cm (x/y/z) or a close-enough yaw.
        if (!have_status_ || status_.seq != seq_) {
            ok_ticks_ = 0;
            return false;
        }
        bool ok = false;
        if (s.type == "z") {
            ok = std::abs(status_.err_z) < s.z_tolerance;
        } else if (s.type == "yaw") {
            ok = std::abs(status_.err_yaw) < s.yaw_tolerance_deg * kDegToRad;
        } else if (s.type == "x" || s.type == "y") {
            ok = status_.err_xy < s.xy_tolerance;
        }
        ok_ticks_ = ok ? ok_ticks_ + 1 : 0;
        return ok_ticks_ >= settle_ticks_;
    }

    void fail_step(const Step & s) {
        if (s.on_fail == "skip") {
            RCLCPP_WARN(get_logger(), "Step '%s' timed out after %.1f s — skipping",
                s.name.c_str(), s.timeout);
            advance();
        } else {
            finish("step '" + s.name + "' timed out");
        }
    }

    void advance() {
        idx_++;
        if (idx_ >= mission_.steps.size()) {
            finish("mission complete");
        } else {
            start_step();
        }
    }

    // Mission over (success, failure, or clock expiry): kill the controller so
    // it stops the motors, then shut this node down too (see Phase::DONE).
    void finish(const std::string & reason) {
        RCLCPP_INFO(get_logger(), "Ending mission (%s) — sending kill, shutting down", reason.c_str());
        send_task("move", "kill", 0.0, true);
        done_time_ = now();
        phase_ = Phase::DONE;
    }

    void send_task(const std::string & type, const std::string & direction,
                   double magnitude, bool absolute) {
        snappy_cpp::msg::Task task;
        task.type = type;
        task.direction = direction;
        task.magnitude = magnitude;
        task.absolute = absolute;
        task.overwrite = true;
        task.seq = ++seq_;
        task_publisher_->publish(task);
    }

    void status_callback(const snappy_cpp::msg::ControllerStatus & msg) {
        status_ = msg;
        have_status_ = true;
    }

    rclcpp::Publisher<snappy_cpp::msg::Task>::SharedPtr task_publisher_;
    rclcpp::Subscription<snappy_cpp::msg::ControllerStatus>::SharedPtr status_subscription_;
    rclcpp::TimerBase::SharedPtr timer_;

    Mission mission_;
    Phase phase_ = Phase::WAIT_CONTROLLER;
    size_t idx_ = 0;
    int32_t seq_ = 0;
    int ok_ticks_ = 0;
    rclcpp::Time boot_time_, mission_start_, step_start_, done_time_;

    snappy_cpp::msg::ControllerStatus status_;
    bool have_status_ = false;

    double start_delay_;
    int settle_ticks_;
    bool validate_only_;
};

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    std::shared_ptr<Planner> node;
    try {
        node = std::make_shared<Planner>();
    } catch (const std::exception & e) {
        fprintf(stderr, "Mission rejected: %s\n", e.what());
        rclcpp::shutdown();
        return 1;
    }
    if (!node->validate_only()) {
        rclcpp::spin(node);
    }
    rclcpp::shutdown();
    return 0;
}

// Mission planner: executes a YAML mission file one step at a time, like a
// tape machine — each step runs until its completion condition, its timeout,
// or the global mission clock fires.
//
// The planner speaks in the sub's local terms (x/y N metres, turn by N
// degrees, be at N metres depth); the controller's set_x/set_y/set_z/set_yaw
// convert these into global setpoints and handle all movement.
//
// Change the mission by editing the YAML and relaunching — no rebuild needed:
//   ros2 run snappy_cpp planner --ros-args -p mission_file:=/ros2_ws/src/snappy_cpp/missions/pool_test.yaml
// Check a mission file without moving the sub: add -p validate_only:=true
//
// Step types, their fields and completion conditions are documented in
// missions/pool_test.yaml. The wire contract with the controller is documented
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
#include "snappy_cpp/msg/detection_array.hpp"
#include "snappy_cpp/msg/task.hpp"

using namespace std::chrono_literals;
using std::placeholders::_1;

namespace {

constexpr double kDegToRad = M_PI / 180.0;

// One mission step: a flat superset of every step type's fields. Per-type
// requirements are enforced in load_mission so a bad file is rejected at
// launch, on land — not at step 7, underwater.
struct Step {
    std::string name;
    std::string type;              // depth | heading | x | y | hold | wait_for_detection | kill 
    double depth = 0.0;            // metres, +down                     (depth)
    double yaw_deg = 0.0;          // degrees                          (heading)
    bool absolute = false;         // true: face heading, false: turn by (heading)
    double meters = 0.0;           // metres in the sub's local frame   (x, y)
    double seconds = 0.0;          // duration                          (hold)
    std::string camera = "front";  // front = /d455, bottom = /d405    (wait_for_detection)
    std::string object;            // detection class                  (wait_for_detection)
    double min_confidence = 0.5;   //                                  (wait_for_detection)

    // Filled from mission defaults unless the step overrides them
    double timeout = 30.0;
    std::string on_fail = "abort";  // skip | abort | surface
    double depth_tolerance = 0.15;  // metres
    double yaw_tolerance_deg = 7.0;
    double xy_tolerance = 0.3;      // metres
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
        if (d["depth_tolerance"]) mission.defaults.depth_tolerance = d["depth_tolerance"].as<double>();
        if (d["yaw_tolerance_deg"]) mission.defaults.yaw_tolerance_deg = d["yaw_tolerance_deg"].as<double>();
        if (d["xy_tolerance"]) mission.defaults.xy_tolerance = d["xy_tolerance"].as<double>();
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
        if (n["depth_tolerance"]) s.depth_tolerance = n["depth_tolerance"].as<double>();
        if (n["yaw_tolerance_deg"]) s.yaw_tolerance_deg = n["yaw_tolerance_deg"].as<double>();
        if (n["xy_tolerance"]) s.xy_tolerance = n["xy_tolerance"].as<double>();
        if (s.on_fail != "skip" && s.on_fail != "abort" && s.on_fail != "surface") {
            bad_step(s.name, "on_fail must be skip, abort or surface (got '" + s.on_fail + "')");
        }

        if (s.type == "depth") {
            if (!n["depth"]) bad_step(s.name, "depth step needs 'depth' (metres, +down)");
            s.depth = n["depth"].as<double>();
        } else if (s.type == "heading") {
            if (!n["yaw_deg"]) bad_step(s.name, "heading step needs 'yaw_deg'");
            s.yaw_deg = n["yaw_deg"].as<double>();
            // Explicit on purpose: 'turn by 90' and 'face 90' are very different moves
            if (!n["absolute"]) {
                bad_step(s.name, "heading step needs 'absolute' "
                    "(true = face yaw_deg, 0 = launch heading; false = turn by yaw_deg)");
            }
            s.absolute = n["absolute"].as<bool>();
        } else if (s.type == "x" || s.type == "y") {
            if (!n["meters"]) bad_step(s.name, s.type + " step needs 'meters'");
            s.meters = n["meters"].as<double>();
        } else if (s.type == "hold") {
            if (!n["seconds"]) bad_step(s.name, "hold step needs 'seconds'");
            s.seconds = n["seconds"].as<double>();
            // A hold must never be cut short by its own timeout
            s.timeout = std::max(s.timeout, s.seconds + 5.0);
        } else if (s.type == "wait_for_detection") {
            if (!n["object"]) bad_step(s.name, "wait_for_detection step needs 'object'");
            s.object = n["object"].as<std::string>();
            if (n["camera"]) s.camera = n["camera"].as<std::string>();
            if (s.camera != "front" && s.camera != "bottom") {
                bad_step(s.name, "camera must be front or bottom (got '" + s.camera + "')");
            }
            if (n["min_confidence"]) s.min_confidence = n["min_confidence"].as<double>();
        } else if (s.type == "kill") {
            // No payload: this step is an explicit request to stop the mission.
            s.on_fail = "abort";  // ignore on_fail, always abort
        } else if (s.type == "abort") {
            // No payload: this step is an explicit request to stop the mission.
            s.on_fail = "abort";  // ignore on_fail, always abort
        } else {
            bad_step(s.name, "unknown type '" + s.type + "'");
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
        declare_parameter("detection_max_age", 1.0);
        declare_parameter("surface_depth", 0.0);
        declare_parameter("validate_only", false);

        start_delay_ = get_parameter("start_delay").as_double();
        settle_ticks_ = static_cast<int>(get_parameter("settle_ticks").as_int());
        detection_max_age_ = get_parameter("detection_max_age").as_double();
        surface_depth_ = get_parameter("surface_depth").as_double();
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
            RCLCPP_INFO(get_logger(), "  %2zu. %-20s %-20s timeout %5.1fs on_fail %s",
                i + 1, s.name.c_str(), s.type.c_str(), s.timeout, s.on_fail.c_str());
        }
        if (validate_only_) {
            RCLCPP_INFO(get_logger(), "Mission file OK (validate_only, not executing)");
            return;  // main() exits before spinning
        }

        task_publisher_ = create_publisher<snappy_cpp::msg::Task>("/planner/task", 10);
        status_subscription_ = create_subscription<snappy_cpp::msg::ControllerStatus>(
            "/controller/status", 10, std::bind(&Planner::status_callback, this, _1));
        front_subscription_ = create_subscription<snappy_cpp::msg::DetectionArray>(
            "/d455/detections", rclcpp::SensorDataQoS(),
            std::bind(&Planner::front_callback, this, _1));
        bottom_subscription_ = create_subscription<snappy_cpp::msg::DetectionArray>(
            "/d405/detections", rclcpp::SensorDataQoS(),
            std::bind(&Planner::bottom_callback, this, _1));

        boot_time_ = now();
        timer_ = create_wall_timer(100ms, std::bind(&Planner::tick, this));
        RCLCPP_INFO(get_logger(), "Planner started; arming in %.0f s", start_delay_);
    }

    bool validate_only() const { return validate_only_; }

private:
    enum class Phase { WAIT_CONTROLLER, RUNNING, SURFACING, DONE };

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
                    begin_surface("mission clock expired");
                    return;
                }
                const Step & s = mission_.steps[idx_];
                if (step_done(s)) {
                    RCLCPP_INFO(get_logger(), "Step '%s' done (%.1f s)",
                        s.name.c_str(), (t - step_start_).seconds());
                    advance();
                } else if ((t - step_start_).seconds() > s.timeout) {
                    fail_step(s);
                }
                return;
            }
            case Phase::SURFACING: {
                bool at_surface = have_status_ && status_.seq == seq_ &&
                    std::abs(status_.err_z) < mission_.defaults.depth_tolerance;
                if (at_surface || (t - surface_start_).seconds() > 30.0) {
                    RCLCPP_INFO(get_logger(), "Surfaced; mission over");
                    phase_ = Phase::DONE;
                }
                return;
            }
            case Phase::DONE:
                return;
        }
    }

    void start_step() {
        const Step & s = mission_.steps[idx_];
        step_start_ = now();
        ok_ticks_ = 0;
        RCLCPP_INFO(get_logger(), "Step %zu/%zu: '%s' (%s)",
            idx_ + 1, mission_.steps.size(), s.name.c_str(), s.type.c_str());

        if (s.type == "depth") {
            send_task("move", "z", s.depth, true);
        } else if (s.type == "heading") {
            send_task("move", "yaw", s.yaw_deg * kDegToRad, s.absolute);
        } else if (s.type == "x" || s.type == "y") {
            send_task("move", s.type, s.meters, false);
        } else if (s.type == "abort") {
            begin_surface("explicit abort step '" + s.name + "'");
            return;
        }
        // hold and wait_for_detection send nothing; they only observe
    }

    bool step_done(const Step & s) {
        if (s.type == "hold") {
            return (now() - step_start_).seconds() >= s.seconds;
        }
        if (s.type == "wait_for_detection") {
            return detection_seen(s);
        }

        // Setpoint steps: only trust the errors once the controller confirms it
        // applied our latest command (seq echo), then require the error to stay
        // inside tolerance for settle_ticks consecutive ticks.
        if (!have_status_ || status_.seq != seq_) {
            ok_ticks_ = 0;
            return false;
        }
        bool ok = false;
        if (s.type == "depth") {
            ok = std::abs(status_.err_z) < s.depth_tolerance;
        } else if (s.type == "heading") {
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
            begin_surface("step '" + s.name + "' timed out");
        }
    }

    void advance() {
        idx_++;
        if (idx_ >= mission_.steps.size()) {
            RCLCPP_INFO(get_logger(),
                "Mission complete — controller keeps holding the final setpoints");
            phase_ = Phase::DONE;
        } else {
            start_step();
        }
    }

    void begin_surface(const std::string & reason) {
        RCLCPP_ERROR(get_logger(), "Aborting mission (%s) — surfacing", reason.c_str());
        send_task("move", "stop", 0.0, false);
        send_task("move", "kill", 0.0, true);
        surface_start_ = now();
        phase_ = Phase::SURFACING;
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

    bool detection_seen(const Step & s) {
        const bool bottom = (s.camera == "bottom");
        if (bottom ? !have_bottom_ : !have_front_) return false;
        const auto & cache = bottom ? bottom_ : front_;
        const auto & stamp = bottom ? bottom_stamp_ : front_stamp_;
        if ((now() - stamp).seconds() > detection_max_age_) return false;

        for (const auto & det : cache.detections) {
            if (det.object_class == s.object && det.confidence >= s.min_confidence) {
                return true;
            }
        }
        return false;
    }

    void status_callback(const snappy_cpp::msg::ControllerStatus & msg) {
        status_ = msg;
        have_status_ = true;
    }

    void front_callback(const snappy_cpp::msg::DetectionArray & msg) {
        front_ = msg;
        front_stamp_ = now();
        have_front_ = true;
    }

    void bottom_callback(const snappy_cpp::msg::DetectionArray & msg) {
        bottom_ = msg;
        bottom_stamp_ = now();
        have_bottom_ = true;
    }

    rclcpp::Publisher<snappy_cpp::msg::Task>::SharedPtr task_publisher_;
    rclcpp::Subscription<snappy_cpp::msg::ControllerStatus>::SharedPtr status_subscription_;
    rclcpp::Subscription<snappy_cpp::msg::DetectionArray>::SharedPtr front_subscription_;
    rclcpp::Subscription<snappy_cpp::msg::DetectionArray>::SharedPtr bottom_subscription_;
    rclcpp::TimerBase::SharedPtr timer_;

    Mission mission_;
    Phase phase_ = Phase::WAIT_CONTROLLER;
    size_t idx_ = 0;
    int32_t seq_ = 0;
    int ok_ticks_ = 0;
    rclcpp::Time boot_time_, mission_start_, step_start_, surface_start_;

    snappy_cpp::msg::ControllerStatus status_;
    bool have_status_ = false;
    snappy_cpp::msg::DetectionArray front_, bottom_;
    rclcpp::Time front_stamp_, bottom_stamp_;
    bool have_front_ = false, have_bottom_ = false;

    double start_delay_, detection_max_age_, surface_depth_;
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
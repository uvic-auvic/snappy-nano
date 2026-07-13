#pragma once
// Shared context + the single Task wire contract for the competition
// behaviour tree.
//
// Ground rules:
//  1. Every Task is published with overwrite=true. The controller's queued
//     task path stalls between tasks, so the newest command must always take
//     effect immediately.
//  2. /controller/status is never consumed — it is an uncorrelated string
//     with no task id. Leaves judge their own completion from sensors
//     (depth, heading, detections).
//  3. Every motion verb the tree can emit is defined ONCE, in this file.
//     When the controller interface changes, this is the only place to touch.
//
// Wire contract (Task.msg: type / direction / magnitude / absolute / overwrite):
//   move  / z          magnitude = depth in metres, +down, absolute
//   move  / yaw        magnitude = RADIANS; absolute, or relative (the
//                      controller adds the delta to its own IMU heading)
//   drive / forward|backward|left|right|stop
//                      magnitude = thrust pct [0,100]. The controller deadman
//                      auto-stops ~3 s after the last message, so any leaf
//                      that keeps a drive RUNNING re-publishes every <= 1 s.
//   spin  / roll|pitch magnitude = body rate in deg/s, signed; 0 stops.
//                      Rolls/flips are rate commands, never Euler setpoints:
//                      pitch past 90 deg is unrepresentable in Euler angles,
//                      and a shortest-path attitude controller nets zero
//                      rotation out of a "roll to 360" setpoint.
//   track / <object_class>
//                      magnitude: 0 = off, 1 = center-ahead (front camera),
//                      2 = center-over (down camera). The visual-servo loop
//                      lives controller-side; the tree only switches it on
//                      and watches convergence. On "off" the controller must
//                      re-latch current depth/heading as its hold targets.
//   actuate / marker_left|marker_right|torpedo_left|torpedo_right|claw_open|claw_close
//                      magnitude = solenoid pulse seconds.
//
// Verbs the current controller does not parse are ignored by it — the
// contract is defined ahead so the controller side has a spec to implement.

#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "behaviortree_cpp/action_node.h"
#include "behaviortree_cpp/condition_node.h"
#include "behaviortree_cpp/bt_factory.h"
#include "rclcpp/rclcpp.hpp"
#include "snappy_cpp/msg/task.hpp"

namespace snappy::bt
{

// steady_clock for all freshness/duration checks: immune to sim-time jumps
// and wall-clock corrections mid-run.
using Clock = std::chrono::steady_clock;

inline constexpr double kDeg2Rad = M_PI / 180.0;

// Wrap an angle into (-pi, pi].
inline double wrapToPi(double a) { return std::atan2(std::sin(a), std::cos(a)); }

// How many seconds ago `t` happened — used for sensor ages and leaf timers.
inline double secondsSince(Clock::time_point t)
{
    return std::chrono::duration<double>(Clock::now() - t).count();
}

// One vision detection, reduced to what the tree needs.
struct DetectionSnapshot
{
    std::string object_class;
    double bearing_rad = 0.0;  // + = target right of image centre (linear-in-FOV approx)
    double norm_off_x = 0.0;   // bbox centre offset from image centre, in [-0.5, 0.5]
    double norm_off_y = 0.0;   // + = target below image centre
    double distance_m = 0.0;
    double confidence = 0.0;
    Clock::time_point stamp{};  // receipt time
};

struct CameraModel
{
    double hfov_rad = 90.0 * kDeg2Rad;
    double width_px = 848.0;
    double height_px = 480.0;
};

// ---------------------------------------------------------------------------
// Role → vision-class tables (RoboSub 2026 "Restore and Recovery").
// Gate divider icons: restore = compass + hammer&wrench, recovery = SOS +
// life ring. VERIFY with the vision team: "buoy" is assumed to be the
// life-ring class, and the bin-icon mapping (tools vs medical) is a best
// guess against kBottomDetectionClasses — neither is confirmed.
// ---------------------------------------------------------------------------
// The gate-divider icon classes that belong to a role ("restore"/"recovery").
inline const std::vector<std::string>& roleGateIcons(const std::string& role)
{
    static const std::vector<std::string> restore{"compass", "hammer_and_wrench"};
    static const std::vector<std::string> recovery{"sos", "buoy"};
    return role == "recovery" ? recovery : restore;
}

// The bin-lid icon classes that match a role (which bin we should drop into).
inline const std::vector<std::string>& roleBinIcons(const std::string& role)
{
    static const std::vector<std::string> restore{"nut_and_bolt", "plug"};
    static const std::vector<std::string> recovery{"bandage", "pill"};
    return role == "recovery" ? recovery : restore;
}

// The opposing role — used to find the OTHER side's icons at the gate.
inline std::string otherRole(const std::string& role)
{
    return role == "recovery" ? "restore" : "recovery";
}

// Shared state between the ROS node and every leaf. One writer per entry:
// sensor caches are written by the subscriptions, mission state by exactly
// one leaf each. Guarded by a mutex so the tree stays correct if the
// executor ever goes multi-threaded.
class BTContext
{
public:
    rclcpp::Node* node = nullptr;
    rclcpp::Publisher<snappy_cpp::msg::Task>::SharedPtr task_pub;

    // Read-only after node init.
    std::string role = "restore";             // assigned pre-run by the coin flip
    std::string default_gate_side = "left";   // fallback when vision can't classify
    double mission_time_s = 0.0;              // run clock limit; <= 0 disables it
    // When false the safety guards only WARN instead of aborting the run —
    // for bench/pool debugging where an auto-surface mid-test is confusing.
    // Competition runs leave this true.
    bool guards_enabled = true;
    CameraModel front_cam;
    CameraModel down_cam;

    struct Sample
    {
        double value;
        double age_s;
    };

    // --- sensor cache -------------------------------------------------------

    // Called by the depth_data subscription: store the newest depth reading.
    void updateDepth(double metres)
    {
        std::lock_guard lk(mu_);
        depth_ = metres;
        depth_stamp_ = Clock::now();
    }

    // Called by the IMU subscription: store the newest yaw (radians, wrapped).
    void updateHeading(double rad)
    {
        std::lock_guard lk(mu_);
        heading_ = wrapToPi(rad);
        heading_stamp_ = Clock::now();
    }

    // Called by a camera subscription: replace that camera's detection list.
    void updateDetections(const std::string& camera, std::vector<DetectionSnapshot> dets)
    {
        std::lock_guard lk(mu_);
        detections_[camera] = std::move(dets);
    }

    // Latest depth (metres, +down) and how old it is; empty until the first message.
    std::optional<Sample> depth() const
    {
        std::lock_guard lk(mu_);
        if (!depth_) return std::nullopt;
        return Sample{*depth_, secondsSince(depth_stamp_)};
    }

    // Latest yaw (radians) and how old it is; empty until the first IMU message.
    std::optional<Sample> heading() const
    {
        std::lock_guard lk(mu_);
        if (!heading_) return std::nullopt;
        return Sample{*heading_, secondsSince(heading_stamp_)};
    }

    // Detections from one camera that are recent enough, confident enough,
    // and of one of the wanted classes — what every vision leaf filters on.
    std::vector<DetectionSnapshot> freshDetections(const std::string& camera,
                                                   const std::vector<std::string>& classes,
                                                   double max_age_s, double min_confidence) const
    {
        std::lock_guard lk(mu_);
        std::vector<DetectionSnapshot> out;
        auto it = detections_.find(camera);
        if (it == detections_.end()) return out;
        for (const auto& d : it->second) {
            if (secondsSince(d.stamp) > max_age_s || d.confidence < min_confidence) continue;
            if (std::find(classes.begin(), classes.end(), d.object_class) == classes.end()) continue;
            out.push_back(d);
        }
        return out;
    }

    // Pick the highest-confidence detection from a list (empty if the list is).
    static std::optional<DetectionSnapshot> best(const std::vector<DetectionSnapshot>& dets)
    {
        auto it = std::max_element(dets.begin(), dets.end(),
                                   [](const auto& a, const auto& b) { return a.confidence < b.confidence; });
        if (it == dets.end()) return std::nullopt;
        return *it;
    }

    // --- mission state (one writer each) -------------------------------------

    // Record which side of the gate we passed ("left"/"right"); first write wins.
    void latchGateSide(const std::string& side)
    {
        std::lock_guard lk(mu_);
        if (!gate_side_) gate_side_ = side;  // written once, never re-derived
    }

    // The latched gate side, or empty if the gate task never classified one.
    std::optional<std::string> gateSide() const
    {
        std::lock_guard lk(mu_);
        return gate_side_;
    }

    // Remember the current heading under a name (e.g. "gate") so a later leg
    // can turn back to it (ReturnHome uses the reciprocal of "gate").
    void saveHeading(const std::string& key, double rad)
    {
        std::lock_guard lk(mu_);
        saved_headings_[key] = wrapToPi(rad);
    }

    // Look up a heading saved earlier; empty if that name was never saved.
    std::optional<double> savedHeading(const std::string& key) const
    {
        std::lock_guard lk(mu_);
        auto it = saved_headings_.find(key);
        if (it == saved_headings_.end()) return std::nullopt;
        return it->second;
    }

    // Fired-latches: a retried branch must never double-fire an actuator
    // with a consumable budget (2 markers, 2 torpedoes).
    // Has this one-shot actuator (e.g. "marker_left") already fired this run?
    bool alreadyFired(const std::string& key) const
    {
        std::lock_guard lk(mu_);
        return fired_.count(key) > 0;
    }

    // Record that a one-shot actuator fired, so a retried branch can't re-fire it.
    void markFired(const std::string& key)
    {
        std::lock_guard lk(mu_);
        fired_.insert(key);
    }

    // --- mission clock --------------------------------------------------------

    // Start the run clock at the first real tick; calling again does nothing.
    void startMissionClock()
    {
        std::lock_guard lk(mu_);
        if (!mission_start_) mission_start_ = Clock::now();
    }

    // Seconds since the mission started ticking (0 until it does) — feeds the
    // MissionTimeLeft guard.
    double missionElapsedS() const
    {
        std::lock_guard lk(mu_);
        if (!mission_start_) return 0.0;
        return secondsSince(*mission_start_);
    }

private:
    mutable std::mutex mu_;
    std::optional<double> depth_;    // metres, +down
    std::optional<double> heading_;  // rad, (-pi, pi]
    Clock::time_point depth_stamp_{};
    Clock::time_point heading_stamp_{};
    std::unordered_map<std::string, std::vector<DetectionSnapshot>> detections_;
    std::optional<std::string> gate_side_;
    std::unordered_map<std::string, double> saved_headings_;
    std::unordered_set<std::string> fired_;
    std::optional<Clock::time_point> mission_start_;
};

// ---------------------------------------------------------------------------
// The wire contract — the only functions in the codebase allowed to build a
// Task message for the tree.
// ---------------------------------------------------------------------------
// Fill in and send one Task message to the controller. Every cmd* helper
// below funnels through here; `absolute` distinguishes absolute vs relative
// setpoints for "move" commands (it means nothing for the other verbs, where
// we pass true by convention).
inline void publishTask(BTContext& ctx, const std::string& type, const std::string& direction,
                        double magnitude, bool absolute)
{
    snappy_cpp::msg::Task msg;
    msg.type = type;
    msg.direction = direction;
    msg.magnitude = magnitude;
    msg.absolute = absolute;
    msg.overwrite = true;  // ground rule 1 — never exercise the task queue
    ctx.task_pub->publish(msg);
}

// Tell the controller to go to (and then hold) `metres` below the surface.
inline void cmdDepth(BTContext& ctx, double metres)
{
    publishTask(ctx, "move", "z", metres, true);
}

// Tell the controller to turn to (and then hold) an absolute heading, radians.
inline void cmdHeadingAbs(BTContext& ctx, double yaw_rad)
{
    publishTask(ctx, "move", "yaw", wrapToPi(yaw_rad), true);
}

// Tell the controller to turn BY delta_rad from wherever it is now
// (absolute=false → the controller adds the delta to its own heading).
inline void cmdTurnRel(BTContext& ctx, double delta_rad)
{
    publishTask(ctx, "move", "yaw", delta_rad, false);
}

// Open-loop translate: direction is "forward"/"backward"/"left"/"right",
// thrust_pct is how hard to push as a % of thruster power. The caller picks
// the speed (mission XML uses ~30-35); the clamp(|pct|, 0, 100) only CAPS a
// bad value into the legal 0..100 range — it does not command full speed.
// The controller's 3 s deadman stops the sub if these commands stop coming.
inline void cmdDrive(BTContext& ctx, const std::string& direction, double thrust_pct)
{
    publishTask(ctx, "drive", direction, std::clamp(std::abs(thrust_pct), 0.0, 100.0), true);
}

// Kill all open-loop translation immediately (thrust 0).
inline void cmdDriveStop(BTContext& ctx)
{
    publishTask(ctx, "drive", "stop", 0.0, true);
}

// Command a continuous body rotation ("roll" or "pitch") at rate_dps deg/s;
// signed for direction, 0 stops it and hands back to the stabiliser.
inline void cmdSpin(BTContext& ctx, const std::string& axis, double rate_dps)
{
    publishTask(ctx, "spin", axis, rate_dps, true);
}

// Stop any roll/pitch rotation (rate 0 on both axes) — used by the abort path.
inline void cmdSpinStopAll(BTContext& ctx)
{
    cmdSpin(ctx, "roll", 0.0);
    cmdSpin(ctx, "pitch", 0.0);
}

// Turn ON the controller's camera-tracking servo for one object class.
// magnitude carries the mode: 1 = keep it centred AHEAD (front camera),
// 2 = keep it centred BELOW (down camera). The tree only enables/monitors;
// the actual steering loop runs in the controller.
inline void cmdTrack(BTContext& ctx, const std::string& camera, const std::string& object_class)
{
    const double mode = (camera == "down") ? 2.0 : 1.0;
    publishTask(ctx, "track", object_class, mode, true);
}

// Turn the tracking servo OFF (mode 0); the controller must then re-hold its
// current depth/heading as its setpoints.
inline void cmdTrackOff(BTContext& ctx)
{
    publishTask(ctx, "track", "", 0.0, true);
}

// Pulse a solenoid for pulse_s seconds: actuator is "marker_left",
// "torpedo_right", "claw_open", etc.
inline void cmdActuate(BTContext& ctx, const std::string& actuator, double pulse_s)
{
    publishTask(ctx, "actuate", actuator, pulse_s, true);
}

// ---------------------------------------------------------------------------
// Object specs: mission XML stays role-agnostic; leaves resolve these tokens
// against the assigned role so no branch re-derives the role from vision.
// ---------------------------------------------------------------------------
inline std::vector<std::string> resolveObjectSpec(const BTContext& ctx, const std::string& spec)
{
    if (spec == "role_gate_icon") return roleGateIcons(ctx.role);
    if (spec == "other_gate_icon") return roleGateIcons(otherRole(ctx.role));
    if (spec == "gate_icons") {
        std::vector<std::string> all = roleGateIcons("restore");
        const auto& rec = roleGateIcons("recovery");
        all.insert(all.end(), rec.begin(), rec.end());
        return all;
    }
    if (spec == "role_bin_icon") return roleBinIcons(ctx.role);

    // Literal class name, or a comma-separated any-of list.
    std::vector<std::string> classes;
    std::size_t start = 0;
    while (start <= spec.size()) {
        std::size_t comma = spec.find(',', start);
        if (comma == std::string::npos) comma = spec.size();
        if (comma > start) classes.push_back(spec.substr(start, comma - start));
        start = comma + 1;
    }
    return classes;
}

// Unwrap a required port or fail loudly at the first tick — a missing port is
// a mission-XML authoring error, not a runtime condition.
template <typename T>
T expect(BT::Expected<T> value, const std::string& node_name, const std::string& port)
{
    if (!value) {
        throw BT::RuntimeError(node_name, ": port '", port, "': ", value.error());
    }
    return std::move(value.value());
}

// Within threshold for one tick is not success — the sub can blow through a
// target at speed. Tracks how long the error has stayed continuously in band.
class SettleTracker
{
public:
    // Forget any progress — call whenever a new target is commanded.
    void reset() { settled_since_.reset(); }

    // Feed one tick's "am I within tolerance" answer; returns true once the
    // error has stayed inside the band for settle_s continuous seconds.
    bool update(bool in_band, double settle_s)
    {
        if (!in_band) {
            settled_since_.reset();
            return false;
        }
        if (!settled_since_) settled_since_ = Clock::now();
        return secondsSince(*settled_since_) >= settle_s;
    }

private:
    std::optional<Clock::time_point> settled_since_;
};

// ---------------------------------------------------------------------------
// Leaf base classes. The context is injected through the factory's
// extra-constructor-argument mechanism, not the blackboard, so subtree
// blackboard isolation can never hide it from a leaf.
// ---------------------------------------------------------------------------
class CtxAction : public BT::StatefulActionNode
{
public:
    CtxAction(const std::string& name, const BT::NodeConfig& config, std::shared_ptr<BTContext> ctx)
    : BT::StatefulActionNode(name, config), ctx_(std::move(ctx))
    {
    }

protected:
    BTContext& ctx() const { return *ctx_; }
    rclcpp::Logger logger() const { return ctx_->node->get_logger(); }

private:
    std::shared_ptr<BTContext> ctx_;
};

class CtxCondition : public BT::ConditionNode
{
public:
    CtxCondition(const std::string& name, const BT::NodeConfig& config, std::shared_ptr<BTContext> ctx)
    : BT::ConditionNode(name, config), ctx_(std::move(ctx))
    {
    }

protected:
    BTContext& ctx() const { return *ctx_; }
    rclcpp::Logger logger() const { return ctx_->node->get_logger(); }

private:
    std::shared_ptr<BTContext> ctx_;
};

class CtxSync : public BT::SyncActionNode
{
public:
    CtxSync(const std::string& name, const BT::NodeConfig& config, std::shared_ptr<BTContext> ctx)
    : BT::SyncActionNode(name, config), ctx_(std::move(ctx))
    {
    }

protected:
    BTContext& ctx() const { return *ctx_; }
    rclcpp::Logger logger() const { return ctx_->node->get_logger(); }

private:
    std::shared_ptr<BTContext> ctx_;
};

}  // namespace snappy::bt

#pragma once
// Motion leaves. Conventions, project-wide:
//  - Ports take degrees/metres (human-facing); the wire carries radians.
//  - Settling means continuously inside the band for settle_s, not one tick.
//    Velocity-bound settling waits on the estimator publishing twist.
//  - Timeouts come from BT.CPP `Timeout` decorators in the XML, and mean
//    FAILURE. Leaves that are inherently time-bounded (Surge, Spin, Wait)
//    bound themselves.
//  - Halt rule: every leaf that turns something ON turns it OFF in
//    onHalted(). Pure setpoint leaves halt as no-ops — holding the latest
//    depth/heading target is the controller's idle behaviour, and yanking
//    the setpoint on halt would leave it undefined instead of safe.
//
// Deliberately absent: MoveBody / GotoWorld / SaveWaypoint / GotoWaypoint.
// Writing them against raw DVL dead-reckoning would produce leaves that lie
// about where the sub is; they land once the ESKF merge publishes a real
// position estimate.

#include "bt_context.hpp"

namespace snappy::bt
{

// Absolute depth setpoint, metres +down (Bar02 convention).
class SetDepth : public CtxAction
{
public:
    using CtxAction::CtxAction;

    static BT::PortsList providedPorts()
    {
        return {
            BT::InputPort<double>("depth_m", "target depth, metres +down"),
            BT::InputPort<double>("tolerance_m", 0.15, "settle band"),
            BT::InputPort<double>("settle_s", 1.0, "time inside band before SUCCESS"),
        };
    }

    BT::NodeStatus onStart() override
    {
        target_ = expect(getInput<double>("depth_m"), name(), "depth_m");
        tolerance_ = expect(getInput<double>("tolerance_m"), name(), "tolerance_m");
        settle_s_ = expect(getInput<double>("settle_s"), name(), "settle_s");
        settle_.reset();
        cmdDepth(ctx(), target_);
        RCLCPP_INFO(logger(), "SetDepth: -> %.2f m", target_);
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override
    {
        const auto depth = ctx().depth();
        if (!depth) return BT::NodeStatus::RUNNING;  // staleness is the guards' call
        const bool in_band = std::abs(depth->value - target_) <= tolerance_;
        if (settle_.update(in_band, settle_s_)) {
            RCLCPP_INFO(logger(), "SetDepth: settled at %.2f m", depth->value);
            return BT::NodeStatus::SUCCESS;
        }
        return BT::NodeStatus::RUNNING;
    }

    void onHalted() override {}  // setpoint leaf: controller keeps holding depth

private:
    double target_ = 0.0;
    double tolerance_ = 0.15;
    double settle_s_ = 1.0;
    SettleTracker settle_;
};

class SetHeading : public CtxAction
{
public:
    using CtxAction::CtxAction;

    static BT::PortsList providedPorts()
    {
        return {
            BT::InputPort<double>("yaw_deg", "absolute heading, degrees"),
            BT::InputPort<double>("tolerance_deg", 7.0, "settle band"),
            BT::InputPort<double>("settle_s", 1.0, "time inside band before SUCCESS"),
        };
    }

    BT::NodeStatus onStart() override
    {
        target_rad_ = wrapToPi(expect(getInput<double>("yaw_deg"), name(), "yaw_deg") * kDeg2Rad);
        tolerance_rad_ = expect(getInput<double>("tolerance_deg"), name(), "tolerance_deg") * kDeg2Rad;
        settle_s_ = expect(getInput<double>("settle_s"), name(), "settle_s");
        settle_.reset();
        cmdHeadingAbs(ctx(), target_rad_);
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override
    {
        const auto heading = ctx().heading();
        if (!heading) return BT::NodeStatus::RUNNING;
        const bool in_band = std::abs(wrapToPi(target_rad_ - heading->value)) <= tolerance_rad_;
        return settle_.update(in_band, settle_s_) ? BT::NodeStatus::SUCCESS : BT::NodeStatus::RUNNING;
    }

    void onHalted() override {}

private:
    double target_rad_ = 0.0;
    double tolerance_rad_ = 0.12;
    double settle_s_ = 1.0;
    SettleTracker settle_;
};

// Relative turn. Publishes the delta (the controller adds it to its own IMU
// heading) and settles against the composed target it computed itself,
// wrap-aware.
class TurnRelative : public CtxAction
{
public:
    using CtxAction::CtxAction;

    static BT::PortsList providedPorts()
    {
        return {
            BT::InputPort<double>("yaw_deg", "relative turn, degrees, |value| < 180"),
            BT::InputPort<double>("tolerance_deg", 7.0, "settle band"),
            BT::InputPort<double>("settle_s", 1.0, "time inside band before SUCCESS"),
        };
    }

    BT::NodeStatus onStart() override
    {
        double delta_deg = expect(getInput<double>("yaw_deg"), name(), "yaw_deg");
        // Keep increments < 180 deg so turn direction stays unambiguous.
        if (std::abs(delta_deg) >= 180.0) {
            RCLCPP_WARN(logger(), "TurnRelative: %.0f deg clamped to ±179", delta_deg);
            delta_deg = std::copysign(179.0, delta_deg);
        }
        tolerance_rad_ = expect(getInput<double>("tolerance_deg"), name(), "tolerance_deg") * kDeg2Rad;
        settle_s_ = expect(getInput<double>("settle_s"), name(), "settle_s");

        const auto heading = ctx().heading();
        if (!heading) {
            RCLCPP_ERROR(logger(), "TurnRelative: no heading available");
            return BT::NodeStatus::FAILURE;
        }
        const double delta_rad = delta_deg * kDeg2Rad;
        target_rad_ = wrapToPi(heading->value + delta_rad);
        settle_.reset();
        cmdTurnRel(ctx(), delta_rad);
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override
    {
        const auto heading = ctx().heading();
        if (!heading) return BT::NodeStatus::RUNNING;
        const bool in_band = std::abs(wrapToPi(target_rad_ - heading->value)) <= tolerance_rad_;
        return settle_.update(in_band, settle_s_) ? BT::NodeStatus::SUCCESS : BT::NodeStatus::RUNNING;
    }

    void onHalted() override {}

private:
    double target_rad_ = 0.0;
    double tolerance_rad_ = 0.12;
    double settle_s_ = 1.0;
    SettleTracker settle_;
};

// Open-loop timed translation — the only working translation until the
// controller regains closed-loop x/y. Registered twice: "Surge"
// (forward/backward) and "StrafeBody" (right/left), signed thrust_pct picks
// the direction. Re-publishes every <=1 s so the controller's 3 s deadman
// never cuts thrust mid-leg, while a hung BT process still auto-stops the
// sub — the deadman is the point, not an obstacle.
class TimedDrive : public CtxAction
{
public:
    enum class Axis { Surge, Sway };

    TimedDrive(const std::string& name, const BT::NodeConfig& config,
               std::shared_ptr<BTContext> ctx, Axis axis)
    : CtxAction(name, config, std::move(ctx)), axis_(axis)
    {
    }

    static BT::PortsList providedPorts()
    {
        return {
            BT::InputPort<double>("thrust_pct", "signed thrust %, + = forward/right"),
            BT::InputPort<double>("seconds", "leg duration"),
        };
    }

    BT::NodeStatus onStart() override
    {
        const double pct = expect(getInput<double>("thrust_pct"), name(), "thrust_pct");
        duration_s_ = expect(getInput<double>("seconds"), name(), "seconds");
        direction_ = (axis_ == Axis::Surge) ? (pct >= 0 ? "forward" : "backward")
                                            : (pct >= 0 ? "right" : "left");
        thrust_pct_ = std::abs(pct);
        start_ = Clock::now();
        publishDrive();
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override
    {
        if (secondsSince(start_) >= duration_s_) {
            cmdDriveStop(ctx());
            return BT::NodeStatus::SUCCESS;
        }
        if (secondsSince(last_pub_) >= kRepublishS) publishDrive();
        return BT::NodeStatus::RUNNING;
    }

    // A halted drive must not keep thrusting until the deadman notices.
    void onHalted() override { cmdDriveStop(ctx()); }

private:
    void publishDrive()
    {
        cmdDrive(ctx(), direction_, thrust_pct_);
        last_pub_ = Clock::now();
    }

    static constexpr double kRepublishS = 1.0;
    Axis axis_;
    std::string direction_;
    double thrust_pct_ = 0.0;
    double duration_s_ = 0.0;
    Clock::time_point start_{};
    Clock::time_point last_pub_{};
};

// Body-rate rotation for rolls/flips. Open-loop by design: commanded as
// rate x duration, never as Euler setpoints — pitch past ±90° is
// unrepresentable in Euler angles, and an absolute-setpoint "roll" nets zero
// rotation under a shortest-path attitude controller. The stabiliser
// re-levels once the rate command returns to zero.
class Spin : public CtxAction
{
public:
    using CtxAction::CtxAction;

    static BT::PortsList providedPorts()
    {
        return {
            BT::InputPort<std::string>("axis", "roll | pitch"),
            BT::InputPort<double>("rate_dps", 45.0, "body rate magnitude, deg/s"),
            BT::InputPort<double>("angle_deg", 360.0, "signed total rotation"),
        };
    }

    BT::NodeStatus onStart() override
    {
        axis_ = expect(getInput<std::string>("axis"), name(), "axis");
        if (axis_ != "roll" && axis_ != "pitch") {
            throw BT::RuntimeError(name(), ": axis must be roll or pitch, got '", axis_, "'");
        }
        const double rate = std::abs(expect(getInput<double>("rate_dps"), name(), "rate_dps"));
        const double angle = expect(getInput<double>("angle_deg"), name(), "angle_deg");
        if (rate < 1.0) throw BT::RuntimeError(name(), ": rate_dps must be >= 1");
        rate_dps_ = std::copysign(rate, angle);
        duration_s_ = std::abs(angle) / rate;
        start_ = Clock::now();
        publishRate();
        RCLCPP_INFO(logger(), "Spin: %s %.0f deg at %.0f deg/s (%.1f s)", axis_.c_str(), angle,
                    rate_dps_, duration_s_);
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override
    {
        if (secondsSince(start_) >= duration_s_) {
            cmdSpin(ctx(), axis_, 0.0);  // hand back to the stabiliser
            return BT::NodeStatus::SUCCESS;
        }
        if (secondsSince(last_pub_) >= kRepublishS) publishRate();
        return BT::NodeStatus::RUNNING;
    }

    void onHalted() override { cmdSpin(ctx(), axis_, 0.0); }

private:
    void publishRate()
    {
        cmdSpin(ctx(), axis_, rate_dps_);
        last_pub_ = Clock::now();
    }

    static constexpr double kRepublishS = 1.0;
    std::string axis_;
    double rate_dps_ = 0.0;
    double duration_s_ = 0.0;
    Clock::time_point start_{};
    Clock::time_point last_pub_{};
};

class Wait : public CtxAction
{
public:
    using CtxAction::CtxAction;

    static BT::PortsList providedPorts()
    {
        return {BT::InputPort<double>("seconds", "how long to wait")};
    }

    BT::NodeStatus onStart() override
    {
        duration_s_ = expect(getInput<double>("seconds"), name(), "seconds");
        start_ = Clock::now();
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override
    {
        return secondsSince(start_) >= duration_s_ ? BT::NodeStatus::SUCCESS
                                                   : BT::NodeStatus::RUNNING;
    }

    void onHalted() override {}

private:
    double duration_s_ = 0.0;
    Clock::time_point start_{};
};

// Explicit station-keep: kill any open-loop drive, then sit for `seconds`
// while the controller holds its latest depth/heading targets.
class Hold : public CtxAction
{
public:
    using CtxAction::CtxAction;

    static BT::PortsList providedPorts()
    {
        return {BT::InputPort<double>("seconds", "how long to hold")};
    }

    BT::NodeStatus onStart() override
    {
        duration_s_ = expect(getInput<double>("seconds"), name(), "seconds");
        start_ = Clock::now();
        cmdDriveStop(ctx());
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override
    {
        return secondsSince(start_) >= duration_s_ ? BT::NodeStatus::SUCCESS
                                                   : BT::NodeStatus::RUNNING;
    }

    void onHalted() override {}

private:
    double duration_s_ = 0.0;
    Clock::time_point start_{};
};

// Mission surface — NOT the emergency ascent. Station-keeps (drive stop) and
// ascends vertically on a depth setpoint of zero; the octagon task depends
// on surfacing inside the 2.7 m ring, and the run continues after it. Can
// legitimately FAIL via a Timeout decorator.
class Surface : public CtxAction
{
public:
    using CtxAction::CtxAction;

    static BT::PortsList providedPorts()
    {
        return {
            BT::InputPort<double>("surfaced_depth_m", 0.3, "consider surfaced above this"),
            BT::InputPort<double>("settle_s", 1.0, "time above threshold before SUCCESS"),
        };
    }

    BT::NodeStatus onStart() override
    {
        threshold_ = expect(getInput<double>("surfaced_depth_m"), name(), "surfaced_depth_m");
        settle_s_ = expect(getInput<double>("settle_s"), name(), "settle_s");
        settle_.reset();
        cmdDriveStop(ctx());
        cmdDepth(ctx(), 0.0);
        RCLCPP_INFO(logger(), "Surface: ascending");
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override
    {
        const auto depth = ctx().depth();
        if (!depth) return BT::NodeStatus::RUNNING;
        if (settle_.update(depth->value <= threshold_, settle_s_)) {
            RCLCPP_INFO(logger(), "Surface: at %.2f m", depth->value);
            return BT::NodeStatus::SUCCESS;
        }
        return BT::NodeStatus::RUNNING;
    }

    void onHalted() override {}

private:
    double threshold_ = 0.3;
    double settle_s_ = 1.0;
    SettleTracker settle_;
};

// Abort ascent. Runs only when a safety guard trips or the whole mission
// fails, and must not depend on the sensor whose failure sent us here: it
// succeeds on a *fresh* shallow depth reading OR unconditionally after
// max_ascent_s, and keeps re-publishing the ascent commands either way.
// Never returns FAILURE — there is nothing below it to fall back to. The
// controller-side drive deadman is the independent second layer.
class EmergencySurface : public CtxAction
{
public:
    using CtxAction::CtxAction;

    static BT::PortsList providedPorts()
    {
        return {
            BT::InputPort<double>("max_ascent_s", 45.0, "declare surfaced after this long"),
            BT::InputPort<double>("surfaced_depth_m", 0.3, "fresh-depth success threshold"),
        };
    }

    BT::NodeStatus onStart() override
    {
        max_ascent_s_ = expect(getInput<double>("max_ascent_s"), name(), "max_ascent_s");
        threshold_ = expect(getInput<double>("surfaced_depth_m"), name(), "surfaced_depth_m");
        start_ = Clock::now();
        RCLCPP_ERROR(logger(), "EMERGENCY SURFACE: mission aborted, ascending");
        cmdTrackOff(ctx());
        cmdSpinStopAll(ctx());
        publishAscent();
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override
    {
        const auto depth = ctx().depth();
        const bool depth_says_up = depth && depth->age_s < 1.0 && depth->value <= threshold_;
        if (depth_says_up || secondsSince(start_) >= max_ascent_s_) {
            RCLCPP_ERROR(logger(), "EmergencySurface: done (%s)",
                         depth_says_up ? "depth confirms surfaced" : "time limit — depth unverified");
            return BT::NodeStatus::SUCCESS;
        }
        if (secondsSince(last_pub_) >= 1.0) publishAscent();
        return BT::NodeStatus::RUNNING;
    }

    void onHalted() override {}  // last child of the root Fallback; nothing halts it

private:
    void publishAscent()
    {
        cmdDriveStop(ctx());
        cmdDepth(ctx(), 0.0);
        last_pub_ = Clock::now();
    }

    double max_ascent_s_ = 45.0;
    double threshold_ = 0.3;
    Clock::time_point start_{};
    Clock::time_point last_pub_{};
};

// Heading snapshots — the estimator-free stand-in for position waypoints.
// ReturnHome runs on the reciprocal of the saved gate heading until a real
// position estimate makes GotoWaypoint honest.
class SaveHeading : public CtxSync
{
public:
    using CtxSync::CtxSync;

    static BT::PortsList providedPorts()
    {
        return {BT::InputPort<std::string>("key", "name for this heading snapshot")};
    }

    BT::NodeStatus tick() override
    {
        const auto key = expect(getInput<std::string>("key"), name(), "key");
        const auto heading = ctx().heading();
        if (!heading) {
            RCLCPP_ERROR(logger(), "SaveHeading: no heading to save for '%s'", key.c_str());
            return BT::NodeStatus::FAILURE;
        }
        ctx().saveHeading(key, heading->value);
        RCLCPP_INFO(logger(), "SaveHeading: '%s' = %.0f deg", key.c_str(), heading->value / kDeg2Rad);
        return BT::NodeStatus::SUCCESS;
    }
};

class FaceSavedHeading : public CtxAction
{
public:
    using CtxAction::CtxAction;

    static BT::PortsList providedPorts()
    {
        return {
            BT::InputPort<std::string>("key", "heading snapshot to face"),
            BT::InputPort<bool>("reciprocal", false, "face the opposite direction"),
            BT::InputPort<double>("tolerance_deg", 7.0, "settle band"),
            BT::InputPort<double>("settle_s", 1.0, "time inside band before SUCCESS"),
        };
    }

    BT::NodeStatus onStart() override
    {
        const auto key = expect(getInput<std::string>("key"), name(), "key");
        const bool reciprocal = expect(getInput<bool>("reciprocal"), name(), "reciprocal");
        tolerance_rad_ = expect(getInput<double>("tolerance_deg"), name(), "tolerance_deg") * kDeg2Rad;
        settle_s_ = expect(getInput<double>("settle_s"), name(), "settle_s");

        const auto saved = ctx().savedHeading(key);
        if (!saved) {
            RCLCPP_ERROR(logger(), "FaceSavedHeading: no snapshot '%s'", key.c_str());
            return BT::NodeStatus::FAILURE;
        }
        target_rad_ = wrapToPi(*saved + (reciprocal ? M_PI : 0.0));
        settle_.reset();
        cmdHeadingAbs(ctx(), target_rad_);
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override
    {
        const auto heading = ctx().heading();
        if (!heading) return BT::NodeStatus::RUNNING;
        const bool in_band = std::abs(wrapToPi(target_rad_ - heading->value)) <= tolerance_rad_;
        return settle_.update(in_band, settle_s_) ? BT::NodeStatus::SUCCESS : BT::NodeStatus::RUNNING;
    }

    void onHalted() override {}

private:
    double target_rad_ = 0.0;
    double tolerance_rad_ = 0.12;
    double settle_s_ = 1.0;
    SettleTracker settle_;
};

inline void registerMotionNodes(BT::BehaviorTreeFactory& factory, std::shared_ptr<BTContext> ctx)
{
    factory.registerNodeType<SetDepth>("SetDepth", ctx);
    factory.registerNodeType<SetHeading>("SetHeading", ctx);
    factory.registerNodeType<TurnRelative>("TurnRelative", ctx);
    factory.registerNodeType<TimedDrive>("Surge", ctx, TimedDrive::Axis::Surge);
    factory.registerNodeType<TimedDrive>("StrafeBody", ctx, TimedDrive::Axis::Sway);
    factory.registerNodeType<Spin>("Spin", ctx);
    factory.registerNodeType<Wait>("Wait", ctx);
    factory.registerNodeType<Hold>("Hold", ctx);
    factory.registerNodeType<Surface>("Surface", ctx);
    factory.registerNodeType<EmergencySurface>("EmergencySurface", ctx);
    factory.registerNodeType<SaveHeading>("SaveHeading", ctx);
    factory.registerNodeType<FaceSavedHeading>("FaceSavedHeading", ctx);
}

}  // namespace snappy::bt

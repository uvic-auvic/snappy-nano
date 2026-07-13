#pragma once
// Vision leaves: debounced sighting, controller-side tracking enable +
// convergence monitor, and the gate-side latch.
//
// Path-marker following (AlignToPath) is missing on purpose: the down-camera
// model has no path-marker class yet. When the vision team adds one, it is
// AlignToObject(camera=down) to center over the marker plus a
// yaw-to-long-axis step that needs the mask polygon, not just the bbox.
// Until then the gate→slalom and slalom→bins transitions are dead-reckoned
// in the mission XML.

#include "bt_context.hpp"

namespace snappy::bt
{

// Debounced sighting: N consecutive ticks with a fresh, confident detection
// of the *same class*, judged inside a bounded observation window
// so a Fallback can move on (rotate-and-search) when nothing shows up.
// RUNNING while observing — a bare FAILURE on the first empty tick would make
// the search spin past objects the camera needed two frames to confirm.
class Locate : public CtxAction
{
public:
    using CtxAction::CtxAction;

    static BT::PortsList providedPorts()
    {
        return {
            BT::InputPort<std::string>("camera", "front", "front | down"),
            BT::InputPort<std::string>("object", "class name, comma list, or role_* spec"),
            BT::InputPort<double>("min_confidence", 0.4, "min detection confidence"),
            BT::InputPort<double>("max_age_s", 0.3, "max detection age per tick"),
            BT::InputPort<int>("consecutive", 3, "consecutive same-class ticks required"),
            BT::InputPort<double>("window_s", 2.0, "observation window before FAILURE"),
            BT::OutputPort<DetectionSnapshot>("detection", "confirmed sighting"),
        };
    }

    BT::NodeStatus onStart() override
    {
        camera_ = expect(getInput<std::string>("camera"), name(), "camera");
        classes_ = resolveObjectSpec(ctx(), expect(getInput<std::string>("object"), name(), "object"));
        min_conf_ = expect(getInput<double>("min_confidence"), name(), "min_confidence");
        max_age_s_ = expect(getInput<double>("max_age_s"), name(), "max_age_s");
        required_ = expect(getInput<int>("consecutive"), name(), "consecutive");
        window_s_ = expect(getInput<double>("window_s"), name(), "window_s");
        streak_ = 0;
        streak_class_.clear();
        window_start_ = Clock::now();
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override
    {
        const auto hit = BTContext::best(ctx().freshDetections(camera_, classes_, max_age_s_, min_conf_));
        if (hit) {
            streak_ = (hit->object_class == streak_class_) ? streak_ + 1 : 1;
            streak_class_ = hit->object_class;
            if (streak_ >= required_) {
                setOutput("detection", *hit);
                RCLCPP_INFO(logger(), "Locate: confirmed %s (conf %.2f, bearing %.0f deg)",
                            hit->object_class.c_str(), hit->confidence, hit->bearing_rad / kDeg2Rad);
                return BT::NodeStatus::SUCCESS;
            }
        } else {
            streak_ = 0;
        }
        return secondsSince(window_start_) >= window_s_ ? BT::NodeStatus::FAILURE
                                                        : BT::NodeStatus::RUNNING;
    }

    void onHalted() override {}  // pure observer, nothing to undo

private:
    std::string camera_;
    std::vector<std::string> classes_;
    double min_conf_ = 0.4;
    double max_age_s_ = 0.3;
    int required_ = 3;
    double window_s_ = 2.0;
    int streak_ = 0;
    std::string streak_class_;
    Clock::time_point window_start_{};
};

// The visual-servo loop lives in the controller — a 10 Hz tree publishing
// position nudges would be a control loop with tick jitter and message
// latency inside it. This leaf only enables tracking for one concrete class,
// then monitors convergence (bbox centred) and detection freshness:
//   front camera  — |horizontal offset| inside tolerance (center-ahead; range
//                   stand-off is the controller's job via distance_m),
//   down camera   — radial offset inside tolerance (center-over).
// On SUCCESS tracking stays ENABLED so the follow-up act (drop, fire) happens
// while the servo holds station — the mission XML disables it explicitly with
// DisableTracking. On halt or target loss it is switched off here.
class AlignToObject : public CtxAction
{
public:
    using CtxAction::CtxAction;

    static BT::PortsList providedPorts()
    {
        return {
            BT::InputPort<std::string>("camera", "front", "front | down"),
            BT::InputPort<std::string>("object", "class name, comma list, or role_* spec"),
            BT::InputPort<double>("tolerance", 0.08, "max |offset| from image centre, fraction of frame"),
            BT::InputPort<double>("settle_s", 1.0, "time inside tolerance before SUCCESS"),
            BT::InputPort<double>("min_confidence", 0.35, "min detection confidence"),
            BT::InputPort<double>("lost_timeout_s", 1.5, "FAILURE after unseen this long"),
        };
    }

    BT::NodeStatus onStart() override
    {
        camera_ = expect(getInput<std::string>("camera"), name(), "camera");
        const auto classes = resolveObjectSpec(ctx(), expect(getInput<std::string>("object"), name(), "object"));
        tolerance_ = expect(getInput<double>("tolerance"), name(), "tolerance");
        settle_s_ = expect(getInput<double>("settle_s"), name(), "settle_s");
        min_conf_ = expect(getInput<double>("min_confidence"), name(), "min_confidence");
        lost_timeout_s_ = expect(getInput<double>("lost_timeout_s"), name(), "lost_timeout_s");

        // Bind tracking to one concrete class, chosen from what is actually
        // visible right now (this leaf runs right after a successful Locate).
        const auto hit = BTContext::best(ctx().freshDetections(camera_, classes, 1.0, min_conf_));
        if (!hit) {
            RCLCPP_WARN(logger(), "AlignToObject: nothing visible to align to");
            return BT::NodeStatus::FAILURE;
        }
        class_ = hit->object_class;
        last_seen_ = Clock::now();
        settle_.reset();
        cmdTrack(ctx(), camera_, class_);
        RCLCPP_INFO(logger(), "AlignToObject: tracking %s on %s camera", class_.c_str(), camera_.c_str());
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override
    {
        const auto hit =
            BTContext::best(ctx().freshDetections(camera_, {class_}, lost_timeout_s_, min_conf_));
        if (!hit) {
            if (secondsSince(last_seen_) >= lost_timeout_s_) {
                RCLCPP_WARN(logger(), "AlignToObject: lost %s", class_.c_str());
                cmdTrackOff(ctx());
                return BT::NodeStatus::FAILURE;
            }
            return BT::NodeStatus::RUNNING;
        }
        last_seen_ = hit->stamp;

        const double err = (camera_ == "down") ? std::hypot(hit->norm_off_x, hit->norm_off_y)
                                               : std::abs(hit->norm_off_x);
        if (settle_.update(err <= tolerance_, settle_s_)) {
            RCLCPP_INFO(logger(), "AlignToObject: centred on %s", class_.c_str());
            return BT::NodeStatus::SUCCESS;  // tracking stays on — see class comment
        }
        return BT::NodeStatus::RUNNING;
    }

    void onHalted() override { cmdTrackOff(ctx()); }

private:
    std::string camera_;
    std::string class_;
    double tolerance_ = 0.08;
    double settle_s_ = 1.0;
    double min_conf_ = 0.35;
    double lost_timeout_s_ = 1.5;
    Clock::time_point last_seen_{};
    SettleTracker settle_;
};

class DisableTracking : public CtxSync
{
public:
    using CtxSync::CtxSync;

    static BT::PortsList providedPorts() { return {}; }

    BT::NodeStatus tick() override
    {
        cmdTrackOff(ctx());
        return BT::NodeStatus::SUCCESS;
    }
};

// Which side of the gate we pass under fixes everything downstream (slalom
// red-pipe side, bins, torpedo images, octagon items). The role itself is
// assigned pre-run by the coin flip and arrives as a launch parameter; this
// leaf only latches WHERE our role's icons are. It runs after AlignToObject
// centred us on our own icon, so geometry is read off the *other* role's
// icon: other icon to our right → we are on the left half.
// Never blocks the mission: on an inconclusive window it latches the launch
// fallback and reports SUCCESS with a loud warning (the rules assign side by
// coin flip when the indicator is unreadable, so the fallback must exist).
class LatchGateSide : public CtxAction
{
public:
    using CtxAction::CtxAction;

    static BT::PortsList providedPorts()
    {
        return {
            BT::InputPort<double>("window_s", 3.0, "how long to look before falling back"),
            BT::InputPort<double>("max_age_s", 0.5, "max detection age"),
            BT::InputPort<double>("min_confidence", 0.4, "min detection confidence"),
        };
    }

    BT::NodeStatus onStart() override
    {
        if (ctx().gateSide()) return BT::NodeStatus::SUCCESS;  // already latched
        window_s_ = expect(getInput<double>("window_s"), name(), "window_s");
        max_age_s_ = expect(getInput<double>("max_age_s"), name(), "max_age_s");
        min_conf_ = expect(getInput<double>("min_confidence"), name(), "min_confidence");
        window_start_ = Clock::now();
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override
    {
        const auto other = BTContext::best(ctx().freshDetections(
            "front", roleGateIcons(otherRole(ctx().role)), max_age_s_, min_conf_));
        if (other) {
            const std::string side = other->bearing_rad > 0 ? "left" : "right";
            ctx().latchGateSide(side);
            RCLCPP_INFO(logger(), "LatchGateSide: role '%s' passes on the %s (other icon at %.0f deg)",
                        ctx().role.c_str(), side.c_str(), other->bearing_rad / kDeg2Rad);
            return BT::NodeStatus::SUCCESS;
        }
        if (secondsSince(window_start_) >= window_s_) {
            ctx().latchGateSide(ctx().default_gate_side);
            RCLCPP_WARN(logger(), "LatchGateSide: could not classify divider — falling back to '%s'",
                        ctx().default_gate_side.c_str());
            return BT::NodeStatus::SUCCESS;
        }
        return BT::NodeStatus::RUNNING;
    }

    void onHalted() override {}

private:
    double window_s_ = 3.0;
    double max_age_s_ = 0.5;
    double min_conf_ = 0.4;
    Clock::time_point window_start_{};
};

inline void registerVisionNodes(BT::BehaviorTreeFactory& factory, std::shared_ptr<BTContext> ctx)
{
    factory.registerNodeType<Locate>("Locate", ctx);
    factory.registerNodeType<AlignToObject>("AlignToObject", ctx);
    factory.registerNodeType<DisableTracking>("DisableTracking", ctx);
    factory.registerNodeType<LatchGateSide>("LatchGateSide", ctx);
}

}  // namespace snappy::bt

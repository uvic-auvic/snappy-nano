#pragma once
// One-shot actuators. The hardware node does not exist yet; the wire verb
// ("actuate", pulse seconds) is defined in bt_context.hpp and should become
// a ROS service (one-shot + ack) when it does.
//
// Markers and torpedoes have consumable budgets (2 each) and are guarded by
// fired-latches: a retried branch sees SUCCESS instead of firing twice. The
// claw is repeatable and carries no latch.

#include "bt_context.hpp"

namespace snappy::bt
{

// DropMarker / FireTorpedo — `kind` ("marker" / "torpedo") is fixed at
// registration, the side comes from the XML.
class FireOnce : public CtxAction
{
public:
    FireOnce(const std::string& name, const BT::NodeConfig& config,
             std::shared_ptr<BTContext> ctx, std::string kind)
    : CtxAction(name, config, std::move(ctx)), kind_(std::move(kind))
    {
    }

    static BT::PortsList providedPorts()
    {
        return {
            BT::InputPort<std::string>("side", "left | right"),
            BT::InputPort<double>("pulse_s", 0.5, "solenoid pulse duration"),
        };
    }

    BT::NodeStatus onStart() override
    {
        const auto side = expect(getInput<std::string>("side"), name(), "side");
        if (side != "left" && side != "right") {
            throw BT::RuntimeError(name(), ": side must be left or right, got '", side, "'");
        }
        pulse_s_ = expect(getInput<double>("pulse_s"), name(), "pulse_s");
        key_ = kind_ + "_" + side;

        if (ctx().alreadyFired(key_)) {
            RCLCPP_WARN(logger(), "%s: '%s' already fired — skipping (latched)", name().c_str(),
                        key_.c_str());
            return BT::NodeStatus::SUCCESS;
        }
        ctx().markFired(key_);  // latch before the pulse: a retry mid-pulse must not re-fire
        cmdActuate(ctx(), key_, pulse_s_);
        RCLCPP_INFO(logger(), "%s: firing %s (%.2f s pulse)", name().c_str(), key_.c_str(), pulse_s_);
        fired_at_ = Clock::now();
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override
    {
        // Give the pulse time to complete before the tree moves the sub.
        return secondsSince(fired_at_) >= pulse_s_ ? BT::NodeStatus::SUCCESS
                                                   : BT::NodeStatus::RUNNING;
    }

    // The pulse is already in flight and cannot be recalled; the latch stays.
    void onHalted() override {}

private:
    std::string kind_;
    std::string key_;
    double pulse_s_ = 0.5;
    Clock::time_point fired_at_{};
};

class ActuateClaw : public CtxAction
{
public:
    using CtxAction::CtxAction;

    static BT::PortsList providedPorts()
    {
        return {
            BT::InputPort<std::string>("action", "open | close"),
            BT::InputPort<double>("pulse_s", 1.0, "actuation duration"),
        };
    }

    BT::NodeStatus onStart() override
    {
        const auto action = expect(getInput<std::string>("action"), name(), "action");
        if (action != "open" && action != "close") {
            throw BT::RuntimeError(name(), ": action must be open or close, got '", action, "'");
        }
        pulse_s_ = expect(getInput<double>("pulse_s"), name(), "pulse_s");
        cmdActuate(ctx(), "claw_" + action, pulse_s_);
        started_at_ = Clock::now();
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override
    {
        return secondsSince(started_at_) >= pulse_s_ ? BT::NodeStatus::SUCCESS
                                                     : BT::NodeStatus::RUNNING;
    }

    void onHalted() override {}

private:
    double pulse_s_ = 1.0;
    Clock::time_point started_at_{};
};

inline void registerActuatorNodes(BT::BehaviorTreeFactory& factory, std::shared_ptr<BTContext> ctx)
{
    factory.registerNodeType<FireOnce>("DropMarker", ctx, std::string("marker"));
    factory.registerNodeType<FireOnce>("FireTorpedo", ctx, std::string("torpedo"));
    factory.registerNodeType<ActuateClaw>("ActuateClaw", ctx);
}

}  // namespace snappy::bt

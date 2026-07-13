#pragma once
// Safety guards for the ReactiveSequence at the tree root, plus a cheap
// visibility check. A guard returning FAILURE aborts the mission and hands
// control to EmergencySurface, so every trip is logged at ERROR — the sub
// should never surface without the console saying exactly why.
//
// With the `guards_enabled` parameter false, guards log what WOULD have
// tripped and pass anyway (shadow mode) — use it while debugging on the
// bench or tethered in the pool, never in competition.
//
// KillSwitchOK and BatteryOK are intentionally absent: neither signal exists
// on a topic yet. Add them here the day they do.

#include "bt_context.hpp"

namespace snappy::bt
{

// Aborts when a control-critical sensor stops publishing mid-run. The tick
// loop refuses to start the mission before the first depth + heading samples
// exist, so "no data yet" here can only mean the sensor died.
class SensorsFreshOK : public CtxCondition
{
public:
    using CtxCondition::CtxCondition;

    static BT::PortsList providedPorts()
    {
        return {BT::InputPort<double>("max_age_s", 4.5, "max sensor age before abort")};
    }

    BT::NodeStatus tick() override
    {
        const double max_age = expect(getInput<double>("max_age_s"), name(), "max_age_s");
        const auto depth = ctx().depth();
        const auto heading = ctx().heading();
        const bool ok = depth && heading && depth->age_s <= max_age && heading->age_s <= max_age;
        if (ok) return BT::NodeStatus::SUCCESS;

        if (!ctx().guards_enabled) {
            RCLCPP_WARN_THROTTLE(logger(), *ctx().node->get_clock(), 2000,
                                 "SensorsFreshOK would have tripped (guards disabled): "
                                 "depth age %.2fs, heading age %.2fs",
                                 depth ? depth->age_s : -1.0, heading ? heading->age_s : -1.0);
            return BT::NodeStatus::SUCCESS;
        }
        RCLCPP_ERROR(logger(), "SensorsFreshOK TRIPPED: depth age %.2fs, heading age %.2fs "
                     "(limit %.2fs) — aborting to surface",
                     depth ? depth->age_s : -1.0, heading ? heading->age_s : -1.0, max_age);
        return BT::NodeStatus::FAILURE;
    }
};

class DepthEnvelopeOK : public CtxCondition
{
public:
    using CtxCondition::CtxCondition;

    static BT::PortsList providedPorts()
    {
        return {BT::InputPort<double>("max_depth_m", 4.0, "abort past this depth")};
    }

    BT::NodeStatus tick() override
    {
        const double max_depth = expect(getInput<double>("max_depth_m"), name(), "max_depth_m");
        const auto depth = ctx().depth();
        // Missing depth is SensorsFreshOK's problem; this guard only judges the value.
        if (!depth || depth->value <= max_depth) return BT::NodeStatus::SUCCESS;

        if (!ctx().guards_enabled) {
            RCLCPP_WARN_THROTTLE(logger(), *ctx().node->get_clock(), 2000,
                                 "DepthEnvelopeOK would have tripped (guards disabled): %.2f m",
                                 depth->value);
            return BT::NodeStatus::SUCCESS;
        }
        RCLCPP_ERROR(logger(), "DepthEnvelopeOK TRIPPED: %.2f m exceeds %.2f m — aborting to surface",
                     depth->value, max_depth);
        return BT::NodeStatus::FAILURE;
    }
};

// Run clock. Disabled unless the `mission_time_s` parameter is set > 0, so a
// long pool-debug session never surfaces the sub by surprise; competition
// launches set it explicitly (~900 s).
class MissionTimeLeft : public CtxCondition
{
public:
    using CtxCondition::CtxCondition;

    static BT::PortsList providedPorts() { return {}; }

    BT::NodeStatus tick() override
    {
        if (ctx().mission_time_s <= 0.0) return BT::NodeStatus::SUCCESS;
        if (ctx().missionElapsedS() < ctx().mission_time_s) return BT::NodeStatus::SUCCESS;

        if (!ctx().guards_enabled) {
            RCLCPP_WARN_THROTTLE(logger(), *ctx().node->get_clock(), 2000,
                                 "MissionTimeLeft would have tripped (guards disabled)");
            return BT::NodeStatus::SUCCESS;
        }
        RCLCPP_ERROR(logger(), "MissionTimeLeft TRIPPED: %.0f s limit reached — aborting to surface",
                     ctx().mission_time_s);
        return BT::NodeStatus::FAILURE;
    }
};

// Stateless "is it on screen right now" — distinct from the debounced Locate.
class ObjectVisible : public CtxCondition
{
public:
    using CtxCondition::CtxCondition;

    static BT::PortsList providedPorts()
    {
        return {
            BT::InputPort<std::string>("camera", "front", "front | down"),
            BT::InputPort<std::string>("object", "class name, comma list, or role_* spec"),
            BT::InputPort<double>("max_age_s", 0.5, "max detection age"),
            BT::InputPort<double>("min_confidence", 0.4, "min detection confidence"),
        };
    }

    BT::NodeStatus tick() override
    {
        const auto camera = expect(getInput<std::string>("camera"), name(), "camera");
        const auto object = expect(getInput<std::string>("object"), name(), "object");
        const double max_age = expect(getInput<double>("max_age_s"), name(), "max_age_s");
        const double min_conf = expect(getInput<double>("min_confidence"), name(), "min_confidence");

        const auto classes = resolveObjectSpec(ctx(), object);
        const auto dets = ctx().freshDetections(camera, classes, max_age, min_conf);
        return dets.empty() ? BT::NodeStatus::FAILURE : BT::NodeStatus::SUCCESS;
    }
};

inline void registerConditionNodes(BT::BehaviorTreeFactory& factory, std::shared_ptr<BTContext> ctx)
{
    factory.registerNodeType<SensorsFreshOK>("SensorsFreshOK", ctx);
    factory.registerNodeType<DepthEnvelopeOK>("DepthEnvelopeOK", ctx);
    factory.registerNodeType<MissionTimeLeft>("MissionTimeLeft", ctx);
    factory.registerNodeType<ObjectVisible>("ObjectVisible", ctx);
}

}  // namespace snappy::bt

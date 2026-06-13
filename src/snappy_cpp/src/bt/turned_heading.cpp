// Turn (StatefulActionNode) Publishes a relative yaw
// command (turn `relative_yaw_deg` from the current heading) and stays RUNNING
// until the controller's heading hold (pid_yaw_) has driven the sub onto the
// new heading, or a timeout elapses.
//
// The controller snapshots its OWN live IMU heading and adds the delta, so the
// turn is referenced to reality at the instant it starts (drift doesn't poison
// later corners). We also snapshot the heading here, but only to know when the
// turn is *done*. The planner's heading copy and the controller's are the same
// /filter/euler signal. yaw-error wrap is what lets the four +90 turns
// cross the +-180 seam the short way.

#include <chrono>

#include "bt_nodes.hpp"
#include "behaviortree_cpp/action_node.h"

namespace {

class Turn : public BT::StatefulActionNode {
public:
    Turn(const std::string & name, const BT::NodeConfig & config)
        : BT::StatefulActionNode(name, config), ctx_(contextFromBlackboard(config)) {}

    static BT::PortsList providedPorts() {
        return {
            BT::InputPort<double>("relative_yaw_deg", 90.0, "turn this many degrees from current heading"),
            BT::InputPort<double>("absolute_yaw_deg", "turn TO this heading (deg); overrides relative_yaw_deg. "
                                                      "Use for turns that must not inherit accumulated error"),
            BT::InputPort<double>("heading_tolerance_deg", 8.0, "succeed within this heading error"),
            BT::InputPort<double>("timeout_s", 8.0, "succeed anyway after this many seconds"),
        };
    }

    BT::NodeStatus onStart() override {
        // Absolute mode: drive TO a fixed heading (e.g. {return_heading} from
        // LockHeading), so relative-turn slop from earlier corners is erased
        // instead of inherited.
        if (auto absolute_yaw_deg = getInput<double>("absolute_yaw_deg")) {
            target_rad_ = wrapToPi(static_cast<float>(*absolute_yaw_deg * DEG2RAD));
            setYawAbs(ctx_, target_rad_);
        } else {
            double relative_yaw_deg = 90.0;
            getInput("relative_yaw_deg", relative_yaw_deg);
            const float delta = static_cast<float>(relative_yaw_deg * DEG2RAD);

            // Tell the controller to turn by delta (it adds to its own heading).
            setYawRel(ctx_, delta);
            // Snapshot the expected target locally, only for the completion check.
            target_rad_ = wrapToPi(ctx_->heading_rad + delta);
        }
        start_ = std::chrono::steady_clock::now();
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override {
        double tolerance_deg = 8.0, timeout = 8.0;
        getInput("heading_tolerance_deg", tolerance_deg);
        getInput("timeout_s", timeout);

        const float err = wrapToPi(target_rad_ - ctx_->heading_rad);
        if (std::fabs(err) <= tolerance_deg * DEG2RAD) {
            return BT::NodeStatus::SUCCESS;
        }
        if (secondsSince(start_) >= timeout) {
            return BT::NodeStatus::SUCCESS;  // timed fallback
        }
        return BT::NodeStatus::RUNNING;
    }

    void onHalted() override {}  // heading target stays set; nothing to undo

private:
    std::shared_ptr<BTContext> ctx_;
    std::chrono::steady_clock::time_point start_;
    float target_rad_ = 0.0f;
};

// LockHeading (SyncActionNode): snapshot the current heading (+ optional
// offset) into a blackboard entry, in degrees. Run it while still aligned with
// the gate so later absolute Turns reference the gate frame instead of
// accumulating relative-turn error (PREQUAL_BT_DESIGN "gate_heading").
class LockHeading : public BT::SyncActionNode {
public:
    LockHeading(const std::string & name, const BT::NodeConfig & config)
        : BT::SyncActionNode(name, config), ctx_(contextFromBlackboard(config)) {}

    static BT::PortsList providedPorts() {
        return {
            BT::InputPort<double>("offset_deg", 0.0, "add this to the captured heading"),
            BT::OutputPort<double>("heading_deg", "captured heading + offset, degrees, wrapped"),
        };
    }

    BT::NodeStatus tick() override {
        double offset_deg = 0.0;
        getInput("offset_deg", offset_deg);
        const float locked = wrapToPi(ctx_->heading_rad + static_cast<float>(offset_deg * DEG2RAD));
        setOutput("heading_deg", static_cast<double>(locked / DEG2RAD));
        return BT::NodeStatus::SUCCESS;
    }

private:
    std::shared_ptr<BTContext> ctx_;
};

}  // namespace

void registerHeadingNodes(BT::BehaviorTreeFactory & factory) {
    factory.registerNodeType<Turn>("Turn");
    factory.registerNodeType<LockHeading>("LockHeading");
}

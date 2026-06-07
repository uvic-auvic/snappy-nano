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
            BT::InputPort<double>("heading_tolerance_deg", 8.0, "succeed within this heading error"),
            BT::InputPort<double>("timeout_s", 8.0, "succeed anyway after this many seconds"),
        };
    }

    BT::NodeStatus onStart() override {
        double relative_yaw_deg = 90.0;
        getInput("relative_yaw_deg", relative_yaw_deg);
        const float delta = static_cast<float>(relative_yaw_deg * DEG2RAD);

        // Tell the controller to turn by delta (it adds to its own heading).
        setYawRel(ctx_, delta);
        // Snapshot the expected target locally, only for the completion check.
        target_rad_ = wrapToPi(ctx_->heading_rad + delta);
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

}  // namespace

void registerHeadingNodes(BT::BehaviorTreeFactory & factory) {
    factory.registerNodeType<Turn>("Turn");
}

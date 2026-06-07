// HoldDepthStable (StatefulActionNode). After Submerge sets the depth
// target, this waits without blocking the tick  until the sub has actually
// settled within tolerance of the target, or a timeout elapses (dead-reckoned
// fallback). Depth hold itself is run continuously by the controller's pid_z_.

#include <chrono>

#include "bt_nodes.hpp"
#include "behaviortree_cpp/action_node.h"

namespace {

class HoldDepthStable : public BT::StatefulActionNode {
public:
    HoldDepthStable(const std::string & name, const BT::NodeConfig & config)
        : BT::StatefulActionNode(name, config), ctx_(contextFromBlackboard(config)) {}

    static BT::PortsList providedPorts() {
        return {
            BT::InputPort<double>("target_depth", 1.2, "depth to settle at, metres"),
            BT::InputPort<double>("tolerance", 0.1, "|depth-target| considered stable, metres"),
            BT::InputPort<double>("timeout_s", 10.0, "succeed anyway after this many seconds"),
        };
    }

    BT::NodeStatus onStart() override {
        start_ = std::chrono::steady_clock::now();
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override {
        double target = 1.2, tolerance = 0.1, timeout = 10.0;
        getInput("target_depth", target);
        getInput("tolerance", tolerance);
        getInput("timeout_s", timeout);

        if (std::fabs(static_cast<double>(ctx_->depth) - target) <= tolerance) {
            return BT::NodeStatus::SUCCESS;
        }
        if (secondsSince(start_) >= timeout) {
            return BT::NodeStatus::SUCCESS;  // timed fallback proceed regardless
        }
        return BT::NodeStatus::RUNNING;
    }

    void onHalted() override {}  // depth target stays set; nothing to undo

private:
    std::shared_ptr<BTContext> ctx_;
    std::chrono::steady_clock::time_point start_;
};

}  // namespace

void registerDepthNodes(BT::BehaviorTreeFactory & factory) {
    factory.registerNodeType<HoldDepthStable>("HoldDepthStable");
}

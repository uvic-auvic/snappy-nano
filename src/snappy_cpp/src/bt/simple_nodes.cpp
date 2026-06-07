// Instantaneous target-set leaves (SyncActionNode). They publish one Task
// and return SUCCESS on the same tick with no waiting.

#include "bt_nodes.hpp"
#include "behaviortree_cpp/action_node.h"

namespace {

    // Publish an absolute depth target. 
    class Submerge : public BT::SyncActionNode {
    public:
        Submerge(const std::string & name, const BT::NodeConfig & config)
            : BT::SyncActionNode(name, config), ctx_(contextFromBlackboard(config)) {}

        static BT::PortsList providedPorts() {
            return {BT::InputPort<double>("depth", 1.2, "absolute depth target, metres (+down)")};
        }

        BT::NodeStatus tick() override {
            double depth = 1.2;
            getInput("depth", depth);
            setDepth(ctx_, depth);
            return BT::NodeStatus::SUCCESS;
        }

    private:
        std::shared_ptr<BTContext> ctx_;
    };

    // End of run: kill surge and command depth 0 (surface). Nothing should latch.
    class Surface : public BT::SyncActionNode {
    public:
        Surface(const std::string & name, const BT::NodeConfig & config)
            : BT::SyncActionNode(name, config), ctx_(contextFromBlackboard(config)) {}

        static BT::PortsList providedPorts() { return {}; }

        BT::NodeStatus tick() override {
            setSurge(ctx_, 0.0);
            setDepth(ctx_, 0.0);
            return BT::NodeStatus::SUCCESS;
        }

    private:
        std::shared_ptr<BTContext> ctx_;
    };

}  // namespace

void registerSimpleNodes(BT::BehaviorTreeFactory & factory) {
    factory.registerNodeType<Submerge>("Submerge");
    factory.registerNodeType<Surface>("Surface");
}

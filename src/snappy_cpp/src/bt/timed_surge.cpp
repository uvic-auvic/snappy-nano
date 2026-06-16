// Surge (StatefulActionNode) the workhorse timed leaf. Used for every
// dead-reckoned straight leg (through the gate, to the marker, each box leg,
// back through the gate): turn surge on, stay RUNNING for `duration_s`, then
// turn surge off. distance ~= surge_thrust x duration_s, tuned pool-side (2.5).
//
// Surge LATCHES in the controller (no firmware watchdog), so onRunning ends by
// commanding 0, and onHalted does the same if the tree aborts mid-leg.

#include <chrono>

#include "bt_nodes.hpp"
#include "behaviortree_cpp/action_node.h"

namespace {

class Surge : public BT::StatefulActionNode {
public:
    Surge(const std::string & name, const BT::NodeConfig & config)
        : BT::StatefulActionNode(name, config), ctx_(contextFromBlackboard(config)) {}

    static BT::PortsList providedPorts() {
        return {
            BT::InputPort<double>("surge_thrust", 40.0, "open-loop forward thrust pct (-100..100)"),
            BT::InputPort<double>("duration_s", 5.0, "how long to surge, seconds"),
        };
    }

    BT::NodeStatus onStart() override {
        double thrust = 40.0;
        getInput("surge_thrust", thrust);
        setSurge(ctx_, thrust);
        start_ = std::chrono::steady_clock::now();
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override {
        double duration = 5.0;
        getInput("duration_s", duration);
        if (secondsSince(start_) >= duration) {
            setSurge(ctx_, 0.0);  // stop — surge would otherwise latch
            return BT::NodeStatus::SUCCESS;
        }
        return BT::NodeStatus::RUNNING;
    }

    void onHalted() override {
        setSurge(ctx_, 0.0);  // aborted mid-leg: do not leave surge latched
    }

private:
    std::shared_ptr<BTContext> ctx_;
    std::chrono::steady_clock::time_point start_;
};

}  // namespace

void registerSurgeNodes(BT::BehaviorTreeFactory & factory) {
    factory.registerNodeType<Surge>("Surge");
}

#ifndef SNAPPY_BT_NODES_HPP
#define SNAPPY_BT_NODES_HPP

// Shared context + Task contract for the pre-qual BehaviorTree.
//
// The leaf nodes do not talk to ROS directly. They are handed a BTContext (via
// the blackboard entry "context") that owns the /planner/task publisher and the
// latest depth / heading the planner has received. Every Task the tree emits is
// defined ONCE here (setDepth / setYawAbs / setSurge) so both ends agree on the
// contract:
//   move/z  (absolute) -> depth hold        (controller pid_z_)
//   move/yaw(absolute) -> heading hold       (controller pid_yaw_, radians)
//   surge              -> open-loop forward   (controller surge_thrust_, signed pct)

#include <chrono>
#include <cmath>
#include <memory>
#include <string>

#include "behaviortree_cpp/bt_factory.h"
#include "rclcpp/rclcpp.hpp"
#include "snappy_cpp/msg/task.hpp"

inline constexpr double DEG2RAD = M_PI / 180.0;

// ROS handles shared with every BT leaf. The planner owns the single instance
// and keeps depth / heading_rad fresh from its subscriptions.
struct BTContext {
    rclcpp::Publisher<snappy_cpp::msg::Task>::SharedPtr task_pub;
    float depth = 0.0f;        // latest depth (m); grows as the sub descends
    float heading_rad = 0.0f;  // latest yaw (rad), from /filter/euler
};

// Wrap an angle into (-pi, pi].
inline float wrapToPi(float a) {
    return std::atan2(std::sin(a), std::cos(a));
}

inline double secondsSince(const std::chrono::steady_clock::time_point & start) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

// Single definition of how a leaf emits a Task. overwrite=true so the newest
// target/command takes effect immediately (the controller holds each axis from
// its stored PID target, independent of the task queue).
inline void publishTask(const std::shared_ptr<BTContext> & ctx, const std::string & type,
                        const std::string & direction, double magnitude, bool absolute) {
    snappy_cpp::msg::Task t;
    t.type = type;
    t.direction = direction;
    t.magnitude = magnitude;
    t.absolute = absolute;
    t.overwrite = true;
    ctx->task_pub->publish(t);
}

inline void setDepth(const std::shared_ptr<BTContext> & ctx, double depth_m) {
    publishTask(ctx, "move", "z", depth_m, /*absolute=*/true);
}
inline void setYawAbs(const std::shared_ptr<BTContext> & ctx, double yaw_rad) {
    publishTask(ctx, "move", "yaw", yaw_rad, /*absolute=*/true);
}
inline void setYawRel(const std::shared_ptr<BTContext> & ctx, double delta_rad) {
    publishTask(ctx, "move", "yaw", delta_rad, /*absolute=*/false);
}
inline void setSurge(const std::shared_ptr<BTContext> & ctx, double thrust_pct) {
    publishTask(ctx, "surge", "forward", thrust_pct, /*absolute=*/true);
}

// Fetch the shared context out of the blackboard. Every leaf calls this in its
// constructor; the planner must set "context" before building the tree.
inline std::shared_ptr<BTContext> contextFromBlackboard(const BT::NodeConfig & config) {
    return config.blackboard->get<std::shared_ptr<BTContext>>("context");
}

// Per-file node registration (definitions live in the matching .cpp).
void registerSimpleNodes(BT::BehaviorTreeFactory & factory);   // Submerge, Surface
void registerDepthNodes(BT::BehaviorTreeFactory & factory);    // HoldDepthStable
void registerSurgeNodes(BT::BehaviorTreeFactory & factory);    // Surge
void registerHeadingNodes(BT::BehaviorTreeFactory & factory);  // Turn

#endif  // SNAPPY_BT_NODES_HPP

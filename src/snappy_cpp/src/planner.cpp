// Planner. Owns the BehaviorTree that sequences
// the pre-qual run, ticks it on a ~10 Hz wall timer, and feeds the leaves the
// ROS context (the /planner/task publisher + the latest depth / heading) via a
// shared BTContext placed on the blackboard. The leaves only set targets,
// toggle surge and time transitions — heading + depth hold run continuously in
// the controller underneath.

#include <chrono>
#include <memory>

#include <ament_index_cpp/get_package_share_directory.hpp>
#include "behaviortree_cpp/bt_factory.h"
#include "geometry_msgs/msg/vector3_stamped.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float32.hpp"

#include "bt/bt_nodes.hpp"

using namespace std::chrono_literals;

class Planner : public rclcpp::Node {
public:
    Planner() : Node("planner") {
        // Shared context handed to every leaf via the blackboard.
        ctx_ = std::make_shared<BTContext>();
        ctx_->task_pub = create_publisher<snappy_cpp::msg::Task>("/planner/task", 10);

        // Keep depth / heading fresh for the condition checks in the leaves.
        depth_sub_ = create_subscription<std_msgs::msg::Float32>(
            "depth_data", 10,
            [this](const std_msgs::msg::Float32 & msg) { ctx_->depth = msg.data; });
        imu_sub_ = create_subscription<geometry_msgs::msg::Vector3Stamped>(
            "/filter/euler", 10,
            [this](const geometry_msgs::msg::Vector3Stamped & msg) {
                // /filter/euler is in degrees; the controller and tree work in radians.
                ctx_->heading_rad = static_cast<float>(msg.vector.z * DEG2RAD);
            });

        std::string bt_path;
        try {
            bt_path = ament_index_cpp::get_package_share_directory("snappy_cpp") +
                      "/bt_trees/prequal.xml";
        } catch (const std::exception & e) {
            RCLCPP_WARN(get_logger(), "Could not get package share directory: %s", e.what());
            bt_path = "bt_trees/prequal.xml";  // fallback to relative path
        }

        BT::BehaviorTreeFactory factory;
        registerSimpleNodes(factory); // <Submerge>, <Surface>
        registerDepthNodes(factory); // <HoldDepthStable>
        registerSurgeNodes(factory); // <Surge>
        registerHeadingNodes(factory); // <Turn>

        auto blackboard = BT::Blackboard::create();
        blackboard->set("context", ctx_);  // MUST precede tree creation

        try {
            tree_ = factory.createTreeFromFile(bt_path, blackboard);
        } catch (const std::exception & e) {
            RCLCPP_FATAL(get_logger(), "Failed to load BT '%s': %s", bt_path.c_str(), e.what());
            throw;
        }
        RCLCPP_INFO(get_logger(), "Planner loaded BT: %s", bt_path.c_str());

        // ~10 Hz, non-blocking: tick once per timer, let RUNNING leaves return.
        tick_timer_ = create_wall_timer(100ms, std::bind(&Planner::tick, this));
    }

private:
    void tick() {
        if (finished_) {
            return;
        }
        const BT::NodeStatus status = tree_.tickOnce();
        if (status != BT::NodeStatus::RUNNING) {
            RCLCPP_INFO(get_logger(), "PreQualRun finished: %s",
                        status == BT::NodeStatus::SUCCESS ? "SUCCESS" : "FAILURE");
            finished_ = true;
        }
    }

    std::shared_ptr<BTContext> ctx_;
    BT::Tree tree_;
    bool finished_ = false;

    rclcpp::Subscription<std_msgs::msg::Float32>::SharedPtr depth_sub_;
    rclcpp::Subscription<geometry_msgs::msg::Vector3Stamped>::SharedPtr imu_sub_;
    rclcpp::TimerBase::SharedPtr tick_timer_;
};

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<Planner>());
    rclcpp::shutdown();
    return 0;
}

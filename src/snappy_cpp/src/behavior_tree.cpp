// Snappy competition behaviour tree — mission planner node.
//
// Sequencing only: this node publishes Task setpoints/toggles on
// /planner/task and never touches /motor_cmd or runs control loops. The
// depth/heading hold and the visual servo live in the controller;
//
// The only three things that can autonomously abort a run and surface the
// sub (all log at ERROR when they do):
//   1. SensorsFreshOK — depth or heading stopped arriving (default > 1.5 s)
//   2. DepthEnvelopeOK — deeper than max_depth_m (default 4 m)
//   3. MissionTimeLeft — run clock, DISABLED unless mission_time_s > 0
// Set the `guards_enabled` parameter false to turn all three into
// warn-only for bench/pool debugging.
//
// Bench test: run scripts/fake_sensors.py and `ros2 topic echo /planner/task`.

#include <memory>
#include <string>
#include <vector>

#include "ament_index_cpp/get_package_share_directory.hpp"
#include "behaviortree_cpp/bt_factory.h"
#include "geometry_msgs/msg/vector3_stamped.hpp"
#include "rclcpp/rclcpp.hpp"
#include "snappy_cpp/msg/detection_array.hpp"
#include "snappy_cpp/msg/task.hpp"
#include "std_msgs/msg/float32.hpp"

#include "bt/actuator_nodes.hpp"
#include "bt/bt_context.hpp"
#include "bt/condition_nodes.hpp"
#include "bt/motion_nodes.hpp"
#include "bt/vision_nodes.hpp"

using namespace std::chrono_literals;
using snappy::bt::BTContext;
using snappy::bt::Clock;

namespace
{

// Reduce a DetectionArray to what the tree consumes. Bearing uses the
// linear-in-FOV approximation (offset fraction x HFOV) — good to a couple of
// degrees inside the central half of the frame, which is all the tree needs.
// Positive bearing = target right of image centre.
std::vector<snappy::bt::DetectionSnapshot> toSnapshots(
    const snappy_cpp::msg::DetectionArray& msg, const snappy::bt::CameraModel& cam)
{
    std::vector<snappy::bt::DetectionSnapshot> out;
    out.reserve(msg.detections.size());
    const auto now = Clock::now();
    for (const auto& det : msg.detections) {
        snappy::bt::DetectionSnapshot s;
        s.object_class = det.object_class;
        s.confidence = det.confidence;
        s.distance_m = det.distance_m;
        const double cx = det.bounding_box.x + det.bounding_box.width / 2.0;
        const double cy = det.bounding_box.y + det.bounding_box.height / 2.0;
        s.norm_off_x = cx / cam.width_px - 0.5;
        s.norm_off_y = cy / cam.height_px - 0.5;
        s.bearing_rad = s.norm_off_x * cam.hfov_rad;
        s.stamp = now;
        out.push_back(std::move(s));
    }
    return out;
}

}  // namespace

class BehaviorTreeNode : public rclcpp::Node
{
public:
    BehaviorTreeNode()
    : rclcpp::Node("behavior_tree")
    {
        ctx_ = std::make_shared<BTContext>();
        ctx_->node = this;
        ctx_->task_pub = create_publisher<snappy_cpp::msg::Task>("/planner/task", 10);

        declareParameters();
        createSubscriptions();

        snappy::bt::registerConditionNodes(factory_, ctx_);
        snappy::bt::registerMotionNodes(factory_, ctx_);
        snappy::bt::registerVisionNodes(factory_, ctx_);
        snappy::bt::registerActuatorNodes(factory_, ctx_);

        // Loading the XML validates every node name and port at startup —
        // fail here on the bench, not mid-run. Default mission lives in
        // bt_trees/mission.xml (installed to the package share dir); the
        // tree_file parameter points at any other mission file.
        auto tree_file = get_parameter("tree_file").as_string();
        if (tree_file.empty()) {
            tree_file = ament_index_cpp::get_package_share_directory("snappy_cpp")
                        + "/bt_trees/mission.xml";
        }
        tree_ = factory_.createTreeFromFile(tree_file);

        start_time_ = Clock::now();
        timer_ = create_wall_timer(100ms, [this]() { tick(); });
        RCLCPP_INFO(get_logger(), "behaviour tree up: role '%s', run clock %s, guards %s, "
                    "start delay %.0f s, tree '%s'",
                    ctx_->role.c_str(),
                    ctx_->mission_time_s > 0 ? "on" : "off",
                    ctx_->guards_enabled ? "armed" : "WARN-ONLY",
                    start_delay_s_, tree_file.c_str());
    }

private:
    void declareParameters()
    {
        // Role is assigned pre-run by the coin flip, so it must always be
        // available as a parameter — vision only refines the gate side.
        ctx_->role = declare_parameter<std::string>("role", "restore");
        if (ctx_->role != "restore" && ctx_->role != "recovery") {
            RCLCPP_ERROR(get_logger(), "invalid role '%s' — defaulting to 'restore'",
                         ctx_->role.c_str());
            ctx_->role = "restore";
        }
        ctx_->default_gate_side = declare_parameter<std::string>("default_gate_side", "left");
        // Run clock disabled by default so a long debug session never
        // surfaces the sub by surprise; competition launches set ~900.
        ctx_->mission_time_s = declare_parameter<double>("mission_time_s", 0.0);
        ctx_->guards_enabled = declare_parameter<bool>("guards_enabled", true);
        start_delay_s_ = declare_parameter<double>("start_delay_s", 0.0);
        declare_parameter<std::string>("tree_file", "");

        // Camera geometry for pixel -> bearing. Defaults match the 848x480
        // colour profiles in snappy_realsense.launch.py; HFOV is the D455
        // (90 deg) / D405 (87 deg) colour spec.
        ctx_->front_cam.hfov_rad =
            declare_parameter<double>("front_hfov_deg", 90.0) * snappy::bt::kDeg2Rad;
        ctx_->front_cam.width_px = declare_parameter<double>("front_image_width", 848.0);
        ctx_->front_cam.height_px = declare_parameter<double>("front_image_height", 480.0);
        ctx_->down_cam.hfov_rad =
            declare_parameter<double>("down_hfov_deg", 87.0) * snappy::bt::kDeg2Rad;
        ctx_->down_cam.width_px = declare_parameter<double>("down_image_width", 848.0);
        ctx_->down_cam.height_px = declare_parameter<double>("down_image_height", 480.0);
    }

    void createSubscriptions()
    {
        depth_sub_ = create_subscription<std_msgs::msg::Float32>(
            "depth_data", 10,
            [this](std_msgs::msg::Float32::SharedPtr m) { ctx_->updateDepth(m->data); });

        // Xsens /filter/euler is DEGREES (XsEuler); radians from here on.
        euler_sub_ = create_subscription<geometry_msgs::msg::Vector3Stamped>(
            "/filter/euler", 10, [this](geometry_msgs::msg::Vector3Stamped::SharedPtr m) {
                ctx_->updateHeading(m->vector.z * snappy::bt::kDeg2Rad);
            });

        front_det_sub_ = create_subscription<snappy_cpp::msg::DetectionArray>(
            "/d455/detections", 10, [this](snappy_cpp::msg::DetectionArray::SharedPtr m) {
                ctx_->updateDetections("front", toSnapshots(*m, ctx_->front_cam));
            });
        down_det_sub_ = create_subscription<snappy_cpp::msg::DetectionArray>(
            "/d405/detections", 10, [this](snappy_cpp::msg::DetectionArray::SharedPtr m) {
                ctx_->updateDetections("down", toSnapshots(*m, ctx_->down_cam));
            });
    }

    void tick()
    {
        if (latched_) {
            // Tree finished: keep the surface targets alive at 1 Hz so the
            // controller holds the surface until the process is stopped.
            if (snappy::bt::secondsSince(last_keepalive_) >= 1.0) {
                snappy::bt::cmdDriveStop(*ctx_);
                snappy::bt::cmdDepth(*ctx_, 0.0);
                last_keepalive_ = Clock::now();
            }
            return;
        }

        // Never command anything before the first depth + heading samples
        // exist (and give the diver start_delay_s to clear).
        if (snappy::bt::secondsSince(start_time_) < start_delay_s_ ||
            !ctx_->depth() || !ctx_->heading()) {
            RCLCPP_INFO_THROTTLE(get_logger(), *get_clock(), 2000,
                                 "waiting to start (delay %.0fs, depth %s, heading %s)",
                                 start_delay_s_, ctx_->depth() ? "ok" : "missing",
                                 ctx_->heading() ? "ok" : "missing");
            return;
        }

        if (!mission_started_) {
            ctx_->startMissionClock();
            mission_started_ = true;
            RCLCPP_INFO(get_logger(), "mission started");
        }

        const auto status = tree_.tickOnce();
        if (status != BT::NodeStatus::RUNNING) {
            latch(status);
        }
    }

    // Once the root finishes, STOP ticking. Re-ticking a finished root
    // restarts the whole tree — after an abort-and-surface, recovered guards
    // would happily re-submerge the sub to re-run the mission it just aborted.
    void latch(BT::NodeStatus status)
    {
        latched_ = true;
        snappy::bt::cmdTrackOff(*ctx_);
        snappy::bt::cmdSpinStopAll(*ctx_);
        snappy::bt::cmdDriveStop(*ctx_);
        snappy::bt::cmdDepth(*ctx_, 0.0);
        last_keepalive_ = Clock::now();
        RCLCPP_WARN(get_logger(), "mission tree finished: %s after %.0f s — holding surface",
                    BT::toStr(status).c_str(), ctx_->missionElapsedS());
    }

    std::shared_ptr<BTContext> ctx_;
    rclcpp::Subscription<std_msgs::msg::Float32>::SharedPtr depth_sub_;
    rclcpp::Subscription<geometry_msgs::msg::Vector3Stamped>::SharedPtr euler_sub_;
    rclcpp::Subscription<snappy_cpp::msg::DetectionArray>::SharedPtr front_det_sub_;
    rclcpp::Subscription<snappy_cpp::msg::DetectionArray>::SharedPtr down_det_sub_;
    BT::BehaviorTreeFactory factory_;
    BT::Tree tree_;
    rclcpp::TimerBase::SharedPtr timer_;

    double start_delay_s_ = 0.0;
    Clock::time_point start_time_{};
    Clock::time_point last_keepalive_{};
    bool mission_started_ = false;
    bool latched_ = false;
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<BehaviorTreeNode>());
    rclcpp::shutdown();
    return 0;
}

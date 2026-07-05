// Controller node for the submarine.
//
// PID-only control (no thruster allocator, no state estimator on this branch).
// Six independent PIDs (x, y, depth, roll, pitch, yaw) each produce a scalar
// thrust in [-100, 100]. timer_callback() mixes those onto the 8 thrusters by
// SUMMING each DOF's contribution (never overwriting), then clamps once and
// publishes a ThrusterCommand on /motor_cmd (mask=255, thrust_pct[8]).
//
// Thruster map (index = bit position, see motorboard.h):
//   0 FRONT_YAW   1 FRONT_RIGHT*  2 FORWARD_RIGHT  3 BACK_RIGHT*
//   4 BACK_YAW    5 BACK_LEFT*    6 FORWARD_LEFT   7 FRONT_LEFT*
//   (* = vertical thruster, shared by depth/roll/pitch)

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <optional>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "std_msgs/msg/float32.hpp"
#include "snappy_cpp/msg/task.hpp"
#include "snappy_cpp/msg/pose.hpp"
#include "snappy_interfaces/msg/thruster_command.hpp"
#include <tf2/LinearMath/Quaternion.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>
#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <queue>

#include "include/Inc/pid.h"
#include "include/Inc/motorboard.h"

using namespace std::chrono_literals;
using std::placeholders::_1;



class Controller : public rclcpp::Node {
    public:
        Controller() : Node("controller"),
            pid_x_(0.0f, 0.0f, 0.0f),
            pid_y_(0.0f, 0.0f, 0.0f),
            pid_z_(0.0f, 0.0f, 0.0f),
            pid_roll_(0.0f, 0.0f, 0.0f),
            pid_pitch_(0.0f, 0.0f, 0.0f),
            pid_yaw_(0.0f, 0.0f, 0.0f)
         {
            //publish motor command
            // Match the STM32 micro-ROS subscription: same type AND best-effort QoS.
            motor_publisher_ = create_publisher<snappy_interfaces::msg::ThrusterCommand>(
                "/motor_cmd", rclcpp::QoS(10).best_effort());
            motorboard_ = std::make_unique<Motor::Motorboard>(motor_publisher_);
            // Publish state status to planner
            status_publisher_ = this->create_publisher<std_msgs::msg::String>("/controller/status", 10);

            // Publish trajectory vectors to state estimator
            trajectory_publisher_ = this->create_publisher<snappy_cpp::msg::Pose>("/controller/trajectory", 10);

            // Receive tasks from planner
            task_subscription_ = this->create_subscription<snappy_cpp::msg::Task>(
                "/planner/task", 10, std::bind(&Controller::task_callback, this, _1));

            // Receive states from state estimator
            state_subscription_ = this->create_subscription<snappy_cpp::msg::Pose>(
                "state_estimator/state", 10, std::bind(&Controller::state_callback, this, _1));

            // Receive dpeth value from pressure node
            depth_subscription_ = this->create_subscription<std_msgs::msg::Float32>(
              "depth_data", 10, std::bind(&Controller::depth_callback, this, _1));

            //get yaw info from the imu
            imu_subscription_ = this->create_subscription<geometry_msgs::msg::Vector3Stamped>(
                "/filter/euler", 10, std::bind(&Controller::imu_callback, this, _1));
            //currently state estimator does not publish state. maybe parse through IMU..

            // imu_d455_subscription_ = this->create_subscription<geometry_msgs::msg::Vector3Stamped>(
            //   "/d455/imu", 10, std::bind(&Controller::imu_d455_subscription_callback, this, _1));

            // Receive DVL pose
            dvl_subscription_ = this->create_subscription<nav_msgs::msg::Odometry>(
                "/waterlinked_dvl_driver/odom", 10, std::bind(&Controller::dvl_callback, this, _1));

            // Publish every 100ms ??
            status_timer_ = this->create_wall_timer(
                100ms, std::bind(&Controller::status_callback, this));
            trajectory_timer_ = this->create_wall_timer(
                100ms, std::bind(&Controller::trajectory_callback, this));


            last_drive_cmd_time_ = this->now();
            timer_ = this->create_wall_timer(10ms, std::bind(&Controller::timer_callback, this));
            RCLCPP_INFO(this->get_logger(), "Timer Node started");

        }

    private:
        // Publish state to planner
        void status_callback() {
            auto static_task_ = snappy_cpp::msg::Task();
            static_task_.type = "hold";
            static_task_.direction = "on";
            static_task_.magnitude = 1;
            static_task_.absolute= false;
            static_task_.overwrite = false;

            auto status_message = std_msgs::msg::String();
            auto [position_error, orientation_error ] = computeError();

            if (position_error == -1 && orientation_error == -1) {
                status_message.data = "No target. Staying static.";
                parseTask(static_task_);
            } else if ((position_error < error_threshold) && (orientation_error < error_threshold)) {
                status_message.data = "Success";
                in_progress_ = false;
            } else {
                status_message.data = "In progress";
            }

            status_publisher_->publish(status_message);
            //uncomment for debugging
            // RCLCPP_INFO(this->get_logger(), "Publishing status: '%s'", status_message.data.c_str());
        }

        // Mix depth/pitch/roll onto the 4 vertical thrusters (indices 1,3,5,7)
        // with strict priority: depth > pitch > roll. Depth is applied first as a
        // common-mode term so it always keeps full authority; pitch, then roll, each
        // receive only the thruster headroom that is left, scaled uniformly so the
        // term stays a pure moment instead of being distorted per-thruster.
        void vertical_thrust_pid(float cmd[8]) {
            // Run the three vertical-axis PIDs. Stored as members so logControlDebug()
            // can print them without re-running update() (which would corrupt PID state).
            z_thrust_     = pid_z_.update(current_depth);
            pitch_thrust_ = pid_pitch_.update(pitch);
            roll_thrust_  = pid_roll_.update(roll);

            // Vertical thrusters in order [FRONT_RIGHT, BACK_RIGHT, BACK_LEFT, FRONT_LEFT].
            const int   idx[4]        = {1, 3, 5, 7};
            // Per-thruster sign of each DOF's contribution (from motorboard.cpp).
            const float z_sign[4]     = {+1.0f, +1.0f, +1.0f, +1.0f};
            const float pitch_sign[4] = {-1.0f, +1.0f, +1.0f, -1.0f};
            const float roll_sign[4]  = {-1.0f, -1.0f, +1.0f, +1.0f};

            // Priority 1: depth. |z_thrust_| <= 100 (PID clamps it) so this always fits.
            float u[4];
            for (int i = 0; i < 4; i++) {
                u[i] = z_thrust_ * z_sign[i];
            }

            // Priority 2: pitch. Priority 3: roll. Each scaled to the leftover headroom.
            addScaledMoment(u, pitch_sign, pitch_thrust_);
            addScaledMoment(u, roll_sign,  roll_thrust_);

            for (int i = 0; i < 4; i++) {
                cmd[idx[i]] = u[i];
            }
        }

        void timer_callback() {
            // Snapshot the latest heading (updated asynchronously by the IMU callback).
            float yaw_master = yaw;

            // Deadman: open-loop surge/sway auto-stop if no fresh "drive" command
            // arrived within drive_timeout_ seconds. Depth + heading keep holding.
            // This is why publishing one "forward" command drives for ~drive_timeout_
            // seconds and then stops on its own.
            if ((this->now() - last_drive_cmd_time_).seconds() > drive_timeout_) {
                open_surge_ = 0.0f;
                open_sway_  = 0.0f;
            }

            // Heading hold: wrap yaw into [-180, 180] so 179 -> -179 reads as a
            // 2 degree error, not 358, then run the yaw PID.
            if (yaw_master < -180) {
                yaw_master += 360;
            } else if (yaw_master > 180) {
                yaw_master -= 360;
            }
            yaw_thrust_ = pid_yaw_.update(yaw_master);

            // Build the per-thruster command by SUMMING each contribution; shared
            // thrusters add up rather than overwrite, then we clamp once at the end.
            float cmd[8] = {0};

            // Vertical group: depth > pitch > roll priority (closed-loop PID).
            vertical_thrust_pid(cmd);

            // Lateral group: open-loop strafe (sway) + closed-loop heading hold (yaw).
            cmd[0] = -open_sway_ - yaw_thrust_; // FRONT_YAW
            cmd[4] =  open_sway_ - yaw_thrust_; // BACK_YAW

            // Forward group: open-loop surge.
            cmd[2] = open_surge_; // FORWARD_RIGHT
            cmd[6] = open_surge_; // FORWARD_LEFT

            int8_t motors[8];
            for (int i = 0; i < 8; i++) {
                motors[i] = clampThrust(cmd[i]);
            }

            motorboard_->sendCmd(255, motors);
            logControlDebug(motors);
        }

        // Human-readable, rate-limited tuning readout. Prints each axis' target,
        // current value, error and thrust output, plus the final motor commands.
        // Throttled to ~4 Hz so the 100 Hz control loop does not flood the console.
        void logControlDebug(const int8_t motors[8]) {
            RCLCPP_INFO_THROTTLE(this->get_logger(), *this->get_clock(), 250,
                "\n[PID] DEPTH tgt=%6.2f cur=%6.2f err=%+6.2f out=%+6.1f"
                "\n      YAW   tgt=%6.1f cur=%6.1f err=%+6.1f out=%+6.1f"
                "\n      ROLL  tgt=%6.1f cur=%6.1f err=%+6.1f out=%+6.1f"
                "\n      PITCH tgt=%6.1f cur=%6.1f err=%+6.1f out=%+6.1f"
                "\n      DRIVE surge=%+6.1f sway=%+6.1f (open-loop)"
                "\n[MTR] FYaw=%4d FR=%4d FwdR=%4d BR=%4d BYaw=%4d BL=%4d FwdL=%4d FL=%4d",
                pid_z_.get_target(),    current_depth, pid_z_.get_target()    - current_depth, z_thrust_,
                pid_yaw_.get_target(),  yaw,           pid_yaw_.get_target()  - yaw,           yaw_thrust_,
                pid_roll_.get_target(), roll,          pid_roll_.get_target() - roll,          roll_thrust_,
                pid_pitch_.get_target(),pitch,         pid_pitch_.get_target()- pitch,         pitch_thrust_,
                open_surge_, open_sway_,
                motors[0], motors[1], motors[2], motors[3],
                motors[4], motors[5], motors[6], motors[7]);
        }


        // Publish trajectory to state estimator
        void trajectory_callback() {
            auto trajectory_msg = snappy_cpp::msg::Pose();

            if (position_target_.has_value() && orientation_target_.has_value()) {
                trajectory_msg.position = *position_target_;
                trajectory_msg.orientation = *orientation_target_;

                trajectory_publisher_->publish(trajectory_msg);
                //uncomment for debugging
                // RCLCPP_INFO(this->get_logger(), "Publishing trajectory: (%.2f, %.2f, %.2f)",
                // position_target_->x, position_target_->y, position_target_->z);
            }
        }

        // Receive tasks from planner node, update PID target
        void task_callback(const snappy_cpp::msg::Task & msg) {
            //uncomment for debugging
            // RCLCPP_INFO(this->get_logger(), "Received task (%s, %s, %f)",
                // msg.type.c_str(), msg.direction.c_str(), msg.magnitude);

            if (msg.overwrite) { // discard the current task(s) and start this one
                std::queue<snappy_cpp::msg::Task> empty;
                std::swap(tasks_, empty);
                current_task_ = msg;
                parseTask(*current_task_);
                return;
            }

            tasks_.push(msg); // put task in queue

            if (!in_progress_ && !tasks_.empty()) { // if done executing, move onto next task
                current_task_ = tasks_.front();
                tasks_.pop();
                parseTask(*current_task_);
            }
        }

        // Receive states from state estimator nodes, update PID values
        void state_callback(const snappy_cpp::msg::Pose & msg) {
            // RCLCPP_INFO(this->get_logger(), "Received current position: (%.2f, %.2f, %.2f)",
            // msg.position.x, msg.position.y, msg.position.z);

            position_current_ = msg.position;
            orientation_current_ = msg.orientation;

            // NOTE: state estimator is disabled for now (buggy, being worked on).
            // These PID .update() calls are commented out because timer_callback is the
            // single owner of PID state. Calling update() here too would corrupt the
            // shared integral / prev_err / dt that the depth/pitch/roll mixing relies on.
            // Re-enable (and remove the duplicate updates from timer_callback) only when
            // state_estimator/state is trusted as the sole state source.
            // tf2::Quaternion q_current;
            // tf2::fromMsg(orientation_current_, q_current);
            // double roll, pitch, yaw;
            // tf2::Matrix3x3(q_current).getRPY(roll, pitch, yaw);
            // float thrust_x = pid_x_.update(position_current_.x);
            // float thrust_y = pid_y_.update(position_current_.y);
            // //float thrust_z = pid_z_.update(position_current_.z);
            // float thrust_roll = pid_roll_.update(roll);
            // float thrust_pitch = pid_pitch_.update(pitch);
            // float thrust_yaw = pid_yaw_.update(yaw);
        }

        void depth_callback(const std_msgs::msg::Float32 & msg) {
            current_depth = msg.data;
            if (depth_first_ == 0) {
                depth_first_ = 1;
                // Hold the depth we started at until the planner commands a new one.
                pid_z_.set_target(current_depth);
            }
        }

        void imu_callback(const geometry_msgs::msg::Vector3Stamped & msg) {
            //RCLCPP_INFO(this->get_logger(), "Received IMU data: roll=%.2f, pitch=%.2f, yaw=%.2f", msg.vector.x, msg.vector.y, msg.vector.z);
            //rollroll
    	    roll = msg.vector.x;
            //pitch
            pitch = msg.vector.y;
            //yaw
            yaw = msg.vector.z;
    	    if (flag_ == 0) {
          		flag_ = 1;
          		// first reference of yaw
          		pid_yaw_.set_target(yaw);
    	    }
        }

        void dvl_callback(const nav_msgs::msg::Odometry & msg) {
            x = msg.pose.pose.position.x;
            y = msg.pose.pose.position.y;

            float orientationX = msg.pose.pose.orientation.x;
            float orientationY = msg.pose.pose.orientation.y;
            float orientationZ = msg.pose.pose.orientation.z;
            float orientationW = msg.pose.pose.orientation.w;

           Eigen::Quaterniond q(orientationW, orientationX, orientationY, orientationZ);
          q.normalize();
          Eigen::Matrix3d R = q.toRotationMatrix();

          Eigen::Vector3d euler = R.eulerAngles(2,1,0);

            yaw = euler[0];
       	    if (dvl_first_ == 0) {
          		dvl_first_ = 1;
          		// first reference of yaw
          		pid_yaw_.set_target(yaw);
          		pid_y_.set_target(y);
          		pid_x_.set_target(x);
            }
        }

        // Compute difference between current and target
        std::pair<double, double> computeError() {
            if (!position_target_.has_value() || !orientation_target_.has_value()) { // if no target received:
                return {-1.0, -1.0};
            }

            tf2::Quaternion q_current;
            tf2::Quaternion q_target;
            tf2::fromMsg(orientation_current_, q_current);
            tf2::fromMsg(*orientation_target_, q_target);

            return {
                // position_error
                (std::sqrt(std::pow(position_current_.x - position_target_->x, 2)
                + std::pow(position_current_.y - position_target_->y, 2)
                + std::pow(position_current_.z - position_target_->z, 2))
                ),
                // orientation_error
                q_current.angleShortestPath(q_target)
            };
        }

        void parseTask(const snappy_cpp::msg::Task & msg) {
            // Open-loop drive: set a surge/sway thrust setpoint directly (percent).
            // Depth and heading keep holding via their PIDs. The setpoint auto-zeros
            // after drive_timeout_ seconds (deadman in timer_callback), so publishing
            // one command drives for ~drive_timeout_ s and then stops.
            if (msg.type == "drive") {
                last_drive_cmd_time_ = this->now();
                float pct = static_cast<float>(msg.magnitude);
                if (msg.direction == "forward") {
                    open_surge_ = pct;
                } else if (msg.direction == "backward") {
                    open_surge_ = -pct;
                } else if (msg.direction == "right") {
                    open_sway_ = pct;
                } else if (msg.direction == "left") {
                    open_sway_ = -pct;
                } else if (msg.direction == "stop") {
                    open_surge_ = 0.0f;
                    open_sway_ = 0.0f;
                } else {
                    RCLCPP_ERROR(this->get_logger(), "Invalid drive direction '%s'", msg.direction.c_str());
                }
                return;
            }

            // If there is no current target:
            if (!position_target_.has_value()) {
                position_target_ = position_current_;
            }
            if (!orientation_target_.has_value()) {
                orientation_target_ = orientation_current_;
            }

            if (msg.type == "move") {
                // Direction commands
                if (msg.direction == "x" || msg.direction == "y" || msg.direction == "z") {
                    // if absolute: target = magnitude,
                    // else: target = magnitude + current_position
                    if (msg.direction == "x") {
                        position_target_->x = msg.absolute ? msg.magnitude : msg.magnitude + position_current_.x;
                        pid_x_.set_target(position_target_->x);
                    } else if (msg.direction == "y") {
                        position_target_->y = msg.absolute ? msg.magnitude : msg.magnitude + position_current_.y;
                        pid_y_.set_target(position_target_->y);
                    } else {
                        // Depth feedback is the pressure sensor (current_depth), not the
                        // disabled state estimator -- use it so "go down X from here"
                        // works. (+magnitude = deeper, assuming depth grows downward.)
                        position_target_->z = msg.absolute ? msg.magnitude : (current_depth + msg.magnitude);
                        pid_z_.set_target(position_target_->z);
                    }

                // Orientation commands
                } else {
                    tf2::Quaternion q_current;
                    tf2::fromMsg(orientation_current_, q_current);
                    tf2::Quaternion q_temp;

                    if (msg.direction == "yaw") {
                        q_temp.setRPY(0, 0, msg.magnitude);
                    } else if (msg.direction == "roll") {
                        q_temp.setRPY(msg.magnitude, 0, 0);
                    } else if (msg.direction == "pitch") {
                        q_temp.setRPY(0, msg.magnitude, 0);
                    } else {
                        RCLCPP_ERROR(this->get_logger(), "Invalid orientation '%s'", msg.direction.c_str());
                    }
                    orientation_target_ = tf2::toMsg(msg.absolute ? q_temp : q_current * q_temp);

                    double roll, pitch, yaw;
                    tf2::Matrix3x3(q_temp).getRPY(roll, pitch, yaw);
                    pid_roll_.set_target(roll);
                    pid_pitch_.set_target(pitch);
                    pid_yaw_.set_target(yaw);
                }
            } else {
                // type = dropper/grabber/torpedo ??
                if (msg.direction == "on") {} else if (msg.direction == "off") {
                } else {
                    RCLCPP_ERROR(this->get_logger(), "Invalid direction '%s'", msg.direction.c_str());
                }
            }
            in_progress_ = true;
        }



        static int8_t clampThrust(float value) {
            return static_cast<int8_t>(std::lround(std::clamp(value, -100.0f, 100.0f)));
        }

        // Add gain*sign[i] to each u[i], but scale the whole term by a single factor
        // alpha in [0,1] so that no |u[i]| exceeds 100. Uniform scaling keeps the term
        // a pure moment; a lower-priority DOF simply gets weaker (or drops to zero) when
        // the higher-priority DOFs have already used up a thruster's headroom.
        static void addScaledMoment(float u[4], const float sign[4], float gain) {
            float alpha = 1.0f;
            for (int i = 0; i < 4; i++) {
                float d = sign[i] * gain;
                if (d == 0.0f) {
                    continue;
                }
                float room = (d > 0.0f) ? (100.0f - u[i]) : (-100.0f - u[i]);
                float a = room / d; // room and d share a sign, so a >= 0
                if (a < alpha) {
                    alpha = a;
                }
            }
            if (alpha < 0.0f) {
                alpha = 0.0f;
            }
            for (int i = 0; i < 4; i++) {
                u[i] += alpha * sign[i] * gain;
            }
        }

        std::unique_ptr<Motor::Motorboard> motorboard_;
        rclcpp::Publisher<snappy_interfaces::msg::ThrusterCommand>::SharedPtr motor_publisher_;
        rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_publisher_;
        rclcpp::Publisher<snappy_cpp::msg::Pose>::SharedPtr trajectory_publisher_;
        rclcpp::Subscription<snappy_cpp::msg::Task>::SharedPtr task_subscription_;
        rclcpp::Subscription<snappy_cpp::msg::Pose>::SharedPtr state_subscription_;
    	rclcpp::Subscription<std_msgs::msg::Float32>::SharedPtr depth_subscription_;
        rclcpp::Subscription<geometry_msgs::msg::Vector3Stamped>::SharedPtr imu_subscription_;
        rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr dvl_subscription_;
    	rclcpp::TimerBase::SharedPtr status_timer_;
        rclcpp::TimerBase::SharedPtr trajectory_timer_;

        PID pid_x_;
        PID pid_y_;
        PID pid_z_;
        PID pid_roll_;
        PID pid_pitch_;
        PID pid_yaw_;

        std::queue<snappy_cpp::msg::Task> tasks_;
        std::optional<snappy_cpp::msg::Task> current_task_;
        bool in_progress_ = false; // set to true in task_callback, reset to false in status_callback

        geometry_msgs::msg::Point position_current_;
        std::optional<geometry_msgs::msg::Point> position_target_;
        geometry_msgs::msg::Quaternion orientation_current_;
        std::optional<geometry_msgs::msg::Quaternion> orientation_target_;
        const double error_threshold = 0.005;

        rclcpp::TimerBase::SharedPtr timer_;

        // First-sample flags: seed each PID target from the first reading it gets.
        int dvl_first_ = 0;
        int depth_first_ = 0;
        int flag_ = 0; // first yaw reference from the IMU

        // Latest sensor values, updated by the subscription callbacks.
        // Initialized to 0 so that before any sensor data arrives, error == target
        // (also 0) and no thrust is commanded -- fail-safe on startup.
        float pitch = 0.0f;
        float yaw = 0.0f;
        float roll = 0.0f;
        float x = 0.0f;
        float y = 0.0f;
        float current_depth = 0.0f;

        // Last PID outputs, cached each cycle for logControlDebug().
        float z_thrust_ = 0.0f;
        float roll_thrust_ = 0.0f;
        float pitch_thrust_ = 0.0f;
        float yaw_thrust_ = 0.0f;

        // Open-loop surge/sway setpoints (percent, -100..100), set by "drive" tasks
        // and applied straight to the forward/lateral thrusters while depth + heading
        // hold. Auto-zeroed by the deadman in timer_callback after drive_timeout_ s.
        float open_surge_ = 0.0f;
        float open_sway_ = 0.0f;
        rclcpp::Time last_drive_cmd_time_;
        const double drive_timeout_ = 3.0; // s; open-loop drive auto-stops if not refreshed
};



int main(int argc, char* argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<Controller>());
    rclcpp::shutdown();
    return 0;
}

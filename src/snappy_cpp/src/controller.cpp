// this file will have all the controller logic for the submarine
// Motor command publisher — publishes Float32MultiArray on /motor_cmd
// data[0] = MotorSelect (0–255, cast to uint8 on the STM32 side) 11111111 (255) = all 8 motors
// data[1] = Speed       (–100.0 to 100.0)

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <optional>
#include <std_msgs/msg/float32_multi_array.hpp>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_msgs/msg/float32.hpp"
#include "snappy_cpp/msg/task.hpp"
#include "snappy_cpp/msg/pose.hpp"
#include "snappy_interfaces/msg/thruster_command.hpp"
#include <tf2/LinearMath/Quaternion.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>
#include <queue>

#include "snappy_cpp/pid.h"
#include "snappy_cpp/motorboard.h"

using namespace std::chrono_literals;
using std::placeholders::_1;



class Controller : public rclcpp::Node {
    public:
        Controller() : Node("controller"),
            pid_x_(0.5f, 0.0f, 0.1f),
            pid_y_(0.5f, 0.0f, 0.1f),
            pid_z_(15.0f, 0.0f, 0.1f),
            pid_roll_(0.5f, 0.0f, 0.1f),
            pid_pitch_(0.5f, 0.0f, 0.1f),
            pid_yaw_(0.5f, 0.0f, 0.1f)
         {
            // count_ = 0;
            flag_ = 0;
            pid_z_.set_target(0);

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

            // Publish every 100ms ??
            status_timer_ = this->create_wall_timer(
                100ms, std::bind(&Controller::status_callback, this));
            trajectory_timer_ = this->create_wall_timer(
                100ms, std::bind(&Controller::trajectory_callback, this));


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

        void timer_callback() {
            count_++;
            // RCLCPP_INFO(this->get_logger(), "Tick #%d", count_);
	    //Depth STUFF
            float depth_master = current_depth;
            RCLCPP_INFO(this->get_logger(), "Depth: %.4f m", depth_master);
            // Update the Z-axis PID with current depth
            float z_thrust = pid_z_.update(depth_master);

            // Apply the thrust to the vertical motors
            const int8_t thrust = clampThrust(z_thrust);
            if (thrust > 0) {
                motorboard_->down(thrust);
                RCLCPP_INFO(this->get_logger(), "Down called: thrust=%d", thrust);
            } else if (thrust < 0) {
                motorboard_->up(thrust);
                RCLCPP_INFO(this->get_logger(), "Up called: thrust=%d", thrust);
            }



	    // YAW stuff
            float roll_master = roll;
            //pitch
            float pitch_master = pitch;
            //yaw
            float yaw_master = yaw;

            //yaw correction
            RCLCPP_INFO(this->get_logger(), "Roll: %.2f, Pitch: %.2f, Yaw: %.2f", roll_master, pitch_master, yaw_master);
            float yaw_thrust = pid_yaw_.update(yaw_master);
            const int8_t yaw_thrust_clamped = clampThrust(yaw_thrust);
            if (yaw_thrust_clamped > 0) {
                motorboard_->yaw_cw(yaw_thrust_clamped);
                RCLCPP_INFO(this->get_logger(), "Yaw CW called: thrust=%d", yaw_thrust_clamped);
            } else if (yaw_thrust_clamped < 0) {
                motorboard_->yaw_ccw(-yaw_thrust_clamped);
                RCLCPP_INFO(this->get_logger(), "Yaw CCW called: thrust=%d", -yaw_thrust_clamped);
            }
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

            // Update PID values
            tf2::Quaternion q_current;
            tf2::fromMsg(orientation_current_, q_current);
            double roll, pitch, yaw;
            tf2::Matrix3x3(q_current).getRPY(roll, pitch, yaw);
            float thrust_x = pid_x_.update(position_current_.x);
            float thrust_y = pid_y_.update(position_current_.y);
            //float thrust_z = pid_z_.update(position_current_.z);
            float thrust_roll = pid_roll_.update(roll);
            float thrust_pitch = pid_pitch_.update(pitch);
            float thrust_yaw = pid_yaw_.update(yaw);
        }

        void depth_callback(const std_msgs::msg::Float32 & msg) {
	    current_depth = msg.data;
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
                        position_target_->z = msg.absolute ? msg.magnitude : msg.magnitude + position_current_.z;
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
            return static_cast<int8_t>(std::lround(std::clamp(value, -5.0f, 5.0f)));
        }

        std::unique_ptr<Motor::Motorboard> motorboard_;
        rclcpp::Publisher<snappy_interfaces::msg::ThrusterCommand>::SharedPtr motor_publisher_;
        rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_publisher_;
        rclcpp::Publisher<snappy_cpp::msg::Pose>::SharedPtr trajectory_publisher_;
        rclcpp::Subscription<snappy_cpp::msg::Task>::SharedPtr task_subscription_;
        rclcpp::Subscription<snappy_cpp::msg::Pose>::SharedPtr state_subscription_;
    	rclcpp::Subscription<std_msgs::msg::Float32>::SharedPtr depth_subscription_;
        rclcpp::Subscription<geometry_msgs::msg::Vector3Stamped>::SharedPtr imu_subscription_;
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

        rclcpp::Publisher<std_msgs::msg::Float32MultiArray>::SharedPtr pub_;
        rclcpp::TimerBase::SharedPtr timer_;

        int flag_;
        int count_;
	float pitch;
	float yaw;
	float roll;
	float current_depth;
};



int main(int argc, char* argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<Controller>());
    rclcpp::shutdown();
    return 0;
}

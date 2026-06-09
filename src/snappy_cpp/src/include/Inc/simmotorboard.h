#ifndef SIMMOTORBOARD_HPP
#define SIMMOTORBOARD_HPP

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <memory>


#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float64.hpp"

namespace rclcpp { class Node; }

namespace Motor {
    class SimMotorboard {
        public:
            using MotorPubPtr = rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr;

            MotorPubPtr FRONT_LEFT;
            MotorPubPtr FRONT_RIGHT;
            MotorPubPtr BACK_RIGHT;
            MotorPubPtr BACK_LEFT;
            MotorPubPtr FRONT_YAW;
            MotorPubPtr BACK_YAW;
            MotorPubPtr FORWARD_RIGHT;
            MotorPubPtr FORWARD_LEFT;

            std::array<MotorPubPtr, 8> ALL;
            std::array<MotorPubPtr, 4> VERTICAL;
            std::array<MotorPubPtr, 2> FORWARD;
            std::array<MotorPubPtr, 2> LATERAL;


            SimMotorboard() = delete;

            explicit SimMotorboard(rclcpp::Node* node){

                FRONT_LEFT = node->create_publisher<std_msgs::msg::Float64>(
                    "/model/auv/joint/thruster_vertical_port_fore_joint/cmd_thrust", 10
                );
                FRONT_RIGHT = node->create_publisher<std_msgs::msg::Float64>(
                    "/model/auv/joint/thruster_vertical_starboard_fore_joint/cmd_thrust", 10
                );
                BACK_LEFT = node->create_publisher<std_msgs::msg::Float64>(
                    "/model/auv/joint/thruster_vertical_port_aft_joint/cmd_thrust", 10
                );
                BACK_RIGHT = node->create_publisher<std_msgs::msg::Float64>(
                    "/model/auv/joint/thruster_vertical_starboard_aft_joint/cmd_thrust", 10
                );
                // POINTED FORWARD AND BACK
                FORWARD_RIGHT = node->create_publisher<std_msgs::msg::Float64>(
                    "/model/auv/joint/thruster_forward_port_joint/cmd_thrust", 10
                );
                FORWARD_LEFT = node->create_publisher<std_msgs::msg::Float64>(
                    "/model/auv/joint/thruster_forward_starboard_joint/cmd_thrust", 10
                );
                // POINTED LEFT AND RIGHT
                FRONT_YAW = node->create_publisher<std_msgs::msg::Float64>(
                    "/model/auv/joint/thruster_lateral_fore_joint/cmd_thrust", 10
                );
                BACK_YAW = node->create_publisher<std_msgs::msg::Float64>(
                    "/model/auv/joint/thruster_lateral_aft_joint/cmd_thrust", 10
                );

                ALL = {
                    FRONT_LEFT,
                    FRONT_RIGHT,
                    BACK_RIGHT,
                    BACK_LEFT,
                    FRONT_YAW,
                    BACK_YAW,
                    FORWARD_RIGHT,
                    FORWARD_LEFT
                };
                
                VERTICAL = {
                    FRONT_LEFT,
                    FRONT_RIGHT,
                    BACK_RIGHT,
                    BACK_LEFT
                };

                FORWARD = {
                    FORWARD_RIGHT,
                    FORWARD_LEFT
                };

                LATERAL = {
                    FRONT_YAW,
                    BACK_YAW
                }; 
            
            }
            // publishes an array of floats to an array of publishers.
            template<size_t N>
            void publish_to_group(const std::array<MotorPubPtr, N>& group, const std::array<double, N>& commands) {
                std_msgs::msg::Float64 msg;
                for (size_t i = 0; i < N; ++i) {
                    msg.data = commands[i];
                    group[i]->publish(msg);
                }
            }

            // Set all motor speeds to 0
            void stop(){
                std_msgs::msg::Float64 msg;
                msg.data = 0;
                for(size_t i = 0; i < ALL.size(); ++i){
                    ALL[i]->publish(msg);
                }
            };

            void forward(double speed){
                std_msgs::msg::Float64 msg;
                msg.data = speed;
                for(size_t i = 0; i < FORWARD.size(); ++i){
                    FORWARD[i]->publish(msg);
                }

            };

            void backward(double speed){
                forward(-speed);
            };

            void right(double speed){
                std_msgs::msg::Float64 msg;
                msg.data = speed;
                for (size_t i = 0; i< LATERAL.size(); ++i){
                    LATERAL[i]->publish(msg);
                }
            };

            void left(double speed){
                right(-speed);
            };

            void up(double speed){
                std_msgs::msg::Float64 msg;
                msg.data = speed;
                for (size_t i = 0; i< VERTICAL.size(); ++i){
                    VERTICAL[i]->publish(msg);
                }
            };

            void down(double speed){
                up(-speed);
            };

            void yaw_cw(double speed){
                std_msgs::msg::Float64 msg;
                msg.data = -speed;
                FRONT_YAW->publish(msg);
                BACK_YAW->publish(msg);
                FORWARD_LEFT->publish(msg);
                msg.data = -msg.data;
                FORWARD_RIGHT->publish(msg);
                
            };

            void yaw_ccw(double speed){
                yaw_cw(speed);
            };

            void roll_left(double speed){
                std_msgs::msg::Float64 msg;
                msg.data = speed;

                FRONT_RIGHT->publish(msg);
                BACK_RIGHT->publish(msg);
                msg.data = -msg.data;
                FRONT_LEFT->publish(msg);
                BACK_LEFT->publish(msg);
            };

            void roll_right(double speed){
                roll_left(-speed);
            };

            void pitch_up(double speed){
                std_msgs::msg::Float64 msg;
                msg.data = speed;

                FRONT_RIGHT->publish(msg);
                FRONT_LEFT->publish(msg);
                msg.data = -msg.data;
                BACK_RIGHT->publish(msg);
                BACK_LEFT->publish(msg);
            };

            void pitch_down(double speed){
                pitch_up(-speed);
            };
    };
};

#endif //MOTORBOARD_HPP

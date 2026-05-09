//
//  movement.hpp
//  
//
//  Created by miniman on 2026-05-09.
//

#pragma once
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/float32_multi_array.hpp>
#include <chrono>

//Motor number/index is just like a clock, (motor 1 starts at the 12:00, moves around clockwise incrementing by 1 for each motor index

namespace MotorCalls {
    constexpr uint8_t ALL          = 255;  // 11111111
    constexpr uint8_t NONE         = 0;

    // Individual motors
    // POINTED UP AND DOWN
    constexpr uint8_t FRONT_LEFT   = 1;    // 00000001
    constexpr uint8_t FRONT_RIGHT  = 4;    // 00000010
    constexpr uint8_t BACK_RIGHT = 16; //00010000
    constexpr uint8_t BACKLEFT = 64; //01000000

    // POINTED LEFT AND RIGHT
    constexpr uint8_t FRONT_YAW = 2; //00000010
    constexpr uint8_t BACK_YAW = 32; //00100000

    // POINTED FORWARD AND BACK
    constexpr uint8_t FORWARD_RIGHT = 8; //00001000
    constexpr uint8_t FORWARD_LEFT = 128; //10000000

    constexpr uint8_t VERTICAL = FRONT_LEFT | FRONT_RIGHT | BACK_RIGHT  | BACK_LEFT;   // 85
    constexpr uint8_t FORWARD = FORWARD_RIGHT | FORWARD_LEFT;        // 136
}

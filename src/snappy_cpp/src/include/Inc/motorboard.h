#ifndef MOTORBOARD_HPP
#define MOTORBOARD_HPP

#include <array>
#include <cstdint>

#include "rclcpp/rclcpp.hpp"
#include "snappy_interfaces/msg/thruster_command.hpp"

namespace MotorCalls {
    constexpr uint8_t ALL          = 255;  // 11111111
    constexpr uint8_t NONE         = 0;

    // Individual motors
    // POINTED UP AND DOWN
    constexpr uint8_t FRONT_LEFT   = 128;    // 00000001
    constexpr uint8_t FRONT_RIGHT  = 2;    // 00000010
    constexpr uint8_t BACK_RIGHT = 8; //00010000
    constexpr uint8_t BACK_LEFT = 32; //01000000

    // POINTED LEFT AND RIGHT
    constexpr uint8_t FRONT_YAW = 1; //00000010
    constexpr uint8_t BACK_YAW = 16; //00100000

    // POINTED FORWARD AND BACK
    constexpr uint8_t FORWARD_RIGHT = 4; //00001000
    constexpr uint8_t FORWARD_LEFT = 64; //10000000

    constexpr uint8_t VERTICAL = FRONT_LEFT | FRONT_RIGHT | BACK_RIGHT  | BACK_LEFT;   // 85
    constexpr uint8_t FORWARD = FORWARD_RIGHT | FORWARD_LEFT;        // 136
    constexpr uint8_t LATERAL = FRONT_YAW | BACK_YAW;                // 34
}

namespace Motor {
    class Motorboard {
        public:

            Motorboard() = delete;

            explicit Motorboard(rclcpp::Publisher<snappy_interfaces::msg::ThrusterCommand>::SharedPtr motor_publisher);

            // Initialize the motorboard
            void on();

            // Uninitialize the motorboard
            void off();

            // Set all motor speeds to 0
            void stop();

            void forward(int8_t speed);

            void backward(int8_t speed);

            void right(int8_t speed);

            void left(int8_t speed);

            void up(int8_t speed);

            void down(int8_t speed);

            void yaw_cw(int8_t speed);

            void yaw_ccw(int8_t speed);

            void roll_left(int8_t speed);

            void roll_right(int8_t speed);

            void pitch_up(int8_t speed);

            void pitch_down(int8_t speed);

            /*
            8 integers:
            0: Front
            1: Back
            2: Left
            3: Right
            4: Front left
            5: Front right
            6: Back left
            7: Back right
            */
            void sendCmd(uint8_t mask, const int8_t speeds[8]);
            void sendCmd(uint8_t mask, int8_t speed);

            void set_leds(const int& led, const int& mode); // Which LED and blink mode

        private:
            rclcpp::Publisher<snappy_interfaces::msg::ThrusterCommand>::SharedPtr motor_publisher_;
    };
}

#endif //MOTORBOARD_HPP

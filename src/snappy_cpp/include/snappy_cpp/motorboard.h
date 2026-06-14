#ifndef MOTORBOARD_HPP
#define MOTORBOARD_HPP

// Host-side abstraction over the 8 thrusters. Each motion primitive (forward,
// down, yaw_cw, ...) builds an 8-element signed-percent array and ships it to
// the STM32 motor board as a ThrusterCommand on /motor_cmd. The board only
// touches the thrusters named in `thruster_mask`, so the three disjoint groups
// (VERTICAL / FORWARD / LATERAL) can be commanded on the same tick without
// clobbering one another — the controller relies on this to stack depth, surge
// and yaw. Index/group/sign conventions are bench-verified in docs/THRUSTER_MAP.md.

#include <array>
#include <cstdint>

#include "rclcpp/rclcpp.hpp"
#include "snappy_interfaces/msg/thruster_command.hpp"

// Thruster select masks, one bit per thruster. Bit i enables thruster index i
// (e.g. FRONT_RIGHT = 1<<1 -> speeds[1]). The constant *names* are nominal
// labels from the firmware header, not verified physical corners; what the
// bench proved is each index's group, direction and sign (docs/THRUSTER_MAP.md).
namespace MotorCalls {
    constexpr uint8_t ALL          = 255;  // every thruster
    constexpr uint8_t NONE         = 0;

    // Vertical group {1,3,5,7}: common command -> heave (set depth/pitch/roll).
    constexpr uint8_t FRONT_RIGHT  = 2;    // speeds[1]
    constexpr uint8_t BACK_RIGHT   = 8;    // speeds[3]
    constexpr uint8_t BACK_LEFT    = 32;   // speeds[5]
    constexpr uint8_t FRONT_LEFT   = 128;  // speeds[7]

    // Lateral group {0,4}, mounted anti-parallel: common command -> yaw,
    // differential -> sway.
    constexpr uint8_t FRONT_YAW    = 1;    // speeds[0]
    constexpr uint8_t BACK_YAW     = 16;   // speeds[4]

    // Surge group {2,6}: common command -> forward/backward drive.
    constexpr uint8_t FORWARD_RIGHT = 4;   // speeds[2]
    constexpr uint8_t FORWARD_LEFT  = 64;  // speeds[6]

    constexpr uint8_t VERTICAL = FRONT_LEFT | FRONT_RIGHT | BACK_RIGHT | BACK_LEFT;  // 170
    constexpr uint8_t FORWARD  = FORWARD_RIGHT | FORWARD_LEFT;                       // 68
    constexpr uint8_t LATERAL  = FRONT_YAW | BACK_YAW;                               // 17
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

            // Publish one ThrusterCommand: `speeds[i]` is the signed percent
            // (-100..100) for thruster index i, `mask` selects which indices the
            // board applies. Index->group map: {1,3,5,7} vertical, {2,6} surge,
            // {0,4} lateral. See docs/THRUSTER_MAP.md.
            void sendCmd(uint8_t mask, const int8_t speeds[8]);

            void set_leds(const int& led, const int& mode); // Which LED and blink mode

        private:
            rclcpp::Publisher<snappy_interfaces::msg::ThrusterCommand>::SharedPtr motor_publisher_;
    };
}

#endif //MOTORBOARD_HPP

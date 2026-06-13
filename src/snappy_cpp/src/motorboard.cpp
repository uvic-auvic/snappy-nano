// Implements the thruster motion primitives declared in motorboard.h. Each one
// maps a single scalar "speed" onto the relevant thruster group and emits one
// ThrusterCommand. Signs and index layout are bench-verified in
// docs/THRUSTER_MAP.md; in particular common +100% on the vertical group is
// heave DOWN, and on the lateral group is yaw COUNTER-CLOCKWISE.
#include "snappy_cpp/motorboard.h"
#include <cstdint>

namespace Motor {

Motorboard::Motorboard(rclcpp::Publisher<snappy_interfaces::msg::ThrusterCommand>::SharedPtr motor_publisher)
    : motor_publisher_(motor_publisher) {}

    // Surge group {2,6} common forward.
    void Motorboard::forward(int8_t speed) {
        int8_t speedArr[8] = {0, 0, speed, 0, 0, 0, speed, 0};
        sendCmd(MotorCalls::FORWARD, speedArr);
    }

    // Surge group {2,6} common reverse.
    void Motorboard::backward(int8_t speed) {
        int8_t negSpeed = static_cast<int8_t>(-speed);
        int8_t speedArr[8] = {0, 0, negSpeed, 0, 0, 0, negSpeed, 0};
        sendCmd(MotorCalls::FORWARD, speedArr);
    }

    // Vertical group {1,3,5,7} common positive = heave down (per bench map).
    void Motorboard::down(int8_t speed) {
        int8_t speedArr[8] = {0, speed, 0, speed, 0, speed, 0, speed};
        sendCmd(MotorCalls::VERTICAL, speedArr);
    }

    // Vertical group common negative = heave up.
    void Motorboard::up(int8_t speed) {
        int8_t negSpeed = static_cast<int8_t>(-speed);
        int8_t speedArr[8] = {0, negSpeed, 0, negSpeed, 0, negSpeed, 0, negSpeed};
        sendCmd(MotorCalls::VERTICAL, speedArr);
    }

    // Lateral group {0,4} differential (opposite signs) = sway right.
    void Motorboard::right(int8_t speed) {
        int8_t negSpeed = static_cast<int8_t>(-speed);
        int8_t speedArr[8] = {negSpeed, 0, 0, 0, speed, 0, 0, 0};
        sendCmd(MotorCalls::LATERAL, speedArr);
    }

    // Lateral group differential, opposite of right() = sway left.
    void Motorboard::left(int8_t speed) {
        int8_t negSpeed = static_cast<int8_t>(-speed);
        int8_t speedArr[8] = {speed, 0, 0, 0, negSpeed, 0, 0, 0};
        sendCmd(MotorCalls::LATERAL, speedArr);
    }

    // Lateral group {0,4} common negative = yaw clockwise (CCW is positive).
    void Motorboard::yaw_cw(int8_t speed) {
        int8_t negSpeed = static_cast<int8_t>(-speed);
        int8_t speedArr[8] = {negSpeed, 0, 0, 0, negSpeed, 0, 0, 0};
        sendCmd(MotorCalls::LATERAL, speedArr);
    }

    // Lateral group common positive = yaw counter-clockwise (per bench map).
    void Motorboard::yaw_ccw(int8_t speed) {
        int8_t speedArr[8] = {speed, 0, 0, 0, speed, 0, 0, 0};
        sendCmd(MotorCalls::LATERAL, speedArr);
    }

    // Vertical group left/right differential = roll left.
    void Motorboard::roll_left(int8_t speed) {
        int8_t negSpeed = static_cast<int8_t>(-speed);
        int8_t speedArr[8] = {0, negSpeed, 0, negSpeed, 0, speed, 0, speed};
        sendCmd(MotorCalls::VERTICAL, speedArr);
    }

    // Vertical group left/right differential, opposite of roll_left().
    void Motorboard::roll_right(int8_t speed) {
        int8_t negSpeed = static_cast<int8_t>(-speed);
        int8_t speedArr[8] = {0, speed, 0, speed, 0, negSpeed, 0, negSpeed};
        sendCmd(MotorCalls::VERTICAL, speedArr);
    }

    // Vertical group front/back differential = pitch nose up.
    void Motorboard::pitch_up(int8_t speed) {
        int8_t negSpeed = static_cast<int8_t>(-speed);
        int8_t speedArr[8] = {0, negSpeed, 0, speed, 0, speed, 0, negSpeed};
        sendCmd(MotorCalls::VERTICAL, speedArr);
    }

    // Vertical group front/back differential, opposite of pitch_up().
    void Motorboard::pitch_down(int8_t speed) {
        int8_t negSpeed = static_cast<int8_t>(-speed);
        int8_t speedArr[8] = {0, speed, 0, negSpeed, 0, negSpeed, 0, speed};
        sendCmd(MotorCalls::VERTICAL, speedArr);
    }

    // All-stop. NOTE: firmware has no watchdog, so the last command latches —
    // callers must send this explicitly to halt (docs/THRUSTER_MAP.md).
    void Motorboard::stop() {
        int8_t speedArr[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        sendCmd(MotorCalls::ALL, speedArr);
    }

    // Pack the speed array + mask into a ThrusterCommand and publish it.
    void Motorboard::sendCmd(uint8_t mask, const int8_t speeds[8]) {
        snappy_interfaces::msg::ThrusterCommand msg{};
        msg.thruster_mask = mask;
        for (size_t i = 0; i < msg.thrust_pct.size(); ++i) {
            msg.thrust_pct[i] = speeds[i];
        }
        motor_publisher_->publish(msg);
    }
} // namespace Motor

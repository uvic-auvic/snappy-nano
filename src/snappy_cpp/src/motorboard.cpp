//cpp file for the embeded code that runs on the motorboard
#include "snappy_cpp/motorboard.h"
#include <cstdint>

namespace Motor {

Motorboard::Motorboard(rclcpp::Publisher<snappy_interfaces::msg::ThrusterCommand>::SharedPtr motor_publisher)
    : motor_publisher_(motor_publisher) {}

    void Motorboard::forward(int8_t speed) {
        // int8_t negSpeed = static_cast<int8_t>(-speed);
        int8_t speedStatic = static_cast<int8_t>(speed);
        int8_t speedArr[8] = {0, 0, speedStatic, 0, 0, 0, speedStatic, 0};
        sendCmd(MotorCalls::FORWARD, speedArr);
    }

    void Motorboard::backward(int8_t speed) {
        int8_t negSpeed = static_cast<int8_t>(-speed);
        // int8_t speedStatic = static_cast<int8_t>(speed);
        int8_t speedArr[8] = {0, 0, negSpeed, 0, 0, 0, negSpeed, 0};
        sendCmd(MotorCalls::FORWARD, speedArr);
    }

    void Motorboard::down(int8_t speed) {
        // int8_t negSpeed = static_cast<int8_t>(-speed);
        int8_t speedStatic = static_cast<int8_t>(speed);
        int8_t speedArr[8] = {0, speedStatic, 0, speedStatic, 0, speedStatic, 0, speedStatic};
        sendCmd(MotorCalls::VERTICAL, speedArr);
    }

    void Motorboard::up(int8_t speed) {
        int8_t negSpeed = static_cast<int8_t>(-speed);
        // int8_t speedStatic = static_cast<int8_t>(speed);
        int8_t speedArr[8] = {0, negSpeed, 0, negSpeed, 0, negSpeed, 0, negSpeed};
        sendCmd(MotorCalls::VERTICAL, speedArr);
    }

    void Motorboard::right(int8_t speed) {
        int8_t negSpeed = static_cast<int8_t>(-speed);
        int8_t speedStatic = static_cast<int8_t>(speed);
        int8_t speedArr[8] = {negSpeed, 0, 0, 0, speedStatic, 0, 0, 0};
        sendCmd(MotorCalls::LATERAL, speedArr);
    }

    void Motorboard::left(int8_t speed) {
        int8_t negSpeed = static_cast<int8_t>(-speed);
        int8_t speedStatic = static_cast<int8_t>(speed);
        int8_t speedArr[8] = {speedStatic, 0, 0, 0, negSpeed, 0, 0, 0};
        sendCmd(MotorCalls::LATERAL, speedArr);
    }

    void Motorboard::yaw_cw(int8_t speed) {
        int8_t negSpeed = static_cast<int8_t>(-speed);
        // int8_t speedStatic = static_cast<int8_t>(speed);
        int8_t speedArr[8] = {negSpeed, 0, 0, 0, negSpeed, 0, 0, 0};
        sendCmd(MotorCalls::LATERAL, speedArr);
    }

    void Motorboard::yaw_ccw(int8_t speed) {
        // int8_t negSpeed = static_cast<int8_t>(-speed);
        int8_t speedStatic = static_cast<int8_t>(speed);
        int8_t speedArr[8] = {speedStatic, 0, 0, 0, speedStatic, 0, 0, 0};
        sendCmd(MotorCalls::LATERAL, speedArr);
    }

    void Motorboard::roll_left(int8_t speed) {
        int8_t negSpeed = static_cast<int8_t>(-speed);
        int8_t speedStatic = static_cast<int8_t>(speed);
        int8_t speedArr[8] = {0, negSpeed, 0, negSpeed, 0, speedStatic, 0, speedStatic};
        sendCmd(MotorCalls::VERTICAL, speedArr);
    }

    void Motorboard::roll_right(int8_t speed) {
        int8_t negSpeed = static_cast<int8_t>(-speed);
        int8_t speedStatic = static_cast<int8_t>(speed);
        int8_t speedArr[8] = {0, speedStatic, 0, speedStatic, 0, negSpeed, 0, negSpeed};
        sendCmd(MotorCalls::VERTICAL, speedArr);
    }

    void Motorboard::pitch_up(int8_t speed) {
        int8_t negSpeed = static_cast<int8_t>(-speed);
        int8_t speedStatic = static_cast<int8_t>(speed);
        int8_t speedArr[8] = {0, negSpeed, 0, speedStatic, 0, speedStatic, 0, negSpeed};
        sendCmd(MotorCalls::VERTICAL, speedArr);
    }

    void Motorboard::pitch_down(int8_t speed) {
        int8_t negSpeed = static_cast<int8_t>(-speed);
        int8_t speedStatic = static_cast<int8_t>(speed);
        int8_t speedArr[8] = {0, speedStatic, 0, negSpeed, 0, negSpeed, 0, speedStatic};
        sendCmd(MotorCalls::VERTICAL, speedArr);
    }

    void Motorboard::stop() {
        int8_t speedArr[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        sendCmd(MotorCalls::ALL, speedArr);
    }

    void Motorboard::sendCmd(uint8_t mask, const int8_t speeds[8]) {
        snappy_interfaces::msg::ThrusterCommand msg{};
        msg.thruster_mask = mask;
        for (size_t i = 0; i < msg.thrust_pct.size(); ++i) {
            msg.thrust_pct[i] = speeds[i];
        }
        motor_publisher_->publish(msg);
    }
} // namespace Motor

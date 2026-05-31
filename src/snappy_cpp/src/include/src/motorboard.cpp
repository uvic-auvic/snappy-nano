//cpp file for the embeded code that runs on the motorboard
#include "Inc/motorboard.h"

namespace Motor {

Motorboard::Motorboard(rclcpp::Publisher<snappy_cpp::msg::ThrusterCommand>::SharedPtr motor_publisher)
    : motor_publisher_(motor_publisher) {}

void Motorboard::forward(int8_t speed) {
    sendCmd(MotorCalls::FORWARD, speed);
}

void Motorboard::backward(int8_t speed) {
    sendCmd(MotorCalls::FORWARD, static_cast<int8_t>(-speed));
}

void Motorboard::down(int8_t speed) {
    sendCmd(MotorCalls::VERTICAL, static_cast<int8_t>(-speed));
}

void Motorboard::up(int8_t speed) {
    sendCmd(MotorCalls::VERTICAL, speed);
}

void Motorboard::right(int8_t speed) {
    sendCmd(MotorCalls::LATERAL, speed);
}

void Motorboard::left(int8_t speed) {
    sendCmd(MotorCalls::LATERAL, static_cast<int8_t>(-speed));
}

void Motorboard::yaw_cw(int8_t speed) {
    sendCmd(MotorCalls::BACK_YAW, speed);
    sendCmd(MotorCalls::FRONT_YAW, static_cast<int8_t>(-speed));
}

void Motorboard::yaw_ccw(int8_t speed) {
    sendCmd(MotorCalls::BACK_YAW, static_cast<int8_t>(-speed));
    sendCmd(MotorCalls::FRONT_YAW, speed);
}

void Motorboard::roll_left(int8_t speed) {
    sendCmd(MotorCalls::BACK_LEFT, speed);
    sendCmd(MotorCalls::BACK_RIGHT, static_cast<int8_t>(-speed));
    sendCmd(MotorCalls::FRONT_LEFT, speed);
    sendCmd(MotorCalls::FRONT_RIGHT, static_cast<int8_t>(-speed));
}

void Motorboard::roll_right(int8_t speed) {
    sendCmd(MotorCalls::BACK_LEFT, static_cast<int8_t>(-speed));
    sendCmd(MotorCalls::BACK_RIGHT, speed);
    sendCmd(MotorCalls::FRONT_LEFT, static_cast<int8_t>(-speed));
    sendCmd(MotorCalls::FRONT_RIGHT, speed);
}

void Motorboard::pitch_up(int8_t speed) {
    sendCmd(MotorCalls::BACK_LEFT, speed);
    sendCmd(MotorCalls::BACK_RIGHT, speed);
    sendCmd(MotorCalls::FRONT_LEFT, static_cast<int8_t>(-speed));
    sendCmd(MotorCalls::FRONT_RIGHT, static_cast<int8_t>(-speed));
}

void Motorboard::pitch_down(int8_t speed) {
    sendCmd(MotorCalls::BACK_LEFT, static_cast<int8_t>(-speed));
    sendCmd(MotorCalls::BACK_RIGHT, static_cast<int8_t>(-speed));
    sendCmd(MotorCalls::FRONT_LEFT, speed);
    sendCmd(MotorCalls::FRONT_RIGHT, speed);
}

void Motorboard::stop() {
    sendCmd(MotorCalls::ALL, static_cast<int8_t>(0));
}

void Motorboard::sendCmd(uint8_t mask, const int8_t speeds[8]) {
    snappy_cpp::msg::ThrusterCommand msg{};
    msg.thruster_mask = mask;
    for (size_t i = 0; i < msg.thrust_pct.size(); ++i) {
        msg.thrust_pct[i] = speeds[i];
    }
    motor_publisher_->publish(msg);
}

void Motorboard::sendCmd(uint8_t mask, int8_t speed) {
    int8_t speeds[8] = {};
    for (size_t i = 0; i < 8; ++i) {
        const auto bit = static_cast<uint8_t>(1u << i);
        if (mask & bit) {
            speeds[i] = speed;
        }
    }
    sendCmd(mask, speeds);
}

} // namespace Motor

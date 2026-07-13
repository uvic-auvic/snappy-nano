#include "Inc/pid.h"

PID::PID(float Kp, float Ki, float Kd) {
    this->Kp_ = Kp;
    this->Ki_ = Ki;
    this->Kd_ = Kd;
    this->target_ = 0.0f;
    this->integral_ = 0.0f;
    this->prev_err_ = 0.0f;
    this->first_update_ = true;
    this->prev_time_ = std::chrono::steady_clock::now();
    this->MIN = -100.0f;
    this->MAX = 100.0f;
}

void PID::set_target(float target) {
    target_ = target;
    integral_ = 0; // Reset integral
}

float PID::update(float current) {
    // Get error
    float err = target_ - current;

    // Get time
    auto cur_time = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(cur_time - prev_time_).count();
    if (dt < 0.001f) { // Protect against division by 0
        dt = 0.001f;
    }
    prev_time_ = cur_time;

    // First call: prev_time_ is construction time and prev_err_ is 0, so dt can be
    // seconds and the derivative sees a step — P only, no I/D transient.
    if (first_update_) {
        first_update_ = false;
        prev_err_ = err;
        float output = Kp_ * err;
        if (output < MIN) {
            output = MIN;
        } else if (output > MAX) {
            output = MAX;
        }
        return output;
    }

    // Proportional term
    float p_term = Kp_ * err;

    // Integral term. The clamp is on i_term (post-Ki), so integral authority is
    // the same fraction of output for every axis regardless of gain; integral_ is
    // back-calculated to the bound so it unwinds immediately when the error flips.
    integral_ += err * dt;
    float i_term = Ki_ * integral_;
    if (Ki_ != 0.0f) {
        if (i_term < MIN) {
            i_term = MIN;
            integral_ = MIN / Ki_;
        } else if (i_term > MAX) {
            i_term = MAX;
            integral_ = MAX / Ki_;
        }
    }

    // Derivative term (on-error)
    float d_term = Kd_ * ((err - prev_err_) / dt);
    prev_err_ = err;

    // Clamp output
    float output = p_term + i_term + d_term;
    if (output < MIN) {
        output = MIN;
    } else if (output > MAX) {
        output = MAX;
    }

    return output;
}

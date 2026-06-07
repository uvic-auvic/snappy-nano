#ifndef PID_H
#define PID_H

#include <chrono>

class PID {
    private:
        float Kp_;
        float Ki_;
        float Kd_;
        float target_;
        float integral_;
        float prev_err_;
        std::chrono::steady_clock::time_point prev_time_;
        float MIN;
        float MAX;
        bool angular_ = false; // when true, wrap the error into (-pi, pi]

    public:
        PID(float Kp, float Ki, float Kd); // Constructor
        void set_target(float target);
        // Treat the controlled variable as an angle in radians: the error is
        // wrapped into (-pi, pi] so the controller always takes the short way
        // around the +-pi seam (e.g. heading hold while circling a marker).
        void set_angular(bool angular);
        float update(float current); // Return the magnitude of movement
};

#endif
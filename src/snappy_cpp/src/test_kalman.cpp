// Test program for Kalman Filter
// Reads IMU data from CSV and tests prediction and update steps

#include "kalman.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <cmath>
#include <algorithm>

using namespace Eigen;

// load CSV data
std::vector<VectorXd> loadIMUData(const std::string& filename, int cols) {
    std::vector<VectorXd> data;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Error: Could not open " << filename << std::endl;
        return data;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string value;
        VectorXd row(cols);
        
        int i = 0;
        while (std::getline(ss, value, ',') && i < cols) {
            try {
                row(i) = std::stod(value);
                i++;
            } catch (...) {
                // Skip invalid lines
                break;
            }
        }
        
        if (i == cols) {  // Only add complete rows
            data.push_back(row);
        }
    }
    
    file.close();
    std::cout << "Loaded " << data.size() << " rows from " << filename << std::endl;
    return data;
}

int main(int argc, char** argv) {

    // ------------------------------------------------------------------
    // Load CSV data
    // imu1: [accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z]  (camera frame)
    // imu2: [accel_x, accel_y, accel_z, quat_x, quat_y, quat_z, quat_w] (optional)
    // ------------------------------------------------------------------
    std::string imu1_file;
    std::string imu2_file;

    if (argc >= 2) {
        imu1_file = argv[1];
    }
    if (argc >= 3) {
        imu2_file = argv[2];
    }

    std::vector<VectorXd> imu1_data = loadIMUData(imu1_file, 6);
    if (imu1_data.empty()) {
        std::cerr << "Error: Missing IMU1 data. Provide a valid imu1 CSV path." << std::endl;
        return 1;
    }

    std::vector<VectorXd> imu2_data;
    bool use_imu2 = false;
    if (!imu2_file.empty()) {
        imu2_data = loadIMUData(imu2_file, 7);
        use_imu2 = !imu2_data.empty();
        if (!use_imu2) {
            std::cerr << "Warning: IMU2 file could not be loaded. Running prediction-only with IMU1." << std::endl;
        }
    } else {
        std::cout << "No IMU2 file provided. Running prediction-only with IMU1." << std::endl;
    }

    // ------------------------------------------------------------------
    // Initialize filter
    // ------------------------------------------------------------------

    // Seed orientation from IMU2 if available, else use identity.
    Quaterniond q_init = Quaterniond::Identity();
    if (use_imu2) {
        // IMU2 format: [accel(0:2), quat_x(3), quat_y(4), quat_z(5), quat_w(6)]
        const VectorXd& first_imu2 = imu2_data[0];
        q_init = Quaterniond(first_imu2(6), first_imu2(3), first_imu2(4), first_imu2(5));
        q_init.normalize();
    }

    VectorXd x0 = VectorXd::Zero(16);
    x0(6) = q_init.w();
    x0(7) = q_init.x();
    x0(8) = q_init.y();
    x0(9) = q_init.z();

    // P0: initial error-state covariance (15x15)
    // Layout: pos(0-2), vel(3-5), orientation_err(6-8), gyro_bias(9-11), accel_bias(12-14)
    MatrixXd P0 = MatrixXd::Identity(15, 15);
    P0.block<3,3>(0,0)   *= 0.05;  // position uncertainty
    P0.block<3,3>(3,3)   *= 0.1;   // velocity uncertainty
    P0.block<3,3>(6,6)   *= 0.01;  // orientation error uncertainty
    P0.block<3,3>(9,9)   *= 1.0;   // gyro bias uncertainty
    P0.block<3,3>(12,12) *= 1.0;   // accel bias uncertainty

    // Q: process noise covariance (12x12)
    // Layout matches G columns: accel(0-2), gyro(3-5), gyro_bias(6-8), accel_bias(9-11)
    MatrixXd Q = MatrixXd::Identity(12, 12);
    Q.block<3,3>(0,0) *= 0.1;    // accel noise
    Q.block<3,3>(3,3) *= 0.01;   // gyro noise
    Q.block<3,3>(6,6) *= 1e-8;   // gyro bias random walk
    Q.block<3,3>(9,9) *= 1e-6;   // accel bias random walk

    MatrixXd R_imu2  = MatrixXd::Identity(4, 4) * 0.01;  // IMU2 orientation noise
    MatrixXd R_depth = MatrixXd::Identity(1, 1) * 0.05;  // depth sensor noise

    KalmanFilter kf(x0, P0, Q, R_imu2, R_depth, Quaterniond::Identity());

    // ------------------------------------------------------------------
    // Replay loop
    // ------------------------------------------------------------------
    const double dt = 0.005;  // 200 Hz
    size_t num_steps = use_imu2 ? std::min(imu1_data.size(), imu2_data.size()) : imu1_data.size();
    std::cout << "Running " << num_steps << " steps at dt=" << dt << "\n\n";

    std::ofstream output("filter_output.csv");
    output << "step,pos_x,pos_y,pos_z,vel_x,vel_y,vel_z,quat_w,quat_x,quat_y,quat_z\n";

    for (size_t i = 0; i < num_steps; i++) {
        // IMU1: raw [accel(3), gyro(3)] in camera frame
        VectorXd U = imu1_data[i];

        kf.predict(U, dt);
        if (use_imu2) {
            // IMU2: orientation quaternion from AHRS
            const VectorXd& m2 = imu2_data[i];
            Quaterniond quat_imu2(m2(6), m2(3), m2(4), m2(5));
            quat_imu2.normalize();
            kf.updateIMU2(quat_imu2);
        }

        Vector3d pos = kf.getPosition();
        Vector3d vel = kf.getVelocity();
        Quaterniond q = kf.getOrientation();

        output << i
               << "," << pos.x() << "," << pos.y() << "," << pos.z()
               << "," << vel.x() << "," << vel.y() << "," << vel.z()
               << "," << q.w()   << "," << q.x()   << "," << q.y()   << "," << q.z() << "\n";

        // Print every 100 steps
        if (i % 100 == 0) {
            std::cout << "Step " << i << "/" << num_steps << "\n"
                      << "  Position: [" << pos.transpose() << "]\n"
                      << "  Velocity: [" << vel.transpose() << "]\n"
                      << "  Quat:     [" << q.w() << ", " << q.x() << ", " << q.y() << ", " << q.z() << "]\n\n";
        }
    }

    output.close();
    std::cout << "Results saved to filter_output.csv\n";

    std::cout << "\n=== FINAL STATE ===\n"
              << "Position: [" << kf.getPosition().transpose() << "]\n"
              << "Velocity: [" << kf.getVelocity().transpose() << "]\n";
    Quaterniond fq = kf.getOrientation();
    std::cout << "Quat:     [" << fq.w() << ", " << fq.x() << ", " << fq.y() << ", " << fq.z() << "]\n";

    return 0;
}


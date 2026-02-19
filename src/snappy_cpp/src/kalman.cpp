// Kalman Filter Implementation

#include "kalman.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>

using namespace Eigen;

KalmanFilter::KalmanFilter()
{
    // Default constructor - initialize with default values
    // TODO: Set reasonable defaults for AUV application
}
//x0: initial state
//P0: initial covariance
//Q: process noise covariance
//R_imu1: imu1 measurement noise covariance
//R_imu2: imu2 measurement noise covariance
//R_depth: depth sensor measurement noise covariance
// x: state vector
// P_: covariance matrix
// Q_: process noise covariance
// R_imu2_: measurement noise covariance for imu2
// R_depth_: measurement noise covariance for depth sensor

KalmanFilter::KalmanFilter(VectorXd x0Input, MatrixXd P0Input, MatrixXd QInput,
                           MatrixXd R_imu2Input, MatrixXd R_depthInput)
{
    // Initialize state and covariance with provided values
    // Normalize the quaternion part of initial state
    x = x0Input;
    P = P0Input;
    Q = QInput;
    R_imu2 = R_imu2Input;
    R_depth = R_depthInput;


    normalizeQuaternion();
    std::cout << "Kalman Filter initialized with " << x.size() << " states" << std::endl;
}


// EKF predict/update equations based on:
// https://en.wikipedia.org/wiki/Extended_Kalman_filter#Discrete-time_predict_and_update_equations
// Todo: add bias states for accel and gyro, and include in prediction/update steps
void KalmanFilter::predict(Eigen::VectorXd& U, double dt)
{
    // State: [pos(3), vel(3), quat(4), gyro_bias(3), accel_bias(3)]
    Vector3d position = x.segment<3>(0);
    Vector3d velocity = x.segment<3>(3);
    Quaterniond q(x(6), x(7), x(8), x(9));
    q.normalize();

    Vector3d gyro_bias = x.segment<3>(10);
    Vector3d accel_bias = x.segment<3>(13);

    // Control input
    Vector3d accel_body = U.segment<3>(0) - accel_bias;
    Vector3d gyro = U.segment<3>(3) - gyro_bias;

    // Convert acceleration to world frame and remove gravity
    // IMU accelerometer measures specific force (includes gravity).
    // Assumes q is body->world.
    Vector3d accel_world = q * accel_body - gravity;

    // Integrate position & velocity
    position += velocity * dt + 0.5 * accel_world * dt * dt;
    velocity += accel_world * dt;

    // Integrates gyroscope angular velocity into the quaternion using a first-order approximation (small angle assumption)
    // For body->world quaternion with body-frame gyro: q_dot = 0.5 * q ⊗ omega
    Quaterniond dq(
        1.0,
        0.5 * gyro.x() * dt,
        0.5 * gyro.y() * dt,
        0.5 * gyro.z() * dt
    );

    q = q * dq;
    q.normalize();

    // Write back state
    x.segment<3>(0) = position;
    x.segment<3>(3) = velocity;
    x.segment<4>(6) << q.w(), q.x(), q.y(), q.z();

    
    //  Jacobian
    MatrixXd F = computeF(dt);
    P = F * P * F.transpose() + Q;
}


void KalmanFilter::updateIMU2(const Quaterniond& quat_measured)
{
    // Measurement: quaternion only (4x1)
    Quaterniond q_meas = quat_measured.normalized();

    // Measurement model H (4x16): quaternion at indices 6-9
    MatrixXd H = MatrixXd::Zero(4, 16);
    H.block<4,4>(0, 6) = Matrix4d::Identity();

    // Predicted measurement from current state
    Quaterniond q_state(x(6), x(7), x(8), x(9));
    q_state.normalize();

    // Ensure shortest path (q and -q represent same rotation)
    if (q_state.dot(q_meas) < 0.0) {
        q_meas.coeffs() *= -1.0;
    }

    VectorXd z_pred(4);
    z_pred << q_state.w(), q_state.x(), q_state.y(), q_state.z();

    VectorXd z_meas(4);
    z_meas << q_meas.w(), q_meas.x(), q_meas.y(), q_meas.z();

    update(z_meas, H, R_imu2);
    normalizeQuaternion();
}


void KalmanFilter::updateDepth(double depth_measured)
{
    //JUST TESTING IMUS right now, implement depth later
    
    std::cout << "UpdateDepth called with depth=" << depth_measured << std::endl;
}

void KalmanFilter::updateVelocity(const Vector3d& v_measured, const Matrix3d& R_vel)
{
    // Measurement: velocity only (3x1)
    VectorXd z(3);
    z << v_measured.x(), v_measured.y(), v_measured.z();

    // Measurement model H (3x16): velocity at indices 3-5
    MatrixXd H = MatrixXd::Zero(3, 16);
    H.block<3,3>(0, 3) = Matrix3d::Identity();

    update(z, H, R_vel);
}

void KalmanFilter::update(const VectorXd& z, const MatrixXd& H, const MatrixXd& R)
{
    VectorXd z_pred = H * x;
    VectorXd y = z - z_pred;
    // Innovation covariance
    MatrixXd S = H * P * H.transpose() + R;
    // Near-optimal Kalman gain
    MatrixXd K = P * H.transpose() * S.inverse();
    // Update state estimate
    x = x + K * y;

    MatrixXd I = MatrixXd::Identity(x.size(), x.size());
    // Update estimate covariance
    P = (I - K * H) * P;
}

// ============================================================================
// GETTERS
// ============================================================================

Vector3d KalmanFilter::getPosition() const
{
    return x.segment<3>(0);
}

Vector3d KalmanFilter::getVelocity() const
{
    return x.segment<3>(3);
}

Quaterniond KalmanFilter::getOrientation() const
{
    // State stores quaternion as [w, x, y, z] at indices 6-9
    return Quaterniond(x(6), x(7), x(8), x(9));
}

void KalmanFilter::reset(const VectorXd& x0, const MatrixXd& P0)
{
    x = x0;
    P = P0;
    normalizeQuaternion();
    
    std::cout << "Kalman Filter reset" << std::endl;
}


void KalmanFilter::normalizeQuaternion()
{
    // Extract quaternion from state
    Vector4d quat = x.segment<4>(6);
    
    // Normalize
    double norm = quat.norm();
    if (norm > 1e-6) {
        x.segment<4>(6) = quat / norm;
    } else {
        // If quaternion is near zero, reset to identity
        x(6) = 1.0;  // w
        x(7) = 0.0;  // x
        x(8) = 0.0;  // y
        x(9) = 0.0;  // z
    }
}

VectorXd KalmanFilter::computeInnovation(const VectorXd& z_measured, const VectorXd& z_predicted)
{
    // TODO: Compute innovation (measurement residual)
    // For quaternions, need special handling (shortest path on sphere)
    // For now, simple subtraction
    return z_measured - z_predicted;
}

//P=FPF'  + Q
MatrixXd KalmanFilter::computeF(double dt) const
{
    MatrixXd F = MatrixXd::Identity(16, 16);
    F.block<3,3>(0,3) = Matrix3d::Identity() * dt;  // p depends on v
    return F;
}

// ============================================================================
// CSV I/O FOR TESTING
// ============================================================================

// Helper function to save state to CSV file
void saveStateToCSV(const std::string& filename, const VectorXd& state, double timestamp)
{
    std::ofstream file(filename, std::ios::app);
    if (file.is_open()) {
        file << timestamp;
        for (int i = 0; i < state.size(); ++i) {
            file << "," << state(i);
        }
        file << std::endl;
        file.close();
    }
}

// Helper function to load measurements from CSV file
MatrixXd loadCSV(const std::string& filename)
{
    std::ifstream file(filename);
    std::vector<std::vector<double>> data;
    
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::vector<double> row;
            std::string value;
            
            while (std::getline(ss, value, ',')) {
                row.push_back(std::stod(value));
            }
            data.push_back(row);
        }
        file.close();
    }
    
    // Convert to Eigen matrix
    if (data.empty()) return MatrixXd();
    
    int rows = data.size();
    int cols = data[0].size();
    MatrixXd matrix(rows, cols);
    
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            matrix(i, j) = data[i][j];
        }
    }
    
    return matrix;
}

MatrixXd KalmanFilter::openData(const std::string& fileToOpen)
{
    return loadCSV(fileToOpen);
}

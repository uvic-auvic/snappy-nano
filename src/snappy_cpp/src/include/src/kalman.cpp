// Kalman Filter Implementation
// Using an Error-state Extended Kalman Filter 
// https://notanymike.github.io/Error-State-Extended-Kalman-Filter/
/*
Note that this does not work due to dead-reckoning drift from the IMU. 
position and velcoity estimate drifts
A DVL should fix this (there are functions "update_velocity" for when we get a DVL)
 */
// Good paper for quaternion EKF: https://www.iri.upc.edu/people/jsola/JoanSola/objectes/notes/kinematics.pdf


/*
All releivite to the sub's local frame, not the world frame.

Position: 
Postive x is always forward in refernce to the sub
Postive y is always to the right of the sub
Postive z is always down from the sub

*/


#include "kalman.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>

using namespace Eigen;

KalmanFilter::KalmanFilter()
{
    // Default constructor with zero state and identity covariances
    x = VectorXd::Zero(16);
    P = MatrixXd::Identity(15, 15);
    Q = MatrixXd::Identity(12, 12);
    R_imu2 = MatrixXd::Identity(3, 3);
    R_depth = MatrixXd::Identity(1, 1);
}

KalmanFilter::KalmanFilter(VectorXd x0Input, MatrixXd P0Input, MatrixXd QInput,
                           MatrixXd R_imu2Input, MatrixXd R_depthInput,
                           Quaterniond q_imu1_to_body)
{
    x = x0Input;
    P = P0Input;
    Q = QInput;
    R_imu2 = (R_imu2Input.rows() == 4) ? R_imu2Input.block<3,3>(1,1) : R_imu2Input;
    R_depth = R_depthInput;
    q_imu1_to_body_ = q_imu1_to_body.normalized();

    normalizeQuaternion();
    std::cout << "Kalman Filter initialized with " << x.size() << " states" << std::endl;
}



// Math from Sola 5,6
// Sola Eq. (248–252)
void KalmanFilter::predict(Eigen::VectorXd& U, double dt)
{
    //U : raw IMU measurements in the camera frame: [accel(3), gyro(3)]


    // Rotate raw IMU1 (camera frame) measurements into the filter body frame.
    Vector3d accel_raw = U.segment<3>(0);
    Vector3d gyro_raw  = U.segment<3>(3);
    Vector3d accel_in = q_imu1_to_body_ * accel_raw;
    Vector3d gyro_in  = q_imu1_to_body_ * gyro_raw;
    VectorXd U_body(6);
    U_body << accel_in, gyro_in;

    // State x: [pos(3), vel(3), quat(4), gyro_bias(3), accel_bias(3)]
    Vector3d position = x.segment<3>(0);
    Vector3d velocity = x.segment<3>(3);
    Quaterniond q(x(6), x(7), x(8), x(9));
    q.normalize();

    Vector3d gyro_bias = x.segment<3>(10);
    Vector3d accel_bias = x.segment<3>(13);

    // Control input
    Vector3d accel_body = U_body.segment<3>(0) - accel_bias;
    Vector3d gyro = U_body.segment<3>(3) - gyro_bias;

    // Convert acceleration to world frame and remove gravity
    Vector3d accel_world = q * accel_body;
    accel_world -= gravity;

    // Integrate position & velocity
    position += velocity * dt + 0.5 * accel_world * dt * dt;

    velocity += accel_world * dt;

    // Integrates gyroscope angular velocity into the quaternion using a first-order approximation (small angle assumption)
    //  q_dot = 0.5 * q ⊗ omega
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

    
    // Error-state Jacobian
    MatrixXd F = computeF(dt, U);


    MatrixXd Phi = MatrixXd::Identity(15, 15) + F * dt;
   // MatrixXd Phi = (F * dt).exp(); // using matrix exponential


    // Solà 7.3 prediction step — covariance propagation:
    //Qd : new uncertainty
    //   P = Phi * P * Phi^T + G * Q * G^T * dt
    MatrixXd G = computeG();
    MatrixXd Qd = G * Q * G.transpose() * dt;

    P = Phi * P * Phi.transpose() + Qd;
    P = 0.5 * (P + P.transpose());  // fixes floating point rounding error
}


void KalmanFilter::updateIMU2(const Quaterniond& quat_measured)
{
    Quaterniond q_meas = quat_measured.normalized();
    Quaterniond q_state(x(6), x(7), x(8), x(9));
    q_state.normalize();

    // Sola 7.4.2
    Quaterniond q_err = q_meas * q_state.conjugate();
    if (q_err.w() < 0.0) {
        q_err.coeffs() *= -1.0;
    }

    Vector3d residual = 2.0 * q_err.vec();

    // Only affects orientation error states (indices 6-8)
    MatrixXd H = MatrixXd::Zero(3, 15);
    H.block<3,3>(0, 6) = Matrix3d::Identity();

    updateErrorState(residual, H, R_imu2);
}


void KalmanFilter::updateDepth(double depth_measured)
{
    // Simple measurement: depth directly observes z-position state x(2)
    VectorXd residual(1);
    residual << (depth_measured - x(2));

    MatrixXd H = MatrixXd::Zero(1, 15);
    H(0, 2) = 1.0;

    updateErrorState(residual, H, R_depth);
}

// For when we get a DVL
void KalmanFilter::updateVelocity(const Vector3d& v_measured, const Matrix3d& R_vel)
{
    // Measurement: velocity only (3x1)
    VectorXd residual(3);
    residual = v_measured - x.segment<3>(3);

    // Measurement model H (3x15): dv at indices 3-5
    MatrixXd H = MatrixXd::Zero(3, 15);
    H.block<3,3>(0, 3) = Matrix3d::Identity();

    updateErrorState(residual, H, R_vel);
}


// Solà 7.3 "The error-state Kalman filter" — update step
void KalmanFilter::updateErrorState(const VectorXd& residual, const MatrixXd& H, const MatrixXd& R)
{
    // Solà 7.3 innovation covariance: S = H*P*H^T + R
    // Total uncertainty in the residual = filter uncertainty + sensor noise
    MatrixXd S = H * P * H.transpose() + R;

    // Solà 7.3 Kalman gain: K = P*H^T * S^-1
    // Weights how much to trust the measurement vs. the current estimate
    MatrixXd K = P * H.transpose() * S.inverse();

    // Solà 7.3 error state correction: dx = K * residual
    // Then inject into nominal state (additive for pos/vel/bias, multiplicative for quaternion)
    VectorXd dx = K * residual;
    injectErrorState(dx);

    // Solà 7.3 Joseph-form covariance update: P = (I-KH)*P*(I-KH)^T + K*R*K^T
    MatrixXd I = MatrixXd::Identity(15, 15);
    MatrixXd I_KH = I - K * H;
    P = I_KH * P * I_KH.transpose() + K * R * K.transpose();
    P = 0.5 * (P + P.transpose());  // fixes rounding errors
}


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

void KalmanFilter::setIMU2MeasurementNoise(const MatrixXd& R_imu2Input)
{
    R_imu2 = (R_imu2Input.rows() == 4) ? R_imu2Input.block<3,3>(1,1) : R_imu2Input;
}

void KalmanFilter::setDepthMeasurementNoise(const MatrixXd& R_depthInput)
{
    R_depth = R_depthInput;
}

void KalmanFilter::setProcessNoise(const MatrixXd& QInput)
{
    Q = QInput;
}

void KalmanFilter::setInitialCovariance(const MatrixXd& P0Input)
{
    P = P0Input;
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
    Vector4d quat = x.segment<4>(6);
    double norm = quat.norm();
    if (norm > 1e-9) {
        x.segment<4>(6) = quat / norm;
    } else {
        x(6) = 1.0;
        x(7) = 0.0;
        x(8) = 0.0;
        x(9) = 0.0;
    }
}

//  F : Jacobian of the error-state transition function
MatrixXd KalmanFilter::computeF(double dt, const VectorXd& U) const
{
    (void)dt;
    MatrixXd F = MatrixXd::Zero(15, 15);

    Quaterniond q(x(6), x(7), x(8), x(9));
    q.normalize();
    Matrix3d Rwb = q.toRotationMatrix();

    Vector3d accel_bias = x.segment<3>(13);
    // Apply same IMU1->body rotation as predict() so the Jacobian is consistent.
    Vector3d accel_in = q_imu1_to_body_ * U.segment<3>(0);
    Vector3d accel_body = accel_in - accel_bias;
    Vector3d gyro_unbiased = q_imu1_to_body_ * U.segment<3>(3) - x.segment<3>(10);
    //Eq 270-274 from Sola 7.3.1
    F.block<3,3>(0, 3) = Matrix3d::Identity();       
    F.block<3,3>(3, 6) = -Rwb * skew(accel_body);   
    F.block<3,3>(3, 12) = -Rwb;                    
    F.block<3,3>(6, 6) = -skew(gyro_unbiased);    
    F.block<3,3>(6, 9) = -Matrix3d::Identity();    

    return F;
}
// G maps process noise into the error-state space
MatrixXd KalmanFilter::computeG() const
{
    MatrixXd G = MatrixXd::Zero(15, 12);

    Quaterniond q(x(6), x(7), x(8), x(9));
    q.normalize();
    Matrix3d Rwb = q.toRotationMatrix();
 // Eq 276
    G.block<3,3>(3, 0) = Rwb;
    G.block<3,3>(6, 3) = Matrix3d::Identity();
    G.block<3,3>(9, 6) = Matrix3d::Identity();
    G.block<3,3>(12, 9) = Matrix3d::Identity();

    return G;
}
// Helper for computeF
Matrix3d KalmanFilter::skew(const Vector3d& v) const
{
    Matrix3d S;
    S <<     0.0, -v.z(),  v.y(),
          v.z(),    0.0, -v.x(),
         -v.y(),  v.x(),   0.0;
    return S;
}

// updates the nominal state with the error-state correction dx
void KalmanFilter::injectErrorState(const VectorXd& dx)
{
    x.segment<3>(0) += dx.segment<3>(0);
    x.segment<3>(3) += dx.segment<3>(3);

    Vector3d dtheta = dx.segment<3>(6);
    Quaterniond dq(
        1.0, 
        0.5 * dtheta.x(), 
        0.5 * dtheta.y(), 
        0.5 * dtheta.z()
    );

    Quaterniond q(x(6), x(7), x(8), x(9));
    q = q * dq;
    q.normalize();
    x.segment<4>(6) << q.w(), q.x(), q.y(), q.z();

    x.segment<3>(10) += dx.segment<3>(9);
    x.segment<3>(13) += dx.segment<3>(12);
}


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

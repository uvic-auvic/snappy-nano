// Kalman Filter header file
// Fuses two IMUs and depth sensor to estimate position, velocity, and orientation

#ifndef KALMANFILTER_H
#define KALMANFILTER_H

#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <string>

using namespace Eigen;

class KalmanFilter
{
public:
    
    KalmanFilter();

    // - x0: Initial state [pos(3), vel(3), quat(4), gyro_bias(3), accel_bias(3)] = 16x1
    // - P0: Initial covariance matrix 16x16
    // - Q: Process noise covariance 16x16
    // - R_imu2: IMU2 measurement noise covariance (quaternion)
    // - R_depth: Depth sensor measurement noise covariance
   KalmanFilter(VectorXd x0Input, MatrixXd P0Input, MatrixXd QInput,
                           MatrixXd R_imu2Input, MatrixXd R_depthInput);


    void predict(Eigen::VectorXd& U, double dt);


    void updateIMU2(const Quaterniond& quat_measured);

    // Update step: correct state estimate using depth sensor
    void updateDepth(double depth_measured);

    // Update step: correct velocity estimate (e.g., zero-velocity update)
    void updateVelocity(const Vector3d& v_measured, const Matrix3d& R_vel);

    // For testing using csv files
    static MatrixXd openData(const std::string& fileToOpen);

    // Getters for state vector components
    Vector3d getPosition() const;
    Vector3d getVelocity() const;
    Quaterniond getOrientation() const;

    // Get full state vector
    VectorXd getState() const { return x; }
    
    // Get covariance matrix
    MatrixXd getCovariance() const { return P; }

    // Reset/reinitialize the filter
    void reset(const VectorXd& x0, const MatrixXd& P0);

private:
    // State vector: [pos(3), vel(3), quat(4), gyro_bias(3), accel_bias(3)] = 16x1
    // Indices: [0-2: position, 3-5: velocity, 6-9: quaternion(w,x,y,z), 10-12: gyro_bias, 13-15: accel_bias]
    VectorXd x;

    // Covariance matrix 16x16
    MatrixXd P;

    // Process noise covariance 16x16
    MatrixXd Q;

    // Measurement noise covariances
    MatrixXd R_imu2;  // For IMU 2 (accel + gyro + orientation)
    MatrixXd R_depth; // For depth sensor

    // Gravity vector 
    const Vector3d gravity = Vector3d(0, 0, 9.81);

    // Helper function: compute state transition Jacobian F
    MatrixXd computeF(double dt) const;

    // Helper function: normalize quaternion in state vector
    void normalizeQuaternion();

    // Helper function: compute innovation (measurement residual)
    VectorXd computeInnovation(const VectorXd& z_measured, const VectorXd& z_predicted);

    // Generic measurement update: x, P <- update(z, H, R)
    void update(const VectorXd& z, const MatrixXd& H, const MatrixXd& R);
};

#endif // KALMANFILTER_H

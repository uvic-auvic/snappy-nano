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
    // - Q: Process noise covariance 16x16 Smaller the Q = more trust in imu
    // - R_imu2: IMU2 measurement noise covariance (quaternion)
    // - R_depth: Depth sensor measurement noise covariance
    // - q_imu1_to_body: rotation from IMU1 (camera) frame to the filter body frame (IMU2/AHRS frame).
    KalmanFilter(VectorXd x0Input, MatrixXd P0Input, MatrixXd QInput,
                 MatrixXd R_imu2Input, MatrixXd R_depthInput,
                 Quaterniond q_imu1_to_body = Quaterniond::Identity());


    void predict(Eigen::VectorXd& U, double dt);


    void updateIMU2(const Quaterniond& quat_measured);

    // Update step: correct state estimate using depth sensor
    void updateDepth(double depth_measured);

    // Update step: correct velocity estimate (e.g., zero-velocity update if we know we're stationary, or if we have something to measure velo)
    // Not currently used
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

    // Setters for noise covariances and initial conditions
    void setIMU2MeasurementNoise(const MatrixXd& R_imu2Input);
    void setDepthMeasurementNoise(const MatrixXd& R_depthInput);
    void setProcessNoise(const MatrixXd& QInput);
    void setInitialCovariance(const MatrixXd& P0Input);
    void setIMU1ToBodyRotation(const Quaterniond& q_imu1_to_body);



    // Reset/reinitialize the filter
    void reset(const VectorXd& x0, const MatrixXd& P0);

private:
    // State vector
    VectorXd x;

    // Error-state covariance matrix
    MatrixXd P;

    // Process noise covariance 
    MatrixXd Q;

    // Measurement noise covariances

    MatrixXd R_imu2;  
    MatrixXd R_depth;
    //MatrixXd R_vel;

    // Rotation from IMU1 (camera) body frame to the filter body frame (AHRS/IMU2 frame).
    // Defaults to identity; set by initializeFromStaticData().
    Quaterniond q_imu1_to_body_ = Quaterniond::Identity();

    // IMU2 orientation reference used to define a local world frame at startup:
    // +x forward, +y right, +z down relative to the sub at t0.
    Quaterniond q_imu2_ref_ = Quaterniond::Identity();
    bool imu2_ref_initialized_ = false;

    // Gravity vector 
    const Vector3d gravity = Vector3d(0, 0, 9.81);

    // Computes error-state transition Jacobian F 
    MatrixXd computeF(double dt, const VectorXd& U) const;

    // Computes process noise mapping G (15x12)
    MatrixXd computeG() const;

    // Normalize quaternion in state vector
    void normalizeQuaternion();

    // Helper function: skew-symmetric matrix from 3-vector
    Matrix3d skew(const Vector3d& v) const;

    // Inject error-state correction into nominal state
    void injectErrorState(const VectorXd& dx);

    // Generic ESKF measurement update with residual and error-state Jacobian H
    void updateErrorState(const VectorXd& residual, const MatrixXd& H, const MatrixXd& R);

};

#endif // KALMANFILTER_H

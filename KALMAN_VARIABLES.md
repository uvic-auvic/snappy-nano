# Kalman Filter Variables Documentation

This document explains all the key variables and matrices used in the error-state extended Kalman filter (ESEKF) implementation.

---

## State Vector (`x`) — 16 elements

The full state being estimated:

```
x = [pos(3), vel(3), quat(4), gyro_bias(3), accel_bias(3)]
    [0-2]    [3-5]   [6-9]    [10-12]       [13-15]
```

| Index | Variable | Units | Description | Range |
|-------|----------|-------|-------------|-------|
| 0-2 | Position | m | [x, y, z] in world frame (x=forward, y=right, z=down at t=0) | Unbounded |
| 3-5 | Velocity | m/s | [vx, vy, vz] in world frame | ±5 m/s (typical ROV) |
| 6-9 | Orientation Quaternion | - | [w, x, y, z] representing rotation from world to body frame | Normalized: norm = 1 |
| 10-12 | Gyro Bias | rad/s | Constant bias in gyroscope measurements | ±0.01 rad/s (typical) |
| 13-15 | Accel Bias | m/s² | Constant bias in accelerometer measurements | ±0.5 m/s² (typical) |

**Initialization** (state_estimator.cpp, lines 73-82):
- Position, velocity: zero
- Quaternion: identity [1, 0, 0, 0]
- Biases: loaded from calibration data

---

## Error-State Vector (implicit) — 15 elements

The Kalman filter actually works with the **error state**, not the full state:

```
dx = [dpos(3), dvel(3), dtheta(3), d_gyro_bias(3), d_accel_bias(3)]
     [0-2]     [3-5]    [6-8]      [9-11]          [12-14]
```

- **dpos, dvel, d_gyro_bias, d_accel_bias**: Additive corrections
- **dtheta**: Small angle rotation error (converted to quaternion correction internally)

---

## Covariance Matrices

### `P` — Error-state covariance (15×15)

**What it does**: Quantifies uncertainty in each error state. Diagonal elements = variance, off-diagonal = correlations.

**Range**: 0 to ∞ (typically 0.001 to 10)

**Initialization** (state_estimator.cpp, lines 84-89):
```cpp
P_init.block<3,3>(0,0)   *= 0.05;   // position uncertainty (high = don't trust initial position)
P_init.block<3,3>(3,3)   *= 0.1;    // velocity uncertainty
P_init.block<3,3>(6,6)   *= 0.01;   // orientation error uncertainty (low = high confidence)
P_init.block<3,3>(9,9)   *= 0.01;   // gyro bias uncertainty
P_init.block<3,3>(12,12) *= 0.01;   // accel bias uncertainty
```

**Evolution**: Updated in predict() and update() steps
- **Predict**: `P = Phi * P * Phi^T + Qd` (increases uncertainty)
- **Update**: `P = (I - KH) * P * (I - KH)^T + K*R*K^T` (decreases uncertainty)

**Interpretation**:
- **Small P**: High confidence in estimate
- **Large P**: Low confidence, filter will trust measurements more

---

### `R_imu2` — IMU2 measurement noise (3×3)

**What it does**: Measurement noise covariance for orientation updates from IMU2.

**Range**: Small positive values (0.001 to 1.0)

**Default** (state_estimator.cpp, line 99):
```cpp
MatrixXd R_imu2_init = MatrixXd::Identity(3, 3) * 0.01;
```
- **Meaning**: Assume ±0.1 rad (±5.7°) uncertainty on each orientation measurement
- **Effect**: Higher value → trust sensor less, rely more on dead-reckoning

---

### `R_depth` — Depth sensor measurement noise (1×1)

**What it does**: Measurement noise covariance for depth (z-position) updates from pressure sensor.

**Range**: Small positive values (0.001 to 1.0)

**Default** (state_estimator.cpp, line 100):
```cpp
MatrixXd R_depth_init = MatrixXd::Identity(1, 1) * 0.05;
```
- **Meaning**: Assume ±0.22 m (±22 cm) uncertainty on depth
- **Effect**: Higher value → trust depth less, rely more on IMU integration

---

### `Q` — Process noise (12×12)

**What it does**: Uncertainty in the physical process model (how much IMU noise corrupts the state over time).

**Range**: Small positive values (0.001 to 1.0 per time step)

**Initialization** (state_estimator.cpp, lines 92-97):
```cpp
Q_init.block<3,3>(0,0) *= 0.1;    // accel noise
Q_init.block<3,3>(3,3) *= 0.1;    // gyro noise
Q_init.block<3,3>(6,6) *= 0.01;   // gyro bias random walk
Q_init.block<3,3>(9,9) *= 0.01;   // accel bias random walk
```

**Structure** (mapped by G in kalman.cpp):
```
Q = [accel_noise(3), gyro_noise(3), gyro_bias_walk(3), accel_bias_walk(3)]
```

**Interpretation**:
- **Higher Q**: Assume model is unreliable, don't trust predictions, update from measurements more
- **Lower Q**: Assume model is accurate, trust predictions, update less aggressively
- **Tuning**: Run with sensor on stationary surface, compute standard deviation of measurements

---

## Measurement & Jacobian Matrices

### `H` — Measurement matrix (varies by update)

**What it does**: Maps error states to measurement space. Defines which states each sensor observes.

**Range**: Binary (0 or 1) or continuous

**Examples**:

#### IMU2 Orientation Update (kalman.cpp, line 164):
```cpp
MatrixXd H = MatrixXd::Zero(3, 15);
H.block<3,3>(0, 6) = Matrix3d::Identity();
```
- **3 rows**: 3 orientation measurements (error quaternion x,y,z)
- **Column 6-8**: Only affects orientation error states
- **Meaning**: "We measure orientation, nothing else"

#### Depth Update (kalman.cpp, lines 172-173):
```cpp
MatrixXd H = MatrixXd::Zero(1, 15);
H(0, 2) = 1.0;
```
- **1 row**: 1 depth measurement
- **Column 2**: Only affects z-position
- **Meaning**: "We measure depth (z), nothing else"

#### Velocity Update (kalman.cpp, lines 187):
```cpp
MatrixXd H = MatrixXd::Zero(3, 15);
H.block<3,3>(0, 3) = Matrix3d::Identity();
```
- **3 rows**: 3 velocity measurements
- **Column 3-5**: Only affects velocity states
- **Meaning**: "We measure velocity, nothing else"

---

### `G` — Process noise mapping (15×12)

**What it does**: Maps process noise (Q) into error-state space during prediction.

**Range**: Binary (0 or 1)

**Code** (kalman.cpp, lines 274-284):
```cpp
MatrixXd G = MatrixXd::Zero(15, 12);
G.block<3,3>(3, 0) = -Rwb;              // accel noise affects velocity
G.block<3,3>(6, 3) = -1 * Matrix3d::Identity();  // gyro noise affects orientation
G.block<3,3>(9, 6) = Matrix3d::Identity();   // gyro bias random walk
G.block<3,3>(12, 9) = Matrix3d::Identity();  // accel bias random walk
```

**Structure** (maps 12-dim process noise to 15-dim error state):
```
G[3-5, 0-2]   = -Rotation matrix (accel noise → velocity error)
G[6-8, 3-5]   = -Identity (gyro noise → orientation error)
G[9-11, 6-8]  = Identity (gyro bias walk)
G[12-14, 9-11] = Identity (accel bias walk)
```

**Interpretation**: "This is how random sensor noise corrupts our estimates"

---

### `F` — Error-state transition Jacobian (15×15)

**What it does**: Linearized dynamics. How error states evolve during the predict step.

**Code** (kalman.cpp, lines 248-266):
```cpp
F.block<3,3>(0, 3) = Matrix3d::Identity();       // pos changes with velocity
F.block<3,3>(3, 6) = -Rwb * skew(accel_body);   // vel changes with orientation (gravity rotates)
F.block<3,3>(3, 12) = -Rwb;                    // vel affected by accel bias
F.block<3,3>(6, 6) = -skew(gyro_unbiased);    // orientation affected by gyro
F.block<3,3>(6, 9) = -Matrix3d::Identity();    // orientation affected by gyro bias
```

**Key insight**: F shows how errors propagate:
- Position error → velocity error (over time)
- Gyro bias error → orientation error (over time)
- Orientation error → velocity error (gravity points in wrong direction)

---

## Quaternion (`q_imu1_to_body_`)

**What it does**: Transformation from IMU1 (camera) frame to IMU2 (body) frame.

**Range**: Normalized quaternion (norm = 1)

**Value** (state_estimator.cpp, lines 103-109):
```cpp
const Quaterniond q_imu1_to_body(
    0.5,   // w
   -0.5,   // x
    0.5,   // y
   -0.5    // z
);
```

**Interpretation**: Represents a 90° rotation between the two IMUs

**Use**: Converts accelerometer/gyro from camera frame to body frame in predict() (kalman.cpp, lines 53-56)

---

## Kalman Gain (`K`)

**What it does**: Weights how much to trust measurements vs. current estimate.

**Computed in**: updateErrorState() (kalman.cpp, line 201)
```cpp
MatrixXd K = P * H.transpose() * S.inverse();
```

**Range**: 0 to 1 (dimensionally: varies by state/measurement)

**Interpretation**:
- **K near 0**: Don't trust measurement, stay with estimate
- **K near 1**: Trust measurement heavily, adjust estimate a lot
- **K = P·H^T / (H·P·H^T + R)**
  - High P → high K (uncertain estimate, trust sensor)
  - High R → low K (uncertain sensor, trust estimate)

---

## Innovation Covariance (`S`)

**What it does**: Total uncertainty in the measurement (filter uncertainty + sensor noise).

**Computed in**: updateErrorState() (kalman.cpp, line 198)
```cpp
MatrixXd S = H * P * H.transpose() + R;
```

**Range**: Positive semi-definite matrix

**Interpretation**: `S = H·P·H^T + R`
- First term: Uncertainty from filter (H projects P to measurement space)
- Second term: Sensor noise (R)
- Used to compute Kalman gain: K = P·H^T·S^-1

---

## Summary Table

| Variable | Dimension | Type | Higher Value Means | Tuning Notes |
|----------|-----------|------|-------------------|--------------|
| P | 15×15 | Covariance | Less confident | Start high, decrease as sensors improve |
| Q | 12×12 | Process noise | Less trust in model | Tune from stationary sensor noise data |
| R_imu2 | 3×3 | Measurement noise | Sensor less reliable | Tune from IMU2 spec or empirical test |
| R_depth | 1×1 | Measurement noise | Depth sensor less reliable | Tune from pressure sensor spec |
| H | var×15 | Selection matrix | (Binary) | Define which states each sensor observes |
| G | 15×12 | Noise mapping | (Binary) | Maps Q to error state space |
| F | 15×15 | Dynamics Jacobian | (Normalized) | Shows error propagation |
| K | var×15 | Kalman gain | Trust measurements more | Computed automatically |
| S | var×var | Innovation cov | Measurement more uncertain | Computed automatically |

---

## Typical Tuning Workflow

1. **Start conservative**: High P, low-moderate Q and R
2. **Run on stationary surface**: Measure gyro/accel noise → set Q
3. **Run on stable motion**: Check if estimate drifts → increase Q if needed
4. **Use known reference**: Compare with ground truth depth → tune R_depth
5. **Monitor Kalman gain K**: If always near 1, increase R (don't trust sensor); if always near 0, increase Q (don't trust model)

---

## References

- **ESEKF theory**: Solà, J. (2015). "Quaternion kinematics for the error-state Kalman filter"
  https://www.iri.upc.edu/people/jsola/JoanSola/objectes/notes/kinematics.pdf
- **IMU integration**: Dead-reckoning drift accumulates without external references
- **DVL future**: Velocity updates from Doppler velocity log will constrain drift

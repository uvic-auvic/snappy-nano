# Known Issues — Controls / Estimation / Mission Stack

Audit date: 13 July 2026, at merge commit `5e0e15a` (branch `finalBehaviorTree` = `main` + behaviour tree).
(Note: the mission XML has since moved from `src/bt/mission_tree.hpp` to `bt_trees/mission.xml` —
`mission_tree.hpp` line references below map to the same XML elements there.)
Method: multi-agent code review with independent verification passes; every finding below was
confirmed against the actual source lines (line numbers are as of `5e0e15a`). The behaviour-tree
section was additionally exercised by an end-to-end bench run (`scripts/fake_sensors.py`).

**Severity key**
- 🔴 **Blocker** — unsafe or nonfunctional in the water; fix before any pool test.
- 🟠 **Major** — wrong behaviour in realistic conditions; fix before competition.
- 🟡 **Minor / latent** — dead code, traps for future work, maintenance debt.

Tick the box when fixed and note the commit.

---

## 1. Controller (`src/snappy_cpp/src/controller.cpp`)

The current controller is the old `rewrite-controller` pipeline merged to main with YAML gains.
Its architecture (trajectory → wrench → allocation) is sound; the math inside has the following bugs.

- [x] 🔴 **C1 — Surge is hardcoded to a constant 2.0 kgf** (`controller.cpp:236`) — fixed in `2509365`
  `wrench << 2.0, thrust_y, ...` discards `thrust_x` (computed at line 226 — the compiler even
  warns "unused variable"). The sub is commanded +2 kgf forward every 200 ms tick, forever,
  regardless of error, sensors, or task. It can never hold station and will drive forward on the
  bench the moment the node starts.
  **Fix:** `wrench << thrust_x, ...` — this was clearly a pool-test hack that got merged.

- [x] 🔴 **C2 — `current_orientation` initialised to the zero quaternion → NaN motor commands**
  (`controller.cpp:83`) — fixed in `2509365` (C4's output gating still open)
  `Quaterniond(0,0,0,0)` has `squaredNorm()==0`, so `.inverse()` in `generate_trajectory` is
  0/0 = NaN. NaN flows through eulerAngles → PIDs (the ±100 clamps don't catch NaN) → the whole
  wrench → the allocator (all NaN comparisons are false, so no scaling) → `static_cast<int8_t>(NaN)`
  = undefined behaviour → `/motor_cmd`. This runs on **every tick until the estimator's first pose
  arrives** — and forever if the estimator never initialises (no depth, dead Xsens).
  **Fix:** `Eigen::Quaterniond::Identity()`, plus C4's output gating.

- [ ] 🔴 **C3 — World→body transforms are inverted** (`controller.cpp:186–187`)
  `relative_position = current_orientation * (target − current)` and
  `relative_orientation = target_orientation * current_orientation.inverse()`.
  For a body→world state quaternion both need the conjugate:
  `current.conjugate() * (target − current)` and `current.conjugate() * target`.
  Identity-correct at yaw 0 (which is what bench tests exercise); at 90° yaw the x/y
  corrections swap axes, at 180° they invert — the PIDs push **away** from the target.

- [ ] 🔴 **C4 — No arm/disarm and no first-sensor gating; default target is [0, 10, 0]**
  (`controller.cpp:149` + `config/controller_params.yaml:3`)
  The 200 ms timer starts in the constructor and `sendCmd` runs unconditionally. With the YAML
  defaults the node immediately drives toward y = 10 m (starboard, in the estimator's
  x-fwd/y-stbd/z-down frame) and z = 0 (**the surface**), plus C1's constant surge — uncommanded
  motion at launch with no operator arm step.
  **Fix:** zero thrust until first valid state + an `armed` parameter/topic; change the default
  target to "hold current pose"; review what [0,10,0] was supposed to mean.

- [ ] 🟠 **C5 — Attitude error via `eulerAngles(0,1,2)`: first angle is constrained to [0, π]**
  (`controller.cpp:199`, also 158, 312, 330, 348)
  Eigen's `eulerAngles` returns its first angle in [0, π], so the ±π wrap-fix below it
  (lines 219–223) is dead code, and near-level attitudes can decompose into the alternate
  (~π-flipped) solution — the attitude PIDs then see a huge spurious error around level trim.
  **Fix:** use a rotation-vector error instead (AngleAxis of the shortest-path error quaternion,
  `if (q.w()<0) q.coeffs()*=-1`) — no wrap, no gimbal issues, matches the ESKF's error formalism.

- [ ] 🟠 **C6 — No staleness handling on the state input** (`controller.cpp:430`)
  `state_callback` latches the last pose; if the estimator dies mid-run the controller keeps
  servoing a frozen estimate indefinitely (see also E2 — the message has no timestamp to check).
  **Fix:** track receipt time; zero thrust after ~300 ms without a fresh state.

- [ ] 🟡 **C7 — Uninitialised members (latent)** (`controller.cpp:477–486`)
  `dvl_first_`, `timer_first_`, `state_`, and floats `x, y, roll, pitch, yaw, current_depth` are
  never initialised. Currently unread on the live path (their writers are commented out), but the
  moment the IMU/DVL subscriptions or the task interface are re-enabled, `set_x(local=true)`
  composes a rotation from garbage. Brace-initialise everything.

- [ ] 🟡 **C8 — Header comment describes the old message type** (`controller.cpp:2–4`)
  Says `Float32MultiArray` on `/motor_cmd`; actual is `snappy_interfaces/ThrusterCommand`
  (best-effort QoS). Update before it misleads someone.

- [ ] 🟡 **C9 — Task interface is gone, not just disabled** (`controller.cpp:119`)
  The `/planner/task` subscription is commented out **and `task_callback` is not defined
  anywhere** — re-enabling is not a one-line uncomment. The behaviour tree's wire contract
  (documented in `src/bt/bt_context.hpp`) is the spec to implement: `move/z`, `move/yaw`
  (radians, abs/rel), `drive/*` (open-loop pct + 3 s deadman), `spin/*` (body rates),
  `track/*` (visual servo enable), `actuate/*`. Note the current controller has **no home** for
  `drive` (open-loop) or `spin` (body rate) — mapping spin onto Euler setpoints reproduces the
  exact zero-net-rotation failure the BT avoids.

## 2. PID (`src/snappy_cpp/src/include/src/pid.cpp`)

- [x] 🟠 **P1 — Integral clamped in error·seconds units, before Ki** (`pid.cpp:36–41`) — fixed in `f81bc6d`
  (post-Ki clamp with back-calculation; saturation-aware anti-windup via T3 still open)
  `integral_` is clamped to ±100 **before** multiplying by Ki, so the effective integral
  authority scales with Ki (pid_z Ki=1.0 → i_term up to ±100 "kgf" against ±5 kgf thrusters).
  Massive windup and overshoot. **Fix:** clamp `i_term` (or scale the bound by 1/Ki), and add
  saturation-aware anti-windup (see T3).

- [x] 🟠 **P2 — First-update transient** (`pid.cpp:26, 45`) — fixed in `f81bc6d` (first call is P-only)
  `prev_time_` is set at construction, so the first `update()` uses dt = time since node startup
  (can be seconds): oversized integral step + derivative kick from `prev_err_ = 0`
  (pid_yaw Kd=5.0 → large transient thruster command on the first control cycle).
  **Fix:** skip I/D on the first call (or reset `prev_time_` on first use).

- [ ] 🟡 **P3 — Derivative-on-error** (`pid.cpp:45`)
  Every target change produces a derivative spike. Prefer derivative-on-measurement.

- [ ] 🟡 **P4 — No wrap-aware angular mode**
  `update()` computes `target − current` linearly; nothing in the class handles ±π wrap.
  Currently masked because the controller feeds it pre-computed attitude errors, but the moment
  someone feeds it a raw heading it turns the long way around. (The reverted `steve/prequal`
  branch had a working `set_angular` to steal.)

- [ ] 🟡 **P5 — Output units don't match the actuators** (`pid.cpp:11–12`)
  MIN/MAX = ±100 while thrusters saturate at −4/+5 kgf and torque lever arms are ~0.3 m: any
  large error saturates the allocator (ratio ≈ 20) and the uniform scaling lets one axis steal
  authority from all others (the depth integral then winds up during every turn). **Fix:** scale
  PID outputs into physical wrench units with per-axis caps chosen to just-saturate.

## 3. Thruster allocator (`src/snappy_cpp/src/thruster_allocator.cpp`)

- [ ] 🟡 **T1 — Pseudo-inverse recomputed on every `allocate()` call** (`thruster_allocator.cpp:35`)
  A completeOrthogonalDecomposition of a constant 6×8 matrix, every 200 ms. Compute once in the
  constructor and store it.

- [ ] 🟡 **T2 — Scalar `set_min_thrust`/`set_max_thrust` size the limit vectors with `rows()` (=6)
  instead of `cols()` (=8)** (`thruster_allocator.cpp:96, 100`)
  `getMaxSaturationRatio_` then indexes past the end for i = 6,7 → out-of-bounds read / UB.
  Nothing calls these today (the controller uses the float constructor) — the first caller gets
  memory corruption. Fix to `cols()` and add a unit test.

- [ ] 🟡 **T3 — `allocate()` gives the caller no saturation signal** (`thruster_allocator.cpp:59`)
  Anti-windup (P1) needs the max saturation ratio; return it
  (`std::pair<VectorXd, double>`) and freeze the integrals when ratio > 1.

- [ ] 🟡 **T4 — Silent fallback to ±1.0 limits on ctor size mismatch** (`thruster_allocator.cpp:17–21`)
  Masks a caller bug with limits wrong by 4–5×. Throw/assert instead.

- [ ] 🟡 **T5 — NaN passes straight through saturation, and `thrust_to_speed` casts without
  clamping or rounding** (`thruster_allocator.cpp:43–57`, `controller.cpp:258–264`)
  All NaN comparisons are false → no scaling → `static_cast<int8_t>(NaN * 20)` is UB (see C2).
  Also truncates toward zero instead of rounding. Clamp → `lround` → cast, and reject
  non-finite wrenches.

## 4. State estimator (`src/snappy_cpp/src/state_estimator.cpp`)

- [x] 🔴 **E1 — NEW regression: DVL forward axis negated** (`state_estimator.cpp:319`) — fixed in `4eee463`.
  Pool test 12 Jul + WaterLinked docs settled it: the DVL raw frame is x-fwd/y-stbd/z-DOWN, i.e.
  already the body frame — the z flip (not just x) was wrong and caused the runaway descent.
  Signs are now identity by default and exposed as `dvl_sign_x/y/z` params for rotated mounts.
  `v_body(-x, +y, -z)` — but the frame convention (kalman.cpp:33 comment: "just flipping the z
  sign") and the offline test (`test_kalman.cpp:205`, `(+x, +y, -z)`) both say only z flips.
  The in-code comment literally says "Test this … Should maybe flip this with a rotation matrix".
  Driving forward makes the filter believe the sub moves backward; since DVL velocity is the
  high-trust correction (R=0.09), world-x position integrates in the wrong direction — and the
  regression test can't catch it because it replays the opposite sign (see K5).
  **Fix:** verify against a recorded run, then make node and test agree.

- [ ] 🟠 **E2 — Output contract: headerless `Pose`, published only from the Xsens callback**
  (`msg/Pose.msg`, `state_estimator.cpp:247`)
  No timestamp/twist/covariance → consumers cannot detect staleness (see C6). If the Xsens dies,
  pose publication stops entirely. **Fix:** publish stamped `nav_msgs/Odometry` on `/state`, only
  after `frame_initialized_` (the init gating itself is now correctly in place).

- [ ] 🟠 **E3 — Silent freeze on bad IMU timestamps** (`state_estimator.cpp:169, 232`)
  `dt > 0.001` on header-stamp deltas gates predict; a driver publishing stuck/duplicate stamps
  silently stops the filter while the node **keeps publishing** a fresh-looking pose.
  Warn on repeated rejections and/or stop publishing when predict hasn't run.

- [x] 🟠 **E4 — Hardcoded accel bias with a frozen bias state** (`state_estimator.cpp:93`) — improved in
  `4eee463` (bias P 0.01→0.25, random walk 1e-6→1e-5 so the filter can correct the guess); re-deriving
  the init value from a stationary capture is still worth doing.
  Initial bias (−0.5, 0.5, 0.2) is injected with tiny covariance (×0.01) and near-zero random
  walk (Q=1e-6), and no measurement observes bias — the filter can never correct a wrong value,
  giving steady velocity ramp / quadratic position drift between DVL fixes. Re-derive from a
  stationary capture; loosen the bias covariance.

- [ ] 🟡 **E5 — Five CSV logs opened unconditionally in CWD every run** (`state_estimator.cpp:56`)
  Nanosecond-named, never rotated — this is the 795 MB `imu2.csv` incident pattern, and it can
  fill the Jetson's disk over a competition day. Gate behind a ROS parameter; `.gitignore *.csv`.

- [ ] 🟡 **E6 — Serial ports by enumeration order** (`pressure_sensor.cpp:27` +
  `launch/snappy_realsense.launch.py:59`)
  Pressure Arduino hardcodes `/dev/ttyUSB0`, micro-ROS agent gets `/dev/ttyUSB1`; if they
  enumerate swapped, the depth node opens the wrong device, logs one error, and idles —
  no depth → estimator never initialises → controller runs on the zero pose (C2).
  Use `/dev/serial/by-id/` paths.

## 5. Kalman filter (`src/snappy_cpp/src/include/src/kalman.cpp`, `Inc/kalman.h`)

- [x] 🟠 **K1 — `updateIMU1` is a second predict disguised as a measurement** (`kalman.cpp:141`,
  called from `state_estimator.cpp:173`) — fixed in `4eee463` (call removed from node + test; the
  RealSense IMU is still CSV-logged; the function remains with a DO-NOT-CALL note)
  Its residual is built **from the current state** (v·dt + ½a·dt², a·dt, ω·dt), so it confirms
  the filter's own dead-reckoning while shrinking covariance — real corrections (DVL/depth) get
  ever-smaller gains — and raw RealSense accel is used with no bias handling.
  **Fix:** delete the call (one line; the MTi-620 outclasses the RealSense IMU), or replace with
  a proper gravity-vector attitude update.

- [x] 🟠 **K2 — Depth gate is a lockout, not an outlier filter** (`kalman.cpp:179`) — fixed in `4eee463`
  (3-sigma Mahalanobis gate on P_zz + R; widens as rejections accumulate, so the filter recovers)
  `if (residual > 0.5 || residual < -0.5) return;` — once estimated z drifts > 0.5 m from the
  Bar02, **every** subsequent depth measurement is rejected forever and z rides accel
  integration alone. Gate on innovation covariance (Mahalanobis), or widen/disable while P is
  large so the filter can recover.

- [x] 🟡 **K3 — `updateDVL()` declared but never defined** (`kalman.h:49–50`) — fixed in `4eee463` (deleted)

- [x] 🟡 **K4 — Stale dimension comment** (`kalman.h:152`) — fixed in `4eee463`

- [x] 🟠 **K5 — `test_kalman` has drifted from the node it exists to protect**
  (`tests/test_kalman.cpp:205–209`) — fixed in `4eee463`: node and test now share
  `KalmanFilter::dvl_axis_signs_default`, the test's NaN gate is restored, and its config mirrors the
  node. NOTE: the checked-in reference recording predates the mission-frame change (constant ~90°
  offset, failed before and after this fix) — record a fresh reference run to re-arm the bounds check.
  The offline replay uses `(+x, +y, -z)` DVL convention (node uses `-x`, see E1) and its NaN
  gate is commented out — CI validates behaviour the vehicle doesn't run, and green-lights
  exactly the bug class it exists to catch. Extract the callback math into a shared function
  both compile, or mirror the node byte-for-byte.

**Checked OK (don't re-chase):** frame math is now self-consistent (ENU→z-down conversion,
gravity +z-down, depth residual sign, deferred init on first depth + first Xsens quat);
publish-before-init is fixed; orientation has a single writer; DVL NaN gating present in the
node; subscriptions are the hardware topics (`/imu/data`, `depth_data` Float32).

## 6. Planner (`src/snappy_cpp/src/planner.cpp`) — legacy, recommend retiring

- [ ] 🟡 **PL1 — Subscribes a dead topic** (`planner.cpp:22`)
  `/cuda_node/detections` — the vision package publishes `/d455/detections` and
  `/d405/detections`. The detection logging never fires.

- [ ] 🟡 **PL2 — `starter_task()` builds a Task and never publishes it** (`planner.cpp:63–72`)
  `task_publisher_` is also never created. Dead code.

- [ ] 🟡 **PL3 — Superseded** — the behaviour tree (`behavior_tree` executable) replaces this
  node. Retire the target and remove it from launch files once the BT is wired in.

## 7. Behaviour tree (`src/snappy_cpp/src/behavior_tree.cpp`, `src/bt/*.hpp`) — owner: Steve

Verified by multi-agent review + bench run. Top 10:

- [ ] 🔴 **B1 — `tickOnce()` has no try/catch** (`behavior_tree.cpp:190`)
  XML loading validates node/port *names*, not that required port *values* are supplied. A custom
  `tree_file` omitting e.g. Surge's `seconds` (or `Spin axis="yaw"`) loads fine and throws
  `BT::RuntimeError` from `onStart` mid-run → the exception exits `rclcpp::spin`, the planner
  process dies, and EmergencySurface never runs. Wrap the tick; on exception publish
  surface commands and latch.

- [ ] 🔴 **B2 — `Locate`'s same-class debounce self-defeats on multi-class specs**
  (`vision_nodes.hpp:58`)
  With both role icons visible at the gate, the max-confidence class flips tick-to-tick and
  resets the streak — the gate task can fail despite the gate being plainly in view (the bench
  harness masks this: it publishes only one role icon). Debounce on "any class in the spec".

- [ ] 🔴 **B3 — Guards have no debounce: one stale tick permanently aborts the mission**
  (`condition_nodes.hpp:37`)
  A single ~1.5 s sensor gap trips SensorsFreshOK once → memory Fallback commits to
  EmergencySurface → tick-loop latches, no re-arm. Require N consecutive stale ticks.

- [ ] 🔴 **B4 — Bins search is structurally unwinnable** (`mission_tree.hpp:156`)
  SearchAndAlign yaw-rotates in place; a yaw spin never changes what the **down** camera sees, so
  a bin not already underneath can never be acquired. Needs a translation search pattern.

- [ ] 🟠 **B5 — Octagon leg dead-reckons at the torpedo-board heading** (`mission_tree.hpp:189`)
  `pre_octagon` is saved but never read; nothing re-asserts a bins→octagon bearing before the
  15 s surge. Add a heading assert (param or saved waypoint) before the leg.

- [ ] 🟠 **B6 — `AlignToObject` can settle-SUCCEED on a frozen frame** (`vision_nodes.hpp:141`)
  Convergence accepts detections up to `lost_timeout_s`=1.5 s old while `settle_s`=1.0 s — a
  stream freeze at the centred instant fires the torpedo/marker on stale data. Use a tight
  freshness bound (~0.3 s) for convergence; keep 1.5 s only for the lost-target failure.

- [ ] 🟠 **B7 — Detection age measured from receipt, not capture** (`behavior_tree.cpp:52`)
  Vision stamps at capture then spends inference time; the BT restamps at receipt, understating
  age by the pipeline latency. Use the header stamp.

- [ ] 🟠 **B8 — `gate_side` is write-only** (`vision_nodes.hpp:197`)
  LatchGateSide's only reader is its own idempotency check; the "every later task reads it"
  comment is unimplemented (the slalom side rule has no consumer). Implement or delete + fix comments.

- [ ] 🟠 **B9 — Object-spec typos fail silently** (`bt_context.hpp:346`)
  An unknown `role_*` token becomes a literal class that never matches — the task burns its
  timeout, indistinguishable from "object absent". Validate specs at startup.

- [ ] 🟠 **B10 — 60 s task timeouts ≈ worst-case search sweep alone** (`mission_tree.hpp:143,155,171`)
  12 × (2 s window + settled turn) can exhaust the outer timeout before AlignToObject starts →
  spurious task skips. Budget the search separately or raise the timeouts.

Cleanup backlog (below the cap, all verified): heading-settle logic triplicated
(SetHeading/TurnRelative/FaceSavedHeading); 1 s republish pattern triplicated
(TimedDrive/Spin/EmergencySurface); role→class tables duplicate `detection_classes.hpp` strings
with no compile-time tie; `bearing_rad` + the HFOV params exist for one sign test; `{found}`
output port and `distance_m` have no readers; five leaves registered but unused by the default
mission; `best(freshDetections())` allocates per tick; six flat camera params want a
`declareCameraModel(prefix)` helper.

## 8. Integration / launch / config

- [ ] 🔴 **I1 — Nothing consumes the BT and nothing launches it**
  See C9 (no Task consumer) and: no launch file starts `behavior_tree`;
  `snappy_realsense.launch.py` still launches the legacy `planner`. The mission stack is
  end-to-end dead until both are wired.

- [ ] 🟠 **I2 — `snappy.launch.py` is broken as committed** (`launch/snappy.launch.py:107`)
  References `executable='computer_vision'`, which no package builds (the real executables are
  `front_camera_vision`/`bottom_camera_vision` in `snappy_computer_vision`). It also pulls real
  `controller_params.yaml` into a *simulation* launch with the pressure sensor defaulted off —
  the estimator then never initialises and the controller runs the C2 NaN path.

- [ ] 🟡 **I3 — DVL/depth subscriptions use default reliable QoS** (`state_estimator.cpp:80, 86`)
  If the vendored WaterLinked driver publishes best-effort (common for sensor streams), a
  reliable subscription receives **nothing**, silently. Verify the driver's QoS; prefer
  `SensorDataQoS()` on sensor inputs.

- [ ] 🟡 **I4 — BT camera geometry must track the vision profile** (`behavior_tree.cpp:130`)
  `front/down_image_width/height` default 848×480 independently of the vision launch's
  `rgb_camera.color_profile`; changing one without the other silently mis-scales bearings and
  can flip the gate-side classification. Single source of truth (param file both read).

- [ ] 🟡 **I5 — Relative topic name `state_estimator/state`** (`controller.cpp:124`,
  `state_estimator.cpp:137`) — resolves differently if the nodes are ever namespaced.
  Use an absolute name on both sides.

## Checked and refuted (verified non-issues — don't re-chase)

- EmergencySurface's single spin-stop is fine: `/planner/task` is reliable QoS; the 1 s drive
  re-publish exists for the deadman, not message loss.
- BT.CPP v4 SubTree static-string port injection works on the deployed apt version
  (proven by the bench run).
- TUNE constants compiled into the mission XML: the `tree_file` runtime override is the
  intended tuning workflow.
- Guard port re-parsing per tick: negligible at 10 Hz.
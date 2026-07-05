# Movement & PID Tuning Guide

How to drive the AUV and tune the controller at the pool — **PID-only, no planner**
(`marcus/newMovement` branch).

## What the controller does

Six PIDs run in `controller.cpp`. On this branch we mix their outputs directly onto
the 8 thrusters (no thruster allocator). Some axes are **closed-loop** (hold a target
using sensor feedback), some are **open-loop** (drive thrusters at a set % for a short
burst):

| Axis            | Mode        | Feedback source            | How you command it        |
|-----------------|-------------|----------------------------|---------------------------|
| Depth           | closed-loop | pressure sensor (`depth_data`) | `move z`              |
| Heading (yaw)   | closed-loop | IMU (`/filter/euler`)      | auto-holds; `move yaw` to turn |
| Roll / pitch    | closed-loop | IMU                        | auto-holds level (target 0°) |
| Forward (surge) | open-loop   | none                       | `drive forward` / `backward` |
| Strafe (sway)   | open-loop   | none                       | `drive right` / `left`    |

On startup the controller **captures its current depth and heading as the hold targets**,
so once it's in the water it tries to stay put until you tell it to move.

Thruster map (index = motor):
```
0 FRONT_YAW   1 FRONT_RIGHT*  2 FORWARD_RIGHT  3 BACK_RIGHT*
4 BACK_YAW    5 BACK_LEFT*    6 FORWARD_LEFT   7 FRONT_LEFT*
(* = vertical thruster, shared by depth/roll/pitch)
```

## Launching

```bash
ros2 launch snappy_cpp snappy_realsense.launch.py
```

The controller subscribes to `/planner/task`. You don't need the planner — publish
`snappy_cpp/msg/Task` messages to that topic yourself (below).

## Movement commands (run these from a second terminal)

All commands use `overwrite: true` so they execute immediately.

### Depth (closed-loop hold)
```bash
# Go DOWN 0.5 m from current depth, then hold
ros2 topic pub --once /planner/task snappy_cpp/msg/Task \
  '{type: "move", direction: "z", magnitude: 0.5, absolute: false, overwrite: true}'

# Go UP 0.3 m from current depth
ros2 topic pub --once /planner/task snappy_cpp/msg/Task \
  '{type: "move", direction: "z", magnitude: -0.3, absolute: false, overwrite: true}'

# Hold an ABSOLUTE depth of 1.0 (sensor units)
ros2 topic pub --once /planner/task snappy_cpp/msg/Task \
  '{type: "move", direction: "z", magnitude: 1.0, absolute: true, overwrite: true}'
```
> If depth drives the wrong way, flip the sign of `magnitude` (and note it — it means the
> pressure sensor's sign convention is opposite to what the vertical thrusters expect).

### Forward / strafe (open-loop, auto-stops after ~3 s)
```bash
# Forward at 40%
ros2 topic pub --once /planner/task snappy_cpp/msg/Task \
  '{type: "drive", direction: "forward", magnitude: 40, absolute: false, overwrite: true}'

# Reverse at 30%
ros2 topic pub --once /planner/task snappy_cpp/msg/Task \
  '{type: "drive", direction: "backward", magnitude: 30, absolute: false, overwrite: true}'

# Strafe right / left at 40%
ros2 topic pub --once /planner/task snappy_cpp/msg/Task \
  '{type: "drive", direction: "right", magnitude: 40, absolute: false, overwrite: true}'
ros2 topic pub --once /planner/task snappy_cpp/msg/Task \
  '{type: "drive", direction: "left", magnitude: 40, absolute: false, overwrite: true}'
```
Depth and heading keep holding the whole time. Each command drives for `drive_timeout_`
seconds (default **3 s**, set in `controller.cpp`) then auto-stops. To drive longer, either
raise `drive_timeout_` and rebuild, or republish the command every couple seconds:
```bash
# Drive forward continuously (Ctrl-C to stop). --rate republishes so the deadman never trips.
ros2 topic pub --rate 2 /planner/task snappy_cpp/msg/Task \
  '{type: "drive", direction: "forward", magnitude: 40, absolute: false, overwrite: true}'
```

### STOP (kills surge + sway immediately; depth & heading keep holding)
```bash
ros2 topic pub --once /planner/task snappy_cpp/msg/Task \
  '{type: "drive", direction: "stop", magnitude: 0, absolute: false, overwrite: true}'
```
Keep this one ready in a terminal at all times.

### Turn (closed-loop heading)
```bash
# Turn to face 90 degrees, then hold that heading
ros2 topic pub --once /planner/task snappy_cpp/msg/Task \
  '{type: "move", direction: "yaw", magnitude: 90, absolute: true, overwrite: true}'
```

## Combining commands (e.g. hold depth AND drive forward)

Commands are **independent and sticky** — each one sets one thing, and it stays set until
you change it:
- `move z` sets the **depth target**. The depth PID holds it forever (until you send a new
  `move z`). Driving forward does **not** change it.
- `move yaw` (or the startup capture) sets the **heading target**, held the same way.
- `drive ...` sets the **open-loop surge/sway**, which auto-zeros after `drive_timeout_`.

They act on different thrusters (depth/heading on the vertical + yaw thrusters, surge/sway
on the forward + lateral thrusters), so they run at the same time without fighting.

So "hold 1 m and cruise forward" is **two separate commands**, in order:

```bash
# 1. Set depth -> depth PID drives to 1 m and HOLDS it (stays set the whole time)
ros2 topic pub --once /planner/task snappy_cpp/msg/Task \
  '{type: "move", direction: "z", magnitude: 1.0, absolute: true, overwrite: true}'

# 2. Now drive forward. Depth is STILL held at 1 m by step 1 while it moves.
#    --rate 2 refreshes the command so the 3 s deadman never trips (continuous drive).
ros2 topic pub --rate 2 /planner/task snappy_cpp/msg/Task \
  '{type: "drive", direction: "forward", magnitude: 40, absolute: false, overwrite: true}'
```

When you Ctrl-C step 2, the surge deadmans to 0 after ~3 s and forward motion stops — but
the sub **keeps holding 1 m**, because step 1's depth target never went away. To also change
depth, send another `move z`; to stop everything, send `drive stop` (surge/sway) — depth/
heading hold until you command otherwise.

> Gotcha: if you skip step 1, the controller holds the depth it captured **at startup**
> (~surface), so `drive forward` alone cruises you forward at the surface, not at 1 m.
> Always set your depth first.

## Auto-run on launch (optional)

`snappy_realsense.launch.py` has a `drive_test` block (depth-down + forward) that is
**disabled by default**. Uncomment `drive_test` in the `LaunchDescription([...])` list to
have the sub run that sequence ~20 s after launch. Prefer the CLI for the first tests so
*you* decide when it moves.

## Pre-dive checks (do these first, out of / at the surface)

1. **Thruster directions.** Before submerging, send each command and confirm the right
   thrusters spin the right way:
   - `drive forward 20` → both forward thrusters push the sub forward.
   - `drive right 20` → sub pushes right.
   - `move z 0.3` (relative) → vertical thrusters push **down**.
   If any is reversed, fix the sign convention before diving.
2. **Watch the debug log** (below) to confirm sensors are alive: depth `cur` changes when
   you move it up/down by hand, yaw `cur` changes when you rotate it.

## Reading the debug log (for tuning)

The controller prints a throttled (~4 Hz) block:
```
[PID] DEPTH tgt=  1.50 cur=  1.32 err= +0.18 out= +42.0
      YAW   tgt= 90.0 cur= 84.3 err=  +5.7 out= +12.4
      ROLL  tgt=  0.0 cur=  3.1 err=  -3.1 out=  -4.8
      PITCH tgt=  0.0 cur= -1.2 err=  +1.2 out=  +2.1
      DRIVE surge= +40.0 sway=  +0.0 (open-loop)
[MTR] FYaw=  12 FR=  38 FwdR=  40 BR=  46 BYaw= -12 BL=  46 FwdL=  40 FL=  38
```
- **tgt** = where it's trying to go, **cur** = sensor reading, **err** = tgt − cur,
  **out** = PID thrust output for that axis.
- **DRIVE** = the open-loop surge/sway setpoints.
- **[MTR]** = final command sent to each thruster (−100..100).

Watch `err` and `out` together — that's the whole tuning signal.

## Tuning the PIDs at the pool

Gains live in the `Controller` constructor in `controller.cpp`:
```cpp
pid_x_(0.5f, 0.0f, 0.1f),      // unused on this branch (open-loop surge)
pid_y_(0.5f, 0.0f, 0.1f),      // unused on this branch (open-loop sway)
pid_z_(0.7f, 0.3f, 5.0f),      // DEPTH   (Kp, Ki, Kd)
pid_roll_(0.5f, 0.0f, 0.1f),   // ROLL
pid_pitch_(0.5f, 0.0f, 0.1f),  // PITCH
pid_yaw_(0.15f, 0.0f, 5.0f)    // YAW / heading
```
Each change needs a rebuild (`colcon build --packages-select snappy_cpp`) and relaunch.

### Order of operations

1. **Start simple. Isolate depth.** Zero the roll and pitch gains (`pid_roll_`, `pid_pitch_`
   → `(0,0,0)`) so nothing fights the vertical thrusters. Get depth-hold working first.
2. **Tune DEPTH.** Command a depth (`move z`) and watch the `DEPTH` row:
   - **Kp**: raise until the sub reaches depth reasonably fast. Too high → it overshoots
     and oscillates around the target (`err` flips sign repeatedly).
   - **Kd**: raise to damp overshoot/oscillation. Too high → jittery `out`, twitchy motors.
   - **Ki**: only add if there's a steady `err` that never closes (sub settles a bit above
     or below target and stays there). Raise slowly — too much Ki = slow oscillation / windup.
   - Goal: reaches target, minimal overshoot, `err` settles near 0 and stays.
3. **Tune YAW (heading hold)** the same way: rotate the sub by hand (or `move yaw`) and
   watch the `YAW` row settle back. Kp for speed, Kd for damping.
4. **Re-enable ROLL / PITCH** (put gains back, start small e.g. `(0.3, 0, 0.1)`). These
   keep the sub level. If it fights or wobbles, lower Kp. If it's sluggish to level, raise
   Kp. Remember: **depth has priority over pitch over roll** — if a vertical thruster
   saturates, roll gives way first, so tune depth to feel solid before leaning on roll.
5. **Add forward motion.** With depth + heading holding, `drive forward 40`. Confirm the
   sub goes straight and depth/heading stay locked while it moves. If it dives or pitches
   when driving forward, that's a thruster-trim / CG issue, not a PID gain — note it.

### Quick symptom → fix cheat sheet
| Symptom                                   | Try                          |
|-------------------------------------------|------------------------------|
| Slow to reach target                      | ↑ Kp                         |
| Overshoots then oscillates                | ↓ Kp and/or ↑ Kd             |
| Motors buzz / jitter                      | ↓ Kd                         |
| Settles off-target and stays there        | ↑ Ki (a little)              |
| Slow drift / winds up over time           | ↓ Ki                         |
| Oscillates no matter what                 | ↓ Kp, ↓ Ki, add a little Kd  |

## Notes / known limitations
- **Forward & strafe are open-loop** — no position feedback. The sub will drift; that's
  expected. Closed-loop position (DVL) is a later step.
- **Relative depth uses the pressure sensor** (`current_depth`); relative x/y are not used
  on this branch (the state estimator is disabled).
- The `drive` deadman means a single command only lasts `drive_timeout_` seconds by design.

# Mission Planner

The planner reads a mission YAML and runs it **one step at a time, top to bottom**,
like a tape machine. For each step it:

1. Sends one setpoint command to the controller (`/planner/task`) — e.g. "depth 1 m",
   "turn by 90°", "forward 3 m". The controller does all the actual driving.
2. Watches the controller's feedback (`/controller/status`) and waits until the
   error for that step stays inside tolerance for ~1 second → **step done, next step**.
3. If the step's `timeout` expires first → the step **failed**, and its `on_fail`
   decides what happens:
   - `skip` — log it, move on to the next step
   - `abort` / `surface` — stop the mission, stop moving, go to the surface

A global clock (`max_seconds`) runs over the whole mission; if it expires the sub
surfaces no matter what step it's on. When the last step finishes, the controller
just keeps holding the final position/depth/heading.

All movement is written from the **sub's point of view** (x/y N metres, turn by
N degrees, be at N metres depth). The controller's `set_x`/`set_y`/`set_z`/`set_yaw`
convert that to global targets and do all the driving.

## Running a mission

```bash
# pick any mission file at launch — edit YAML, relaunch, NO rebuild needed:
ros2 launch snappy_cpp snappy_realsense.launch.py \
    mission_file:=/ros2_ws/src/snappy_cpp/missions/pool_test.yaml

# check a mission file for mistakes WITHOUT moving the sub:
ros2 run snappy_cpp planner --ros-args \
    -p mission_file:=/ros2_ws/src/snappy_cpp/missions/pool_test.yaml \
    -p validate_only:=true
```

A bad mission file (unknown type, missing field, bad on_fail) is rejected at launch
with the step's name and what's wrong — it will never fail mid-water.

## The YAML, exactly

```yaml
mission:                    # required top-level key
  name: pool_test           # optional, string  — shown in logs
  max_seconds: 480          # optional, number  — global clock (default 480 s)

  defaults:                 # optional — starting values for EVERY step;
    timeout: 30.0           #   any step can override any of them
    on_fail: abort
    depth_tolerance: 0.15
    yaw_tolerance_deg: 7.0
    xy_tolerance: 0.3

  steps:                    # required, non-empty list — runs in order
    - name: dive            # each step: name + type + that type's fields
      type: depth
      depth: 1.0
```

### Fields every step can have

| Field | Type | Default | Meaning |
|---|---|---|---|
| `name` | string | `step N` | Label used in every log line — always set it |
| `type` | string | *(required)* | One of the 6 step types below |
| `timeout` | number (s) | 30.0 | Max time before the step counts as failed |
| `on_fail` | string | `abort` | `skip`, `abort`, or `surface` (abort and surface do the same thing in v1) |
| `depth_tolerance` | number (m) | 0.15 | How close is "at depth" |
| `yaw_tolerance_deg` | number (°) | 7.0 | How close is "facing the right way" |
| `xy_tolerance` | number (m) | 0.3 | How close is "arrived" for forward moves |

### The 6 step types

**`depth`** — go to a depth and hold it. Done when depth error < `depth_tolerance`.

| Field | Type | Required | Meaning |
|---|---|---|---|
| `depth` | number (m) | yes | Metres below the surface (positive down), from the pressure sensor |

**`heading`** — point the nose somewhere. Done when yaw error < `yaw_tolerance_deg`.

| Field | Type | Required | Meaning |
|---|---|---|---|
| `yaw_deg` | number (°) | yes | The angle |
| `absolute` | bool | yes | `true` = FACE yaw_deg, where 0° = the direction the sub faced at launch. `false` = TURN BY yaw_deg from the current heading |

`absolute` is deliberately required — "face 90" and "turn by 90" are very different
moves. (Sign of a positive turn: confirm on the first pool run and write it here.)

**`x`** — move forward/backward in the sub's local frame. Done when horizontal
error < `xy_tolerance`.

| Field | Type | Required | Meaning |
|---|---|---|---|
| `meters` | number (m) | yes | Distance along the sub's x axis; + = forward, − = backward |

**`y`** — move sideways in the sub's local frame. Done like `x`.

| Field | Type | Required | Meaning |
|---|---|---|---|
| `meters` | number (m) | yes | Distance along the sub's y axis (which side is + : confirm at the pool) |

**`hold`** — sit still, holding the current depth/heading/position.

| Field | Type | Required | Meaning |
|---|---|---|---|
| `seconds` | number (s) | yes | How long to hold (the step's timeout auto-extends so a hold never times itself out) |

**`wait_for_detection`** — do nothing until vision sees something. Done when a
fresh detection (< 1 s old) matches.

| Field | Type | Required | Meaning |
|---|---|---|---|
| `object` | string | yes | Detection class name from the vision model (e.g. `gate`) |
| `camera` | string | no (`front`) | `front` = /d455, `bottom` = /d405 |
| `min_confidence` | number 0–1 | no (0.5) | Minimum detection confidence |

Pair it with `on_fail: skip` and a `timeout` to mean "look for the gate for 45 s,
then just drive the planned line anyway".

## Planner settings (ROS parameters, not in the mission file)

Set with `-p name:=value` on `ros2 run`, or in the launch file.

| Parameter | Default | Meaning |
|---|---|---|
| `mission_file` | *(required)* | Path to the mission YAML |
| `start_delay` | 10.0 s | Wait after boot before starting (planner also waits until the controller is alive) |
| `settle_ticks` | 10 | Ticks (at 10 Hz) the error must stay in tolerance — 10 ≈ 1 s |
| `detection_max_age` | 1.0 s | How fresh a detection must be to count |
| `surface_depth` | 0.0 m | Depth commanded when aborting |
| `validate_only` | false | Check the file and exit without running |

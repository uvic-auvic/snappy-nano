# Mission Planner

The planner reads a mission YAML and runs it **one step at a time, top to bottom**,
like a tape machine. For each step it:

1. Sends one setpoint command to the controller (`/planner/task`) — e.g. "depth 0.5 m",
   "turn by 90°", "forward 2 m". The controller does all the actual driving.
2. Waits in a loop, doing nothing else, watching the controller's feedback
   (`/controller/status`) until the error for that step stays inside tolerance
   for ~1 second → **step done, next step**. While waiting it logs a heartbeat
   every 2 s with the current errors so you can watch progress live.
3. If the step's `timeout` expires first → the step **failed**, and its `on_fail`
   decides what happens:
   - `skip` — log it, move on to the next step
   - `abort` — kill the controller (motors stop) and end the mission

A global clock (`max_seconds`) runs over the whole mission; if it expires the
mission ends the same way. **When the last step finishes, the planner sends a
kill to the controller and shuts itself down — nothing keeps running.**

All movement is written from the **sub's point of view** (x/y N metres, turn by
N degrees, be at N metres depth). The controller's `set_x`/`set_y`/`set_z`/`set_yaw`
convert that to global targets and do all the driving.

## Running a mission

```bash
# pick any mission file at launch — edit YAML, relaunch, NO rebuild needed:
ros2 launch snappy_cpp snappy_realsense.launch.py \
    mission_file:=/ros2_ws/src/snappy_cpp/missions/testing1.yaml

# check a mission file for mistakes WITHOUT moving the sub:
ros2 run snappy_cpp planner --ros-args \
    -p mission_file:=/ros2_ws/src/snappy_cpp/missions/testing1.yaml \
    -p validate_only:=true
```

A bad mission file (unknown type, missing field, bad on_fail) is rejected at launch
with the step's name and what's wrong — it will never fail mid-water.

## The YAML, exactly

```yaml
mission:                    # required top-level key
  name: testing1            # optional, string  — shown in logs
  max_seconds: 300          # optional, number  — global clock (default 480 s)

  defaults:                 # optional — starting values for EVERY step;
    timeout: 40.0           #   any step can override any of them
    on_fail: abort
    xy_tolerance: 0.10
    z_tolerance: 0.10
    yaw_tolerance_deg: 10.0

  steps:                    # required, non-empty list — runs in order
    - name: dive            # each step: name + type + that type's fields
      type: z
      meters: 0.5
```

### Fields every step can have

| Field | Type | Default | Meaning |
|---|---|---|---|
| `name` | string | `step N` | Label used in every log line — always set it |
| `type` | string | *(required)* | One of the 4 movement types below |
| `timeout` | number (s) | 30.0 | Max time before the step counts as failed |
| `on_fail` | string | `abort` | `skip` or `abort` |
| `xy_tolerance` | number (m) | 0.10 | How close is "arrived" for x/y moves |
| `z_tolerance` | number (m) | 0.10 | How close is "at depth" |
| `yaw_tolerance_deg` | number (°) | 10.0 | How close is "facing the right way" |

### The 4 movement commands

These are the only step types. Anything else is rejected at launch.

**`x`** — move forward/backward in the sub's local frame. Calls the controller's
`set_x`. Done when horizontal error < `xy_tolerance` (10 cm default).

| Field | Type | Required | Meaning |
|---|---|---|---|
| `meters` | number (m) | yes | Distance along the sub's x axis; + = forward, − = backward |

**`y`** — move sideways in the sub's local frame. Calls `set_y`. Done like `x`.

| Field | Type | Required | Meaning |
|---|---|---|---|
| `meters` | number (m) | yes | Distance along the sub's y axis (which side is + : confirm at the pool) |

**`z`** — go to a depth and hold it. Calls `set_z`. Done when depth error <
`z_tolerance` (10 cm default). This is an **absolute depth**, not a delta —
the controller holds it until a later `z` step changes it.

| Field | Type | Required | Meaning |
|---|---|---|---|
| `meters` | number (m) | yes | Metres below the surface, positive down |

**`yaw`** — point the nose somewhere. Calls `set_yaw`. Done when yaw error <
`yaw_tolerance_deg` (10° default).

| Field | Type | Required | Meaning |
|---|---|---|---|
| `yaw_deg` | number (°) | yes | The angle |
| `absolute` | bool | yes | `true` = FACE yaw_deg, where 0° = the direction the sub faced at launch. `false` = TURN BY yaw_deg from the current heading |

`absolute` is deliberately required — "face 90" and "turn by 90" are very different
moves. A single relative "turn by 360" is a no-op (the target orientation is
unchanged), so spin in four 90° quarters — see `testing1.yaml`.

## Planner settings (ROS parameters, not in the mission file)

Set with `-p name:=value` on `ros2 run`, or in the launch file.

| Parameter | Default | Meaning |
|---|---|---|
| `mission_file` | *(required)* | Path to the mission YAML |
| `start_delay` | 10.0 s | Wait after boot before starting (planner also waits until the controller is alive) |
| `settle_ticks` | 10 | Ticks (at 10 Hz) the error must stay in tolerance — 10 ≈ 1 s |
| `validate_only` | false | Check the file and exit without running |

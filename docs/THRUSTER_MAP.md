# Thruster map — Snappy (8× T200)

**Bench-verified 2026-06-06** (sub out of water; verified per group **and per individual index**, both
signs). This is the authoritative index → group → firmware-pin reference. The Jetson side publishes `snappy_interfaces/msg/ThrusterCommand`
to `/motor_cmd`; the STM32 firmware (`Snappy-Firmware-MowerBoard`) subscribes and drives the ESCs.

> `ThrusterCommand.thrust_pct[i]` is the command for **index `i`**; bit `i` of `thruster_mask` enables it.
> Unmasked motors are **left untouched** by the firmware, so disjoint groups compose in one command.

---

## Verified body motions (at +100%)

| Group (indices) | Drive applied | Body motion (verified) |
| --- | --- | --- |
| **{1, 3, 5, 7}** vertical | common +100% | **heave DOWN** |
| **{2, 6}** surge | +100% | **surge FORWARD** |
| **{0, 4}** lateral | common +100% | **yaw COUNTER-CLOCKWISE** |

Notes:
- The `{0,4}` pair is mounted **anti-parallel**: a *common* command (same sign on both) makes a couple →
  **yaw**. A *differential* command (opposite signs) → **sway**. Both confirmed on the bench (each index
  driven individually, both signs).
- **Sign convention that follows:** since vertical common +100% → **down**, `down()` must send the
  positive command and `up()` the negative. This is the convention the controller depth path must match
  (see ledger **B5**).

---

## Index → firmware pin map

From `Snappy-Firmware-MowerBoard/Mower_Board_Firmware/Core/Src/T200MotorControl.c`
(`tim_map = {TIM5×4, TIM3×4}`, `ch_map = {1,2,3,4,1,2,3,4}`, GPIO from the `Start_PWM(...)` calls).

| `thrust_pct` index | `motorboard.h` constant | bit | Timer / channel | STM32 GPIO | Group |
| :---: | --- | :---: | --- | --- | --- |
| 0 | `FRONT_YAW`     | 1   | TIM5 / CH1 | PA0 | lateral (yaw+sway) |
| 1 | `FRONT_RIGHT`   | 2   | TIM5 / CH2 | PA1 | vertical |
| 2 | `FORWARD_RIGHT` | 4   | TIM5 / CH3 | PA2 | surge |
| 3 | `BACK_RIGHT`    | 8   | TIM5 / CH4 | PA3 | vertical |
| 4 | `BACK_YAW`      | 16  | TIM3 / CH1 | PC6 | lateral (yaw+sway) |
| 5 | `BACK_LEFT`     | 32  | TIM3 / CH2 | PC7 | vertical |
| 6 | `FORWARD_LEFT`  | 64  | TIM3 / CH3 | PC8 | surge |
| 7 | `FRONT_LEFT`    | 128 | TIM3 / CH4 | PC9 | vertical |

> The constant *names* (`FRONT_LEFT`, etc.) are nominal labels and may not match each thruster's true
> physical corner — what the bench proved is each index's **group membership, direction, and sign**. The
> names are opaque; trust the index/group/motion rows, which are verified one index at a time.

---

## Firmware behaviour worth knowing

- **PWM:** 50 Hz, 1500 µs neutral; `Set_Duty_Cycle` maps `(1500 + scaled)`.
- **Deadband:** ESCs barely move below `MINIMUM_SPEED_PERCENT` (15%), so any nonzero command is rescaled
  into `[min_speed, MOTOR_RANGE]`. There is **no smooth ramp from 0** — small commands jump to ~15%.
- **`MOTOR_RANGE = 400`** (header comment says "leave at 200 unless told" — confirm with Elec lead before
  a pool run).
- **⚠️ No watchdog:** the `MOTOR_TIMEOUT_MS` block in `main.c` is **commented out**, so the last command
  **latches** — motors keep running if commands stop (node crash / comms drop). The sequencer must
  explicitly send 0 to stop, and a human must own the kill switch.

---

## Related
- Drive-mode physics (common vs differential, co-oriented vs anti-parallel): `../challenges/_reference/thruster-drive-modes.md`
- Bench procedure that produced this: `../challenges/02-thruster-map/BENCH_PROCEDURE.md`
- Code that emits these commands: `src/snappy_cpp/src/include/{Inc/motorboard.h, src/motorboard.cpp}`
- Bug ledger (B3 anti-parallel, B5 depth sign): `../challenges/_reference/known-bugs.md`

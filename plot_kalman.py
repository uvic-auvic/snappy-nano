#!/usr/bin/env python3
"""Plot Kalman-filter output: position (X/Y/Z) and orientation (roll/pitch/yaw) over time.

Works with either output format produced in this repo (both are 11 columns):

  * No header  (kalman_<run>.csv, kalman_replay_<run>.csv):
        t_ns, pos_x, pos_y, pos_z, vel_x, vel_y, vel_z, quat_w, quat_x, quat_y, quat_z
    The first column is a nanosecond timestamp -> plotted as seconds from run start.

  * With header (filter_output.csv):
        step, pos_x, pos_y, pos_z, vel_x, vel_y, vel_z, quat_w, quat_x, quat_y, quat_z
    The first column is a step index -> plotted as "Step".

Quaternions are (w, x, y, z) and are converted to roll/pitch/yaw (deg) with the
aerospace ZYX (yaw-pitch-roll) convention -- the same yaw definition the filter uses.

Examples:
    ./plot_kalman.py snappy-nano/state_estimator_outputs/kalman_1783280617402906537.csv
    ./plot_kalman.py kalman_*.csv --trajectory          # overlay several runs
    ./plot_kalman.py filter_output.csv -o out.png        # save instead of show
"""

from __future__ import annotations

import argparse
import os
import sys
from dataclasses import dataclass

import numpy as np

# Use a non-interactive backend when there is no display, so saving still works.
import matplotlib
if not os.environ.get("DISPLAY"):
    matplotlib.use("Agg")
import matplotlib.pyplot as plt

# Okabe-Ito colorblind-safe categorical palette (one color per input file, fixed order).
PALETTE = [
    "#0072B2",  # blue
    "#D55E00",  # vermillion
    "#009E73",  # bluish green
    "#CC79A7",  # reddish purple
    "#E69F00",  # orange
    "#56B4E9",  # sky blue
    "#000000",  # black
]


@dataclass
class Run:
    """One loaded Kalman output file, ready to plot."""
    path: str              # path as given on the command line
    label: str             # legend label (may be disambiguated from the path)
    t: np.ndarray          # time axis (seconds, or step index)
    t_label: str           # "Time [s]" or "Step"
    pos: np.ndarray        # (N, 3) -> x, y, z  [m]
    rpy: np.ndarray        # (N, 3) -> roll, pitch, yaw  [deg]


def quat_to_euler_deg(quat_wxyz: np.ndarray, unwrap: bool = False) -> np.ndarray:
    """Convert (N,4) quaternions (w,x,y,z) to (N,3) roll/pitch/yaw in degrees (ZYX)."""
    w, x, y, z = (quat_wxyz[:, i] for i in range(4))

    roll = np.arctan2(2.0 * (w * x + y * z), 1.0 - 2.0 * (x * x + y * y))
    # asin argument clamped to [-1, 1] to stay valid through numerical noise.
    pitch = np.arcsin(np.clip(2.0 * (w * y - z * x), -1.0, 1.0))
    yaw = np.arctan2(2.0 * (w * z + x * y), 1.0 - 2.0 * (y * y + z * z))

    rpy = np.column_stack((roll, pitch, yaw))
    if unwrap:
        rpy = np.unwrap(rpy, axis=0)  # remove +/-pi wrap jumps for a continuous curve
    return np.degrees(rpy)


def load_run(path: str, trim_preinit: bool = False, unwrap: bool = False) -> Run:
    """Load one Kalman CSV, auto-detecting header and timestamp-vs-step first column."""
    # Peek at the first line to decide whether there's a header row.
    with open(path) as f:
        first_line = f.readline().strip()
    has_header = any(c.isalpha() for c in first_line)

    data = np.genfromtxt(path, delimiter=",", skip_header=1 if has_header else 0)
    if data.ndim == 1:  # a single data row
        data = data.reshape(1, -1)
    if data.shape[1] < 11:
        raise ValueError(
            f"{path}: expected >=11 columns "
            f"(t/step, pos*3, vel*3, quat*4), got {data.shape[1]}"
        )

    col0 = data[:, 0]
    pos = data[:, 1:4]
    quat = data[:, 7:11]  # w, x, y, z

    if trim_preinit:
        # Drop leading pre-init rows: position exactly 0 and identity quaternion.
        preinit = (
            np.all(pos == 0.0, axis=1)
            & (quat[:, 0] == 1.0)
            & np.all(quat[:, 1:] == 0.0, axis=1)
        )
        keep = np.argmax(~preinit) if (~preinit).any() else 0
        data, col0, pos, quat = data[keep:], col0[keep:], pos[keep:], quat[keep:]

    # A nanosecond ROS timestamp is a large monotonic integer; a step index is small.
    if col0.size and col0[0] > 1e6:
        t = (col0 - col0[0]) * 1e-9
        t_label = "Time [s]"
    else:
        t = col0.astype(float)
        t_label = "Step"

    return Run(
        path=path,
        label=os.path.basename(path),
        t=t,
        t_label=t_label,
        pos=pos,
        rpy=quat_to_euler_deg(quat, unwrap=unwrap),
    )


def assign_labels(runs: list[Run]) -> None:
    """Give each run a unique legend label; fall back to the full path on basename clashes."""
    from collections import Counter
    counts = Counter(os.path.basename(r.path) for r in runs)
    for run in runs:
        base = os.path.basename(run.path)
        run.label = base if counts[base] == 1 else run.path


def _style_axis(ax) -> None:
    ax.grid(True, color="0.85", linewidth=0.6)
    ax.set_axisbelow(True)
    for side in ("top", "right"):
        ax.spines[side].set_visible(False)


def plot_state(runs: list[Run], title: str | None) -> plt.Figure:
    """3x2 grid: left column = position X/Y/Z [m], right column = roll/pitch/yaw [deg]."""
    fig, axes = plt.subplots(3, 2, figsize=(13, 8), sharex=True)

    pos_labels = ("North  (world X) [m]", "East  (world Y) [m]", "Down  (world Z) [m]")
    rpy_labels = ("Roll [deg]", "Pitch [deg]", "Yaw [deg]")

    for r_i, run in enumerate(runs):
        color = PALETTE[r_i % len(PALETTE)]
        for row in range(3):
            axes[row, 0].plot(run.t, run.pos[:, row], color=color, linewidth=1.2,
                              label=run.label)
            axes[row, 1].plot(run.t, run.rpy[:, row], color=color, linewidth=1.2,
                              label=run.label)

    for row in range(3):
        axes[row, 0].set_ylabel(pos_labels[row])
        axes[row, 1].set_ylabel(rpy_labels[row])
        for col in range(2):
            _style_axis(axes[row, col])

    axes[0, 0].set_title("Position")
    axes[0, 1].set_title("Orientation")
    axes[2, 0].set_xlabel(runs[0].t_label)
    axes[2, 1].set_xlabel(runs[0].t_label)

    # Legend only makes sense when there is more than one run to tell apart.
    if len(runs) > 1:
        handles, labels = axes[0, 0].get_legend_handles_labels()
        fig.legend(handles, labels, loc="upper center", ncol=min(len(runs), 4),
                   frameon=False, bbox_to_anchor=(0.5, 1.0))
        top = 0.92
    else:
        top = 0.95

    if title:
        fig.suptitle(title, y=0.995, fontsize=11)
    fig.tight_layout(rect=(0, 0, 1, top))
    return fig


def plot_trajectory(runs: list[Run]) -> plt.Figure:
    """North-up top-down path (equal aspect) plus depth over time.

    World frame is NED (world X = North, world Y = East, Z = Down), so the map
    puts East on the horizontal axis and North on the vertical axis.
    """
    fig, (ax_xy, ax_z) = plt.subplots(1, 2, figsize=(13, 5.5))

    for r_i, run in enumerate(runs):
        color = PALETTE[r_i % len(PALETTE)]
        east, north = run.pos[:, 1], run.pos[:, 0]
        ax_xy.plot(east, north, color=color, linewidth=1.2, label=run.label)
        ax_xy.plot(east[0], north[0], "o", color=color, markersize=7)  # start
        ax_z.plot(run.t, run.pos[:, 2], color=color, linewidth=1.2, label=run.label)

    ax_xy.set_title("Top-down path (start = dot)")
    ax_xy.set_xlabel("East  (world Y) [m]")
    ax_xy.set_ylabel("North  (world X) [m]")
    ax_xy.set_aspect("equal", adjustable="datalim")
    _style_axis(ax_xy)

    ax_z.set_title("Depth (world Z, down-positive)")
    ax_z.set_xlabel(runs[0].t_label)
    ax_z.set_ylabel("Down  (world Z) [m]")
    _style_axis(ax_z)

    if len(runs) > 1:
        ax_xy.legend(frameon=False, fontsize=9)
    fig.tight_layout()
    return fig


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("files", nargs="+",
                        help="one or more Kalman output CSV files (overlaid together)")
    parser.add_argument("-o", "--output",
                        help="save the figure to this path instead of showing it "
                             "(e.g. out.png). A trajectory figure is saved as *_traj.png")
    parser.add_argument("-t", "--trajectory", action="store_true",
                        help="also draw a top-down XY path + depth-over-time figure")
    parser.add_argument("--trim", action="store_true",
                        help="drop leading pre-init rows (position 0, identity quaternion)")
    parser.add_argument("--unwrap", action="store_true",
                        help="unwrap roll/pitch/yaw to remove +/-180 deg jumps")
    parser.add_argument("--title", help="figure title (default: filename for a single run)")
    args = parser.parse_args(argv)

    runs: list[Run] = []
    for path in args.files:
        try:
            runs.append(load_run(path, trim_preinit=args.trim, unwrap=args.unwrap))
        except (OSError, ValueError) as exc:
            print(f"error: {exc}", file=sys.stderr)
            return 1
    if any(run.t.size == 0 for run in runs):
        print("error: a file has no rows to plot (all trimmed?)", file=sys.stderr)
        return 1

    # Warn when two arguments resolve to the same file (e.g. overlapping globs like
    # kalman_*.csv and kalman_replay_*.csv) -- otherwise the lines silently overlap.
    real = [os.path.realpath(p) for p in args.files]
    dup_paths = sorted({p for p in args.files if real.count(os.path.realpath(p)) > 1})
    if dup_paths:
        print("warning: the same file was passed more than once, so its lines overlap "
              "(only the top color shows):\n  " + "\n  ".join(dup_paths), file=sys.stderr)

    assign_labels(runs)

    title = args.title or (runs[0].label if len(runs) == 1 else None)
    fig_state = plot_state(runs, title)
    fig_traj = plot_trajectory(runs) if args.trajectory else None

    # Save when asked, or when there's no display; otherwise show interactively.
    if args.output or not os.environ.get("DISPLAY"):
        out = args.output or (os.path.splitext(os.path.basename(runs[0].path))[0] + "_plot.png")
        fig_state.savefig(out, dpi=150)
        print(f"wrote {out}")
        if fig_traj is not None:
            base, ext = os.path.splitext(out)
            traj_out = f"{base}_traj{ext or '.png'}"
            fig_traj.savefig(traj_out, dpi=150)
            print(f"wrote {traj_out}")
    else:
        plt.show()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

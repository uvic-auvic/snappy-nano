#!/usr/bin/env python3
"""Plot the sub's X-Y ground track (top-down 2D path) from Kalman output CSV(s).

Reuses the loader from plot_kalman.py, so it accepts the same file formats
(kalman_<run>.csv / kalman_replay_<run>.csv with a nanosecond timestamp, or
filter_output.csv with a step index).

  * Single file:  the path is drawn as a line colored by time (dark -> bright),
                  so you can see which way the sub travelled, with start/end markers.
  * Several files: each run is one categorical color, overlaid, with a legend.

The filter's world frame is NED (world X = North, world Y = East, Z = Down), so
the map is drawn North-up / East-right: East on the horizontal axis, North on the
vertical axis. Equal aspect ratio keeps the path's true shape (1 m East looks the
same length as 1 m North).

Examples:
    ./plot_path.py state_estimator_outputs/kalman_1783280617402906537.csv
    ./plot_path.py kalman_a.csv kalman_b.csv           # overlay two runs
    ./plot_path.py filter_output.csv -o path.png --trim
"""

from __future__ import annotations

import argparse
import os
import sys

import numpy as np

# Pick a non-interactive backend before pyplot is imported when there's no display.
import matplotlib
if not os.environ.get("DISPLAY"):
    matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.collections import LineCollection

# Load from the sibling module so the two scripts share one CSV parser / palette.
from plot_kalman import PALETTE, Run, assign_labels, load_run

START_COLOR = "#009E73"  # bluish green
END_COLOR = "#D55E00"    # vermillion


def _style_axis(ax) -> None:
    ax.grid(True, color="0.85", linewidth=0.6)
    ax.set_axisbelow(True)
    for side in ("top", "right"):
        ax.spines[side].set_visible(False)


def _project(pos: np.ndarray, psi0: float | None) -> tuple[np.ndarray, np.ndarray]:
    """Map world position to (horizontal, vertical) plot coords.

    World frame is NED: pos[:,0]=North, pos[:,1]=East (see kalman.h q_enu_to_world_).
      * psi0 is None -> world map: horizontal=East (right), vertical=North (up).
      * psi0 given   -> sub start frame: horizontal=starboard, vertical=forward,
                        where forward is the heading (yaw) psi0 [rad] at t=0.
    """
    north, east = pos[:, 0], pos[:, 1]
    if psi0 is None:
        return east, north
    c, s = np.cos(psi0), np.sin(psi0)
    forward = north * c + east * s        # along the start heading -> "up"
    starboard = -north * s + east * c     # 90 deg right of it       -> "right"
    return starboard, forward


def plot_path(runs: list[Run], title: str | None, color_by_time: bool = True,
              heading_up: bool = False) -> plt.Figure:
    fig, ax = plt.subplots(figsize=(8.5, 8))

    # In heading-up mode, rotate every run by the FIRST run's initial heading so
    # overlaid runs stay comparable and "up" is where the sub pointed at t=0.
    psi0 = np.radians(runs[0].rpy[0, 2]) if heading_up else None

    if len(runs) == 1 and color_by_time:
        run = runs[0]
        h, v = _project(run.pos, psi0)

        # Build per-segment lines so the path color can vary with time.
        points = np.column_stack((h, v)).reshape(-1, 1, 2)
        segments = np.concatenate((points[:-1], points[1:]), axis=1)
        lc = LineCollection(segments, cmap="viridis", linewidth=1.8)
        lc.set_array(run.t[:-1])  # color each segment by its start time
        ax.add_collection(lc)
        ax.autoscale()  # LineCollection doesn't grow the data limits on its own

        cbar = fig.colorbar(lc, ax=ax, pad=0.02)
        cbar.set_label(run.t_label)

        ax.plot(h[0], v[0], "o", color=START_COLOR, markersize=11,
                markeredgecolor="white", label="start", zorder=5)
        ax.plot(h[-1], v[-1], "s", color=END_COLOR, markersize=11,
                markeredgecolor="white", label="end", zorder=5)
        ax.legend(frameon=False, loc="best")
    else:
        for r_i, run in enumerate(runs):
            color = PALETTE[r_i % len(PALETTE)]
            h, v = _project(run.pos, psi0)
            ax.plot(h, v, color=color, linewidth=1.4, label=run.label)
            ax.plot(h[0], v[0], "o", color=color, markersize=8,
                    markeredgecolor="white", zorder=5)   # start
            ax.plot(h[-1], v[-1], "s", color=color, markersize=8,
                    markeredgecolor="white", zorder=5)   # end
        ax.legend(frameon=False, loc="best", title="run (o start, □ end)")

    if heading_up:
        ax.set_xlabel("Starboard  (right of start heading) [m]  →")
        ax.set_ylabel("Forward  (sub heading at t=0) [m]  ↑")
        heading_note = f"start heading = {runs[0].rpy[0, 2]:.0f}° from North"
        ax.set_title(f"{title}\n{heading_note}" if title else heading_note)
    else:
        ax.set_xlabel("East  (world Y) [m]  →")
        ax.set_ylabel("North  (world X) [m]  ↑")
        if title:
            ax.set_title(title)
    ax.set_aspect("equal", adjustable="datalim")  # undistorted ground track
    _style_axis(ax)
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
                        help="save the figure to this path instead of showing it")
    parser.add_argument("--trim", action="store_true",
                        help="drop leading pre-init rows (position 0, identity quaternion)")
    parser.add_argument("--no-time-color", action="store_true",
                        help="for a single run, draw a plain line instead of coloring by time")
    parser.add_argument("--heading-up", action="store_true",
                        help="draw in the sub's start frame (forward=up, starboard=right) "
                             "instead of the world North-up map")
    parser.add_argument("--title", help="plot title (default: filename for a single run)")
    args = parser.parse_args(argv)

    runs: list[Run] = []
    for path in args.files:
        try:
            runs.append(load_run(path, trim_preinit=args.trim))
        except (OSError, ValueError) as exc:
            print(f"error: {exc}", file=sys.stderr)
            return 1
    if any(run.pos.shape[0] < 2 for run in runs):
        print("error: a file has fewer than 2 rows -- nothing to draw a path from",
              file=sys.stderr)
        return 1

    # Warn when two arguments resolve to the same file (e.g. overlapping globs).
    real = [os.path.realpath(p) for p in args.files]
    dup_paths = sorted({p for p in args.files if real.count(os.path.realpath(p)) > 1})
    if dup_paths:
        print("warning: the same file was passed more than once, so its paths overlap:\n  "
              + "\n  ".join(dup_paths), file=sys.stderr)

    assign_labels(runs)

    title = args.title or (runs[0].label if len(runs) == 1 else None)
    fig = plot_path(runs, title, color_by_time=not args.no_time_color,
                    heading_up=args.heading_up)

    if args.output or not os.environ.get("DISPLAY"):
        out = args.output or (os.path.splitext(os.path.basename(runs[0].path))[0] + "_path.png")
        fig.savefig(out, dpi=150)
        print(f"wrote {out}")
    else:
        plt.show()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

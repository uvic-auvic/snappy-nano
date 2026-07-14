#!/usr/bin/env python3
"""Build test_kalman, replay one recorded run, and plot replay vs recorded output.

Usage:
    ./compare_replay.py                      # newest run in state_estimator_outputs/
    ./compare_replay.py 1783986490533919964  # specific run (nanosecond id)
    ./compare_replay.py 1783986490533919964 --offset 1.1   # pin the DVL clock offset
    ./compare_replay.py 1783986490533919964 -o compare.png # save plot instead of showing

Steps:
  1. g++-compiles test_kalman (ROS-free build, same as the docs in test_kalman.cpp)
  2. runs it on state_estimator_outputs/<run_id>/, writing kalman_replay_<run_id>.csv there
  3. runs plot_kalman.py with the recorded and replayed CSVs overlaid
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent
OUTPUTS = REPO / "state_estimator_outputs"

BUILD_CMD = [
    "g++", "-O2", "-std=c++17",
    "-I/usr/include/eigen3",
    "-Isrc/snappy_cpp/src/include/Inc",
    "src/snappy_cpp/tests/test_kalman.cpp",
    "src/snappy_cpp/src/include/src/kalman.cpp",
    "-o", "test_kalman",
]


def newest_run_id() -> str:
    runs = sorted(
        (d.name for d in OUTPUTS.iterdir()
         if d.is_dir() and (d / f"kalman_{d.name}.csv").exists()),
        key=lambda name: (len(name), name),  # ns timestamps: chronological order
    )
    if not runs:
        sys.exit(f"ERROR: no run directories with kalman_<run>.csv in {OUTPUTS}")
    return runs[-1]


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("run_id", nargs="?", default=None,
                    help="nanosecond run id (default: newest run)")
    ap.add_argument("--offset", type=float, default=None,
                    help="DVL clock offset in seconds (default: test_kalman auto-scans)")
    ap.add_argument("-o", "--output", default=None,
                    help="save the plot to this file instead of showing it")
    args = ap.parse_args()

    run_id = args.run_id or newest_run_id()
    run_dir = OUTPUTS / run_id
    recorded = run_dir / f"kalman_{run_id}.csv"
    replay = run_dir / f"kalman_replay_{run_id}.csv"
    if not recorded.exists():
        sys.exit(f"ERROR: {recorded} not found")

    print(f"== Building test_kalman ==")
    subprocess.run(BUILD_CMD, cwd=REPO, check=True)

    print(f"== Replaying run {run_id} ==")
    replay_cmd = ["./test_kalman", str(run_dir)]
    if args.offset is not None:
        replay_cmd.append(str(args.offset))
    # test_kalman exits 1 when the regression bounds fail, which is expected
    # while params are being tuned — plot regardless, but surface the status.
    result = subprocess.run(replay_cmd, cwd=REPO)
    if result.returncode != 0:
        print("(comparison bounds FAILed — plotting anyway)")
    if not replay.exists():
        sys.exit(f"ERROR: {replay} was not written")

    print(f"== Plotting recorded vs replay ==")
    plot_cmd = [sys.executable, str(REPO / "plot_kalman.py"), str(recorded), str(replay)]
    if args.output:
        plot_cmd += ["-o", args.output]
    subprocess.run(plot_cmd, cwd=REPO, check=True)


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""
Assemble the mechanical sub-component STLs into a single combined mesh.

This only performs cheap cleanup and per-part decimation; the output is
intended to feed the heavier watertight + global simplification step that
`scripts/find_added_mass.py` already implements.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path

import vtk


# Default locations (kept relative so invoking from the repo root works)
DEFAULT_PARTS_DIR = Path("src/snappy_cpp/worlds/raw_parts/full_assembly")
DEFAULT_OUTPUT = Path("src/snappy_cpp/worlds/2025-2026_Full_ASSEMBLY_combined_raw.stl")


# Words that clearly identify internal hardware or fasteners we can drop
SKIP_KEYWORDS = {
    "screw",
    "nut",
    "o-ring",
    "spacer",
    "clip",
    "board",
    "jetson",
    "relay",
    "fuse",
    "knob",
    "clevis",
    "washer",
}

# Smaller features that still live on the outer wetted surface
ALLOW_IF_SMALL = {
    "thruster",
    "ball_dropper placeholder",
    "ball dropper placeholder",
    "torpedo",
    "kill_switch",
    "kill switch",
    "hydrophone",
    "dvl",
}


@dataclass
class PartInfo:
    path: Path
    faces: int
    max_extent: float


def triangulate(poly: vtk.vtkPolyData) -> vtk.vtkPolyData:
    tri = vtk.vtkTriangleFilter()
    tri.SetInputData(poly)
    tri.PassLinesOff()
    tri.PassVertsOff()
    tri.Update()
    return tri.GetOutput()


def clean(poly: vtk.vtkPolyData) -> vtk.vtkPolyData:
    cleaner = vtk.vtkCleanPolyData()
    cleaner.SetInputData(poly)
    cleaner.Update()
    return cleaner.GetOutput()


def per_part_decimate(poly: vtk.vtkPolyData, target_faces: int) -> vtk.vtkPolyData:
    current = poly.GetNumberOfPolys()
    if current <= target_faces:
        return poly

    reduction = 1.0 - (target_faces / float(current))
    reduction = min(max(reduction, 0.0), 0.99)

    dec = vtk.vtkQuadricDecimation()
    dec.SetInputData(poly)
    dec.SetTargetReduction(reduction)
    dec.AttributeErrorMetricOn()
    dec.VolumePreservationOn()
    dec.Update()
    out = dec.GetOutput()
    return out if out and out.GetNumberOfPolys() else poly


def should_keep(name: str, max_extent: float, min_extent: float) -> bool:
    lname = name.lower()
    if any(word in lname for word in SKIP_KEYWORDS):
        return False
    if max_extent >= min_extent:
        return True
    return any(word in lname for word in ALLOW_IF_SMALL)


def load_parts(parts_dir: Path, min_extent: float, per_part_faces: int) -> tuple[list[vtk.vtkPolyData], list[PartInfo]]:
    meshes: list[vtk.vtkPolyData] = []
    infos: list[PartInfo] = []

    for path in sorted(parts_dir.glob("*.STL")):
        reader = vtk.vtkSTLReader()
        reader.SetFileName(str(path))
        reader.Update()
        poly = reader.GetOutput()
        if poly is None or poly.GetNumberOfPolys() == 0:
            continue

        bounds = poly.GetBounds()
        max_extent = max(bounds[1] - bounds[0], bounds[3] - bounds[2], bounds[5] - bounds[4])

        if not should_keep(path.name, max_extent, min_extent):
            continue

        poly = triangulate(poly)
        poly = clean(poly)
        poly = per_part_decimate(poly, per_part_faces)
        poly = clean(poly)

        meshes.append(poly)
        infos.append(PartInfo(path=path, faces=poly.GetNumberOfPolys(), max_extent=max_extent))

    return meshes, infos


def append_meshes(meshes: list[vtk.vtkPolyData]) -> vtk.vtkPolyData:
    append = vtk.vtkAppendPolyData()
    for mesh in meshes:
        append.AddInputData(mesh)

    append.Update()
    result = append.GetOutput()
    result = triangulate(result)
    result = clean(result)
    return result


def write_binary_stl(poly: vtk.vtkPolyData, output: Path) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    writer = vtk.vtkSTLWriter()
    writer.SetFileName(str(output))
    writer.SetFileTypeToBinary()
    writer.SetInputData(poly)
    writer.Write()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--parts-dir", type=Path, default=DEFAULT_PARTS_DIR, help="Folder containing the exported per-part STLs.")
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT, help="Destination STL for the combined mesh.")
    parser.add_argument("--min-extent", type=float, default=80.0, help="Keep parts whose largest dimension (mm) exceeds this value.")
    parser.add_argument("--per-part-target", type=int, default=12000, help="Soft triangle budget per part before appending.")
    args = parser.parse_args()

    meshes, infos = load_parts(args.parts_dir, args.min_extent, args.per_part_target)
    if not meshes:
        print("[assemble] No parts kept; check --min-extent or directory content.")
        return 1

    combined = append_meshes(meshes)
    write_binary_stl(combined, args.output)

    total_faces = combined.GetNumberOfPolys()
    print(f"[assemble] wrote {args.output} (faces: {total_faces})")
    print(f"[assemble] kept {len(infos)} parts (per-part target {args.per_part_target})")
    for info in sorted(infos, key=lambda x: x.faces, reverse=True)[:10]:
        print(f"  {info.path.name} -> {info.faces} faces, max extent {info.max_extent:.1f} mm")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())


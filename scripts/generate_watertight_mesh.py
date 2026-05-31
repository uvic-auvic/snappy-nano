#!/usr/bin/env python3
"""
Convert the combined assembly STL into a watertight, decimated hull for hydrodynamic analysis.

Pipeline:
  1. Load the combined mesh (typically from scripts/build_full_assembly_mesh.py).
  2. Lightly decimate to trim triangle count before voxelization.
  3. Voxelize via vtkSignedDistance to obtain a watertight implicit surface.
  4. Extract the iso-surface, keep the largest component, smooth, and fill remaining holes.
  5. Quadric decimate down to the target face budget and write the simplified STL.
"""

from __future__ import annotations

import argparse
import struct
from pathlib import Path

import vtk


DEFAULT_INPUT = Path("src/snappy_cpp/worlds/2025-2026_Full_ASSEMBLY_combined_raw.stl")
DEFAULT_OUTPUT = Path("src/snappy_cpp/worlds/2025-2026_Full_ASSEMBLY_simplified.stl")


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


def ensure_normals(poly: vtk.vtkPolyData) -> vtk.vtkPolyData:
    normals = vtk.vtkPolyDataNormals()
    normals.SetInputData(poly)
    normals.ConsistencyOn()
    normals.AutoOrientNormalsOn()
    normals.SplittingOff()
    normals.Update()
    return normals.GetOutput()


def decimate(poly: vtk.vtkPolyData, target_faces: int, *, preserve_topology: bool = False) -> vtk.vtkPolyData:
    current = poly.GetNumberOfPolys()
    if current <= target_faces:
        return poly
    reduction = 1.0 - (target_faces / float(current))
    reduction = min(max(reduction, 0.0), 0.995)
    if preserve_topology:
        dec = vtk.vtkDecimatePro()
        dec.SetInputData(poly)
        dec.SetTargetReduction(reduction)
        dec.PreserveTopologyOn()
        dec.SplittingOff()
        dec.BoundaryVertexDeletionOff()
        dec.Update()
    else:
        dec = vtk.vtkQuadricDecimation()
        dec.SetInputData(poly)
        dec.SetTargetReduction(reduction)
        dec.AttributeErrorMetricOn()
        dec.VolumePreservationOn()
        dec.Update()
    out = dec.GetOutput()
    return out if out and out.GetNumberOfPolys() else poly


def connectivity_largest(poly: vtk.vtkPolyData) -> vtk.vtkPolyData:
    conn = vtk.vtkConnectivityFilter()
    conn.SetInputData(poly)
    conn.SetExtractionModeToLargestRegion()
    conn.Update()
    return conn.GetOutput()


def fill_holes(poly: vtk.vtkPolyData, size: float) -> vtk.vtkPolyData:
    filler = vtk.vtkFillHolesFilter()
    filler.SetInputData(poly)
    filler.SetHoleSize(float(size))
    filler.Update()
    return filler.GetOutput()


def windowed_sinc(poly: vtk.vtkPolyData, iterations: int = 20, passband: float = 0.01) -> vtk.vtkPolyData:
    smoother = vtk.vtkWindowedSincPolyDataFilter()
    smoother.SetInputData(poly)
    smoother.SetNumberOfIterations(iterations)
    smoother.BoundarySmoothingOff()
    smoother.FeatureEdgeSmoothingOff()
    smoother.SetFeatureAngle(120.0)
    smoother.SetPassBand(passband)
    smoother.NonManifoldSmoothingOn()
    smoother.NormalizeCoordinatesOn()
    smoother.Update()
    return smoother.GetOutput()


def make_signed_distance(poly: vtk.vtkPolyData, dims: int, padding: float, radius_scale: float) -> vtk.vtkImageData:
    bounds = poly.GetBounds()
    max_dim = max(bounds[1] - bounds[0], bounds[3] - bounds[2], bounds[5] - bounds[4])

    signed = vtk.vtkSignedDistance()
    signed.SetInputData(poly)
    signed.SetRadius(max_dim * radius_scale)
    signed.SetBounds(bounds[0] - padding,
                     bounds[1] + padding,
                     bounds[2] - padding,
                     bounds[3] + padding,
                     bounds[4] - padding,
                     bounds[5] + padding)
    signed.SetDimensions(dims, dims, dims)
    signed.Update()
    return signed.GetOutput()


def extract_isosurface(image: vtk.vtkImageData, iso_value: float = 0.0) -> vtk.vtkPolyData:
    contour = vtk.vtkContourFilter()
    contour.SetInputData(image)
    contour.SetValue(0, iso_value)
    contour.Update()
    return contour.GetOutput()


def summarize(poly: vtk.vtkPolyData, label: str) -> None:
    bounds = poly.GetBounds()
    extents = [bounds[1] - bounds[0], bounds[3] - bounds[2], bounds[5] - bounds[4]]
    print(f"[{label}] faces: {poly.GetNumberOfPolys()}, extents: {extents[0]:.1f} x {extents[1]:.1f} x {extents[2]:.1f} mm")


def feature_edge_count(poly: vtk.vtkPolyData) -> int:
    feat = vtk.vtkFeatureEdges()
    feat.SetInputData(poly)
    feat.BoundaryEdgesOn()
    feat.FeatureEdgesOff()
    feat.NonManifoldEdgesOn()
    feat.ManifoldEdgesOff()
    feat.Update()
    return feat.GetOutput().GetNumberOfCells()


def stl_face_count(path: Path) -> int | None:
    """Quick face count for the final STL."""
    try:
        size = path.stat().st_size
        with path.open("rb") as fh:
            header = fh.read(80)
            if len(header) < 80:
                return None
            raw = fh.read(4)
            if len(raw) == 4:
                (triangles,) = struct.unpack("<I", raw)
                expected = 84 + triangles * 50
                if expected == size or (expected < size and size - expected < 100):
                    return int(triangles)
    except OSError:
        return None
    return None


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, default=DEFAULT_INPUT, help="Combined STL produced by build_full_assembly_mesh.py")
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT, help="Destination watertight + simplified STL")
    parser.add_argument("--pre-decimate", type=int, default=250_000, help="Triangle budget before voxelization")
    parser.add_argument("--voxel-dims", type=int, default=220, help="Voxel resolution per axis for signed distance")
    parser.add_argument("--voxel-padding", type=float, default=40.0, help="Padding (mm) applied around the bounding box")
    parser.add_argument("--voxel-radius-scale", type=float, default=1.0 / 18.0, help="Neighborhood radius as a fraction of max dimension")
    parser.add_argument("--hole-fill", type=float, default=150.0, help="Maximum hole radius to fill after iso-surface extraction (mm)")
    parser.add_argument("--smooth-iters", type=int, default=20, help="Windowed sinc smoothing iterations after voxelization")
    parser.add_argument("--smooth-passband", type=float, default=0.02, help="Windowed sinc passband")
    parser.add_argument("--target-faces", type=int, default=80_000, help="Final face budget for hydrodynamic solver")
    parser.add_argument("--post-fill", type=float, default=120.0, help="Hole size to fill after final decimation (0 to disable)")
    parser.add_argument("--post-smooth-iters", type=int, default=10, help="Smoothing iterations after post-fill")
    parser.add_argument("--post-smooth-passband", type=float, default=0.04, help="Smoothing passband after post-fill")
    parser.add_argument("--preserve-topology", action="store_true", help="Use vtkDecimatePro with topology preservation for final reductions.")
    parser.add_argument("--revoxelize", action="store_true", help="Run a low-res re-voxelization after post-fill to guarantee watertightness.")
    parser.add_argument("--revoxelize-dims", type=int, default=160, help="Grid resolution for the re-voxelization pass.")
    parser.add_argument("--revoxelize-padding", type=float, default=20.0, help="Padding (mm) for re-voxelization bounds.")
    parser.add_argument("--revoxelize-radius-scale", type=float, default=1.0 / 25.0, help="Neighborhood radius for the re-voxelization signed distance.")
    args = parser.parse_args()

    if not args.input.exists():
        print(f"[watertight] Input STL missing: {args.input}")
        return 1

    reader = vtk.vtkSTLReader()
    reader.SetFileName(str(args.input))
    reader.Update()
    poly = reader.GetOutput()
    if poly is None or poly.GetNumberOfPolys() == 0:
        print("[watertight] Input mesh empty.")
        return 1

    poly = triangulate(poly)
    poly = clean(poly)
    summarize(poly, "input")

    poly = decimate(poly, args.pre_decimate)
    poly = clean(poly)
    summarize(poly, "pre-decimated")

    poly = ensure_normals(poly)

    bounds = poly.GetBounds()
    max_dim = max(bounds[1] - bounds[0], bounds[3] - bounds[2], bounds[5] - bounds[4])
    padding = max(args.voxel_padding, max_dim * 0.05)

    print(f"[watertight] Generating signed distance grid {args.voxel_dims}^3, padding {padding:.1f} mm, radius={max_dim * args.voxel_radius_scale:.1f} mm")
    signed = make_signed_distance(poly, args.voxel_dims, padding, args.voxel_radius_scale)

    iso = extract_isosurface(signed, 0.0)
    iso = triangulate(iso)
    iso = clean(iso)
    summarize(iso, "voxelized")
    print(f"[watertight] feature edges after voxelization: {feature_edge_count(iso)}")

    iso = connectivity_largest(iso)
    iso = clean(iso)
    summarize(iso, "largest-component")
    print(f"[watertight] feature edges after connectivity: {feature_edge_count(iso)}")

    iso = fill_holes(iso, args.hole_fill)
    iso = clean(iso)
    summarize(iso, "hole-filled")
    print(f"[watertight] feature edges after hole fill: {feature_edge_count(iso)}")

    iso = windowed_sinc(iso, iterations=args.smooth_iters, passband=args.smooth_passband)
    iso = clean(iso)
    iso = ensure_normals(iso)
    summarize(iso, "smoothed")
    print(f"[watertight] feature edges after smoothing: {feature_edge_count(iso)}")

    iso = decimate(iso, args.target_faces, preserve_topology=args.preserve_topology)
    iso = clean(iso)
    iso = ensure_normals(iso)
    summarize(iso, "decimated")
    boundary_edges = feature_edge_count(iso)
    print(f"[watertight] feature edges after decimation: {boundary_edges}")

    if args.post_fill and args.post_fill > 0:
        iso = fill_holes(iso, args.post_fill)
        iso = clean(iso)
        iso = windowed_sinc(iso, iterations=args.post_smooth_iters, passband=args.post_smooth_passband)
        iso = clean(iso)
        iso = ensure_normals(iso)
        summarize(iso, "post-fill")
        boundary_edges = feature_edge_count(iso)
        print(f"[watertight] feature edges after post-fill: {boundary_edges}")

    if args.revoxelize:
        bounds = iso.GetBounds()
        max_dim = max(bounds[1] - bounds[0], bounds[3] - bounds[2], bounds[5] - bounds[4])
        padding = max(args.revoxelize_padding, max_dim * 0.05)
        print(f"[watertight] Re-voxelizing simplified mesh ({args.revoxelize_dims}^3).")
        signed2 = make_signed_distance(iso, args.revoxelize_dims, padding, args.revoxelize_radius_scale)
        iso = extract_isosurface(signed2, 0.0)
        iso = triangulate(iso)
        iso = clean(iso)
        iso = connectivity_largest(iso)
        iso = clean(iso)
        iso = windowed_sinc(iso, iterations=args.post_smooth_iters, passband=args.post_smooth_passband)
        iso = clean(iso)
        iso = decimate(iso, args.target_faces, preserve_topology=args.preserve_topology)
        iso = clean(iso)
        iso = ensure_normals(iso)
        summarize(iso, "revoxelized")
        boundary_edges = feature_edge_count(iso)
        print(f"[watertight] feature edges after revoxelization: {boundary_edges}")

    if boundary_edges > 0:
        print("[watertight] Warning: mesh still has boundary edges; consider increasing --voxel-dims or --hole-fill.")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    writer = vtk.vtkSTLWriter()
    writer.SetFileName(str(args.output))
    writer.SetInputData(iso)
    writer.SetFileTypeToBinary()
    writer.Write()

    final_faces = stl_face_count(args.output)
    print(f"[watertight] wrote {args.output} (faces: {final_faces})")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

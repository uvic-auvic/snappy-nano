#!/usr/bin/env python3
"""
Compute 6x6 added mass A(∞) for your AUV and emit a Gazebo plugin snippet.

- Uses existing STL: src/snappy_cpp/worlds/2025-2026_Full_ASSEMBLY.stl
- Preprocess: repair + decimate to ~8k faces into *_simplified.stl (VTK headless, fallback meshlabserver)
- Frame: CAD is Y-up, X-sideways, Z-forward -> rotate Rx(+90°), Rz(+90°) to Z-up/X-forward
- Deep water (free_surface = ∞, water_depth = ∞), ω = ∞ (fallback to 1e6 if needed)
"""

from __future__ import annotations
from pathlib import Path
import math
import os
import numpy as np
import sys
import shutil
import subprocess
import tempfile
import textwrap
import traceback
import struct

# -------- Paths --------
DAE_PATH = Path("src/snappy_cpp/worlds/2025-2026_Full_ASSEMBLY.dae")
STL_PATH = DAE_PATH.with_suffix(".stl")
WORLD_DIR = DAE_PATH.parent

# -------- Rotation (CAD: Y up, X sideways, Z forward  ->  Z up, X forward, Y sideways) --------
ROT_X = +math.pi / 2
ROT_Z = +math.pi / 2

# -------- Preprocess thresholds --------
SIMPLIFY_IF_FACE_COUNT_OVER = 80_000
TARGET_FACE_COUNT = 8_000
CLOSE_HOLES_MAX = 50  # small holes only
FORCE_RESIMPLIFY = os.environ.get("FIND_ADDED_MASS_FORCE_RESIMPLIFY", "").lower() in {"1", "true", "yes", "y"}

DEFAULT_CAPY_CACHE = Path("build/capytaine_cache")
if "CAPYTAINE_CACHE_DIR" not in os.environ:
    os.environ["CAPYTAINE_CACHE_DIR"] = str((DEFAULT_CAPY_CACHE).resolve())
DEFAULT_CAPY_CACHE.mkdir(parents=True, exist_ok=True)

# ====================== Helpers ======================
def stl_face_count(stl: Path) -> int | None:
    """Return number of triangles in STL (works for binary or ASCII)."""
    try:
        size = stl.stat().st_size
        with stl.open("rb") as fh:
            header = fh.read(80)
            if len(header) < 80:
                return None
            raw = fh.read(4)
            if len(raw) == 4:
                (triangles,) = struct.unpack("<I", raw)
                expected = 84 + triangles * 50
                # binary STL usually matches exactly; allow for minor padding
                if expected == size or (expected < size and size - expected < 100):
                    return int(triangles)
            # ASCII fallback: count 'facet normal'
            fh.seek(0)
            needle = b"facet normal"
            overlap = len(needle) - 1
            count = 0
            tail = b""
            while True:
                chunk = fh.read(1_048_576)
                if not chunk:
                    break
                data = tail + chunk
                count += data.count(needle)
                tail = data[-overlap:] if overlap else b""
            return count or None
    except Exception as e:
        print(f"[preprocess] face count failed for {stl.name}: {e}")
        return None

def export_dae_to_stl(dae: Path, stl: Path) -> bool:
    """Best-effort DAE→STL conversion so users only need the CAD export once."""
    if not dae.exists():
        print(f"[preprocess] DAE not found: {dae}")
        return False

    # Try PyMeshLab first for in-process conversion.
    try:
        os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
        import pymeshlab as pml
        ms = pml.MeshSet()
        ms.load_new_mesh(str(dae))
        ms.save_current_mesh(str(stl), binary=True)
        print(f"[preprocess] Exported STL via PyMeshLab: {stl.name}")
        return stl.exists()
    except Exception as e:
        print(f"[preprocess] PyMeshLab export failed: {e}")

    # Fallback: meshlabserver CLI
    exe = shutil.which("meshlabserver")
    if exe:
        cmd = [
            exe,
            "-i", str(dae),
            "-o", str(stl),
            "-m", "vn",
        ]
        env = os.environ.copy()
        env.setdefault("QT_QPA_PLATFORM", "offscreen")
        env.setdefault("LIBGL_ALWAYS_SOFTWARE", "1")
        print("[preprocess] (meshlabserver) exporting STL:", " ".join(cmd))
        try:
            subprocess.run(cmd, check=True, env=env)
            return stl.exists()
        except subprocess.CalledProcessError as e:
            print("[preprocess] meshlabserver export failed:", e)

    print("[preprocess] Could not convert DAE to STL automatically.")
    return False

def simplify_with_meshlabserver(stl_in: Path, stl_out: Path,
                                target_faces: int = TARGET_FACE_COUNT,
                                close_holes_max: int = CLOSE_HOLES_MAX) -> int | None:
    """Force decimation via meshlabserver using a .mlx filter script. Returns face count or None."""
    exe = shutil.which("meshlabserver")
    if not exe:
        print("[preprocess] meshlabserver not found on PATH. Install 'meshlab' package to enable fallback.")
        return None

    mlx = textwrap.dedent(f"""\
    <!DOCTYPE FilterScript>
    <FilterScript>
      <!-- remove dupes/unref -->
      <filter name="Remove Duplicate Faces"/>
      <filter name="Remove Duplicate Vertices"/>
      <filter name="Remove Unreferenced Vertices"/>
      <!-- try to close small holes -->
      <filter name="Close Holes">
        <Param type="RichInt" name="MaxHoleSize" value="{int(close_holes_max)}"/>
      </filter>
      <!-- main decimation -->
      <filter name="Simplification: Quadric Edge Collapse Decimation">
        <Param type="RichInt"   name="TargetFaceNum" value="{int(target_faces)}"/>
        <Param type="RichFloat" name="TargetPerc"    value="1"/>    <!-- ignored when TargetFaceNum set -->
        <Param type="RichFloat" name="QualityThr"    value="0.3"/>
        <Param type="RichBool"  name="PreserveBoundary" value="true"/>
        <Param type="RichBool"  name="PreserveNormal"   value="true"/>
        <Param type="RichBool"  name="OptimalPlacement" value="true"/>
        <Param type="RichBool"  name="PlanarQuadric"    value="true"/>
      </filter>
      <filter name="Remove Unreferenced Vertices"/>
    </FilterScript>
    """)

    with tempfile.TemporaryDirectory() as tmpd:
        mlx_path = Path(tmpd) / "decimate.mlx"
        mlx_path.write_text(mlx)

        cmd = [
            exe,
            "-i", str(stl_in),
            "-o", str(stl_out),
            "-m", "vn",  # export normals if possible
            "-s", str(mlx_path),
        ]
        print("[preprocess] (meshlabserver) running:", " ".join(cmd))
        try:
            env = os.environ.copy()
            env.setdefault("QT_QPA_PLATFORM", "offscreen")
            subprocess.run(cmd, check=True, env=env)
        except subprocess.CalledProcessError as e:
            print("[preprocess] meshlabserver failed:", e)
            return None

    final = stl_face_count(stl_out)
    print(f"[preprocess] (meshlabserver) wrote {stl_out.name} (faces: {final})")
    return final

def simplify_with_vtk(stl_in: Path, stl_out: Path,
                      target_faces: int = TARGET_FACE_COUNT,
                      close_holes_max: int = CLOSE_HOLES_MAX) -> int | None:
    """Simplify STL using VTK (headless-safe) and enforce watertightness via signed-distance voxelization."""
    try:
        import vtk
    except Exception as e:
        print("[preprocess] VTK not available:", e)
        return None

    def triangulate(poly):
        tri = vtk.vtkTriangleFilter()
        tri.SetInputData(poly)
        tri.PassLinesOff()
        tri.PassVertsOff()
        tri.Update()
        return tri.GetOutput()

    def clean(poly):
        cleaner = vtk.vtkCleanPolyData()
        cleaner.SetInputData(poly)
        cleaner.ConvertLinesToPointsOff()
        cleaner.ConvertPolysToLinesOff()
        cleaner.ConvertStripsToPolysOff()
        cleaner.Update()
        return cleaner.GetOutput()

    def ensure_normals(poly):
        normals = vtk.vtkPolyDataNormals()
        normals.SetInputData(poly)
        normals.ConsistencyOn()
        normals.AutoOrientNormalsOn()
        normals.SplittingOff()
        normals.Update()
        return normals.GetOutput()

    def count_boundary_edges(poly):
        feat = vtk.vtkFeatureEdges()
        feat.SetInputData(poly)
        feat.BoundaryEdgesOn()
        feat.FeatureEdgesOff()
        feat.NonManifoldEdgesOn()
        feat.ManifoldEdgesOff()
        feat.Update()
        return feat.GetOutput().GetNumberOfCells()

    def watertight_from_signed_distance(poly):
        poly = ensure_normals(poly)
        bounds = poly.GetBounds()
        max_dim = max(bounds[1]-bounds[0], bounds[3]-bounds[2], bounds[5]-bounds[4])
        if target_faces <= 8_000:
            dim = 96
        elif target_faces <= 20_000:
            dim = 128
        else:
            dim = 160
        signed = vtk.vtkSignedDistance()
        signed.SetInputData(poly)
        signed.SetRadius(max_dim / 8.0)
        signed.SetBounds(bounds[0]-1, bounds[1]+1,
                         bounds[2]-1, bounds[3]+1,
                         bounds[4]-1, bounds[5]+1)
        signed.SetDimensions(dim, dim, dim)
        signed.Update()

        contour = vtk.vtkContourFilter()
        contour.SetInputConnection(signed.GetOutputPort())
        contour.SetValue(0, 0.0)
        contour.Update()

        poly = contour.GetOutput()
        if poly is None or poly.GetNumberOfPolys() == 0:
            return None
        return clean(triangulate(poly))

    def quadric_decimate(poly, target):
        current = max(1, poly.GetNumberOfPolys())
        if target >= current:
            return poly
        reduction = min(max(1.0 - (target / current), 0.0), 0.995)
        dec = vtk.vtkQuadricDecimation()
        dec.SetInputData(poly)
        dec.SetTargetReduction(reduction)
        dec.AttributeErrorMetricOn()
        dec.VolumePreservationOn()
        dec.Update()
        out = dec.GetOutput()
        if out is None or out.GetNumberOfPolys() == 0:
            return poly
        return clean(triangulate(out))

    def stepwise_decimate(poly, target):
        current = max(1, poly.GetNumberOfPolys())
        if target >= current:
            return poly
        goals = []
        threshold = current
        while threshold > target * 1.05:
            threshold = max(target, int(threshold * 0.75))
            goals.append(threshold)
        for goal in goals:
            current = max(1, poly.GetNumberOfPolys())
            if goal >= current:
                continue
            reduction = min(max(1.0 - (goal / current), 0.0), 0.99)
            dec = vtk.vtkDecimatePro()
            dec.SetInputData(poly)
            dec.SetTargetReduction(reduction)
            dec.PreserveTopologyOn()
            dec.SplittingOff()
            dec.BoundaryVertexDeletionOff()
            dec.Update()
            out = dec.GetOutput()
            if out is None or out.GetNumberOfPolys() == 0:
                break
            poly = clean(triangulate(out))
        return poly

    reader = vtk.vtkSTLReader()
    reader.SetFileName(str(stl_in))
    reader.Update()
    data = reader.GetOutput()
    if data is None or data.GetNumberOfPolys() == 0:
        print("[preprocess] VTK failed to load STL or mesh empty.")
        return None

    data = clean(triangulate(data))
    if close_holes_max and close_holes_max > 0:
        filler = vtk.vtkFillHolesFilter()
        filler.SetInputData(data)
        filler.SetHoleSize(float(close_holes_max))
        filler.Update()
        data = clean(triangulate(filler.GetOutput()))

    data = watertight_from_signed_distance(data)
    if data is None:
        print("[preprocess] (VTK) voxelization failed.")
        return None

    for attempt in range(3):
        data = quadric_decimate(data, int(target_faces * 0.9))
        data = stepwise_decimate(data, target_faces)
        data = ensure_normals(data)
        data = clean(triangulate(data))
        boundary_edges = count_boundary_edges(data)
        if boundary_edges == 0:
            break
        print(f"[preprocess] (VTK) decimation introduced {boundary_edges} boundary edges; re-voxelizing to repair.")
        data = watertight_from_signed_distance(data)
        if data is None:
            print("[preprocess] (VTK) re-voxelization failed.")
            return None
    else:
        boundary_edges = count_boundary_edges(data)
        if boundary_edges:
            print(f"[preprocess] (VTK) unable to produce watertight mesh (boundary edges={boundary_edges}).")
            return None

    writer = vtk.vtkSTLWriter()
    writer.SetFileName(str(stl_out))
    writer.SetInputData(data)
    writer.SetFileTypeToBinary()
    writer.Write()

    boundary_edges = count_boundary_edges(data)
    if boundary_edges:
        print(f"[preprocess] (VTK) mesh still has {boundary_edges} boundary edges after processing.")
        return None

    final = stl_face_count(stl_out)
    print(f"[preprocess] (VTK) wrote {stl_out.name} (faces: {final})")
    return final


# ====================== Hydro (Capytaine) ======================
def compute_added_mass_inf(stl_path: Path) -> tuple[list[str], np.ndarray]:
    import capytaine as cpt
    print(f"[capytaine] Loading mesh: {stl_path}")
    mesh = cpt.load_mesh(str(stl_path))

    # Align axes: Rx(+90°) then Rz(+90°)
    mesh = mesh.rotated_x(ROT_X)
    mesh = mesh.rotated_z(ROT_Z)

    body = cpt.FloatingBody(mesh=mesh, dofs=cpt.rigid_body_dofs(rotation_center=(0.0, 0.0, 0.0)))
    dofs = ["Surge", "Sway", "Heave", "Roll", "Pitch", "Yaw"]

    def mk_problems(omega_val: float):
        return [
            cpt.RadiationProblem(
                body=body,
                radiating_dof=d,
                omega=omega_val,
                water_depth=np.inf,
                free_surface=np.inf,
            )
            for d in dofs
        ]

    solver = cpt.BEMSolver()
    try:
        results = [solver.solve(pb, keep_details=False) for pb in mk_problems(float("inf"))]
    except Exception:
        print("[capytaine] ω=∞ failed; retrying with ω=1e6 rad/s")
        results = [solver.solve(pb, keep_details=False) for pb in mk_problems(1e6)]

    A = np.zeros((6, 6))
    for j, res in enumerate(results):
        for i, d in enumerate(dofs):
            A[i, j] = res.added_masses[d]
    return dofs, A

# ====================== Outputs ======================
def save_matrix_outputs(out_dir: Path, dofs: list[str], A: np.ndarray) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    csv_path = out_dir / "added_mass_inf.csv"
    npy_path = out_dir / "added_mass_inf.npy"
    np.savetxt(csv_path, A, delimiter=",", header="DOFs:"+",".join(dofs), comments="")
    np.save(npy_path, A)
    print(f"[save] {csv_path}")
    print(f"[save] {npy_path}")

    print("\nA(∞) [order: " + ", ".join(dofs) + "]")
    for r in A:
        print("  " + "  ".join(f"{x:10.3f}" for x in r))
    print("\nDiagonal terms (copy into plugin):")
    for name, val in zip(dofs, np.diag(A)):
        print(f"  {name:>5}: {val: .6g}")

def write_plugin_snippet(out_dir: Path, dofs: list[str], A: np.ndarray,
                         default_current=(0.0, 0.0, 0.0)) -> Path:
    order = {name: i for i, name in enumerate(dofs)}
    Ax = A[order["Surge"], order["Surge"]]   # xDotU
    Ay = A[order["Sway"],  order["Sway"]]    # yDotV
    Az = A[order["Heave"], order["Heave"]]   # zDotW
    Ar = A[order["Roll"],  order["Roll"]]    # kDotP
    Ap = A[order["Pitch"], order["Pitch"]]   # mDotQ
    Ayaw = A[order["Yaw"],  order["Yaw"]]    # nDotR

    snippet = f"""\
<!-- Paste inside your model. Do not double-set with SDFormat native added mass. Units: m, kg, s -->
<plugin name="hydro_params" filename="libyour_hydro_plugin.so">
  <xDotU>{Ax:.6g}</xDotU>
  <yDotV>{Ay:.6g}</yDotV>
  <zDotW>{Az:.6g}</zDotW>
  <kDotP>{Ar:.6g}</kDotP>
  <mDotQ>{Ap:.6g}</mDotQ>
  <nDotR>{Ayaw:.6g}</nDotR>

  <!-- damping to fill -->
  <xU>0</xU><yV>0</yV><zW>0</zW>
  <kP>0</kP><mQ>0</mQ><nR>0</nR>
  <xUabsU>0</xUabsU><yVabsV>0</yVabsV><zWabsW>0</zWabsW>
  <kPabsP>0</kPabsP><mQabsQ>0</mQabsQ><nRabsR>0</nRabsR>
  <default_current>{default_current[0]} {default_current[1]} {default_current[2]}</default_current>
</plugin>
"""
    out_path = out_dir / "hydro_plugin_snippet.sdf"
    out_path.write_text(snippet)
    print(f"[save] {out_path}  (ready to paste)")
    return out_path

# ====================== Main ======================
def main() -> int:
    try:
        if not STL_PATH.exists():
            print(f"[preprocess] base STL missing ({STL_PATH}). Attempting to export from {DAE_PATH.name}...")
            if not export_dae_to_stl(DAE_PATH, STL_PATH):
                raise FileNotFoundError(f"STL not found: {STL_PATH} (export/convert the DAE manually)")

        simp = STL_PATH.with_name(STL_PATH.stem + "_simplified.stl")

        orig_faces = stl_face_count(STL_PATH)
        if orig_faces is not None:
            print(f"[preprocess] input faces: {orig_faces}")
        else:
            print("[preprocess] input faces: unknown (could not determine; assuming high).")

        simp_faces = None
        if simp.exists():
            simp_faces = stl_face_count(simp)
            faces_str = simp_faces if simp_faces is not None else "unknown"
            print(f"[preprocess] existing simplified mesh detected ({simp.name}, faces: {faces_str})")

        use_existing_simp = False
        need_simplify = False

        if FORCE_RESIMPLIFY:
            print("[preprocess] FORCE_RESIMPLIFY set; regenerating simplified mesh.")
            need_simplify = True
        elif simp.exists():
            if simp_faces is None:
                print("[preprocess] reusing existing simplified mesh (face count unknown).")
                use_existing_simp = True
            elif simp_faces <= TARGET_FACE_COUNT * 1.2:
                print("[preprocess] reusing existing simplified mesh (faces within target).")
                use_existing_simp = True
            else:
                print(f"[preprocess] existing simplified mesh still heavy (faces={simp_faces}); will resimplify.")
                need_simplify = True

        if not use_existing_simp and not need_simplify:
            if orig_faces is None:
                need_simplify = True
            elif orig_faces > SIMPLIFY_IF_FACE_COUNT_OVER:
                need_simplify = True
            else:
                print("[preprocess] mesh already below threshold; skipping simplification.")

        stl_for_solver = STL_PATH
        if use_existing_simp:
            stl_for_solver = simp
        elif need_simplify:
            print("[preprocess] simplifying mesh with VTK decimation...")
            final_faces = simplify_with_vtk(STL_PATH, simp, TARGET_FACE_COUNT, CLOSE_HOLES_MAX)
            if final_faces is None or final_faces > TARGET_FACE_COUNT * 1.2:
                print(f"[preprocess] VTK simplification {'failed' if final_faces is None else f'resulted in {final_faces} faces (>target)'}; trying meshlabserver fallback...")
                final_faces = simplify_with_meshlabserver(STL_PATH, simp, TARGET_FACE_COUNT, CLOSE_HOLES_MAX)
            if final_faces is not None:
                print(f"[preprocess] simplified faces: {final_faces}")
                stl_for_solver = simp
            else:
                raise RuntimeError("Mesh simplification failed; please provide a cleaned *_simplified.stl.")
        else:
            stl_for_solver = STL_PATH

        if stl_for_solver == STL_PATH and orig_faces and orig_faces > SIMPLIFY_IF_FACE_COUNT_OVER:
            raise RuntimeError("Fallback to original STL refused (too dense). Provide a simplified mesh or set FORCE_RESIMPLIFY=1.")
        if not stl_for_solver.exists():
            raise FileNotFoundError(f"Chosen mesh missing: {stl_for_solver}")

        if stl_for_solver == simp:
            print(f"[preprocess] using simplified mesh: {simp} (faces: {stl_face_count(simp)})")
        else:
            print(f"[preprocess] using original mesh (faces: {orig_faces})")

        # Compute A(∞)
        dofs, A = compute_added_mass_inf(stl_for_solver)

        # Save results
        save_matrix_outputs(WORLD_DIR, dofs, A)
        write_plugin_snippet(WORLD_DIR, dofs, A, default_current=(0.0, 0.0, 0.0))
        print("\n[done] Files written in:", WORLD_DIR.resolve())
        return 0

    except Exception as e:
        print("[error]", e)
        traceback.print_exc()
        return 1

if __name__ == "__main__":
    sys.exit(main())

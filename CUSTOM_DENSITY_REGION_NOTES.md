# Custom Density Region Notes

## Goal

Add optional local density regions to TetGen's tetrahedral meshing path. The
default behavior must remain unchanged when no density regions are configured.

## Files Reviewed

- `README`: repository is TetGen 1.6.0.
- `CMakeLists.txt`: builds static library target `tetgen` from `tetgen.cxx`
  and `predicates.cxx` with `TETLIBRARY`; executable target is optional.
- `makefile`: `make tetlib` builds `libtet.a`; plain `make` builds the
  command line executable.
- `tetgen.h`: public library API, `tetgenio`, `tetgenbehavior`, internal
  `tetgenmesh`, and the `tetrahedralize()` declarations.
- `tetgen.cxx`: file readers/writers, command line parser, mesh construction,
  refinement, size checks, output writers, and the library/CLI entry points.
- `example.poly`: existing PLC sample with points, facets, holes, and region
  entries including maximum volume constraints.

## Architecture Summary

- Public callers use `tetgenio` as the in-memory input/output container.
  It owns arrays for nodes, facets, holes, regions, tetrahedra, faces, edges,
  point metrics, and several constraint lists.
- The library entry point is `tetrahedralize(char *switches, tetgenio *in,
  tetgenio *out, tetgenio *addin = NULL, tetgenio *bgmin = NULL)` when built
  with `TETLIBRARY`. The lower-level entry point takes a `tetgenbehavior *`.
- Command line and library switches are parsed into `tetgenbehavior`.
  Important existing switches for this task:
  - `-p`: PLC input.
  - `-q`: quality/refinement.
  - `-a`: maximum volume constraints. Without a number, per-region or
    per-element volume bounds are used.
  - `-A`: region attributes.
  - `-m`: point/background metric size field.
  - `-k`: VTK output from the command line path.
- `.poly` / `.smesh` are read by `tetgenio::load_poly()`. It reads points,
  facets, holes, and the region list.
- `.node`, `.ele`, `.face`, `.edge`, `.vol`, `.var`, `.mtr`, `.elem` are read
  by the corresponding `tetgenio::load_*()` routines in `tetgen.cxx`.
- Outputs are written by `outnodes()`, `outelements()`, `outfaces()` /
  `outsubfaces()`, `outedges()`, `outmetrics()`, `outmesh2vtk()`, and related
  routines. With an output `tetgenio`, data is copied to the output object;
  with `out == NULL`, files are written using `tetgenbehavior::outfilename`.

## Existing Size Control

- Region volume constraints are represented by `tetgenio::regionlist`: each
  region is `(x, y, z, attribute, max_volume)`. During `carveholes()`, TetGen
  spreads region attributes and volume bounds to tetrahedra when `-A` / `-a`
  are active.
- Per-element volume constraints use `tetrahedronvolumelist` / `.vol` and the
  internal `volumebound()` slot when `-a` is active.
- Point size fields use `pointmtrlist` / `.mtr` or an optional background mesh
  when `-m` is active. New points interpolate mesh size through
  `getpointmeshsize()`.
- Refinement decisions are made mainly in:
  - `tetgenmesh::check_tetrahedron()`: computes tet circumcenter, volume,
    edge lengths, and decides whether a tet should be split.
  - `tetgenmesh::checktet4split()`: similar check path used by repair logic.
  - `tetgenmesh::delaunayrefinement()`: queues and splits bad/oversized
    segments, faces, and tetrahedra.
- There is already a `tetgenio::tetunsuitable` callback, but it has no user
  data pointer and only returns a hard unsuitable/suitable decision. It does
  not directly model smooth density transitions.

## Design Direction

- Add optional density-region data to `tetgenio`, so existing API calls keep
  working and callers can configure one or more regions before calling
  `tetrahedralize()`.
- Reuse the existing tetrahedron refinement path by adding a density-region
  size check to the tet size tests. This avoids modifying point insertion,
  boundary recovery, or file readers.
- Interpret density/target-size multipliers as a local target insertion
  radius. Existing fixed-volume logic converts volume to
  `pow(volume, 1/3) / 3`; the density region implementation will use the same
  style of scalar target size when possible.
- Use `smoothstep` over a signed/approximate boundary distance: inside gets
  the refined target size, outside far from the region returns to the base
  size, and the transition band interpolates smoothly.

## Planned Code Locations

- `tetgen.h`: extend `tetgenio` with density-region types, storage, cleanup,
  and helper API declarations; add internal `tetgenmesh` helper declarations.
- `tetgen.cxx`: implement region evaluation and integrate it into
  `check_tetrahedron()` / `checktet4split()`.
- `examples/custom_density_regions.cxx`: demonstrate library use with
  `example.poly` and write TetGen plus VTK legacy outputs.
- `CMakeLists.txt`: optional `BUILD_DENSITY_EXAMPLE` target.

## Completed Changes

- Created this notes file after initial repository and architecture review.
- Added `tetgenio::densityregion` storage and library helper APIs:
  - `add_density_region_box(minpt, maxpt, sizefactor, transition)`
  - `add_density_region_cylinder(basept, toppt, radius, sizefactor, transition)`
  - `add_density_region_sphere(center, radius, sizefactor, transition)`
  - `add_density_region_plc(surface, sizefactor, transition)`
  - `clear_density_regions()`
- The PLC density-region helper deep-copies the existing `tetgenio` point and
  facet representation so callers can load or construct a closed `.poly`-style
  surface without introducing a new geometry file format.
- Added internal density-region evaluation helpers:
  - signed distance for box, cylinder, and sphere regions;
  - approximate signed distance for closed PLC regions by triangulating each
    polygon as a fan, using ray casting for inside/outside and point-triangle
    distance for transition distance;
  - `smoothstep` blending from refined target size to the base target size
    across each region's outward transition band.
- Connected density regions to the existing tetrahedron refinement checks:
  `check_tetrahedron()` and `checktet4split()` now request a split when the
  local half-edge size exceeds the density-region target size.
- Base size selection for density regions reuses existing TetGen controls in
  this order: fixed `-a#` volume length, per-tet `-a` volume bound, point
  metric `-m`, then a conservative fallback of `longest / 24` when a caller
  supplies density regions without another size field.
- If a caller configures density regions but does not pass a switch that would
  normally enter refinement (`-q` or `-a`), `tetrahedralize()` enables the
  existing quality/refinement stage so the density regions have an effect.
- Added `examples/custom_density_regions.cxx` and optional CMake target
  `density_regions_example`. The example reads `example.poly`, runs baseline,
  box, cylinder, sphere, and custom closed PLC density-region cases, then
  writes `.node`, `.ele`, `.face`, and VTK legacy files.

## Current Issues

- Arbitrary closed density geometry will be implemented through the existing
  `tetgenio` PLC/facet representation instead of a new file format.
- PLC region evaluation assumes the supplied surface is closed and reasonably
  consistently faceted. The implementation triangulates polygon facets as fans,
  which matches the simple polygon storage in `.poly` but is an approximation
  for non-planar or self-intersecting facets.
- The current command line file formats will not be changed unless needed; the
  primary API surface for density regions will be the C++ library API.

## Build and Run Verification

Commands run from repository root:

```powershell
cmake -S . -B build -DBUILD_LIBRARY=ON -DBUILD_EXECUTABLE=OFF
cmake --build build --config Release
cmake -S . -B build -DBUILD_LIBRARY=ON -DBUILD_EXECUTABLE=OFF -DBUILD_DENSITY_EXAMPLE=ON
cmake --build build --config Release --target density_regions_example
.\build\Release\density_regions_example.exe
```

Results:

- Static library generated: `build/Release/tetgen.lib`.
- Example executable generated: `build/Release/density_regions_example.exe`.
- The MSVC build reports existing deprecation warnings for C stdio/string
  routines in TetGen. The example target also prints an environment message
  that `pwsh.exe` is unavailable, but the target is produced and the command
  exits successfully.

Example run summary:

- `baseline`: 245 points, 825 tetrahedra.
- `box_density`: 2032 points, 10521 tetrahedra.
- `cylinder_density`: 2902 points, 15590 tetrahedra.
- `sphere_density`: 886 points, 4089 tetrahedra.
- `custom_plc_density`: 922 points, 4331 tetrahedra.

Generated files in `density_region_results/`:

- `input_geometry.vtk`: input `example.poly` PLC surface.
- `*_density_region.vtk`: density-region geometry surfaces for box, cylinder,
  sphere, and custom closed PLC region.
- `baseline.node`, `baseline.ele`, `baseline.face`, `baseline.vtk`: baseline
  TetGen and VTK tetrahedral mesh output.
- `box_density.*`, `cylinder_density.*`, `sphere_density.*`,
  `custom_plc_density.*`: matching TetGen `.node` / `.ele` / `.face` files and
  VTK unstructured-grid files for each density-region run.

Visualization:

- Open `density_region_results/input_geometry.vtk`, a `*_density_region.vtk`,
  and the matching mesh `.vtk` in ParaView.
- Use surface/wireframe display for the region geometry and mesh display for
  the tetrahedral grid. Comparing `baseline.vtk` with each density case shows
  the local increase in tetrahedron count near the configured region.

## Next Steps

1. Review PLC signed-distance accuracy on complex self-intersecting or
   non-planar region surfaces if those inputs need to be supported.
2. Optionally add a command-line file format for density regions. The current
   implementation intentionally keeps this as a C++ API to avoid changing
   TetGen's existing input formats.

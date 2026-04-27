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
- New example C++ file: demonstrate library use with `example.poly` and write
  TetGen plus VTK legacy outputs.

## Completed Changes

- Created this notes file after initial repository and architecture review.

## Current Issues

- Arbitrary closed density geometry will be implemented through the existing
  `tetgenio` PLC/facet representation instead of a new file format. The first
  implementation will use triangle fan decomposition of polygonal facets and
  ray casting for inside/outside tests.
- The current command line file formats will not be changed unless needed; the
  primary API surface for density regions will be the C++ library API.

## Next Steps

1. Add density-region storage and API to `tetgenio`.
2. Implement analytic box, cylinder, and sphere distance/evaluation helpers.
3. Implement arbitrary closed PLC distance/evaluation helpers.
4. Connect density-region target size to tetrahedron refinement checks.
5. Add the library example and visualization output.
6. Build, run, record generated outputs, and commit each completed stage.

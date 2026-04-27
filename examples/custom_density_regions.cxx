#ifndef TETLIBRARY
#define TETLIBRARY
#endif

#include "../tetgen.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

struct Bounds {
  REAL minpt[3];
  REAL maxpt[3];
  REAL center[3];
  REAL span[3];
};

static void make_directory(const char *path)
{
#ifdef _WIN32
  _mkdir(path);
#else
  mkdir(path, 0755);
#endif
}

static bool load_example(tetgenio &in)
{
  char basename[] = "example";
  if (in.load_poly(basename)) {
    return true;
  }
  char parent_basename[] = "../example";
  return in.load_poly(parent_basename);
}

static Bounds get_bounds(tetgenio &in)
{
  Bounds b;
  for (int j = 0; j < 3; j++) {
    b.minpt[j] = in.pointlist[j];
    b.maxpt[j] = in.pointlist[j];
  }
  for (int i = 1; i < in.numberofpoints; i++) {
    REAL *p = &(in.pointlist[i * 3]);
    for (int j = 0; j < 3; j++) {
      if (p[j] < b.minpt[j]) b.minpt[j] = p[j];
      if (p[j] > b.maxpt[j]) b.maxpt[j] = p[j];
    }
  }
  for (int j = 0; j < 3; j++) {
    b.center[j] = 0.5 * (b.minpt[j] + b.maxpt[j]);
    b.span[j] = b.maxpt[j] - b.minpt[j];
  }
  return b;
}

static int vtk_index(int tetgen_index, int firstnumber)
{
  return tetgen_index - firstnumber;
}

static void write_input_surface_vtk(const std::string &filename, tetgenio &io)
{
  std::ofstream out(filename.c_str());
  int polygon_count = 0;
  int polygon_size = 0;
  for (int i = 0; i < io.numberoffacets; i++) {
    tetgenio::facet *f = &(io.facetlist[i]);
    for (int j = 0; j < f->numberofpolygons; j++) {
      tetgenio::polygon *p = &(f->polygonlist[j]);
      if (p->numberofvertices >= 3) {
        polygon_count++;
        polygon_size += p->numberofvertices + 1;
      }
    }
  }

  out << "# vtk DataFile Version 3.0\n";
  out << "TetGen input PLC\n";
  out << "ASCII\n";
  out << "DATASET POLYDATA\n";
  out << "POINTS " << io.numberofpoints << " double\n";
  for (int i = 0; i < io.numberofpoints; i++) {
    REAL *p = &(io.pointlist[i * 3]);
    out << p[0] << " " << p[1] << " " << p[2] << "\n";
  }
  out << "POLYGONS " << polygon_count << " " << polygon_size << "\n";
  for (int i = 0; i < io.numberoffacets; i++) {
    tetgenio::facet *f = &(io.facetlist[i]);
    for (int j = 0; j < f->numberofpolygons; j++) {
      tetgenio::polygon *p = &(f->polygonlist[j]);
      if (p->numberofvertices < 3) {
        continue;
      }
      out << p->numberofvertices;
      for (int k = 0; k < p->numberofvertices; k++) {
        out << " " << vtk_index(p->vertexlist[k], io.firstnumber);
      }
      out << "\n";
    }
  }
}

static void write_tetmesh_vtk(const std::string &filename, tetgenio &mesh)
{
  std::ofstream out(filename.c_str());
  out << "# vtk DataFile Version 3.0\n";
  out << "TetGen tetrahedral mesh\n";
  out << "ASCII\n";
  out << "DATASET UNSTRUCTURED_GRID\n";
  out << "POINTS " << mesh.numberofpoints << " double\n";
  for (int i = 0; i < mesh.numberofpoints; i++) {
    REAL *p = &(mesh.pointlist[i * 3]);
    out << p[0] << " " << p[1] << " " << p[2] << "\n";
  }
  out << "CELLS " << mesh.numberoftetrahedra << " "
      << mesh.numberoftetrahedra * 5 << "\n";
  for (int i = 0; i < mesh.numberoftetrahedra; i++) {
    int *tet = &(mesh.tetrahedronlist[i * mesh.numberofcorners]);
    out << "4";
    for (int j = 0; j < 4; j++) {
      out << " " << vtk_index(tet[j], mesh.firstnumber);
    }
    out << "\n";
  }
  out << "CELL_TYPES " << mesh.numberoftetrahedra << "\n";
  for (int i = 0; i < mesh.numberoftetrahedra; i++) {
    out << "10\n";
  }
}

static void write_box_vtk(const std::string &filename, REAL *minpt, REAL *maxpt)
{
  const int faces[6][4] = {
    {0, 1, 2, 3}, {4, 7, 6, 5}, {0, 4, 5, 1},
    {1, 5, 6, 2}, {2, 6, 7, 3}, {3, 7, 4, 0}
  };
  REAL p[8][3] = {
    {minpt[0], minpt[1], minpt[2]},
    {maxpt[0], minpt[1], minpt[2]},
    {maxpt[0], maxpt[1], minpt[2]},
    {minpt[0], maxpt[1], minpt[2]},
    {minpt[0], minpt[1], maxpt[2]},
    {maxpt[0], minpt[1], maxpt[2]},
    {maxpt[0], maxpt[1], maxpt[2]},
    {minpt[0], maxpt[1], maxpt[2]}
  };
  std::ofstream out(filename.c_str());
  out << "# vtk DataFile Version 3.0\n";
  out << "Box density region\n";
  out << "ASCII\n";
  out << "DATASET POLYDATA\n";
  out << "POINTS 8 double\n";
  for (int i = 0; i < 8; i++) {
    out << p[i][0] << " " << p[i][1] << " " << p[i][2] << "\n";
  }
  out << "POLYGONS 6 30\n";
  for (int i = 0; i < 6; i++) {
    out << "4 " << faces[i][0] << " " << faces[i][1] << " "
        << faces[i][2] << " " << faces[i][3] << "\n";
  }
}

static void normalize3(REAL *v)
{
  REAL len = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  if (len == 0.0) {
    return;
  }
  v[0] /= len;
  v[1] /= len;
  v[2] /= len;
}

static void cross3(REAL *a, REAL *b, REAL *out)
{
  out[0] = a[1] * b[2] - a[2] * b[1];
  out[1] = a[2] * b[0] - a[0] * b[2];
  out[2] = a[0] * b[1] - a[1] * b[0];
}

static void write_cylinder_vtk(const std::string &filename, REAL *basept,
                               REAL *toppt, REAL radius)
{
  const int sides = 32;
  REAL axis[3] = {
    toppt[0] - basept[0], toppt[1] - basept[1], toppt[2] - basept[2]
  };
  normalize3(axis);
  REAL tmp[3] = {1.0, 0.0, 0.0};
  if (fabs(axis[0]) > 0.9) {
    tmp[0] = 0.0;
    tmp[1] = 1.0;
  }
  REAL u[3], v[3];
  cross3(axis, tmp, u);
  normalize3(u);
  cross3(axis, u, v);
  normalize3(v);

  std::ofstream out(filename.c_str());
  out << "# vtk DataFile Version 3.0\n";
  out << "Cylinder density region\n";
  out << "ASCII\n";
  out << "DATASET POLYDATA\n";
  out << "POINTS " << sides * 2 << " double\n";
  for (int cap = 0; cap < 2; cap++) {
    REAL *origin = cap == 0 ? basept : toppt;
    for (int i = 0; i < sides; i++) {
      REAL a = (2.0 * 3.14159265358979323846 * i) / sides;
      REAL ca = cos(a);
      REAL sa = sin(a);
      out << origin[0] + radius * (ca * u[0] + sa * v[0]) << " "
          << origin[1] + radius * (ca * u[1] + sa * v[1]) << " "
          << origin[2] + radius * (ca * u[2] + sa * v[2]) << "\n";
    }
  }
  out << "POLYGONS " << sides + 2 << " " << (sides * 5 + 2 * (sides + 1))
      << "\n";
  for (int i = 0; i < sides; i++) {
    int j = (i + 1) % sides;
    out << "4 " << i << " " << j << " " << sides + j << " "
        << sides + i << "\n";
  }
  out << sides;
  for (int i = sides - 1; i >= 0; i--) out << " " << i;
  out << "\n";
  out << sides;
  for (int i = 0; i < sides; i++) out << " " << sides + i;
  out << "\n";
}

static void write_sphere_vtk(const std::string &filename, REAL *center,
                             REAL radius)
{
  const int lat = 12;
  const int lon = 24;
  int points = (lat + 1) * lon;
  int polys = lat * lon;
  int poly_size = lon * 4 + (lat - 2) * lon * 5 + lon * 4;
  std::ofstream out(filename.c_str());
  out << "# vtk DataFile Version 3.0\n";
  out << "Sphere density region\n";
  out << "ASCII\n";
  out << "DATASET POLYDATA\n";
  out << "POINTS " << points << " double\n";
  for (int i = 0; i <= lat; i++) {
    REAL theta = 3.14159265358979323846 * i / lat;
    REAL st = sin(theta);
    REAL ct = cos(theta);
    for (int j = 0; j < lon; j++) {
      REAL phi = 2.0 * 3.14159265358979323846 * j / lon;
      out << center[0] + radius * st * cos(phi) << " "
          << center[1] + radius * st * sin(phi) << " "
          << center[2] + radius * ct << "\n";
    }
  }
  out << "POLYGONS " << polys << " " << poly_size << "\n";
  for (int i = 0; i < lat; i++) {
    for (int j = 0; j < lon; j++) {
      int jn = (j + 1) % lon;
      if (i == 0) {
        out << "3 " << j << " " << lon + j << " " << lon + jn << "\n";
      } else if (i == lat - 1) {
        out << "3 " << i * lon + j << " " << i * lon + jn << " "
            << (i + 1) * lon + j << "\n";
      } else {
        out << "4 " << i * lon + j << " " << i * lon + jn << " "
            << (i + 1) * lon + jn << " " << (i + 1) * lon + j << "\n";
      }
    }
  }
}

static void set_triangle_facet(tetgenio::facet &f, int a, int b, int c)
{
  tetgenio::init(&f);
  f.numberofpolygons = 1;
  f.polygonlist = new tetgenio::polygon[1];
  tetgenio::init(&(f.polygonlist[0]));
  f.polygonlist[0].numberofvertices = 3;
  f.polygonlist[0].vertexlist = new int[3];
  f.polygonlist[0].vertexlist[0] = a;
  f.polygonlist[0].vertexlist[1] = b;
  f.polygonlist[0].vertexlist[2] = c;
}

static void make_custom_plc_region(tetgenio &region, const Bounds &b)
{
  REAL rx = 0.30 * b.span[0];
  REAL ry = 0.22 * b.span[1];
  REAL rz = 0.18 * b.span[2];
  region.firstnumber = 0;
  region.numberofpoints = 6;
  region.pointlist = new REAL[18];
  REAL pts[6][3] = {
    {b.center[0] - rx, b.center[1],      b.center[2]},
    {b.center[0] + rx, b.center[1],      b.center[2]},
    {b.center[0],      b.center[1] - ry, b.center[2]},
    {b.center[0],      b.center[1] + ry, b.center[2]},
    {b.center[0],      b.center[1],      b.center[2] + rz},
    {b.center[0],      b.center[1],      b.center[2] - rz}
  };
  for (int i = 0; i < 6; i++) {
    region.pointlist[i * 3] = pts[i][0];
    region.pointlist[i * 3 + 1] = pts[i][1];
    region.pointlist[i * 3 + 2] = pts[i][2];
  }

  region.numberoffacets = 8;
  region.facetlist = new tetgenio::facet[8];
  set_triangle_facet(region.facetlist[0], 0, 2, 4);
  set_triangle_facet(region.facetlist[1], 2, 1, 4);
  set_triangle_facet(region.facetlist[2], 1, 3, 4);
  set_triangle_facet(region.facetlist[3], 3, 0, 4);
  set_triangle_facet(region.facetlist[4], 2, 0, 5);
  set_triangle_facet(region.facetlist[5], 1, 2, 5);
  set_triangle_facet(region.facetlist[6], 3, 1, 5);
  set_triangle_facet(region.facetlist[7], 0, 3, 5);
}

static void save_case_outputs(const std::string &base, tetgenio &out)
{
  out.save_nodes(base.c_str());
  out.save_elements(base.c_str());
  if (out.numberoftrifaces > 0) {
    out.save_faces(base.c_str());
  }
  write_tetmesh_vtk(base + ".vtk", out);
}

static bool run_case(const std::string &outdir, const std::string &name,
                     int region_type, const Bounds &b, tetgenio *custom_region)
{
  tetgenio in;
  tetgenio out;
  if (!load_example(in)) {
    std::cerr << "Failed to load example.poly\n";
    return false;
  }

  REAL sizefactor = 0.60;
  REAL transition = 0.06 * b.span[2];
  if (region_type == 1) {
    REAL minpt[3] = {
      b.center[0] - 0.22 * b.span[0],
      b.center[1] - 0.22 * b.span[1],
      b.center[2] - 0.16 * b.span[2]
    };
    REAL maxpt[3] = {
      b.center[0] + 0.22 * b.span[0],
      b.center[1] + 0.22 * b.span[1],
      b.center[2] + 0.16 * b.span[2]
    };
    in.add_density_region_box(minpt, maxpt, sizefactor, transition);
  } else if (region_type == 2) {
    REAL basept[3] = {b.center[0], b.center[1], b.minpt[2] + 0.20 * b.span[2]};
    REAL toppt[3] = {b.center[0], b.center[1], b.minpt[2] + 0.82 * b.span[2]};
    REAL radius = 0.22 * (b.span[0] < b.span[1] ? b.span[0] : b.span[1]);
    in.add_density_region_cylinder(basept, toppt, radius, sizefactor, transition);
  } else if (region_type == 3) {
    REAL radius = 0.24 * (b.span[0] < b.span[1] ? b.span[0] : b.span[1]);
    in.add_density_region_sphere((REAL *) b.center, radius, sizefactor, transition);
  } else if ((region_type == 4) && (custom_region != NULL)) {
    in.add_density_region_plc(custom_region, sizefactor, transition);
  }

  char switches[] = "pq1.4a0.12fQ";
  tetrahedralize(switches, &in, &out);
  save_case_outputs(outdir + "/" + name, out);
  std::cout << name << ": " << out.numberofpoints << " points, "
            << out.numberoftetrahedra << " tetrahedra\n";
  return true;
}

int main()
{
  const std::string outdir = "density_region_results";
  make_directory(outdir.c_str());

  tetgenio input_for_bounds;
  if (!load_example(input_for_bounds)) {
    std::cerr << "Cannot find example.poly. Run from the repository root or build directory.\n";
    return 1;
  }
  Bounds b = get_bounds(input_for_bounds);
  write_input_surface_vtk(outdir + "/input_geometry.vtk", input_for_bounds);

  REAL boxmin[3] = {
    b.center[0] - 0.22 * b.span[0],
    b.center[1] - 0.22 * b.span[1],
    b.center[2] - 0.16 * b.span[2]
  };
  REAL boxmax[3] = {
    b.center[0] + 0.22 * b.span[0],
    b.center[1] + 0.22 * b.span[1],
    b.center[2] + 0.16 * b.span[2]
  };
  write_box_vtk(outdir + "/box_density_region.vtk", boxmin, boxmax);

  REAL cylbase[3] = {b.center[0], b.center[1], b.minpt[2] + 0.20 * b.span[2]};
  REAL cyltop[3] = {b.center[0], b.center[1], b.minpt[2] + 0.82 * b.span[2]};
  REAL cylradius = 0.22 * (b.span[0] < b.span[1] ? b.span[0] : b.span[1]);
  write_cylinder_vtk(outdir + "/cylinder_density_region.vtk",
                     cylbase, cyltop, cylradius);

  REAL sphereradius = 0.24 * (b.span[0] < b.span[1] ? b.span[0] : b.span[1]);
  write_sphere_vtk(outdir + "/sphere_density_region.vtk",
                   (REAL *) b.center, sphereradius);

  tetgenio custom_region;
  make_custom_plc_region(custom_region, b);
  write_input_surface_vtk(outdir + "/custom_plc_density_region.vtk",
                          custom_region);

  if (!run_case(outdir, "baseline", 0, b, NULL)) return 1;
  if (!run_case(outdir, "box_density", 1, b, NULL)) return 1;
  if (!run_case(outdir, "cylinder_density", 2, b, NULL)) return 1;
  if (!run_case(outdir, "sphere_density", 3, b, NULL)) return 1;
  if (!run_case(outdir, "custom_plc_density", 4, b, &custom_region)) return 1;

  std::cout << "Wrote TetGen and VTK outputs to " << outdir << "\n";
  return 0;
}

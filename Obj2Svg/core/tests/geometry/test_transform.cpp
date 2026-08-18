#include "editor_core/geometry/mesh.hpp"
#include "editor_core/geometry/transform.hpp"

#include <cassert>
#include <cmath>
#include <iostream>

using namespace editor_core;

namespace {

bool approx(float a, float b, float eps = 1e-4f) {
  return std::fabs(a - b) < eps;
}

void test_translation() {
  EditorMesh mesh;
  mesh.vertices = {{1.0f, 0.0f, 0.0f}};
  apply_transform(mesh, make_translation(2.0f, 3.0f, 4.0f));
  assert(approx(mesh.vertices[0].x, 3.0f));
  assert(approx(mesh.vertices[0].y, 3.0f));
  assert(approx(mesh.vertices[0].z, 4.0f));
  std::cout << "OK: translation\n";
}

void test_scale() {
  EditorMesh mesh;
  mesh.vertices = {{1.0f, 1.0f, 1.0f}};
  apply_transform(mesh, make_scale(2.0f, 3.0f, 4.0f));
  assert(approx(mesh.vertices[0].x, 2.0f));
  assert(approx(mesh.vertices[0].y, 3.0f));
  assert(approx(mesh.vertices[0].z, 4.0f));
  std::cout << "OK: scale\n";
}

void test_rotation_90_degrees_z() {
  EditorMesh mesh;
  mesh.vertices = {{1.0f, 0.0f, 0.0f}};
  apply_transform(
      mesh, make_rotation(0.0f, 0.0f, 1.0f, static_cast<float>(M_PI) / 2.0f));
  assert(approx(mesh.vertices[0].x, 0.0f, 1e-3f));
  assert(approx(mesh.vertices[0].y, 1.0f, 1e-3f));
  std::cout << "OK: rotation_90_degrees_z\n";
}

// multiply(a, b) must apply b first, then a — the convention documented in
// transform.hpp.
void test_composition_order() {
  EditorMesh mesh;
  mesh.vertices = {{1.0f, 0.0f, 0.0f}};
  const auto t = make_translation(2.0f, 3.0f, 4.0f);
  const auto r =
      make_rotation(0.0f, 0.0f, 1.0f, static_cast<float>(M_PI) / 2.0f);
  apply_transform(mesh,
                  multiply(t, r)); // (1,0,0) --R--> (0,1,0) --T--> (2,4,4)
  assert(approx(mesh.vertices[0].x, 2.0f, 1e-3f));
  assert(approx(mesh.vertices[0].y, 4.0f, 1e-3f));
  assert(approx(mesh.vertices[0].z, 4.0f, 1e-3f));
  std::cout << "OK: composition_order\n";
}

// A near-zero-length rotation axis is degenerate; make_rotation must fall
// back to identity rather than dividing by zero / producing NaN.
void test_degenerate_rotation_axis() {
  EditorMesh mesh;
  mesh.vertices = {{1.0f, 2.0f, 3.0f}};
  apply_transform(mesh, make_rotation(0.0f, 0.0f, 0.0f, 1.0f));
  assert(approx(mesh.vertices[0].x, 1.0f));
  assert(approx(mesh.vertices[0].y, 2.0f));
  assert(approx(mesh.vertices[0].z, 3.0f));
  assert(!std::isnan(mesh.vertices[0].x));
  std::cout << "OK: degenerate_rotation_axis\n";
}

// apply_transform must only move vertices — normals are documented as
// intentionally left untouched (see transform.hpp).
void test_normals_untouched() {
  EditorMesh mesh;
  mesh.vertices = {{1.0f, 0.0f, 0.0f}};
  mesh.normals = {{0.0f, 0.0f, 1.0f}};
  apply_transform(
      mesh, make_rotation(0.0f, 0.0f, 1.0f, static_cast<float>(M_PI) / 2.0f));
  assert(approx(mesh.normals[0].x, 0.0f));
  assert(approx(mesh.normals[0].y, 0.0f));
  assert(approx(mesh.normals[0].z, 1.0f));
  std::cout << "OK: normals_untouched\n";
}

} // namespace

int main() {
  test_translation();
  test_scale();
  test_rotation_90_degrees_z();
  test_composition_order();
  test_degenerate_rotation_axis();
  test_normals_untouched();

  std::cout << "\nAll test_transform checks passed.\n";
  return 0;
}

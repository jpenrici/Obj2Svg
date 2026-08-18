#include "editor_core/geometry/mesh.hpp"
#include "editor_core/geometry/projection.hpp"
#include "editor_core/geometry/transform.hpp"

#include <cassert>
#include <cmath>
#include <iostream>

using namespace editor_core;

namespace {

bool approx(float a, float b, float eps = 1e-3f) {
  return std::fabs(a - b) < eps;
}

/// Quad on the Z=0 plane, camera at Z=5 looking down -Z (orthographic).
struct QuadFixture {
  EditorMesh mesh;
  Face front_face; // CCW seen from +Z -> normal +Z -> faces the camera
  Face back_face;  // reversed winding -> normal -Z -> faces away
  CameraView camera;
};

QuadFixture make_quad_fixture() {
  QuadFixture fx;
  fx.mesh.vertices = {
      {-1.0f, -1.0f, 0.0f},
      {1.0f, -1.0f, 0.0f},
      {1.0f, 1.0f, 0.0f},
      {-1.0f, 1.0f, 0.0f},
  };
  fx.front_face.vertex_indices = {0, 1, 2, 3};
  fx.back_face.vertex_indices = {0, 3, 2, 1};
  fx.mesh.faces = {fx.front_face, fx.back_face};

  fx.camera.view_matrix = make_translation(0.0f, 0.0f, -5.0f);
  fx.camera.projection_matrix =
      make_identity(); // orthographic for a predictable test
  fx.camera.viewport_width = 800;
  fx.camera.viewport_height = 600;
  return fx;
}

void test_front_and_back_facing() {
  const auto fx = make_quad_fixture();
  assert(is_face_front_facing(fx.mesh, fx.front_face, fx.camera) == true);
  assert(is_face_front_facing(fx.mesh, fx.back_face, fx.camera) == false);
  std::cout << "OK: front_and_back_facing\n";
}

void test_project_mesh_polygon_flags_match_is_face_front_facing() {
  const auto fx = make_quad_fixture();
  const auto projected = project_mesh(fx.mesh, fx.camera);
  assert(projected.polygons.size() == 2);
  assert(projected.polygons[0].front_facing == true);
  assert(projected.polygons[1].front_facing == false);
  std::cout << "OK: project_mesh_polygon_flags_match_is_face_front_facing\n";
}

// depth must be the camera's linear view-space distance (not nonlinear NDC
// z) — every vertex of this Z=0 quad, seen from a camera at Z=5, is
// equidistant.
void test_depth_is_linear_view_space_distance() {
  const auto fx = make_quad_fixture();
  const auto projected = project_mesh(fx.mesh, fx.camera);
  for (const auto &v : projected.vertices) {
    assert(approx(v.depth, 5.0f));
  }
  std::cout << "OK: depth_is_linear_view_space_distance\n";
}

// NDC +Y is up; viewport/screen +Y is down — project_mesh must flip it so
// svg_writer can use the coordinates directly.
void test_viewport_y_is_flipped() {
  const auto fx = make_quad_fixture();
  const auto projected = project_mesh(fx.mesh, fx.camera);
  // vertex 0 = (-1,-1,0) is bottom-left in model space -> should land at
  // a *larger* viewport Y than vertex 2 = (1,1,0), which is top-right.
  assert(projected.vertices[0].y > projected.vertices[2].y);
  assert(projected.vertices[0].x < projected.vertices[2].x);
  std::cout << "OK: viewport_y_is_flipped\n";
}

// The quad's 4 edges are shared by both faces (front_face and back_face
// reference the same 4 vertex pairs, just wound oppositely) — project_mesh
// must not emit them twice.
void test_edges_are_deduplicated() {
  const auto fx = make_quad_fixture();
  const auto projected = project_mesh(fx.mesh, fx.camera);
  assert(projected.edges.size() == 4);
  std::cout << "OK: edges_are_deduplicated\n";
}

} // namespace

int main() {
  test_front_and_back_facing();
  test_project_mesh_polygon_flags_match_is_face_front_facing();
  test_depth_is_linear_view_space_distance();
  test_viewport_y_is_flipped();
  test_edges_are_deduplicated();

  std::cout << "\nAll test_projection checks passed.\n";
  return 0;
}

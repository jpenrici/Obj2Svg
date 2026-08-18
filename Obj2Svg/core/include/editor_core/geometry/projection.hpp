#pragma once

#include "editor_core/geometry/mesh.hpp"
#include "editor_core/geometry/transform.hpp"

#include <cstddef>
#include <vector>

namespace editor_core {

struct CameraView {
  Mat4 view_matrix;
  Mat4 projection_matrix;
  int viewport_width;
  int viewport_height;
};

struct ProjectedVertex {
  float x, y;
  float depth;
};

struct ProjectedEdge {
  std::size_t a, b;
};

struct ProjectedPolygon {
  std::vector<std::size_t> vertex_indices;
  bool front_facing;
};

struct ProjectedMesh {
  std::vector<ProjectedVertex> vertices;
  std::vector<ProjectedEdge> edges;
  std::vector<ProjectedPolygon> polygons;
};

ProjectedMesh project_mesh(const EditorMesh &mesh, const CameraView &camera);

bool is_face_front_facing(const EditorMesh &mesh, const Face &face,
                          const CameraView &camera);

} // namespace editor_core

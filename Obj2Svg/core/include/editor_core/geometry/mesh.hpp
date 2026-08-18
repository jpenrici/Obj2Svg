#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace editor_core {

enum class FaceFlags : uint32_t { None = 0 };

struct Vertex {
  float x, y, z;
};

struct Normal {
  float x, y, z;
};

struct Face {
  std::vector<std::size_t> vertex_indices;
  std::vector<std::size_t> normal_indices;
  FaceFlags flags = FaceFlags::None; ///< Reserved, unused.
  uint32_t material = 0;             ///< Reserved, unused.
};

struct EditorMesh {
  std::vector<Vertex> vertices;
  std::vector<Normal> normals;
  std::vector<Face> faces;
};

struct Edge {
  std::size_t a, b;
};

std::size_t vertex_count(const EditorMesh &mesh);
std::size_t face_count(const EditorMesh &mesh);
std::size_t normal_count(const EditorMesh &mesh);
std::vector<Edge> compute_edges(const EditorMesh &mesh);

} // namespace editor_core

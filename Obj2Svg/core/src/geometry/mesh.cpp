#include "editor_core/geometry/mesh.hpp"

namespace editor_core {

std::size_t vertex_count(const EditorMesh &mesh) {
  return mesh.vertices.size();
}

std::size_t face_count(const EditorMesh &mesh) { return mesh.faces.size(); }

std::size_t normal_count(const EditorMesh &mesh) { return mesh.normals.size(); }

} // namespace editor_core

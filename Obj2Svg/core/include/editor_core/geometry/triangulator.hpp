#pragma once

#include "editor_core/error.hpp"
#include "editor_core/geometry/mesh.hpp"

#include <expected>
#include <vector>

namespace editor_core {

struct Triangle {
  std::size_t v0, v1, v2;
};

using TriangleList = std::vector<Triangle>;

std::expected<TriangleList, EditorError> triangulate(const EditorMesh &mesh);

} // namespace editor_core

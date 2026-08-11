#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace editor_core {

/// @brief Bitmask flags reserved for future per-face metadata.
///
/// No flag is defined yet; the parser always sets FaceFlags::None. Kept as
/// a scoped enum (not a plain bool/uint32_t) so future flags remain
/// type-safe and self-documenting at call sites.
enum class FaceFlags : uint32_t { None = 0 };

/// @brief A single vertex position in model space.
struct Vertex {
  float x, y, z;
};

/// @brief A single vertex/face normal direction.
struct Normal {
  float x, y, z;
};

/// @brief A polygonal face (triangle or n-gon).
///
/// Both index vectors are always absolute, 0-based, and already resolved
/// and validated by the time a Face exists in an EditorMesh — the parser
/// (see io/obj_reader.hpp) is the only place that deals with 1-based or
/// relative (negative) OBJ indices.
///
/// Invariant enforced at parse time: @c normal_indices.size() ==
/// @c vertex_indices.size() (fully normal-aware face) OR
/// @c normal_indices.empty() (face without normals). No other state is
/// representable — Triangulator and projection rely on this and never
/// check for a mismatched size.
struct Face {
  std::vector<std::size_t>
      vertex_indices; ///< Always absolute, 0-based, resolved and validated.
  std::vector<std::size_t> normal_indices; ///< Same size as vertex_indices, or
                                           ///< empty. See class invariant.
  FaceFlags flags =
      FaceFlags::None;   ///< Reserved, unused by the current parser.
  uint32_t material = 0; ///< Reserved, unused by the current parser.
};

/// @brief A polygonal mesh: flat, indexed vertex/normal/face buffers.
///
/// Faces reference vertices and normals by index into the vectors below;
/// there is no ownership or lifetime relationship beyond that of the
/// EditorMesh itself.
struct EditorMesh {
  std::vector<Vertex> vertices;
  std::vector<Normal> normals;
  std::vector<Face> faces;
};

/// @brief Number of vertices in @c mesh.
std::size_t vertex_count(const EditorMesh &mesh);

/// @brief Number of faces in @c mesh.
std::size_t face_count(const EditorMesh &mesh);

/// @brief Number of normals in @c mesh.
std::size_t normal_count(const EditorMesh &mesh);

} // namespace editor_core

#pragma once

#include <expected>
#include <vector>

#include "editor_core/error.hpp"
#include "editor_core/geometry/mesh.hpp"

namespace editor_core {

struct Triangle { std::size_t v0, v1, v2; };
using TriangleList = std::vector<Triangle>;

/// @brief Converts every face of @c mesh (triangle or n-gon, convex or
///        concave) into triangles via ear clipping.
///
/// Strategy: estimate the face normal via Newell's method (independent of
/// per-vertex normals, which serve shading only), project the face's
/// vertices onto its dominant plane, run ear clipping in 2D, and map the
/// resulting indices back to the original 3D space. This is needed
/// because meshes exported from Blender frequently contain non-convex
/// n-gons when "triangulate faces" isn't checked on export.
///
/// @b Signature note: the original spec listed this function as returning
/// TriangleList directly (no error channel). That's incompatible with two
/// hard project rules — "never throw" and "std::expected is the only
/// recoverable-error mechanism" — together with the explicit test
/// requirement (spec section 8) that a face with collinear/duplicated
/// vertices must be *signaled* as ErrorCode::DegenerateFace, not silently
/// turned into a near-zero-area triangle. The signature was corrected to
/// std::expected<TriangleList, EditorError> to make that possible; see
/// docs/HISTORICO.md for the decision record.
///
/// Fails fast with ErrorCode::DegenerateFace on the first face whose
/// Newell normal has ~zero length (collinear or duplicated vertices — no
/// well-defined plane), or whose ear-clipping process cannot proceed
/// (e.g. a self-intersecting polygon, which is out of scope — see
/// triangulator.cpp's planarity/holes note).
std::expected<TriangleList, EditorError> triangulate(const EditorMesh& mesh);

} // namespace editor_core

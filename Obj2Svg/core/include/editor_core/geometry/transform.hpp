#pragma once

#include "editor_core/geometry/mesh.hpp"

namespace editor_core {

/// @brief A 4x4 matrix, stored column-major (OpenGL convention): element
///        at column @c c, row @c r is @c m[c*4 + r].
struct Mat4 { float m[16]; };

/// @brief Identity matrix.
Mat4 make_identity();

/// @brief Translation matrix by (dx, dy, dz).
Mat4 make_translation(float dx, float dy, float dz);

/// @brief Rotation matrix by @c angle_radians around the axis
///        (axis_x, axis_y, axis_z), using Rodrigues' rotation formula.
///
/// The axis is normalized internally. A near-zero-length axis (all three
/// components ~0) is a degenerate input with no well-defined rotation
/// plane; make_rotation returns the identity matrix in that case rather
/// than dividing by zero or producing NaNs.
Mat4 make_rotation(float axis_x, float axis_y, float axis_z, float angle_radians);

/// @brief Scale matrix by (sx, sy, sz) along each axis.
Mat4 make_scale(float sx, float sy, float sz);

/// @brief Matrix product @c a * @c b — applying the result to a point
///        applies @c b first, then @c a (standard column-vector
///        convention: `multiply(a, b)` represents "b, then a").
Mat4 multiply(const Mat4& a, const Mat4& b);

/// @brief Applies @c transform in place to every vertex position in
///        @c mesh.
///
/// @c mesh.normals are intentionally left untouched — this project's
/// scope (Phase 4, basic editing) only requires transforming vertex
/// positions. A non-uniform scale or rotation applied here will make
/// existing per-vertex normals stale relative to the new geometry; this
/// is a known limitation, out of scope until normal re-transformation
/// (inverse-transpose of the linear part) is needed by a later phase.
void apply_transform(EditorMesh& mesh, const Mat4& transform);

} // namespace editor_core

#pragma once

// Internal helper shared by projection.cpp and triangulator.cpp — both need
// Vec3 arithmetic and Newell's-method face-normal estimation. Deliberately
// NOT part of core/include/editor_core/ (the public header tree): this is
// an implementation detail, not part of the project structure fixed in
// docs/Projeto_Obj2Svg.md section 6. See docs/HISTORICO.md for the
// rationale (avoids duplicating this math in two translation units).

#include <cstddef>
#include <vector>

#include "editor_core/geometry/mesh.hpp"
#include "editor_core/geometry/transform.hpp"

namespace editor_core::internal {

struct Vec3 { float x, y, z; };
struct Vec4 { float x, y, z, w; };

Vec3 operator-(const Vec3& a, const Vec3& b);
Vec3 operator+(const Vec3& a, const Vec3& b);
float dot(const Vec3& a, const Vec3& b);
Vec3 cross(const Vec3& a, const Vec3& b);
float length(const Vec3& v);

/// @brief Normalizes @c v. Returns {0,0,0} for a near-zero-length input
///        (degenerate — callers must decide how to handle that case).
Vec3 normalize(const Vec3& v);

/// @brief Best-fit face normal via Newell's method — tolerant of
///        near-coplanar n-gons. See triangulator.hpp for rationale.
///        Returns an unnormalized normal; near-zero length signals a
///        degenerate (collinear/duplicated-vertex) face.
Vec3 newell_normal(const EditorMesh& mesh, const std::vector<std::size_t>& vertex_indices);

/// @brief Applies only the linear (3x3) part of @c m to the direction
///        @c dir — no translation. Correct for transforming
///        directions/normals when @c m is a rigid transform (rotation +
///        translation, no non-uniform scale) — e.g. a camera view matrix.
Vec3 transform_direction(const Mat4& m, const Vec3& dir);

/// @brief Applies the full @c m to the point @c p (with w = 1).
Vec4 transform_point(const Mat4& m, const Vec3& p);

} // namespace editor_core::internal

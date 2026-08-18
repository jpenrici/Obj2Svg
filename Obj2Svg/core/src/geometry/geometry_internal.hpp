#pragma once

#include "editor_core/geometry/mesh.hpp"
#include "editor_core/geometry/transform.hpp"

#include <cstddef>
#include <vector>

namespace editor_core::internal {

struct Vec3 {
  float x, y, z;
};
struct Vec4 {
  float x, y, z, w;
};

Vec3 operator-(const Vec3 &a, const Vec3 &b);
Vec3 operator+(const Vec3 &a, const Vec3 &b);
float dot(const Vec3 &a, const Vec3 &b);
Vec3 cross(const Vec3 &a, const Vec3 &b);
float length(const Vec3 &v);

Vec3 normalize(const Vec3 &v);

Vec3 newell_normal(const EditorMesh &mesh,
                   const std::vector<std::size_t> &vertex_indices);

Vec3 transform_direction(const Mat4 &m, const Vec3 &dir);

Vec4 transform_point(const Mat4 &m, const Vec3 &p);

} // namespace editor_core::internal

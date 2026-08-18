#pragma once

#include "editor_core/geometry/mesh.hpp"

namespace editor_core {

struct Mat4 {
  float m[16];
};

Mat4 make_identity();
Mat4 make_translation(float dx, float dy, float dz);
Mat4 make_rotation(float axis_x, float axis_y, float axis_z,
                   float angle_radians);
Mat4 make_scale(float sx, float sy, float sz);
Mat4 multiply(const Mat4 &a, const Mat4 &b);

void apply_transform(EditorMesh &mesh, const Mat4 &transform);

} // namespace editor_core

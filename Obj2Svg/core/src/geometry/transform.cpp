#include "editor_core/geometry/transform.hpp"

#include <cmath>

namespace editor_core {

Mat4 make_identity() {
  Mat4 result{};
  result.m[0] = 1.0f;
  result.m[5] = 1.0f;
  result.m[10] = 1.0f;
  result.m[15] = 1.0f;
  return result;
}

Mat4 make_translation(float dx, float dy, float dz) {
  Mat4 result = make_identity();
  result.m[12] = dx;
  result.m[13] = dy;
  result.m[14] = dz;
  return result;
}

Mat4 make_rotation(float axis_x, float axis_y, float axis_z,
                   float angle_radians) {
  const float len =
      std::sqrt(axis_x * axis_x + axis_y * axis_y + axis_z * axis_z);
  if (len < 1e-8f) {
    return make_identity();
  }

  const float x = axis_x / len;
  const float y = axis_y / len;
  const float z = axis_z / len;
  const float c = std::cos(angle_radians);
  const float s = std::sin(angle_radians);
  const float t = 1.0f - c;

  Mat4 result{};
  result.m[0] = t * x * x + c;
  result.m[1] = t * x * y + s * z;
  result.m[2] = t * x * z - s * y;
  result.m[3] = 0.0f;

  result.m[4] = t * x * y - s * z;
  result.m[5] = t * y * y + c;
  result.m[6] = t * y * z + s * x;
  result.m[7] = 0.0f;

  result.m[8] = t * x * z + s * y;
  result.m[9] = t * y * z - s * x;
  result.m[10] = t * z * z + c;
  result.m[11] = 0.0f;

  result.m[12] = 0.0f;
  result.m[13] = 0.0f;
  result.m[14] = 0.0f;
  result.m[15] = 1.0f;
  return result;
}

Mat4 make_scale(float sx, float sy, float sz) {
  Mat4 result = make_identity();
  result.m[0] = sx;
  result.m[5] = sy;
  result.m[10] = sz;
  return result;
}

Mat4 multiply(const Mat4 &a, const Mat4 &b) {
  Mat4 result{};
  for (int col = 0; col < 4; ++col) {
    for (int row = 0; row < 4; ++row) {
      float sum = 0.0f;
      for (int k = 0; k < 4; ++k) {
        sum += a.m[k * 4 + row] * b.m[col * 4 + k];
      }
      result.m[col * 4 + row] = sum;
    }
  }
  return result;
}

void apply_transform(EditorMesh &mesh, const Mat4 &transform) {
  for (auto &v : mesh.vertices) {
    const float x = transform.m[0] * v.x + transform.m[4] * v.y +
                    transform.m[8] * v.z + transform.m[12];
    const float y = transform.m[1] * v.x + transform.m[5] * v.y +
                    transform.m[9] * v.z + transform.m[13];
    const float z = transform.m[2] * v.x + transform.m[6] * v.y +
                    transform.m[10] * v.z + transform.m[14];
    v.x = x;
    v.y = y;
    v.z = z;
  }
}

} // namespace editor_core

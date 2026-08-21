#include "editor_api.h"

#include "editor_core/context.hpp"
#include "editor_core/error.hpp"
#include "editor_core/geometry/mesh.hpp"
#include "editor_core/geometry/projection.hpp"
#include "editor_core/geometry/transform.hpp"
#include "editor_core/geometry/triangulator.hpp"
#include "editor_core/io/obj_reader.hpp"
#include "editor_core/io/svg_writer.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace {

using editor_core::EditorContext;
using editor_core::EditorError;
using editor_core::EditorMesh;

constexpr uint32_t kNoNormalSentinel = UINT32_MAX;

EditorErrorCode to_c_error_code(editor_core::ErrorCode code) {
  using editor_core::ErrorCode;
  switch (code) {
  case ErrorCode::FileNotFound:
    return EDITOR_ERROR_FILE_NOT_FOUND;
  case ErrorCode::InvalidOBJ:
    return EDITOR_ERROR_INVALID_OBJ;
  case ErrorCode::InvalidFace:
    return EDITOR_ERROR_INVALID_FACE;
  case ErrorCode::InvalidVertex:
    return EDITOR_ERROR_INVALID_VERTEX;
  case ErrorCode::InvalidIndex:
    return EDITOR_ERROR_INVALID_INDEX;
  case ErrorCode::EmptyMesh:
    return EDITOR_ERROR_EMPTY_MESH;
  case ErrorCode::DegenerateFace:
    return EDITOR_ERROR_DEGENERATE_FACE;
  case ErrorCode::IOError:
    return EDITOR_ERROR_IO_ERROR;
  case ErrorCode::InvalidProjection:
    return EDITOR_ERROR_INVALID_PROJECTION;
  case ErrorCode::InternalError:
    return EDITOR_ERROR_INTERNAL_ERROR;
  }
  return EDITOR_ERROR_INTERNAL_ERROR;
}

} // namespace

struct EditorHandle_ {
  EditorContext context;
  EditorMesh mesh;
  editor_core::TriangleList triangles;
  EditorError last_error{};
  bool has_last_error = false;
  std::vector<editor_core::Vertex> original_vertices;
  float position[3] = {0.0f, 0.0f, 0.0f};
  float scale[3] = {1.0f, 1.0f, 1.0f};
  editor_core::Mat4 rotation = editor_core::make_identity();
};

namespace {

void recompute_vertices(EditorHandle_ &handle) {
  EditorMesh temp;
  temp.vertices = handle.original_vertices;

  const editor_core::Mat4 scale_m = editor_core::make_scale(
      handle.scale[0], handle.scale[1], handle.scale[2]);
  const editor_core::Mat4 translate_m = editor_core::make_translation(
      handle.position[0], handle.position[1], handle.position[2]);

  const editor_core::Mat4 rs = editor_core::multiply(handle.rotation, scale_m);
  const editor_core::Mat4 trs = editor_core::multiply(translate_m, rs);

  editor_core::apply_transform(temp, trs);
  handle.mesh.vertices = std::move(temp.vertices);
}

void extract_euler_degrees(const editor_core::Mat4 &r, float out_degrees[3]) {
  const float r02 = r.m[8], r12 = r.m[9], r22 = r.m[10];
  const float r00 = r.m[0], r01 = r.m[4];
  const float r10 = r.m[1], r11 = r.m[5];

  const float sin_y = std::clamp(r02, -1.0f, 1.0f);
  const float theta_y = std::asin(sin_y);

  float theta_x, theta_z;
  if (std::fabs(std::cos(theta_y)) > 1e-6f) {
    theta_x = std::atan2(-r12, r22);
    theta_z = std::atan2(-r01, r00);
  } else {
    // Gimbal lock: theta_x/theta_z aren't independently recoverable —
    // fold everything into theta_x and report zero for theta_z, a
    // common convention.
    theta_z = 0.0f;
    theta_x = (sin_y > 0.0f) ? std::atan2(r10, r11) : std::atan2(-r10, r11);
  }

  constexpr float kRadToDeg = 180.0f / 3.14159265358979323846f;
  out_degrees[0] = theta_x * kRadToDeg; // X
  out_degrees[1] = theta_y * kRadToDeg; // Y
  out_degrees[2] = theta_z * kRadToDeg; // Z
}

} // namespace

EditorHandle editor_create(void) {
  try {
    return new EditorHandle_();
  } catch (...) {
    return nullptr;
  }
}

void editor_destroy(EditorHandle handle) { delete handle; }

bool editor_load_obj(EditorHandle handle, const char *filepath) {
  if (!handle || !filepath) {
    return false;
  }
  try {
    auto result = editor_core::io::load_obj(handle->context, filepath);
    if (!result) {
      handle->last_error = result.error();
      handle->has_last_error = true;
      return false;
    }
    handle->mesh = std::move(*result);
    handle->triangles.clear();
    handle->original_vertices = handle->mesh.vertices;
    handle->position[0] = handle->position[1] = handle->position[2] = 0.0f;
    handle->scale[0] = handle->scale[1] = handle->scale[2] = 1.0f;
    handle->rotation = editor_core::make_identity();
    handle->has_last_error = false;
    return true;
  } catch (...) {
    handle->last_error = EditorError{
        editor_core::ErrorCode::InternalError, "editor_api::editor_load_obj",
        "unexpected exception crossing the C boundary", std::nullopt};
    handle->has_last_error = true;
    return false;
  }
}

EditorErrorCode editor_get_last_error_code(EditorHandle handle) {
  if (!handle || !handle->has_last_error) {
    return EDITOR_ERROR_NONE;
  }
  return to_c_error_code(handle->last_error.code);
}

size_t editor_get_last_error_message(EditorHandle handle, char *out_buffer,
                                     size_t buffer_size) {
  if (!handle || !handle->has_last_error) {
    if (out_buffer && buffer_size > 0) {
      out_buffer[0] = '\0';
    }
    return 0;
  }
  try {
    const std::string &msg = handle->last_error.message;
    if (out_buffer && buffer_size > 0) {
      const std::size_t copy_len = std::min(msg.size(), buffer_size - 1);
      std::memcpy(out_buffer, msg.data(), copy_len);
      out_buffer[copy_len] = '\0';
    }
    return msg.size();
  } catch (...) {
    if (out_buffer && buffer_size > 0) {
      out_buffer[0] = '\0';
    }
    return 0;
  }
}

size_t editor_get_vertex_count(EditorHandle handle) {
  if (!handle)
    return 0;
  return editor_core::vertex_count(handle->mesh);
}

void editor_get_vertices(EditorHandle handle, float *out_vertices) {
  if (!handle || !out_vertices)
    return;
  try {
    std::size_t i = 0;
    for (const auto &v : handle->mesh.vertices) {
      out_vertices[i * 3 + 0] = v.x;
      out_vertices[i * 3 + 1] = v.y;
      out_vertices[i * 3 + 2] = v.z;
      ++i;
    }
  } catch (...) {
  }
}

size_t editor_get_normal_count(EditorHandle handle) {
  if (!handle)
    return 0;
  return editor_core::normal_count(handle->mesh);
}

void editor_get_normals(EditorHandle handle, float *out_normals) {
  if (!handle || !out_normals)
    return;
  try {
    std::size_t i = 0;
    for (const auto &n : handle->mesh.normals) {
      out_normals[i * 3 + 0] = n.x;
      out_normals[i * 3 + 1] = n.y;
      out_normals[i * 3 + 2] = n.z;
      ++i;
    }
  } catch (...) {
  }
}

size_t editor_get_edge_count(EditorHandle handle) {
  if (!handle)
    return 0;
  try {
    return editor_core::compute_edges(handle->mesh).size();
  } catch (...) {
    return 0;
  }
}

void editor_get_edges(EditorHandle handle, uint32_t *out_edges) {
  if (!handle || !out_edges)
    return;
  try {
    const auto edges = editor_core::compute_edges(handle->mesh);
    for (std::size_t i = 0; i < edges.size(); ++i) {
      out_edges[i * 2 + 0] = static_cast<uint32_t>(edges[i].a);
      out_edges[i * 2 + 1] = static_cast<uint32_t>(edges[i].b);
    }
  } catch (...) {
  }
}

size_t editor_get_face_count(EditorHandle handle) {
  if (!handle)
    return 0;
  return editor_core::face_count(handle->mesh);
}

void editor_get_face_vertex_counts(EditorHandle handle, uint32_t *out_counts) {
  if (!handle || !out_counts)
    return;
  try {
    std::size_t i = 0;
    for (const auto &f : handle->mesh.faces) {
      out_counts[i++] = static_cast<uint32_t>(f.vertex_indices.size());
    }
  } catch (...) {
  }
}

size_t editor_get_face_index_total(EditorHandle handle) {
  if (!handle)
    return 0;
  try {
    std::size_t total = 0;
    for (const auto &f : handle->mesh.faces) {
      total += f.vertex_indices.size();
    }
    return total;
  } catch (...) {
    return 0;
  }
}

void editor_get_face_indices_flat(EditorHandle handle,
                                  uint32_t *out_vertex_indices,
                                  uint32_t *out_normal_indices) {
  if (!handle)
    return;
  try {
    std::size_t offset = 0;
    for (const auto &f : handle->mesh.faces) {
      const bool has_normals = !f.normal_indices.empty();
      for (std::size_t k = 0; k < f.vertex_indices.size(); ++k) {
        if (out_vertex_indices) {
          out_vertex_indices[offset + k] =
              static_cast<uint32_t>(f.vertex_indices[k]);
        }
        if (out_normal_indices) {
          out_normal_indices[offset + k] =
              has_normals ? static_cast<uint32_t>(f.normal_indices[k])
                          : kNoNormalSentinel;
        }
      }
      offset += f.vertex_indices.size();
    }
  } catch (...) {
  }
}

bool editor_triangulate(EditorHandle handle) {
  if (!handle)
    return false;
  try {
    auto result = editor_core::triangulate(handle->mesh);
    if (!result) {
      handle->last_error = result.error();
      handle->has_last_error = true;
      return false;
    }
    handle->triangles = std::move(*result);
    handle->has_last_error = false;
    return true;
  } catch (...) {
    handle->last_error = EditorError{
        editor_core::ErrorCode::InternalError, "editor_api::editor_triangulate",
        "unexpected exception crossing the C boundary", std::nullopt};
    handle->has_last_error = true;
    return false;
  }
}

size_t editor_get_triangle_count(EditorHandle handle) {
  if (!handle)
    return 0;
  return handle->triangles.size();
}

void editor_get_triangles(EditorHandle handle, uint32_t *out_indices) {
  if (!handle || !out_indices)
    return;
  try {
    for (std::size_t i = 0; i < handle->triangles.size(); ++i) {
      out_indices[i * 3 + 0] = static_cast<uint32_t>(handle->triangles[i].v0);
      out_indices[i * 3 + 1] = static_cast<uint32_t>(handle->triangles[i].v1);
      out_indices[i * 3 + 2] = static_cast<uint32_t>(handle->triangles[i].v2);
    }
  } catch (...) {
  }
}

bool editor_translate(EditorHandle handle, float dx, float dy, float dz) {
  if (!handle)
    return false;
  try {
    handle->position[0] += dx;
    handle->position[1] += dy;
    handle->position[2] += dz;
    recompute_vertices(*handle);
  } catch (...) {
  }
  return true;
}

bool editor_rotate(EditorHandle handle, float axis_x, float axis_y,
                   float axis_z, float angle_radians) {
  if (!handle)
    return false;
  try {
    const editor_core::Mat4 delta =
        editor_core::make_rotation(axis_x, axis_y, axis_z, angle_radians);
    handle->rotation = editor_core::multiply(delta, handle->rotation);
    recompute_vertices(*handle);
  } catch (...) {
  }
  return true;
}

bool editor_scale(EditorHandle handle, float sx, float sy, float sz) {
  if (!handle)
    return false;
  try {
    handle->scale[0] *= sx;
    handle->scale[1] *= sy;
    handle->scale[2] *= sz;
    recompute_vertices(*handle);
  } catch (...) {
  }
  return true;
}

bool editor_reset_mesh(EditorHandle handle) {
  if (!handle)
    return false;
  try {
    handle->position[0] = handle->position[1] = handle->position[2] = 0.0f;
    handle->scale[0] = handle->scale[1] = handle->scale[2] = 1.0f;
    handle->rotation = editor_core::make_identity();
    handle->mesh.vertices = handle->original_vertices;
  } catch (...) {
  }
  return true;
}

void editor_get_transform_state(EditorHandle handle, float out_position[3],
                                float out_rotation_euler_degrees[3],
                                float out_scale[3]) {
  if (!handle)
    return;
  try {
    if (out_position) {
      out_position[0] = handle->position[0];
      out_position[1] = handle->position[1];
      out_position[2] = handle->position[2];
    }
    if (out_scale) {
      out_scale[0] = handle->scale[0];
      out_scale[1] = handle->scale[1];
      out_scale[2] = handle->scale[2];
    }
    if (out_rotation_euler_degrees) {
      extract_euler_degrees(handle->rotation, out_rotation_euler_degrees);
    }
  } catch (...) {
  }
}

bool editor_export_svg(EditorHandle handle, const float view_matrix[16],
                       const float projection_matrix[16], int viewport_width,
                       int viewport_height, const char *filepath,
                       const EditorSvgExportOptions *options) {
  if (!handle || !view_matrix || !projection_matrix || !filepath) {
    return false;
  }
  try {
    editor_core::CameraView camera{};
    std::memcpy(camera.view_matrix.m, view_matrix, sizeof(float) * 16);
    std::memcpy(camera.projection_matrix.m, projection_matrix,
                sizeof(float) * 16);
    camera.viewport_width = viewport_width;
    camera.viewport_height = viewport_height;

    const editor_core::ProjectedMesh projected =
        editor_core::project_mesh(handle->mesh, camera);

    editor_core::io::SvgExportOptions svg_options{}; // Core defaults
    if (options) {
      svg_options.cull_back_faces = options->cull_back_faces;
      svg_options.sort_back_to_front = options->sort_back_to_front;
      svg_options.stroke_width = options->stroke_width;
      if (options->stroke_color) {
        svg_options.stroke_color = options->stroke_color;
      }
      svg_options.fill_color =
          options->fill_color ? std::optional<std::string>(options->fill_color)
                              : std::nullopt;
    }

    auto result = editor_core::io::write_svg_file(handle->context, filepath,
                                                  projected, svg_options);
    if (!result) {
      handle->last_error = result.error();
      handle->has_last_error = true;
      return false;
    }
    handle->has_last_error = false;
    return true;
  } catch (...) {
    handle->last_error = EditorError{
        editor_core::ErrorCode::InternalError, "editor_api::editor_export_svg",
        "unexpected exception crossing the C boundary", std::nullopt};
    handle->has_last_error = true;
    return false;
  }
}

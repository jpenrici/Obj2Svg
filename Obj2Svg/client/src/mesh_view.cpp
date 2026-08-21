#include "mesh_view.hpp"

#include <cmath>
#include <limits>
#include <vector>

namespace editor_client {

namespace {

constexpr Color kAmberEdgeColor{232, 126, 34, 255}; // Blender-style

struct Vec3 {
  float x, y, z;
};

Vec3 sub(const Vec3 &a, const Vec3 &b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}
Vec3 cross(const Vec3 &a, const Vec3 &b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
float length(const Vec3 &v) {
  return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

std::vector<float>
compute_smooth_normals(const std::vector<float> &vertices,
                       const std::vector<uint32_t> &indices) {
  const std::size_t vertex_count = vertices.size() / 3;
  std::vector<Vec3> accum(vertex_count, Vec3{0.0f, 0.0f, 0.0f});

  for (std::size_t t = 0; t < indices.size(); t += 3) {
    const uint32_t i0 = indices[t + 0];
    const uint32_t i1 = indices[t + 1];
    const uint32_t i2 = indices[t + 2];

    const Vec3 p0{vertices[i0 * 3 + 0], vertices[i0 * 3 + 1],
                  vertices[i0 * 3 + 2]};
    const Vec3 p1{vertices[i1 * 3 + 0], vertices[i1 * 3 + 1],
                  vertices[i1 * 3 + 2]};
    const Vec3 p2{vertices[i2 * 3 + 0], vertices[i2 * 3 + 1],
                  vertices[i2 * 3 + 2]};

    const Vec3 face_normal = cross(sub(p1, p0), sub(p2, p0));

    accum[i0] = {accum[i0].x + face_normal.x, accum[i0].y + face_normal.y,
                 accum[i0].z + face_normal.z};
    accum[i1] = {accum[i1].x + face_normal.x, accum[i1].y + face_normal.y,
                 accum[i1].z + face_normal.z};
    accum[i2] = {accum[i2].x + face_normal.x, accum[i2].y + face_normal.y,
                 accum[i2].z + face_normal.z};
  }

  std::vector<float> normals(vertex_count * 3, 0.0f);
  for (std::size_t i = 0; i < vertex_count; ++i) {
    const float len = length(accum[i]);
    if (len > std::numeric_limits<float>::epsilon()) {
      normals[i * 3 + 0] = accum[i].x / len;
      normals[i * 3 + 1] = accum[i].y / len;
      normals[i * 3 + 2] = accum[i].z / len;
    } else {
      normals[i * 3 + 1] = 1.0f;
    }
  }
  return normals;
}

} // namespace

MeshView::MeshView() : handle_(editor_create()) {}

MeshView::~MeshView() {
  unload_model();
  editor_destroy(handle_);
}

void MeshView::unload_model() {
  if (has_model_) {
    UnloadModel(model_);
    has_model_ = false;
  }
}

bool MeshView::load(const std::string &filepath) {
  if (!handle_) {
    return false;
  }

  if (!editor_load_obj(handle_, filepath.c_str())) {
    return false;
  }
  if (!editor_triangulate(handle_)) {
    return false;
  }

  const std::size_t vertex_count = editor_get_vertex_count(handle_);
  const std::size_t triangle_count = editor_get_triangle_count(handle_);
  if (vertex_count == 0 || triangle_count == 0) {
    return false;
  }

  std::vector<float> vertices(vertex_count * 3);
  editor_get_vertices(handle_, vertices.data());

  std::vector<uint32_t> indices32(triangle_count * 3);
  editor_get_triangles(handle_, indices32.data());
  cached_triangle_indices_ =
      indices32; // reused by translate/rotate/scale to recompute normals

  const std::vector<float> normals =
      compute_smooth_normals(vertices, indices32);

  std::vector<unsigned short> indices16(indices32.size());
  for (std::size_t i = 0; i < indices32.size(); ++i) {
    indices16[i] = static_cast<unsigned short>(indices32[i]);
  }

  Mesh mesh{};
  mesh.vertexCount = static_cast<int>(vertex_count);
  mesh.triangleCount = static_cast<int>(triangle_count);

  mesh.vertices = static_cast<float *>(
      MemAlloc(static_cast<unsigned int>(vertices.size() * sizeof(float))));
  for (std::size_t i = 0; i < vertices.size(); ++i) {
    mesh.vertices[i] = vertices[i];
  }

  mesh.normals = static_cast<float *>(
      MemAlloc(static_cast<unsigned int>(normals.size() * sizeof(float))));
  for (std::size_t i = 0; i < normals.size(); ++i) {
    mesh.normals[i] = normals[i];
  }

  mesh.indices = static_cast<unsigned short *>(MemAlloc(
      static_cast<unsigned int>(indices16.size() * sizeof(unsigned short))));
  for (std::size_t i = 0; i < indices16.size(); ++i) {
    mesh.indices[i] = indices16[i];
  }

  UploadMesh(&mesh, false);

  unload_model();
  model_ = LoadModelFromMesh(mesh);
  has_model_ = true;
  return true;
}

void MeshView::draw() const {
  if (!has_model_) {
    return;
  }
  DrawModel(model_, Vector3{0.0f, 0.0f, 0.0f}, 1.0f, LIGHTGRAY);
  DrawModelWires(model_, Vector3{0.0f, 0.0f, 0.0f}, 1.0f, kAmberEdgeColor);
}

void MeshView::refresh_gpu_buffers() {
  if (!has_model_) {
    return;
  }

  const std::size_t vertex_count = editor_get_vertex_count(handle_);
  std::vector<float> vertices(vertex_count * 3);
  editor_get_vertices(handle_, vertices.data());

  const std::vector<float> normals =
      compute_smooth_normals(vertices, cached_triangle_indices_);

  Mesh &mesh = model_.meshes[0];
  UpdateMeshBuffer(mesh, 0, vertices.data(),
                   static_cast<int>(vertices.size() * sizeof(float)), 0);
  UpdateMeshBuffer(mesh, 2, normals.data(),
                   static_cast<int>(normals.size() * sizeof(float)), 0);
}

void MeshView::translate(float dx, float dy, float dz) {
  if (!has_model_) {
    return;
  }
  editor_translate(handle_, dx, dy, dz);
  refresh_gpu_buffers();
}

void MeshView::rotate(float axis_x, float axis_y, float axis_z,
                      float angle_radians) {
  if (!has_model_) {
    return;
  }
  editor_rotate(handle_, axis_x, axis_y, axis_z, angle_radians);
  refresh_gpu_buffers();
}

void MeshView::scale(float sx, float sy, float sz) {
  if (!has_model_) {
    return;
  }
  editor_scale(handle_, sx, sy, sz);
  refresh_gpu_buffers();
}

void MeshView::reset() {
  if (!has_model_) {
    return;
  }
  editor_reset_mesh(handle_);
  refresh_gpu_buffers();
}

std::string MeshView::get_last_error() const {
  if (!handle_) {
    return "editor session failed to initialize";
  }
  char buffer[512];
  editor_get_last_error_message(handle_, buffer, sizeof(buffer));
  return std::string(buffer);
}

std::string MeshView::transform_hud_text() const {
  if (!has_model_) {
    return {};
  }
  float position[3], rotation[3], scale_factors[3];
  editor_get_transform_state(handle_, position, rotation, scale_factors);

  char buffer[192];
  std::snprintf(
      buffer, sizeof(buffer),
      "Pos (%.2f, %.2f, %.2f)  Rot (%.1f, %.1f, %.1f) deg  Scale (%.2f, %.2f, "
      "%.2f)",
      static_cast<double>(position[0]), static_cast<double>(position[1]),
      static_cast<double>(position[2]), static_cast<double>(rotation[0]),
      static_cast<double>(rotation[1]), static_cast<double>(rotation[2]),
      static_cast<double>(scale_factors[0]),
      static_cast<double>(scale_factors[1]),
      static_cast<double>(scale_factors[2]));
  return std::string(buffer);
}

} // namespace editor_client

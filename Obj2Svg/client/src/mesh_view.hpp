#pragma once

#include "raylib.h"

#include "editor_api.h"

#include <string>
#include <vector>

namespace editor_client {

class MeshView {
public:
  MeshView();
  ~MeshView();

  MeshView(const MeshView &) = delete;
  MeshView &operator=(const MeshView &) = delete;

  bool load(const std::string &filepath);
  bool has_model() const { return has_model_; }

  void draw() const;

  void translate(float dx, float dy, float dz);
  void rotate(float axis_x, float axis_y, float axis_z, float angle_radians);
  void scale(float sx, float sy, float sz);

  void reset();

  std::string transform_hud_text() const;

  std::string get_last_error() const;
  EditorHandle handle() const { return handle_; }

private:
  void unload_model();
  void refresh_gpu_buffers();

  EditorHandle handle_;
  Model model_{};
  bool has_model_ = false;
  std::vector<uint32_t> cached_triangle_indices_;
};

} // namespace editor_client

#include "svg_export_command.hpp"
#include "editor_api.h"
#include "rcamera.h"

namespace editor_client {

bool export_svg(MeshView &mesh_view, const Camera3D &camera, int viewport_width,
                int viewport_height, const std::string &filepath,
                bool wireframe) {
  EditorHandle handle = mesh_view.handle();
  if (!handle || !mesh_view.has_model()) {
    return false;
  }

  Camera3D camera_copy = camera;
  const Matrix view = GetCameraViewMatrix(&camera_copy);
  const float aspect = (viewport_height != 0)
                           ? static_cast<float>(viewport_width) /
                                 static_cast<float>(viewport_height)
                           : 1.0f;
  const Matrix projection = GetCameraProjectionMatrix(&camera_copy, aspect);

  const float view_matrix[16] = {
      view.m0,  view.m1,  view.m2,  view.m3,  view.m4,  view.m5,
      view.m6,  view.m7,  view.m8,  view.m9,  view.m10, view.m11,
      view.m12, view.m13, view.m14, view.m15,
  };
  const float projection_matrix[16] = {
      projection.m0,  projection.m1,  projection.m2,  projection.m3,
      projection.m4,  projection.m5,  projection.m6,  projection.m7,
      projection.m8,  projection.m9,  projection.m10, projection.m11,
      projection.m12, projection.m13, projection.m14, projection.m15,
  };

  EditorSvgExportOptions options{};
  options.cull_back_faces = true;
  options.sort_back_to_front = true;
  options.stroke_width = 1.0f;
  options.stroke_color = "#000000";
  options.fill_color = wireframe ? nullptr : "#cccccc";

  return editor_export_svg(handle, view_matrix, projection_matrix,
                           viewport_width, viewport_height, filepath.c_str(),
                           &options);
}

} // namespace editor_client

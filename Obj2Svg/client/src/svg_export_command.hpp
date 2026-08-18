#pragma once

#include "raylib.h"

#include "mesh_view.hpp"

#include <string>

namespace editor_client {

bool export_svg(MeshView& mesh_view, const Camera3D& camera, int viewport_width, int viewport_height,
                 const std::string& filepath, bool wireframe);

} // namespace editor_client

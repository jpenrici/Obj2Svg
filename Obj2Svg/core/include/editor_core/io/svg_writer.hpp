#pragma once

#include "editor_core/context.hpp"
#include "editor_core/error.hpp"
#include "editor_core/geometry/projection.hpp"

#include <expected>
#include <optional>
#include <string>
#include <string_view>

namespace editor_core::io {

struct SvgExportOptions {
  bool cull_back_faces = true;
  bool sort_back_to_front = true;
  float stroke_width = 1.0f;
  std::string stroke_color = "#000000";
  std::optional<std::string> fill_color;
};

std::string build_svg(const ProjectedMesh &mesh,
                      const SvgExportOptions &options = {});

std::expected<void, EditorError>
write_svg_file(const EditorContext &ctx, std::string_view filepath,
               const ProjectedMesh &mesh, const SvgExportOptions &options = {});

} // namespace editor_core::io

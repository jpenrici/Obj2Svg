#pragma once

#include "editor_core/context.hpp"
#include "editor_core/error.hpp"
#include "editor_core/geometry/mesh.hpp"

#include <expected>
#include <string_view>

namespace editor_core::io {

std::expected<EditorMesh, EditorError> load_obj(const EditorContext &ctx,
                                                std::string_view filepath);

} // namespace editor_core::io

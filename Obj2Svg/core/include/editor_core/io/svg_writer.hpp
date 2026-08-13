#pragma once

#include <expected>
#include <optional>
#include <string>
#include <string_view>

#include "editor_core/context.hpp"
#include "editor_core/error.hpp"
#include "editor_core/geometry/projection.hpp"

namespace editor_core::io {

struct SvgExportOptions {
    bool  cull_back_faces    = true;
    bool  sort_back_to_front = true;  // painter's algorithm via each polygon's mean depth; only affects solid mode
    float stroke_width       = 1.0f;
    std::string stroke_color = "#000000";
    std::optional<std::string> fill_color; // nullopt = wireframe mode (edges only); set = solid mode (filled, sorted polygons)
};

/// @brief Renders @c mesh to an SVG document (as a string).
///
/// Operates exclusively on ProjectedMesh — no 3D geometry, no camera. Two
/// modes, selected by @c options.fill_color:
///
/// - **Wireframe** (@c fill_color unset): emits one `<line>` per unique
///   edge of the visible polygons (an edge is visible if at least one of
///   its adjacent polygons passes the @c cull_back_faces filter). No
///   sorting needed — lines don't visually overlap in a way that ordering
///   would fix.
/// - **Solid** (@c fill_color set): emits one `<polygon>` per visible
///   polygon (filtered by @c cull_back_faces), each carrying
///   `id="polygon-N"` where N is its index in @c mesh.polygons — stable
///   regardless of sorting, useful for tests and debugging alike. When
///   @c options.sort_back_to_front is true, polygons are emitted in
///   descending order of mean vertex depth (farthest first): since later
///   SVG elements paint over earlier ones, this reproduces the painter's
///   algorithm. This is not an exact z-buffer — meshes with faces that
///   genuinely intersect in 3D can still render incorrectly — but it
///   handles the common case of convex/non-self-intersecting meshes
///   correctly, which is this phase's scope (see spec section 9).
///
/// The output's `viewBox` is derived from the bounding box of
/// @c mesh.vertices (SvgWriter has no access to the original camera
/// viewport size — only ProjectedMesh, by design).
std::string build_svg(const ProjectedMesh& mesh, const SvgExportOptions& options = {});

/// @brief Writes `build_svg(mesh, options)` to @c filepath.
///
/// @return ErrorCode::IOError if the file cannot be opened or the write
///         fails; otherwise an empty (success) result. Logs an Info event
///         through @c ctx's LogSink on success.
std::expected<void, EditorError> write_svg_file(const EditorContext& ctx,
                                                 std::string_view filepath,
                                                 const ProjectedMesh& mesh,
                                                 const SvgExportOptions& options = {});

} // namespace editor_core::io

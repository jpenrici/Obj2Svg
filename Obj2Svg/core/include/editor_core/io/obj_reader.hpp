#pragma once

#include <expected>
#include <string_view>

#include "editor_core/context.hpp"
#include "editor_core/error.hpp"
#include "editor_core/geometry/mesh.hpp"

namespace editor_core::io {

/// @brief Parses an OBJ file into an EditorMesh.
///
/// Supports `v`, `vn` and `f` (with `f v`, `f v/vt`, `f v//vn`, `f v/vt/vn`
/// tokens — `vt` is parsed but discarded, never stored) including
/// non-triangulated n-gons.
///
/// Index resolution happens entirely inside this function: both positive
/// (1-based) and negative (relative — counting backward from the last
/// vertex/normal read up to that point in the file) indices are resolved
/// to absolute, 0-based, range-checked indices before being stored in the
/// resulting EditorMesh. The rest of the Core never sees a relative or
/// 1-based index. Index `0`, or any index that falls outside the valid
/// range once resolved, is ErrorCode::InvalidIndex.
///
/// Every parsed Face satisfies the invariant documented in mesh.hpp:
/// @c normal_indices is either the same size as @c vertex_indices, or
/// empty. A face that mixes vertex refs with and without a normal index
/// (e.g. `f v1//vn1 v2 v3`) is rejected as ErrorCode::InvalidFace.
///
/// `o`, `g`, `s`, `mtllib`, `usemtl`, `vt` and `#` comments — plus any
/// other directive not recognized above — are skipped without producing
/// an error (only a LogLevel::Info event through @c ctx's LogSink, if
/// configured). This is necessary because Blender emits several of these
/// by default on any OBJ export, and being lenient with unrecognized
/// directives keeps the parser robust to minor format variations.
///
/// @param ctx      Editor context (used only for the LogSink here).
/// @param filepath Path to the .obj file to read.
/// @return The parsed EditorMesh, or an EditorError describing the first
///         failure encountered (ErrorCode::FileNotFound if the file can't
///         be opened; ErrorCode::EmptyMesh if parsing succeeds but yields
///         no vertices or no faces; see error.hpp for the rest).
std::expected<EditorMesh, EditorError> load_obj(const EditorContext &ctx,
                                                std::string_view filepath);

} // namespace editor_core::io

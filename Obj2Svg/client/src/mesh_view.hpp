#pragma once

// NOTE (per your request): written against the public Raylib API as
// documented, not built/run here — you'll build against your own Raylib
// install. If something doesn't match, tell me the exact error and I'll
// adjust.

#include <string>

#include "raylib.h"

#include "editor_api.h"

namespace editor_client {

/// @brief Owns an EditorHandle and the raylib Model derived from it.
///
/// Raylib meshes are always triangle lists, while the Core's faces may be
/// n-gons — so load() always triangulates via the C API's
/// editor_triangulate (the Core's ear-clipping triangulator) before
/// building the Model. See mesh_view.cpp for why per-vertex normals are
/// computed here rather than reused from the OBJ file's `vn` data.
class MeshView {
public:
    MeshView();
    ~MeshView();

    MeshView(const MeshView&) = delete;
    MeshView& operator=(const MeshView&) = delete;

    /// Loads @c filepath via the C API, triangulates it, and rebuilds the
    /// raylib Model. Returns true on success; on failure, any
    /// previously-loaded Model is left untouched and get_last_error()
    /// describes what went wrong (parse error or a degenerate face that
    /// couldn't be triangulated).
    bool load(const std::string& filepath);

    /// True once a Model has been successfully built by a prior load().
    bool has_model() const { return has_model_; }

    /// Draws the current Model at the origin. No-op if has_model() is false.
    void draw() const;

    /// Human-readable description of the last load() failure.
    std::string get_last_error() const;

    /// The handle backing this view. Exposed for Phase 5's
    /// svg_export_command, which needs access to the loaded EditorMesh
    /// (through a future editor_api.h extension) to project and export it.
    EditorHandle handle() const { return handle_; }

private:
    void unload_model();

    EditorHandle handle_;
    Model model_{};
    bool has_model_ = false;
};

} // namespace editor_client

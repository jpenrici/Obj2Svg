// editor_api.cpp
//
// Hard rule enforced by every function in this file: no C++ exception may
// ever cross the extern "C" boundary — doing so is undefined behavior for
// any non-C++ caller (and even for a C++ caller compiled against a
// different exception ABI). Every function that touches Core code (which
// could, in principle, throw std::bad_alloc or similar from underlying
// std:: containers) wraps its body in try { ... } catch (...) and
// translates any exception to EDITOR_ERROR_INTERNAL_ERROR / a harmless
// default return value instead.

#include "editor_api.h"

#include <algorithm>
#include <cstring>
#include <new>

#include "editor_core/context.hpp"
#include "editor_core/error.hpp"
#include "editor_core/geometry/mesh.hpp"
#include "editor_core/geometry/transform.hpp"
#include "editor_core/geometry/triangulator.hpp"
#include "editor_core/io/obj_reader.hpp"

namespace {

using editor_core::EditorContext;
using editor_core::EditorError;
using editor_core::EditorMesh;

constexpr uint32_t kNoNormalSentinel = UINT32_MAX;

EditorErrorCode to_c_error_code(editor_core::ErrorCode code) {
    using editor_core::ErrorCode;
    switch (code) {
        case ErrorCode::FileNotFound:       return EDITOR_ERROR_FILE_NOT_FOUND;
        case ErrorCode::InvalidOBJ:         return EDITOR_ERROR_INVALID_OBJ;
        case ErrorCode::InvalidFace:        return EDITOR_ERROR_INVALID_FACE;
        case ErrorCode::InvalidVertex:      return EDITOR_ERROR_INVALID_VERTEX;
        case ErrorCode::InvalidIndex:       return EDITOR_ERROR_INVALID_INDEX;
        case ErrorCode::EmptyMesh:          return EDITOR_ERROR_EMPTY_MESH;
        case ErrorCode::DegenerateFace:     return EDITOR_ERROR_DEGENERATE_FACE;
        case ErrorCode::IOError:            return EDITOR_ERROR_IO_ERROR;
        case ErrorCode::InvalidProjection:  return EDITOR_ERROR_INVALID_PROJECTION;
        case ErrorCode::InternalError:      return EDITOR_ERROR_INTERNAL_ERROR;
    }
    return EDITOR_ERROR_INTERNAL_ERROR; // unreachable — defensive default
}

} // namespace

/// @brief Internal layout behind the opaque EditorHandle pointer.
struct EditorHandle_ {
    EditorContext context;
    EditorMesh mesh;                       ///< Empty until editor_load_obj succeeds.
    editor_core::TriangleList triangles;   ///< Empty until editor_triangulate succeeds.
    EditorError last_error{};
    bool has_last_error = false;
};

EditorHandle editor_create(void) {
    try {
        return new EditorHandle_();
    } catch (...) {
        return nullptr;
    }
}

void editor_destroy(EditorHandle handle) {
    delete handle; // delete on a null pointer is well-defined (no-op)
}

bool editor_load_obj(EditorHandle handle, const char* filepath) {
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
        handle->triangles.clear(); // stale relative to the newly loaded mesh — editor_triangulate must be called again
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

size_t editor_get_last_error_message(EditorHandle handle, char* out_buffer, size_t buffer_size) {
    if (!handle || !handle->has_last_error) {
        if (out_buffer && buffer_size > 0) {
            out_buffer[0] = '\0';
        }
        return 0;
    }
    try {
        const std::string& msg = handle->last_error.message;
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
    if (!handle) return 0;
    return editor_core::vertex_count(handle->mesh);
}

void editor_get_vertices(EditorHandle handle, float* out_vertices) {
    if (!handle || !out_vertices) return;
    try {
        std::size_t i = 0;
        for (const auto& v : handle->mesh.vertices) {
            out_vertices[i * 3 + 0] = v.x;
            out_vertices[i * 3 + 1] = v.y;
            out_vertices[i * 3 + 2] = v.z;
            ++i;
        }
    } catch (...) {
        // Nothing meaningful to do with a partially-filled caller buffer —
        // the exception is swallowed so it never crosses the C boundary.
    }
}

size_t editor_get_normal_count(EditorHandle handle) {
    if (!handle) return 0;
    return editor_core::normal_count(handle->mesh);
}

void editor_get_normals(EditorHandle handle, float* out_normals) {
    if (!handle || !out_normals) return;
    try {
        std::size_t i = 0;
        for (const auto& n : handle->mesh.normals) {
            out_normals[i * 3 + 0] = n.x;
            out_normals[i * 3 + 1] = n.y;
            out_normals[i * 3 + 2] = n.z;
            ++i;
        }
    } catch (...) {
    }
}

size_t editor_get_edge_count(EditorHandle handle) {
    if (!handle) return 0;
    try {
        return editor_core::compute_edges(handle->mesh).size();
    } catch (...) {
        return 0;
    }
}

void editor_get_edges(EditorHandle handle, uint32_t* out_edges) {
    if (!handle || !out_edges) return;
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
    if (!handle) return 0;
    return editor_core::face_count(handle->mesh);
}

void editor_get_face_vertex_counts(EditorHandle handle, uint32_t* out_counts) {
    if (!handle || !out_counts) return;
    try {
        std::size_t i = 0;
        for (const auto& f : handle->mesh.faces) {
            out_counts[i++] = static_cast<uint32_t>(f.vertex_indices.size());
        }
    } catch (...) {
    }
}

size_t editor_get_face_index_total(EditorHandle handle) {
    if (!handle) return 0;
    try {
        std::size_t total = 0;
        for (const auto& f : handle->mesh.faces) {
            total += f.vertex_indices.size();
        }
        return total;
    } catch (...) {
        return 0;
    }
}

void editor_get_face_indices_flat(EditorHandle handle, uint32_t* out_vertex_indices,
                                   uint32_t* out_normal_indices) {
    if (!handle) return;
    try {
        std::size_t offset = 0;
        for (const auto& f : handle->mesh.faces) {
            const bool has_normals = !f.normal_indices.empty();
            for (std::size_t k = 0; k < f.vertex_indices.size(); ++k) {
                if (out_vertex_indices) {
                    out_vertex_indices[offset + k] = static_cast<uint32_t>(f.vertex_indices[k]);
                }
                if (out_normal_indices) {
                    out_normal_indices[offset + k] =
                        has_normals ? static_cast<uint32_t>(f.normal_indices[k]) : kNoNormalSentinel;
                }
            }
            offset += f.vertex_indices.size();
        }
    } catch (...) {
    }
}

bool editor_triangulate(EditorHandle handle) {
    if (!handle) return false;
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
    if (!handle) return 0;
    return handle->triangles.size();
}

void editor_get_triangles(EditorHandle handle, uint32_t* out_indices) {
    if (!handle || !out_indices) return;
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
    if (!handle) return false;
    try {
        editor_core::apply_transform(handle->mesh, editor_core::make_translation(dx, dy, dz));
    } catch (...) {
        // apply_transform never throws by design, but a caught exception
        // here still must not cross the C boundary — see file header.
    }
    return true;
}

bool editor_rotate(EditorHandle handle, float axis_x, float axis_y, float axis_z, float angle_radians) {
    if (!handle) return false;
    try {
        editor_core::apply_transform(handle->mesh,
                                      editor_core::make_rotation(axis_x, axis_y, axis_z, angle_radians));
    } catch (...) {
    }
    return true;
}

bool editor_scale(EditorHandle handle, float sx, float sy, float sz) {
    if (!handle) return false;
    try {
        editor_core::apply_transform(handle->mesh, editor_core::make_scale(sx, sy, sz));
    } catch (...) {
    }
    return true;
}

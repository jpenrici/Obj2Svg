#ifndef OBJ2SVG_EDITOR_API_H
#define OBJ2SVG_EDITOR_API_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opaque handle to an editor session: an EditorContext plus the mesh
 * currently loaded into it (if any) and the last recoverable error, if
 * any. The frontend never sees the layout — only ever holds this pointer.
 *
 * Every function below is safe to call with a NULL handle (returns a
 * harmless default: 0 / false / a no-op) and never lets a C++ exception
 * cross this boundary — any unexpected exception is caught internally and
 * translated to EDITOR_ERROR_INTERNAL_ERROR.
 */
typedef struct EditorHandle_* EditorHandle;

/**
 * Mirrors editor_core::ErrorCode (see core/include/editor_core/error.hpp)
 * one-to-one, translated at the API boundary since std::expected/
 * EditorError aren't C-compatible types. Kept in sync manually — see the
 * static_assert-backed mapping table in editor_api.cpp.
 */
typedef enum EditorErrorCode {
    EDITOR_ERROR_NONE = 0, /**< No error is currently set for this handle. */
    EDITOR_ERROR_FILE_NOT_FOUND,
    EDITOR_ERROR_INVALID_OBJ,
    EDITOR_ERROR_INVALID_FACE,
    EDITOR_ERROR_INVALID_VERTEX,
    EDITOR_ERROR_INVALID_INDEX,
    EDITOR_ERROR_EMPTY_MESH,
    EDITOR_ERROR_DEGENERATE_FACE,
    EDITOR_ERROR_IO_ERROR,
    EDITOR_ERROR_INVALID_PROJECTION,
    EDITOR_ERROR_INTERNAL_ERROR,
} EditorErrorCode;

/* ---------------------------------------------------------------------
 * Session lifecycle
 * ------------------------------------------------------------------- */

/** Creates a new, empty editor session. Returns NULL on allocation failure. */
EditorHandle editor_create(void);

/** Destroys a session created by editor_create. NULL is a safe no-op. */
void editor_destroy(EditorHandle handle);

/**
 * Loads an OBJ file into the session, replacing any previously loaded
 * mesh. Returns true on success. On failure, returns false — call
 * editor_get_last_error_code / editor_get_last_error_message for details.
 */
bool editor_load_obj(EditorHandle handle, const char* filepath);

/* ---------------------------------------------------------------------
 * Error introspection — reflects the outcome of the last fallible call
 * made with this handle (editor_load_obj today; more as the API grows).
 * ------------------------------------------------------------------- */

EditorErrorCode editor_get_last_error_code(EditorHandle handle);

/**
 * Copies the last error's human-readable message into out_buffer
 * (NUL-terminated, truncated to fit if out_buffer/buffer_size is too
 * small). Returns the message's full length (excluding the NUL
 * terminator), snprintf-style, so the caller can detect truncation by
 * comparing the return value against buffer_size.
 *
 * out_buffer may be NULL (e.g. to just query the required length with
 * buffer_size = 0).
 */
size_t editor_get_last_error_message(EditorHandle handle, char* out_buffer, size_t buffer_size);

/* ---------------------------------------------------------------------
 * Mesh queries — fixed convention: query count, allocate in the
 * frontend, then fill. Every out_* buffer must be allocated by the
 * caller with at least the capacity implied by the corresponding count.
 * ------------------------------------------------------------------- */

size_t editor_get_vertex_count(EditorHandle handle);
/** out_vertices: >= 3 * editor_get_vertex_count(handle) floats, [x0,y0,z0, x1,y1,z1, ...]. */
void   editor_get_vertices(EditorHandle handle, float* out_vertices);

size_t editor_get_normal_count(EditorHandle handle);
/** out_normals: >= 3 * editor_get_normal_count(handle) floats, [x0,y0,z0, x1,y1,z1, ...]. */
void   editor_get_normals(EditorHandle handle, float* out_normals);

size_t editor_get_edge_count(EditorHandle handle);
/** out_edges: >= 2 * editor_get_edge_count(handle) uint32_t, [a0,b0, a1,b1, ...]. */
void   editor_get_edges(EditorHandle handle, uint32_t* out_edges);

/*
 * Faces are variable-sized (n-gons), so they use a flat/CSR layout
 * instead of one call per face — see docs/HISTORICO.md decision #3.
 */
size_t editor_get_face_count(EditorHandle handle);
/** out_counts: >= editor_get_face_count(handle) uint32_t — vertex/normal count of each face, in order. */
void   editor_get_face_vertex_counts(EditorHandle handle, uint32_t* out_counts);
/** Sum of every value editor_get_face_vertex_counts would write — required capacity for the flat buffers below. */
size_t editor_get_face_index_total(EditorHandle handle);
/**
 * out_vertex_indices / out_normal_indices: >= editor_get_face_index_total(handle)
 * uint32_t each; single contiguous buffers, one entry per (face, local
 * vertex) pair in face order — the caller reconstructs per-face offsets
 * by summing editor_get_face_vertex_counts incrementally.
 *
 * A face with no normals (see the Face invariant in mesh.hpp — empty
 * normal_indices is a valid state) writes UINT32_MAX as a "no normal"
 * sentinel for each of that face's entries in out_normal_indices, since 0
 * is itself a valid index. Either output pointer may be NULL to skip it.
 */
void   editor_get_face_indices_flat(EditorHandle handle, uint32_t* out_vertex_indices,
                                     uint32_t* out_normal_indices);

/* ---------------------------------------------------------------------
 * Triangulation — required before rendering, since GPU mesh formats
 * (e.g. Raylib's Mesh/UploadMesh) only understand triangle lists, while
 * faces above may be n-gons. Deliberately a separate, explicit step from
 * editor_load_obj (not run automatically) so callers who only need raw
 * n-gon face data (e.g. a future SVG-export path) aren't forced to pay
 * for it.
 * ------------------------------------------------------------------- */

/**
 * Triangulates the currently loaded mesh via the Core's ear-clipping
 * triangulator (editor_core::triangulate — handles concave n-gons
 * correctly). Frontends MUST use this rather than a naive fan
 * triangulation of their own: a fan silently produces wrong geometry for
 * any concave n-gon, exactly the case this triangulator exists to handle
 * (see triangulator.hpp).
 *
 * Returns true on success. On failure (e.g. a face with collinear or
 * duplicated vertices — EDITOR_ERROR_DEGENERATE_FACE), returns false and
 * any previously cached triangulation from an earlier successful call is
 * left untouched.
 */
bool editor_triangulate(EditorHandle handle);

size_t editor_get_triangle_count(EditorHandle handle);
/** out_indices: >= 3 * editor_get_triangle_count(handle) uint32_t, [v0,v1,v2, v0,v1,v2, ...] — indices into the same vertex buffer as editor_get_vertices. */
void   editor_get_triangles(EditorHandle handle, uint32_t* out_indices);

/* ---------------------------------------------------------------------
 * Basic editing (Phase 4) — in-place transforms on the loaded mesh's
 * vertex positions, via editor_core::transform.hpp. Mirrors that header's
 * conventions exactly: normals are NOT updated (see transform.hpp's
 * documented limitation — a non-uniform scale or rotation will make
 * existing normals stale relative to the new geometry), and multiple
 * calls compose in the order they're made (each is applied immediately,
 * not queued).
 *
 * These never fail on their own — make_translation/make_rotation/
 * make_scale + apply_transform always succeed, even for a degenerate
 * rotation axis (falls back to identity, see transform.hpp) — so the
 * bool return here only reflects whether @c handle itself was valid
 * (false only for a NULL handle). Calling any of these before a mesh is
 * loaded is a harmless no-op on an empty vertex list.
 * ------------------------------------------------------------------- */

bool editor_translate(EditorHandle handle, float dx, float dy, float dz);
bool editor_rotate(EditorHandle handle, float axis_x, float axis_y, float axis_z, float angle_radians);
bool editor_scale(EditorHandle handle, float sx, float sy, float sz);

#ifdef __cplusplus
}
#endif

#endif /* OBJ2SVG_EDITOR_API_H */

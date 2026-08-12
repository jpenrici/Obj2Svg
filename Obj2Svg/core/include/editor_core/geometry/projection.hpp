#pragma once

#include <cstddef>
#include <vector>

#include "editor_core/geometry/mesh.hpp"
#include "editor_core/geometry/transform.hpp"

namespace editor_core {

/// @brief Camera parameters for projecting an EditorMesh to 2D.
struct CameraView {
    Mat4 view_matrix;
    Mat4 projection_matrix;
    int  viewport_width;
    int  viewport_height;
};

/// @brief A vertex projected to viewport (pixel) coordinates.
///
/// @c depth is the vertex's view-space distance from the camera along the
/// view direction (i.e. -view_space.z under the OpenGL-style convention
/// used here, where the camera looks down -Z in view space) — linear and
/// monotonic with distance, which is what back-to-front sorting (the
/// painter's algorithm in svg_writer) needs. It is deliberately not NDC
/// z, which is nonlinear after perspective projection.
struct ProjectedVertex { float x, y; float depth; };

struct ProjectedEdge { std::size_t a, b; };

struct ProjectedPolygon { std::vector<std::size_t> vertex_indices; bool front_facing; };

/// @brief A mesh projected to 2D, ready for SvgWriter. Vertex indices here
///        map 1:1 to the source EditorMesh's vertex indices — project_mesh
///        projects every vertex exactly once.
struct ProjectedMesh {
    std::vector<ProjectedVertex>  vertices;
    std::vector<ProjectedEdge>    edges;
    std::vector<ProjectedPolygon> polygons;
};

/// @brief Projects every vertex of @c mesh through @c camera into viewport
///        (pixel) coordinates, and derives the unique wireframe edges and
///        one ProjectedPolygon per face (front-facing computed via
///        is_face_front_facing).
///
/// Y is flipped during the NDC-to-viewport mapping (NDC +Y is up; SVG/
/// screen +Y is down), so viewport coordinates match the convention
/// svg_writer expects directly.
ProjectedMesh project_mesh(const EditorMesh& mesh, const CameraView& camera);

/// @brief Whether @c face is front-facing (normal points toward the
///        camera) under @c camera's view transform.
///
/// The face normal is estimated in model space via Newell's method (see
/// triangulator.hpp), then transformed into view space using only the
/// linear part of @c camera.view_matrix (valid because a view matrix is a
/// rigid transform — rotation + translation, no non-uniform scale). The
/// face is front-facing when that view-space normal points toward the
/// camera, i.e. has a positive Z component (the camera looks down -Z in
/// view space).
bool is_face_front_facing(const EditorMesh& mesh, const Face& face, const CameraView& camera);

} // namespace editor_core

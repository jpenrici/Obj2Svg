#include "editor_core/geometry/projection.hpp"

#include <algorithm>
#include <set>
#include <utility>

#include "geometry_internal.hpp"

namespace editor_core {

namespace {

using internal::Vec3;
using internal::Vec4;

/// @brief Combined view-projection matrix used to project every vertex.
Mat4 view_projection(const CameraView& camera) {
    return multiply(camera.projection_matrix, camera.view_matrix);
}

/// @brief Projects a single model-space point to viewport coordinates.
///
/// Applies @c vp, performs the perspective divide, maps NDC [-1, 1] to
/// pixel coordinates, and flips Y (NDC +Y is up, viewport +Y is down).
/// @c depth is the point's view-space distance from the camera (see
/// ProjectedVertex doc); it needs the view matrix alone, not @c vp.
ProjectedVertex project_point(const Vec3& p, const Mat4& vp, const Mat4& view_matrix,
                               int viewport_width, int viewport_height) {
    const Vec4 clip = internal::transform_point(vp, p);
    const float w = (clip.w != 0.0f) ? clip.w : 1.0f; // guard against a degenerate projection matrix
    const float ndc_x = clip.x / w;
    const float ndc_y = clip.y / w;

    const Vec4 view = internal::transform_point(view_matrix, p);

    ProjectedVertex out{};
    out.x = (ndc_x * 0.5f + 0.5f) * static_cast<float>(viewport_width);
    out.y = (1.0f - (ndc_y * 0.5f + 0.5f)) * static_cast<float>(viewport_height);
    out.depth = -view.z; // camera looks down -Z in view space; see ProjectedVertex doc
    return out;
}

/// @brief Collects the unique undirected edges implied by every face's
///        consecutive vertex pairs (including the closing edge).
std::vector<ProjectedEdge> collect_unique_edges(const EditorMesh& mesh) {
    std::set<std::pair<std::size_t, std::size_t>> seen;
    std::vector<ProjectedEdge> edges;

    for (const auto& face : mesh.faces) {
        const std::size_t count = face.vertex_indices.size();
        for (std::size_t i = 0; i < count; ++i) {
            std::size_t a = face.vertex_indices[i];
            std::size_t b = face.vertex_indices[(i + 1) % count];
            if (a > b) {
                std::swap(a, b);
            }
            if (seen.insert({a, b}).second) {
                edges.push_back(ProjectedEdge{a, b});
            }
        }
    }
    return edges;
}

} // namespace

ProjectedMesh project_mesh(const EditorMesh& mesh, const CameraView& camera) {
    ProjectedMesh result;

    const Mat4 vp = view_projection(camera);
    result.vertices.reserve(mesh.vertices.size());
    for (const auto& v : mesh.vertices) {
        result.vertices.push_back(project_point({v.x, v.y, v.z}, vp, camera.view_matrix,
                                                  camera.viewport_width, camera.viewport_height));
    }

    result.edges = collect_unique_edges(mesh);

    result.polygons.reserve(mesh.faces.size());
    for (const auto& face : mesh.faces) {
        ProjectedPolygon polygon;
        polygon.vertex_indices = face.vertex_indices;
        polygon.front_facing = is_face_front_facing(mesh, face, camera);
        result.polygons.push_back(std::move(polygon));
    }

    return result;
}

bool is_face_front_facing(const EditorMesh& mesh, const Face& face, const CameraView& camera) {
    const Vec3 model_normal = internal::newell_normal(mesh, face.vertex_indices);
    const Vec3 view_normal = internal::transform_direction(camera.view_matrix, model_normal);
    // Degenerate face (zero-length normal): conservatively not front-facing
    // rather than dividing by zero to normalize an undefined direction.
    return view_normal.z > 0.0f;
}

} // namespace editor_core

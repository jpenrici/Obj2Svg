#include "editor_core/geometry/triangulator.hpp"

#include <cmath>

#include "geometry_internal.hpp"

namespace editor_core {

namespace {

using internal::Vec3;

constexpr std::string_view kCtx = "triangulator::triangulate";
constexpr float kNormalEpsilon = 1e-7f;  // below this, a face's Newell normal is "zero-length"
constexpr float kAreaEpsilon   = 1e-9f;  // below this, a 2D ear candidate's signed area is "zero"

struct Point2 { float x, y; };

/// @brief Signed area x2 of the triangle (o, a, b) — positive when (o,a,b)
///        winds counter-clockwise.
float cross2(const Point2& o, const Point2& a, const Point2& b) {
    return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
}

/// @brief Strict point-in-triangle test (boundary points are NOT inside),
///        assuming (a, b, c) is wound counter-clockwise — true for every
///        candidate ear here, since the polygon is normalized to CCW
///        before ear clipping starts (see project_to_dominant_plane).
bool point_strictly_inside(const Point2& p, const Point2& a, const Point2& b, const Point2& c) {
    const float d1 = cross2(a, b, p);
    const float d2 = cross2(b, c, p);
    const float d3 = cross2(c, a, p);
    return d1 > kAreaEpsilon && d2 > kAreaEpsilon && d3 > kAreaEpsilon;
}

/// @brief Projects @c vertex_indices (indices into @c mesh.vertices) onto
///        the 2D plane obtained by dropping @c normal's dominant axis,
///        using the standard sign convention that guarantees a
///        counter-clockwise-wound 3D polygon (w.r.t. @c normal, right-hand
///        rule) projects to a counter-clockwise 2D polygon — no separate
///        signed-area check/reversal needed afterward.
std::vector<Point2> project_to_dominant_plane(const EditorMesh& mesh,
                                               const std::vector<std::size_t>& vertex_indices,
                                               const Vec3& normal) {
    const float ax = std::fabs(normal.x);
    const float ay = std::fabs(normal.y);
    const float az = std::fabs(normal.z);

    std::vector<Point2> points;
    points.reserve(vertex_indices.size());

    for (const auto idx : vertex_indices) {
        const Vertex& v = mesh.vertices[idx];
        if (ax >= ay && ax >= az) {
            points.push_back(normal.x >= 0.0f ? Point2{v.y, v.z} : Point2{v.z, v.y});
        } else if (ay >= ax && ay >= az) {
            points.push_back(normal.y >= 0.0f ? Point2{v.z, v.x} : Point2{v.x, v.z});
        } else {
            points.push_back(normal.z >= 0.0f ? Point2{v.x, v.y} : Point2{v.y, v.x});
        }
    }
    return points;
}

/// @brief Triangulates a single face via ear clipping. @c vertex_indices
///        are global indices into @c mesh.vertices (already validated by
///        the caller to have size >= 3).
std::expected<TriangleList, EditorError> triangulate_face(const EditorMesh& mesh,
                                                            const std::vector<std::size_t>& vertex_indices) {
    const Vec3 normal = internal::newell_normal(mesh, vertex_indices);
    if (internal::length(normal) < kNormalEpsilon) {
        return std::unexpected(EditorError{
            ErrorCode::DegenerateFace, std::string(kCtx),
            "face has collinear or duplicated vertices — no well-defined plane",
            std::nullopt});
    }

    const std::size_t n = vertex_indices.size();
    if (n == 3) {
        return TriangleList{Triangle{vertex_indices[0], vertex_indices[1], vertex_indices[2]}};
    }

    const std::vector<Point2> points = project_to_dominant_plane(mesh, vertex_indices, normal);

    // `remaining` holds local indices (0..n-1) into `points`/`vertex_indices`.
    std::vector<std::size_t> remaining(n);
    for (std::size_t i = 0; i < n; ++i) {
        remaining[i] = i;
    }

    TriangleList triangles;
    triangles.reserve(n - 2);

    while (remaining.size() > 3) {
        bool found_ear = false;
        const std::size_t m = remaining.size();

        for (std::size_t i = 0; i < m; ++i) {
            const std::size_t prev_local = remaining[(i + m - 1) % m];
            const std::size_t cur_local  = remaining[i];
            const std::size_t next_local = remaining[(i + 1) % m];

            const Point2& a = points[prev_local];
            const Point2& b = points[cur_local];
            const Point2& c = points[next_local];

            if (cross2(a, b, c) <= kAreaEpsilon) {
                continue; // reflex or degenerate vertex — not a valid ear
            }

            bool any_inside = false;
            for (std::size_t j = 0; j < m; ++j) {
                const std::size_t candidate = remaining[j];
                if (candidate == prev_local || candidate == cur_local || candidate == next_local) {
                    continue;
                }
                if (point_strictly_inside(points[candidate], a, b, c)) {
                    any_inside = true;
                    break;
                }
            }
            if (any_inside) {
                continue;
            }

            triangles.push_back(Triangle{vertex_indices[prev_local], vertex_indices[cur_local],
                                          vertex_indices[next_local]});
            remaining.erase(remaining.begin() + static_cast<std::ptrdiff_t>(i));
            found_ear = true;
            break;
        }

        if (!found_ear) {
            return std::unexpected(EditorError{
                ErrorCode::DegenerateFace, std::string(kCtx),
                "ear clipping found no valid ear — face may be self-intersecting "
                "or otherwise non-simple (out of scope; see triangulator.hpp)",
                std::nullopt});
        }
    }

    triangles.push_back(Triangle{vertex_indices[remaining[0]], vertex_indices[remaining[1]],
                                  vertex_indices[remaining[2]]});
    return triangles;
}

} // namespace

std::expected<TriangleList, EditorError> triangulate(const EditorMesh& mesh) {
    TriangleList result;

    for (const auto& face : mesh.faces) {
        auto face_triangles = triangulate_face(mesh, face.vertex_indices);
        if (!face_triangles) {
            return std::unexpected(face_triangles.error());
        }
        result.insert(result.end(), face_triangles->begin(), face_triangles->end());
    }

    return result;
}

} // namespace editor_core

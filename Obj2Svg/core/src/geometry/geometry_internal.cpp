#include "geometry_internal.hpp"

#include <cmath>

namespace editor_core::internal {

Vec3 operator-(const Vec3& a, const Vec3& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec3 operator+(const Vec3& a, const Vec3& b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }

float dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

Vec3 cross(const Vec3& a, const Vec3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

float length(const Vec3& v) { return std::sqrt(dot(v, v)); }

Vec3 normalize(const Vec3& v) {
    const float len = length(v);
    if (len < 1e-8f) {
        return {0.0f, 0.0f, 0.0f}; // degenerate — see header doc
    }
    return {v.x / len, v.y / len, v.z / len};
}

Vec3 newell_normal(const EditorMesh& mesh, const std::vector<std::size_t>& vertex_indices) {
    Vec3 n{0.0f, 0.0f, 0.0f};
    const std::size_t count = vertex_indices.size();
    for (std::size_t i = 0; i < count; ++i) {
        const Vertex& cur = mesh.vertices[vertex_indices[i]];
        const Vertex& nxt = mesh.vertices[vertex_indices[(i + 1) % count]];
        n.x += (cur.y - nxt.y) * (cur.z + nxt.z);
        n.y += (cur.z - nxt.z) * (cur.x + nxt.x);
        n.z += (cur.x - nxt.x) * (cur.y + nxt.y);
    }
    return n;
}

Vec3 transform_direction(const Mat4& m, const Vec3& dir) {
    return {
        m.m[0] * dir.x + m.m[4] * dir.y + m.m[8]  * dir.z,
        m.m[1] * dir.x + m.m[5] * dir.y + m.m[9]  * dir.z,
        m.m[2] * dir.x + m.m[6] * dir.y + m.m[10] * dir.z,
    };
}

Vec4 transform_point(const Mat4& m, const Vec3& p) {
    return {
        m.m[0] * p.x + m.m[4] * p.y + m.m[8]  * p.z + m.m[12],
        m.m[1] * p.x + m.m[5] * p.y + m.m[9]  * p.z + m.m[13],
        m.m[2] * p.x + m.m[6] * p.y + m.m[10] * p.z + m.m[14],
        m.m[3] * p.x + m.m[7] * p.y + m.m[11] * p.z + m.m[15],
    };
}

} // namespace editor_core::internal

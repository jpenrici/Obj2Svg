// test_triangulator.cpp
// Simple unit tests for editor_core::triangulate() — no external framework.
// Validates the priority behaviors: convex n-gon, concave (non-convex)
// n-gon (the actual point of ear clipping over a naive fan), the two
// DegenerateFace triggers (collinear / duplicated vertices) required by
// spec section 8, and the trivial triangle passthrough.

#include "editor_core/geometry/mesh.hpp"
#include "editor_core/geometry/triangulator.hpp"

#include <cassert>
#include <cmath>
#include <iostream>

using namespace editor_core;

namespace {

bool approx(float a, float b, float eps = 1e-2f) { return std::fabs(a - b) < eps; }

float triangle_area_xy(const EditorMesh& mesh, const Triangle& t) {
    const Vertex& a = mesh.vertices[t.v0];
    const Vertex& b = mesh.vertices[t.v1];
    const Vertex& c = mesh.vertices[t.v2];
    return 0.5f * std::fabs((b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y));
}

// Regular convex pentagon (radius 1, centered at origin, Z=0 plane).
void test_convex_pentagon() {
    EditorMesh mesh;
    constexpr int n = 5;
    for (int i = 0; i < n; ++i) {
        const float angle = static_cast<float>(i) * 2.0f * static_cast<float>(M_PI) / n;
        mesh.vertices.push_back({std::cos(angle), std::sin(angle), 0.0f});
    }
    Face f;
    f.vertex_indices = {0, 1, 2, 3, 4};
    mesh.faces = {f};

    auto result = triangulate(mesh);
    assert(result.has_value());
    assert(result->size() == 3); // n-2 triangles for a 5-gon

    float total_area = 0.0f;
    for (const auto& tri : *result) {
        const float area = triangle_area_xy(mesh, tri);
        assert(area > 1e-4f); // no degenerate triangle
        total_area += area;
    }
    assert(approx(total_area, 2.3776f)); // regular pentagon, radius 1: (5/2)*sin(72deg)
    std::cout << "OK: convex_pentagon\n";
}

// Dart-shaped concave n-gon (has a reflex vertex) — the case a naive
// triangle fan gets wrong but ear clipping handles correctly.
void test_concave_ngon() {
    EditorMesh mesh;
    mesh.vertices = {
        {0.0f, 0.0f, 0.0f}, // 0
        {4.0f, 0.0f, 0.0f}, // 1
        {4.0f, 4.0f, 0.0f}, // 2
        {2.0f, 1.5f, 0.0f}, // 3 <- reflex vertex, creates the concavity
        {0.0f, 4.0f, 0.0f}, // 4
    };
    Face f;
    f.vertex_indices = {0, 1, 2, 3, 4};
    mesh.faces = {f};

    auto result = triangulate(mesh);
    assert(result.has_value());
    assert(result->size() == 3);
    for (const auto& tri : *result) {
        assert(triangle_area_xy(mesh, tri) > 1e-4f);
    }
    std::cout << "OK: concave_ngon\n";
}

// Collinear vertices -> ErrorCode::DegenerateFace (must be signaled, not
// silently turned into a near-zero-area triangle — spec section 8).
void test_collinear_vertices_is_degenerate() {
    EditorMesh mesh;
    mesh.vertices = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f}};
    Face f;
    f.vertex_indices = {0, 1, 2};
    mesh.faces = {f};

    auto result = triangulate(mesh);
    assert(!result.has_value());
    assert(result.error().code == ErrorCode::DegenerateFace);
    std::cout << "OK: collinear_vertices_is_degenerate\n";
}

// Duplicated vertex position -> ErrorCode::DegenerateFace.
void test_duplicated_vertex_is_degenerate() {
    EditorMesh mesh;
    mesh.vertices = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
    Face f;
    f.vertex_indices = {0, 1, 2};
    mesh.faces = {f};

    auto result = triangulate(mesh);
    assert(!result.has_value());
    assert(result.error().code == ErrorCode::DegenerateFace);
    std::cout << "OK: duplicated_vertex_is_degenerate\n";
}

// A face with exactly 3 vertices is emitted as-is (no ear clipping needed).
void test_triangle_passthrough() {
    EditorMesh mesh;
    mesh.vertices = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    Face f;
    f.vertex_indices = {0, 1, 2};
    mesh.faces = {f};

    auto result = triangulate(mesh);
    assert(result.has_value());
    assert(result->size() == 1);
    const auto& tri = (*result)[0];
    assert(tri.v0 == 0 && tri.v1 == 1 && tri.v2 == 2);
    std::cout << "OK: triangle_passthrough\n";
}

} // namespace

int main() {
    test_convex_pentagon();
    test_concave_ngon();
    test_collinear_vertices_is_degenerate();
    test_duplicated_vertex_is_degenerate();
    test_triangle_passthrough();

    std::cout << "\nAll test_triangulator checks passed.\n";
    return 0;
}

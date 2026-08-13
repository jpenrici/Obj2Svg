#include "editor_core/geometry/mesh.hpp"

#include <set>
#include <utility>

namespace editor_core {

std::size_t vertex_count(const EditorMesh& mesh) {
    return mesh.vertices.size();
}

std::size_t face_count(const EditorMesh& mesh) {
    return mesh.faces.size();
}

std::size_t normal_count(const EditorMesh& mesh) {
    return mesh.normals.size();
}

std::vector<Edge> compute_edges(const EditorMesh& mesh) {
    std::set<std::pair<std::size_t, std::size_t>> seen;
    std::vector<Edge> edges;

    for (const auto& face : mesh.faces) {
        const std::size_t count = face.vertex_indices.size();
        for (std::size_t i = 0; i < count; ++i) {
            std::size_t a = face.vertex_indices[i];
            std::size_t b = face.vertex_indices[(i + 1) % count];
            if (a > b) {
                std::swap(a, b);
            }
            if (seen.insert({a, b}).second) {
                edges.push_back(Edge{a, b});
            }
        }
    }
    return edges;
}

} // namespace editor_core

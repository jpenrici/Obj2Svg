#include "editor_core/io/svg_writer.hpp"
#include "editor_core/logger.hpp"

#include <algorithm>
#include <fstream>
#include <limits>
#include <set>
#include <sstream>
#include <utility>

namespace editor_core::io {

namespace {

constexpr std::string_view kCtx = "svg_writer";

std::string escape_xml_attr(std::string_view s) {
  std::string out;
  out.reserve(s.size());
  for (const char c : s) {
    switch (c) {
    case '&':
      out += "&amp;";
      break;
    case '<':
      out += "&lt;";
      break;
    case '>':
      out += "&gt;";
      break;
    case '"':
      out += "&quot;";
      break;
    default:
      out += c;
    }
  }
  return out;
}

struct BoundingBox {
  float min_x, min_y, max_x, max_y;
};

BoundingBox compute_bounding_box(const ProjectedMesh &mesh) {
  if (mesh.vertices.empty()) {
    return {0.0f, 0.0f, 0.0f, 0.0f};
  }
  BoundingBox box{
      std::numeric_limits<float>::max(),
      std::numeric_limits<float>::max(),
      std::numeric_limits<float>::lowest(),
      std::numeric_limits<float>::lowest(),
  };
  for (const auto &v : mesh.vertices) {
    box.min_x = std::min(box.min_x, v.x);
    box.min_y = std::min(box.min_y, v.y);
    box.max_x = std::max(box.max_x, v.x);
    box.max_y = std::max(box.max_y, v.y);
  }
  return box;
}

float mean_depth(const ProjectedMesh &mesh, const ProjectedPolygon &poly) {
  if (poly.vertex_indices.empty()) {
    return 0.0f;
  }
  float sum = 0.0f;
  for (const auto idx : poly.vertex_indices) {
    sum += mesh.vertices[idx].depth;
  }
  return sum / static_cast<float>(poly.vertex_indices.size());
}

using EdgeKey = std::pair<std::size_t, std::size_t>;

EdgeKey normalize_edge(std::size_t a, std::size_t b) {
  return (a < b) ? EdgeKey{a, b} : EdgeKey{b, a};
}

std::set<EdgeKey> visible_edge_keys(const ProjectedMesh &mesh,
                                    bool cull_back_faces) {
  std::set<EdgeKey> keys;
  for (const auto &poly : mesh.polygons) {
    if (cull_back_faces && !poly.front_facing) {
      continue;
    }
    const std::size_t n = poly.vertex_indices.size();
    for (std::size_t i = 0; i < n; ++i) {
      keys.insert(normalize_edge(poly.vertex_indices[i],
                                 poly.vertex_indices[(i + 1) % n]));
    }
  }
  return keys;
}

void write_wireframe(std::ostringstream &out, const ProjectedMesh &mesh,
                     const SvgExportOptions &options) {
  const std::set<EdgeKey> visible =
      visible_edge_keys(mesh, options.cull_back_faces);
  for (const auto &edge : mesh.edges) {
    if (!visible.contains(normalize_edge(edge.a, edge.b))) {
      continue;
    }
    const auto &a = mesh.vertices[edge.a];
    const auto &b = mesh.vertices[edge.b];
    out << "  <line x1=\"" << a.x << "\" y1=\"" << a.y << "\" x2=\"" << b.x
        << "\" y2=\"" << b.y << "\" stroke=\""
        << escape_xml_attr(options.stroke_color) << "\" stroke-width=\""
        << options.stroke_width << "\" />\n";
  }
}

void write_solid(std::ostringstream &out, const ProjectedMesh &mesh,
                 const SvgExportOptions &options) {
  std::vector<std::size_t> visible;
  for (std::size_t i = 0; i < mesh.polygons.size(); ++i) {
    if (!options.cull_back_faces || mesh.polygons[i].front_facing) {
      visible.push_back(i);
    }
  }

  if (options.sort_back_to_front) {
    std::stable_sort(visible.begin(), visible.end(),
                     [&](std::size_t lhs, std::size_t rhs) {
                       return mean_depth(mesh, mesh.polygons[lhs]) >
                              mean_depth(mesh, mesh.polygons[rhs]);
                     });
  }

  for (const auto idx : visible) {
    const auto &poly = mesh.polygons[idx];
    out << "  <polygon id=\"polygon-" << idx << "\" points=\"";
    for (std::size_t k = 0; k < poly.vertex_indices.size(); ++k) {
      const auto &v = mesh.vertices[poly.vertex_indices[k]];
      out << v.x << "," << v.y;
      if (k + 1 < poly.vertex_indices.size()) {
        out << ' ';
      }
    }
    out << "\" fill=\"" << escape_xml_attr(*options.fill_color)
        << "\" stroke=\"" << escape_xml_attr(options.stroke_color)
        << "\" stroke-width=\"" << options.stroke_width << "\" />\n";
  }
}

} // namespace

std::string build_svg(const ProjectedMesh &mesh,
                      const SvgExportOptions &options) {
  const BoundingBox box = compute_bounding_box(mesh);
  const float width = box.max_x - box.min_x;
  const float height = box.max_y - box.min_y;

  std::ostringstream out;
  out << "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"" << box.min_x
      << " " << box.min_y << " " << width << " " << height << "\" width=\""
      << width << "\" height=\"" << height << "\">\n";

  if (options.fill_color.has_value()) {
    write_solid(out, mesh, options);
  } else {
    write_wireframe(out, mesh, options);
  }

  out << "</svg>\n";
  return out.str();
}

std::expected<void, EditorError>
write_svg_file(const EditorContext &ctx, std::string_view filepath,
               const ProjectedMesh &mesh, const SvgExportOptions &options) {
  const std::string svg = build_svg(mesh, options);

  std::ofstream file{std::string(filepath)};
  if (!file.is_open()) {
    return std::unexpected(
        EditorError{ErrorCode::IOError, std::string(kCtx),
                    "could not open file for writing: " + std::string(filepath),
                    std::nullopt});
  }

  file << svg;
  if (!file) {
    return std::unexpected(EditorError{ErrorCode::IOError, std::string(kCtx),
                                       "failed while writing SVG content to: " +
                                           std::string(filepath),
                                       std::nullopt});
  }

  log(ctx, LogLevel::Info, kCtx, "wrote SVG to " + std::string(filepath));
  return {};
}

} // namespace editor_core::io

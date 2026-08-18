#include "editor_core/io/obj_reader.hpp"

#include <charconv>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>

#include "editor_core/logger.hpp"

namespace editor_core::io {

namespace {

constexpr std::string_view kCtx = "obj_reader::load_obj";

std::string_view trim(std::string_view sv) {
  const auto first = sv.find_first_not_of(" \t\r\n");
  if (first == std::string_view::npos) {
    return {};
  }
  const auto last = sv.find_last_not_of(" \t\r\n");
  return sv.substr(first, last - first + 1);
}

std::expected<long, EditorError> parse_long(std::string_view text,
                                            std::size_t line_number) {
  long value = 0;
  const auto result =
      std::from_chars(text.data(), text.data() + text.size(), value);
  if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) {
    return std::unexpected(
        EditorError{ErrorCode::InvalidFace, std::string(kCtx),
                    "malformed face index token: '" + std::string(text) + "'",
                    line_number});
  }
  return value;
}

std::expected<std::size_t, EditorError> resolve_index(long raw,
                                                      std::size_t count,
                                                      std::size_t line_number,
                                                      std::string_view what) {
  if (raw == 0) {
    return std::unexpected(
        EditorError{ErrorCode::InvalidIndex, std::string(kCtx),
                    std::string(what) + " index cannot be 0", line_number});
  }

  const long long absolute_1based =
      (raw > 0) ? raw : static_cast<long long>(count) + raw + 1;

  if (absolute_1based < 1 ||
      static_cast<unsigned long long>(absolute_1based) > count) {
    return std::unexpected(
        EditorError{ErrorCode::InvalidIndex, std::string(kCtx),
                    std::string(what) + " index out of range (resolved to " +
                        std::to_string(absolute_1based) + ", " +
                        std::to_string(count) + " available)",
                    line_number});
  }
  return static_cast<std::size_t>(absolute_1based - 1);
}

struct FaceVertexRef {
  std::size_t vertex_index;
  std::optional<std::size_t> normal_index;
};

std::expected<FaceVertexRef, EditorError>
parse_face_token(std::string_view token, std::size_t vertex_count,
                 std::size_t normal_count, std::size_t line_number) {
  const auto first_slash = token.find('/');
  const std::string_view vertex_part = token.substr(0, first_slash);

  if (vertex_part.empty()) {
    return std::unexpected(EditorError{
        ErrorCode::InvalidFace, std::string(kCtx),
        "face token has no vertex index: '" + std::string(token) + "'",
        line_number});
  }

  auto raw_vertex = parse_long(vertex_part, line_number);
  if (!raw_vertex) {
    return std::unexpected(raw_vertex.error());
  }
  auto vertex_index =
      resolve_index(*raw_vertex, vertex_count, line_number, "vertex");
  if (!vertex_index) {
    return std::unexpected(vertex_index.error());
  }

  if (first_slash == std::string_view::npos) {
    // "v" — no texture, no normal.
    return FaceVertexRef{*vertex_index, std::nullopt};
  }

  const auto second_slash = token.find('/', first_slash + 1);
  if (second_slash == std::string_view::npos) {
    // "v/vt" — texture present (ignored), no normal slot at all.
    return FaceVertexRef{*vertex_index, std::nullopt};
  }

  // "v//vn" or "v/vt/vn" — normal slot is whatever comes after the second
  // slash.
  const std::string_view normal_part = token.substr(second_slash + 1);
  if (normal_part.empty()) {
    return std::unexpected(EditorError{
        ErrorCode::InvalidFace, std::string(kCtx),
        "face token has an empty normal index: '" + std::string(token) + "'",
        line_number});
  }

  auto raw_normal = parse_long(normal_part, line_number);
  if (!raw_normal) {
    return std::unexpected(raw_normal.error());
  }
  auto normal_index =
      resolve_index(*raw_normal, normal_count, line_number, "normal");
  if (!normal_index) {
    return std::unexpected(normal_index.error());
  }

  return FaceVertexRef{*vertex_index, *normal_index};
}

std::expected<Face, EditorError> parse_face_line(std::istringstream &iss,
                                                 std::size_t vertex_count,
                                                 std::size_t normal_count,
                                                 std::size_t line_number) {
  std::vector<FaceVertexRef> refs;
  std::string token;
  while (iss >> token) {
    auto ref = parse_face_token(token, vertex_count, normal_count, line_number);
    if (!ref) {
      return std::unexpected(ref.error());
    }
    refs.push_back(*ref);
  }

  if (refs.size() < 3) {
    return std::unexpected(
        EditorError{ErrorCode::InvalidFace, std::string(kCtx),
                    "face has fewer than 3 vertex references", line_number});
  }

  const bool expects_normals = refs.front().normal_index.has_value();
  for (const auto &ref : refs) {
    if (ref.normal_index.has_value() != expects_normals) {
      return std::unexpected(EditorError{
          ErrorCode::InvalidFace, std::string(kCtx),
          "face mixes vertex references with and without a normal index",
          line_number});
    }
  }

  Face face;
  face.vertex_indices.reserve(refs.size());
  if (expects_normals) {
    face.normal_indices.reserve(refs.size());
  }
  for (const auto &ref : refs) {
    face.vertex_indices.push_back(ref.vertex_index);
    if (expects_normals) {
      face.normal_indices.push_back(*ref.normal_index);
    }
  }
  return face;
}

} // namespace

std::expected<EditorMesh, EditorError> load_obj(const EditorContext &ctx,
                                                std::string_view filepath) {
  std::ifstream file{std::string(filepath)};
  if (!file.is_open()) {
    return std::unexpected(EditorError{
        ErrorCode::FileNotFound, std::string(kCtx),
        "could not open file: " + std::string(filepath), std::nullopt});
  }

  EditorMesh mesh;
  std::string line;
  std::size_t line_number = 0;

  while (std::getline(file, line)) {
    ++line_number;
    const std::string_view trimmed = trim(line);
    if (trimmed.empty() || trimmed.front() == '#') {
      continue;
    }

    std::istringstream iss{std::string(trimmed)};
    std::string directive;
    iss >> directive;

    if (directive == "v") {
      Vertex v{};
      if (!(iss >> v.x >> v.y >> v.z)) {
        return std::unexpected(
            EditorError{ErrorCode::InvalidVertex, std::string(kCtx),
                        "malformed vertex line", line_number});
      }
      mesh.vertices.push_back(v);
    } else if (directive == "vn") {
      Normal n{};
      if (!(iss >> n.x >> n.y >> n.z)) {
        return std::unexpected(
            EditorError{ErrorCode::InvalidOBJ, std::string(kCtx),
                        "malformed normal line", line_number});
      }
      mesh.normals.push_back(n);
    } else if (directive == "f") {
      auto face = parse_face_line(iss, mesh.vertices.size(),
                                  mesh.normals.size(), line_number);
      if (!face) {
        return std::unexpected(face.error());
      }
      mesh.faces.push_back(std::move(*face));
    } else {
      // o / g / s / mtllib / usemtl / vt / any other unrecognized
      // directive — deliberately ignored (see obj_reader.hpp).
      log(ctx, LogLevel::Info, kCtx,
          "ignored directive '" + directive + "' at line " +
              std::to_string(line_number));
    }
  }

  if (mesh.vertices.empty() || mesh.faces.empty()) {
    return std::unexpected(EditorError{
        ErrorCode::EmptyMesh, std::string(kCtx),
        "file parsed successfully but produced no vertices or no faces",
        std::nullopt});
  }

  return mesh;
}

} // namespace editor_core::io

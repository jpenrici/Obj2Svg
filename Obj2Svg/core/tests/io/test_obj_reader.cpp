#include "editor_core/context.hpp"
#include "editor_core/io/obj_reader.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace editor_core;
using namespace editor_core::io;

namespace {

std::string fixture(const char *name) {
  return std::string(FIXTURES_DIR) + "/" + name;
}

// cube.obj (hand-written) — basic happy path.
void test_hand_written_cube() {
  EditorContext ctx{};
  auto result = load_obj(ctx, fixture("cube.obj"));
  assert(result.has_value());
  assert(result->vertices.size() == 8);
  assert(result->faces.size() == 6);
  for (const auto &f : result->faces) {
    assert(f.vertex_indices.size() == 4);
    assert(f.normal_indices.empty()); // no vn in this fixture
    for (auto idx : f.vertex_indices) {
      assert(idx < result->vertices.size());
    }
  }
  std::cout << "OK: hand_written_cube\n";
}

// blender_cube.obj — parser ignores o/g/s/usemtl/mtllib/vt/comments without
// error.
void test_blender_directives_ignored() {
  std::vector<std::string> logged;
  EditorContext ctx{
      .sink = [&](LogLevel, std::string_view, std::string_view msg) {
        logged.emplace_back(msg);
      }};
  auto result = load_obj(ctx, fixture("blender_cube.obj"));
  assert(result.has_value());
  assert(result->vertices.size() == 8);
  assert(result->normals.size() == 3);
  assert(result->faces.size() == 3);
  for (const auto &f : result->faces) {
    assert(f.vertex_indices.size() ==
           f.normal_indices.size()); // v/vt/vn -> normal-aware
  }
  assert(!logged.empty()); // at least one Info event for the ignored directives
  std::cout << "OK: blender_directives_ignored (" << logged.size()
            << " logged)\n";
}

// Negative relative indices resolved correctly against the file state so far.
void test_negative_relative_indices() {
  EditorContext ctx{};
  auto result = load_obj(ctx, fixture("negative_indices.obj"));
  assert(result.has_value());
  assert(result->faces.size() == 1);
  const std::vector<std::size_t> expected{0, 1, 2, 3};
  assert(result->faces[0].vertex_indices == expected);
  std::cout << "OK: negative_relative_indices\n";
}

// Out-of-range index -> ErrorCode::InvalidIndex.
void test_out_of_range_index() {
  EditorContext ctx{};
  auto result = load_obj(ctx, fixture("invalid_index.obj"));
  assert(!result.has_value());
  assert(result.error().code == ErrorCode::InvalidIndex);
  std::cout << "OK: out_of_range_index\n";
}

// Index 0 -> ErrorCode::InvalidIndex.
void test_zero_index() {
  EditorContext ctx{};
  auto result = load_obj(ctx, fixture("zero_index.obj"));
  assert(!result.has_value());
  assert(result.error().code == ErrorCode::InvalidIndex);
  std::cout << "OK: zero_index\n";
}

// File with vertices but no faces -> ErrorCode::EmptyMesh.
void test_empty_mesh() {
  EditorContext ctx{};
  auto result = load_obj(ctx, fixture("no_faces.obj"));
  assert(!result.has_value());
  assert(result.error().code == ErrorCode::EmptyMesh);
  std::cout << "OK: empty_mesh\n";
}

// Face mixing vertex refs with and without a normal index ->
// ErrorCode::InvalidFace (Face invariant from mesh.hpp / decision #2).
void test_partial_normals_invariant() {
  EditorContext ctx{};
  auto result = load_obj(ctx, fixture("partial_normals.obj"));
  assert(!result.has_value());
  assert(result.error().code == ErrorCode::InvalidFace);
  std::cout << "OK: partial_normals_invariant\n";
}

// Nonexistent file -> ErrorCode::FileNotFound.
void test_file_not_found() {
  EditorContext ctx{};
  auto result = load_obj(ctx, fixture("does_not_exist.obj"));
  assert(!result.has_value());
  assert(result.error().code == ErrorCode::FileNotFound);
  std::cout << "OK: file_not_found\n";
}

} // namespace

int main() {
  test_hand_written_cube();
  test_blender_directives_ignored();
  test_negative_relative_indices();
  test_out_of_range_index();
  test_zero_index();
  test_empty_mesh();
  test_partial_normals_invariant();
  test_file_not_found();

  std::cout << "\nAll test_obj_reader checks passed.\n";
  return 0;
}

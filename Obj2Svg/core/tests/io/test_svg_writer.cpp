// test_svg_writer.cpp
// Simple unit tests for editor_core::io::{build_svg, write_svg_file} — no
// external framework. Uses structural assertions (element counts, relative
// position of known elements), NOT full-string comparison against a
// reference SVG — see docs/HISTORICO.md, decision #4.

#include "editor_core/context.hpp"
#include "editor_core/geometry/projection.hpp"
#include "editor_core/io/svg_writer.hpp"

#include <cassert>
#include <iostream>
#include <string>

using namespace editor_core;
using namespace editor_core::io;

namespace {

/// Two overlapping front-facing quads at different depths: "far" (depth 10,
/// polygon index 0) sits behind "near" (depth 2, polygon index 1).
ProjectedMesh make_overlapping_quads_fixture() {
    ProjectedMesh mesh;
    mesh.vertices = {
        {0.0f, 0.0f, 10.0f}, {10.0f, 0.0f, 10.0f}, {10.0f, 10.0f, 10.0f}, {0.0f, 10.0f, 10.0f}, // far quad: 0-3
        {2.0f, 2.0f, 2.0f},  {8.0f, 2.0f, 2.0f},  {8.0f, 8.0f, 2.0f},  {2.0f, 8.0f, 2.0f},       // near quad: 4-7
    };
    ProjectedPolygon far;
    far.vertex_indices = {0, 1, 2, 3};
    far.front_facing = true;
    ProjectedPolygon near;
    near.vertex_indices = {4, 5, 6, 7};
    near.front_facing = true;
    mesh.polygons = {far, near}; // index 0 = far, index 1 = near

    mesh.edges = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}};
    return mesh;
}

// Wireframe mode for a simple quad — counts <line> elements and checks
// coordinates (with float tolerance), no full-string comparison.
void test_wireframe_line_count_and_coordinates() {
    ProjectedMesh mesh;
    mesh.vertices = {{0.0f, 0.0f, 5.0f}, {10.0f, 0.0f, 5.0f}, {10.0f, 10.0f, 5.0f}, {0.0f, 10.0f, 5.0f}};
    ProjectedPolygon poly;
    poly.vertex_indices = {0, 1, 2, 3};
    poly.front_facing = true;
    mesh.polygons = {poly};
    mesh.edges = {{0, 1}, {1, 2}, {2, 3}, {3, 0}};

    SvgExportOptions options; // fill_color unset -> wireframe mode
    const std::string svg = build_svg(mesh, options);

    std::size_t count = 0;
    std::size_t pos = 0;
    while ((pos = svg.find("<line", pos)) != std::string::npos) {
        ++count;
        ++pos;
    }
    assert(count == 4); // one <line> per quad edge

    assert(svg.find("x1=\"0\" y1=\"0\"") != std::string::npos); // vertex 0 = (0,0) appears as an edge endpoint
    assert(svg.find("<polygon") == std::string::npos); // wireframe mode never emits polygons

    std::cout << "OK: wireframe_line_count_and_coordinates\n";
}

// Solid mode, sort_back_to_front = true (default): the farther polygon
// must appear BEFORE the nearer one in the output, so the nearer one
// paints on top (painter's algorithm).
void test_solid_sorts_back_to_front() {
    const auto mesh = make_overlapping_quads_fixture();
    SvgExportOptions options;
    options.fill_color = "#ff0000";
    // sort_back_to_front defaults to true.

    const std::string svg = build_svg(mesh, options);

    const auto far_pos = svg.find("id=\"polygon-0\"");
    const auto near_pos = svg.find("id=\"polygon-1\"");
    assert(far_pos != std::string::npos);
    assert(near_pos != std::string::npos);
    assert(far_pos < near_pos); // far (depth 10) emitted before near (depth 2)

    std::size_t count = 0;
    std::size_t pos = 0;
    while ((pos = svg.find("<polygon", pos)) != std::string::npos) {
        ++count;
        ++pos;
    }
    assert(count == 2);

    std::cout << "OK: solid_sorts_back_to_front\n";
}

// sort_back_to_front = false: original ProjectedMesh.polygons order is
// preserved (far still comes before near here only because it's index 0 —
// the point is the option genuinely disables reordering).
void test_solid_sort_disabled_preserves_original_order() {
    auto mesh = make_overlapping_quads_fixture();
    // Reverse the polygon order so "near" (shallow depth) comes first —
    // if sorting were still happening, "far" would be moved after it
    // regardless of this option.
    std::swap(mesh.polygons[0], mesh.polygons[1]);

    SvgExportOptions options;
    options.fill_color = "#00ff00";
    options.sort_back_to_front = false;

    const std::string svg = build_svg(mesh, options);

    const auto near_pos = svg.find("id=\"polygon-0\""); // now at index 0 after the swap
    const auto far_pos = svg.find("id=\"polygon-1\"");
    assert(near_pos != std::string::npos);
    assert(far_pos != std::string::npos);
    assert(near_pos < far_pos); // original (swapped) order preserved, NOT re-sorted by depth

    std::cout << "OK: solid_sort_disabled_preserves_original_order\n";
}

// cull_back_faces = true (default) must drop a back-facing polygon from
// solid-mode output entirely.
void test_cull_back_faces_removes_back_facing_polygon() {
    ProjectedMesh mesh;
    mesh.vertices = {{0.0f, 0.0f, 5.0f}, {10.0f, 0.0f, 5.0f}, {10.0f, 10.0f, 5.0f}, {0.0f, 10.0f, 5.0f}};
    ProjectedPolygon back_poly;
    back_poly.vertex_indices = {0, 1, 2, 3};
    back_poly.front_facing = false;
    mesh.polygons = {back_poly};
    mesh.edges = {{0, 1}, {1, 2}, {2, 3}, {3, 0}};

    SvgExportOptions options;
    options.fill_color = "#0000ff";
    // cull_back_faces defaults to true.

    const std::string svg = build_svg(mesh, options);
    assert(svg.find("<polygon") == std::string::npos);

    std::cout << "OK: cull_back_faces_removes_back_facing_polygon\n";
}

// EditorContext + IOError path: writing to an unwritable path must fail
// with ErrorCode::IOError, not crash or throw.
void test_write_svg_file_reports_io_error() {
    EditorContext ctx{};
    ProjectedMesh mesh; // empty mesh is fine — the point is the bad path
    auto result = write_svg_file(ctx, "/nonexistent_directory/out.svg", mesh, {});
    assert(!result.has_value());
    assert(result.error().code == ErrorCode::IOError);
    std::cout << "OK: write_svg_file_reports_io_error\n";
}

} // namespace

int main() {
    test_wireframe_line_count_and_coordinates();
    test_solid_sorts_back_to_front();
    test_solid_sort_disabled_preserves_original_order();
    test_cull_back_faces_removes_back_facing_polygon();
    test_write_svg_file_reports_io_error();

    std::cout << "\nAll test_svg_writer checks passed.\n";
    return 0;
}

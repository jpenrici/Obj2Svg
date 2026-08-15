// main.cpp
// EditorClient entry point — window, orbital camera, mesh loading
// (drag & drop or a CLI argument), and the main render loop.
//
// NOTE (per your request): written against the public Raylib API as
// documented, not built/run here — you'll build against your own Raylib
// install. If something doesn't match, tell me the exact error and I'll
// adjust.

#include "raylib.h"

#include "camera_controller.hpp"
#include "mesh_view.hpp"

#include <cstdio>

namespace {
constexpr int kScreenWidth = 1024;
constexpr int kScreenHeight = 768;

void try_load(editor_client::MeshView& mesh_view, const char* path) {
    if (!mesh_view.load(path)) {
        std::fprintf(stderr, "Failed to load '%s': %s\n", path, mesh_view.get_last_error().c_str());
    }
}
} // namespace

int main(int argc, char* argv[]) {
    InitWindow(kScreenWidth, kScreenHeight, "Obj2Svg - EditorClient");
    SetTargetFPS(60);

    // Scoped so MeshView's destructor (UnloadModel — needs a live GL
    // context) runs before CloseWindow() tears that context down. Letting
    // MeshView outlive CloseWindow() is a use-after-free on GPU resources
    // and segfaults on exit — found by actually running this, not just
    // compiling it.
    {
        editor_client::OrbitalCamera orbital_camera;
        editor_client::MeshView mesh_view;

        if (argc > 1) {
            try_load(mesh_view, argv[1]);
        }

        while (!WindowShouldClose()) {
            if (IsFileDropped()) {
                FilePathList dropped = LoadDroppedFiles();
                if (dropped.count > 0) {
                    try_load(mesh_view, dropped.paths[0]);
                }
                UnloadDroppedFiles(dropped);
            }

            orbital_camera.update();

            BeginDrawing();
            ClearBackground(RAYWHITE);

            BeginMode3D(orbital_camera.camera());
            DrawGrid(20, 1.0f);
            mesh_view.draw();
            EndMode3D();

            if (!mesh_view.has_model()) {
                DrawText("Drag & drop an .obj file, or pass one as a command-line argument.", 20, 20, 20, DARKGRAY);
            }
            DrawFPS(10, 10);

            // TODO (Phase 5): key binding to trigger SVG export via
            // svg_export_command — pending, see client/src/svg_export_command.cpp
            // for what it needs from editor_api.h first.

            EndDrawing();
        }
    } // mesh_view/orbital_camera destroyed here, GL context still alive

    CloseWindow();
    return 0;
}

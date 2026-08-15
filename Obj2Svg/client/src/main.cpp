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

// Phase 4 — basic editing speeds.
constexpr float kTranslateSpeed        = 2.0f;   // world units / second (arrow keys)
constexpr float kMeshRotateMouseSpeed  = 0.01f;  // radians per pixel of left-drag delta
constexpr float kMeshScaleWheelSpeed   = 0.1f;   // scale-factor delta per wheel notch
constexpr float kMinScaleFactor        = 0.01f;  // guards against a runaway zero/negative scale

// Matches DrawGrid(20, 1.0f)'s extent (10 units each way from the origin).
constexpr float kAxisLength = 10.0f;

void try_load(editor_client::MeshView& mesh_view, const char* path) {
    if (!mesh_view.load(path)) {
        std::fprintf(stderr, "Failed to load '%s': %s\n", path, mesh_view.get_last_error().c_str());
    }
}

/// Draws world-space X/Y/Z axes through the origin, Blender's convention:
/// X = red, Y = green, Z = blue. Purely a visual reference — orientation
/// aid alongside DrawGrid, no interaction attached.
void draw_world_axes(float length) {
    DrawLine3D(Vector3{-length, 0.0f, 0.0f}, Vector3{length, 0.0f, 0.0f}, RED);
    DrawLine3D(Vector3{0.0f, -length, 0.0f}, Vector3{0.0f, length, 0.0f}, GREEN);
    DrawLine3D(Vector3{0.0f, 0.0f, -length}, Vector3{0.0f, 0.0f, length}, BLUE);
}

/// Reads continuous input for translate/rotate/scale and applies it to
/// @c mesh_view for this frame. No-op if no mesh is loaded.
///
/// Controls: arrow keys + Page Up/Down translate; left-button drag
/// rotates (horizontal delta around Y, vertical delta around X); the
/// scroll wheel scales uniformly. Left button and the wheel are reserved
/// for this — camera orbit/zoom use the right and middle buttons instead
/// (see camera_controller.cpp).
void handle_editing_input(editor_client::MeshView& mesh_view) {
    if (!mesh_view.has_model()) {
        return;
    }
    const float dt = GetFrameTime();

    float dx = 0.0f, dy = 0.0f, dz = 0.0f;
    if (IsKeyDown(KEY_RIGHT))     dx += kTranslateSpeed * dt;
    if (IsKeyDown(KEY_LEFT))      dx -= kTranslateSpeed * dt;
    if (IsKeyDown(KEY_PAGE_UP))   dy += kTranslateSpeed * dt;
    if (IsKeyDown(KEY_PAGE_DOWN)) dy -= kTranslateSpeed * dt;
    if (IsKeyDown(KEY_UP))        dz -= kTranslateSpeed * dt;
    if (IsKeyDown(KEY_DOWN))      dz += kTranslateSpeed * dt;
    if (dx != 0.0f || dy != 0.0f || dz != 0.0f) {
        mesh_view.translate(dx, dy, dz);
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        const Vector2 delta = GetMouseDelta();
        if (delta.x != 0.0f) {
            mesh_view.rotate(0.0f, 1.0f, 0.0f, delta.x * kMeshRotateMouseSpeed);
        }
        if (delta.y != 0.0f) {
            mesh_view.rotate(1.0f, 0.0f, 0.0f, delta.y * kMeshRotateMouseSpeed);
        }
    }

    const float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        const float f = 1.0f + wheel * kMeshScaleWheelSpeed;
        const float clamped = (f > kMinScaleFactor) ? f : kMinScaleFactor;
        mesh_view.scale(clamped, clamped, clamped);
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
            handle_editing_input(mesh_view);

            BeginDrawing();
            ClearBackground(RAYWHITE);

            BeginMode3D(orbital_camera.camera());
            DrawGrid(20, 1.0f);
            draw_world_axes(kAxisLength);
            mesh_view.draw();
            EndMode3D();

            if (!mesh_view.has_model()) {
                DrawText("Drag & drop an .obj file, or pass one as a command-line argument.", 20, 20, 20, DARKGRAY);
            } else {
                DrawText("Arrows/PgUp/PgDn: move | LMB drag: rotate | Wheel: scale | RMB drag: orbit camera | MMB drag: zoom camera",
                         20, 20, 18, DARKGRAY);
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

// EditorClient entry point — window, orbital camera, mesh loading
// (drag & drop or a CLI argument), and the main render loop.

#include "raylib.h"

#include "camera_controller.hpp"
#include "mesh_view.hpp"
#include "svg_export_command.hpp"

#include <cstdio>

namespace {
constexpr int kScreenWidth = 1024;
constexpr int kScreenHeight = 768;

// Basic editing speeds.
constexpr float kTranslateSpeed = 2.0f;
constexpr float kMeshRotateMouseSpeed = 0.01f;
constexpr float kMeshScaleWheelSpeed = 0.1f;
constexpr float kMinScaleFactor = 0.01f;
constexpr float kAxisLength = 10.0f;

constexpr float kExportNotificationSeconds = 1.0f;

struct ExportNotification {
  std::string message;
  Color color = DARKGRAY;
  float remaining_seconds = 0.0f;

  bool active() const { return remaining_seconds > 0.0f; }
};

void try_load(editor_client::MeshView &mesh_view, const char *path) {
  if (!mesh_view.load(path)) {
    std::fprintf(stderr, "Failed to load '%s': %s\n", path,
                 mesh_view.get_last_error().c_str());
  }
}

void draw_world_axes(float length) {
  DrawLine3D(Vector3{-length, 0.0f, 0.0f}, Vector3{length, 0.0f, 0.0f}, RED);
  DrawLine3D(Vector3{0.0f, -length, 0.0f}, Vector3{0.0f, length, 0.0f}, GREEN);
  DrawLine3D(Vector3{0.0f, 0.0f, -length}, Vector3{0.0f, 0.0f, length}, BLUE);
}

void handle_export_input(editor_client::MeshView &mesh_view,
                         const editor_client::OrbitalCamera &orbital_camera,
                         int viewport_width, int viewport_height,
                         ExportNotification &notification) {

  if (!mesh_view.has_model() || notification.active()) {
    return;
  }

  if (IsKeyPressed(KEY_F2)) {
    const bool ok = editor_client::export_svg(
        mesh_view, orbital_camera.camera(), viewport_width, viewport_height,
        "export_wireframe.svg", true);
    notification.message =
        ok ? "Exported wireframe SVG"
           : ("Export failed: " + mesh_view.get_last_error());
    notification.color = ok ? DARKGREEN : MAROON;
    notification.remaining_seconds = kExportNotificationSeconds;
    if (!ok) {
      std::fprintf(stderr, "SVG export failed: %s\n",
                   mesh_view.get_last_error().c_str());
    }
  }

  if (IsKeyPressed(KEY_F3)) {
    const bool ok = editor_client::export_svg(
        mesh_view, orbital_camera.camera(), viewport_width, viewport_height,
        "export_solid.svg", false);
    notification.message =
        ok ? "Exported solid SVG"
           : ("Export failed: " + mesh_view.get_last_error());
    notification.color = ok ? DARKGREEN : MAROON;
    notification.remaining_seconds = kExportNotificationSeconds;
    if (!ok) {
      std::fprintf(stderr, "SVG export failed: %s\n",
                   mesh_view.get_last_error().c_str());
    }
  }
}

void handle_editing_input(editor_client::MeshView &mesh_view) {
  if (!mesh_view.has_model()) {
    return;
  }
  const float dt = GetFrameTime();

  float dx = 0.0f, dy = 0.0f, dz = 0.0f;
  if (IsKeyDown(KEY_RIGHT))
    dx += kTranslateSpeed * dt;
  if (IsKeyDown(KEY_LEFT))
    dx -= kTranslateSpeed * dt;
  if (IsKeyDown(KEY_PAGE_UP))
    dy += kTranslateSpeed * dt;
  if (IsKeyDown(KEY_PAGE_DOWN))
    dy -= kTranslateSpeed * dt;
  if (IsKeyDown(KEY_UP))
    dz -= kTranslateSpeed * dt;
  if (IsKeyDown(KEY_DOWN))
    dz += kTranslateSpeed * dt;
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

void handle_reset_input(editor_client::MeshView &mesh_view) {
  if (mesh_view.has_model() && IsKeyPressed(KEY_R)) {
    mesh_view.reset();
  }
}

} // namespace

int main(int argc, char *argv[]) {
  InitWindow(kScreenWidth, kScreenHeight, "Obj2Svg - EditorClient");
  SetTargetFPS(60);

  {
    editor_client::OrbitalCamera orbital_camera;
    editor_client::MeshView mesh_view;
    ExportNotification export_notification;

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
      handle_reset_input(mesh_view);
      handle_export_input(mesh_view, orbital_camera, kScreenWidth,
                          kScreenHeight, export_notification);
      if (export_notification.remaining_seconds > 0.0f) {
        export_notification.remaining_seconds -= GetFrameTime();
      }

      BeginDrawing();
      {
        ClearBackground(RAYWHITE);

        BeginMode3D(orbital_camera.camera());
        {
          // Intentionally hidden during the flash window
          if (!export_notification.active()) {
            DrawGrid(20, 1.0f);
            draw_world_axes(kAxisLength);
          }
          mesh_view.draw();
        }
        EndMode3D();

        if (!mesh_view.has_model()) {
          DrawText("Drag & drop an .obj file, or pass one as a command-line "
                   "argument.",
                   20, 20, 20, DARKGRAY);
        } else {
          DrawText("Arrows/PgUp/PgDn: move | LMB drag: rotate | Wheel: scale | "
                   "RMB drag: orbit camera | MMB drag: zoom camera",
                   20, 20, 18, DARKGRAY);
          DrawText("F2: export wireframe SVG | F3: export solid SVG", 20, 44,
                   18, DARKGRAY);
          if (export_notification.active()) {
            DrawText(export_notification.message.c_str(), 20,
                     kScreenHeight - 32, 24, export_notification.color);
          }
        }

        DrawFPS(kScreenWidth - 80, kScreenHeight - 20);
      }
      EndDrawing();
    }
  }

  CloseWindow();

  return 0;
}

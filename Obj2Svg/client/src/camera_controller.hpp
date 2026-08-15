#pragma once

// NOTE (per your request): the Raylib usage here is written against the
// public raylib.h API as documented, but not built/run in this session —
// you'll build it against your own Raylib install. If something doesn't
// match your Raylib version, point me at the exact error and I'll adjust.

#include "raylib.h"

namespace editor_client {

/// @brief Orbital camera state and per-frame update logic, wrapping a
///        raylib Camera3D.
///
/// Standard orbit-camera convention: the camera always looks at @c target,
/// at @c distance from it, rotated by @c yaw_ (around world Y) and
/// @c pitch_ (around the local right axis). All mouse/scroll input
/// handling lives in update() so main.cpp only needs to call it once per
/// frame before reading camera().
class OrbitalCamera {
public:
    explicit OrbitalCamera(Vector3 target = {0.0f, 0.0f, 0.0f}, float distance = 6.0f);

    /// Reads mouse input (left-drag to orbit, wheel to zoom) via raylib
    /// and updates yaw/pitch/distance accordingly. Call once per frame,
    /// before using camera().
    void update();

    /// The raylib Camera3D derived from the current orbit state — ready
    /// to pass to BeginMode3D.
    const Camera3D& camera() const { return camera_; }

private:
    void rebuild_camera();

    Vector3 target_;
    float distance_;
    float yaw_;   ///< Radians, around the world Y axis.
    float pitch_; ///< Radians, clamped away from the poles to avoid gimbal flip.
    Camera3D camera_{};
};

} // namespace editor_client

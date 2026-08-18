#pragma once

#include "raylib.h"

namespace editor_client {

class OrbitalCamera {
public:
  explicit OrbitalCamera(Vector3 target = {0.0f, 0.0f, 0.0f},
                         float distance = 6.0f);

  void update();

  const Camera3D &camera() const { return camera_; }

private:
  void rebuild_camera();

  Vector3 target_;
  float distance_;
  float yaw_;   // Radians, around the world Y axis.
  float pitch_; // Radians, clamped away from the poles to avoid gimbal flip.
  Camera3D camera_{};
};

} // namespace editor_client

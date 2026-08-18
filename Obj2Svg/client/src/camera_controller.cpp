#include "camera_controller.hpp"

#include <algorithm>
#include <cmath>

namespace editor_client {

namespace {
constexpr float kOrbitSpeed = 0.005f;
constexpr float kZoomSpeed = 0.05f;
constexpr float kMinDistance = 1.0f;
constexpr float kMaxDistance = 100.0f;
constexpr float kPitchLimit = 1.5f;
} // namespace

OrbitalCamera::OrbitalCamera(Vector3 target, float distance)
    : target_(target), distance_(distance), yaw_(0.0f), pitch_(0.4f) {
  rebuild_camera();
}

void OrbitalCamera::update() {

  if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
    const Vector2 delta = GetMouseDelta();
    yaw_ -= delta.x * kOrbitSpeed;
    pitch_ -= delta.y * kOrbitSpeed;
    pitch_ = std::clamp(pitch_, -kPitchLimit, kPitchLimit);
  }

  if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
    const Vector2 delta = GetMouseDelta();
    distance_ += delta.y * kZoomSpeed;
    distance_ = std::clamp(distance_, kMinDistance, kMaxDistance);
  }

  rebuild_camera();
}

void OrbitalCamera::rebuild_camera() {
  const float cos_pitch = std::cos(pitch_);
  const Vector3 offset{
      distance_ * cos_pitch * std::sin(yaw_),
      distance_ * std::sin(pitch_),
      distance_ * cos_pitch * std::cos(yaw_),
  };

  camera_.position =
      Vector3{target_.x + offset.x, target_.y + offset.y, target_.z + offset.z};
  camera_.target = target_;
  camera_.up = Vector3{0.0f, 1.0f, 0.0f};
  camera_.fovy = 45.0f;
  camera_.projection = CAMERA_PERSPECTIVE;
}

} // namespace editor_client

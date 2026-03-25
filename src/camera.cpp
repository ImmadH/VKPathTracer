#include "camera.h"

glm::vec3 Camera::forward() const
{
  const float yaw = glm::radians(yawDegrees);
  const float pitch = glm::radians(pitchDegrees);

  glm::vec3 direction{};
  direction.x = glm::cos(yaw) * glm::cos(pitch);
  direction.y = glm::sin(pitch);
  direction.z = glm::sin(yaw) * glm::cos(pitch);
  return glm::normalize(direction);
}

glm::vec3 Camera::right() const
{
  return glm::normalize(glm::cross(forward(), worldUp));
}

glm::vec3 Camera::up() const
{
  return glm::normalize(glm::cross(right(), forward()));
}

void Camera::rotate(float deltaYawDegrees, float deltaPitchDegrees)
{
  yawDegrees += deltaYawDegrees;
  pitchDegrees = glm::clamp(pitchDegrees + deltaPitchDegrees, -89.0f, 89.0f);
}

void Camera::moveLocal(const glm::vec3& delta)
{
  position += right() * delta.x;
  position += worldUp * delta.y;
  position += forward() * delta.z;
}

CameraUBO Camera::buildUBO(float aspect, uint32_t frameIndex) const
{
  const glm::vec3 forwardDir = forward();
  const glm::vec3 rightDir = right();
  const glm::vec3 upDir = up();

  CameraUBO ubo{};
  ubo.position = glm::vec4(position, 1.0f);
  ubo.forward = glm::vec4(forwardDir, 0.0f);
  ubo.right = glm::vec4(rightDir, 0.0f);
  ubo.up = glm::vec4(upDir, 0.0f);
  ubo.params = glm::vec4(glm::radians(verticalFovDegrees), aspect, static_cast<float>(frameIndex), 0.0f);
  return ubo;
}

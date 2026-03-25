#pragma once
#include <cstdint>
#include <glm/glm.hpp>

struct CameraUBO
{
    alignas(16) glm::vec4 position;
    alignas(16) glm::vec4 forward;
    alignas(16) glm::vec4 right;
    alignas(16) glm::vec4 up;
    alignas(16) glm::vec4 params; 
};

struct Camera
{
    glm::vec3 position{0.0f, 0.0f, 3.0f};
    glm::vec3 worldUp{0.0f, 1.0f, 0.0f};
    float yawDegrees = -90.0f;
    float pitchDegrees = 0.0f;
    float verticalFovDegrees = 45.0f;

    glm::vec3 forward() const;
    glm::vec3 right() const;
    glm::vec3 up() const;

    void rotate(float deltaYawDegrees, float deltaPitchDegrees);
    void moveLocal(const glm::vec3& delta);
    CameraUBO buildUBO(float aspect, uint32_t frameIndex) const;
};

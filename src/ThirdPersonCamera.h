#pragma once

#include "World.h"

#include <glm/glm.hpp>

#include <algorithm>

struct CameraSettings {
    float distance         = 7.0f;
    float minDistance      = 2.5f;
    float maxDistance      = 16.0f;
    float heightOffset     = 1.4f;
    float mouseSensitivity = 0.12f;
    float followSmoothness = 10.0f;
    float minPitch         = -75.0f;
    float maxPitch         = 30.0f;
    float groundClearance  = 0.5f;
    float fov              = 60.0f;
};

class ThirdPersonCamera {
public:
    explicit ThirdPersonCamera(const CameraSettings& settings = {});

    CameraSettings settings;

    void update(const glm::vec3& characterPosition, const glm::vec2& mouseDelta,
                float scrollDelta, const World& world, float deltaTime);

    void snapTo(const glm::vec3& characterPosition, const World& world);

    void snapBehind(const glm::vec3& characterPosition, float characterFacing, const World& world);

    void frame(const glm::vec3& a, const glm::vec3& b, const World& world, float deltaTime);

    void shake(float strength) { m_shake = std::max(m_shake, strength); }

    void setAngles(float yaw, float pitch);

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio) const;
    const glm::vec3& getPosition() const { return m_position; }
    const glm::vec3& getTarget()   const { return m_target; }
    float getYaw() const { return m_yaw; }

private:
    void computePosition(const World& world, float deltaTime);

    float     m_yaw   = 90.0f;
    float     m_pitch = -20.0f;
    float     m_distance;
    float     m_currentDistance = 0;
    float     m_framingDistance = 0;
    float     m_shake = 0.0f;
    float     m_shakeTime = 0.0f;
    glm::vec3 m_shakeOffset{0.0f};
    glm::vec3 m_target{0.0f};
    glm::vec3 m_position{0.0f};
};

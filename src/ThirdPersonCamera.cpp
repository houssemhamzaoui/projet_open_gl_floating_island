#include "ThirdPersonCamera.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

ThirdPersonCamera::ThirdPersonCamera(const CameraSettings& settings)
    : settings(settings), m_distance(settings.distance)
{
}

void ThirdPersonCamera::setAngles(float yaw, float pitch)
{
    m_yaw   = yaw;
    m_pitch = std::clamp(pitch, settings.minPitch, settings.maxPitch);
}

void ThirdPersonCamera::update(const glm::vec3& characterPosition, const glm::vec2& mouseDelta,
                               float scrollDelta, const World& world, float deltaTime)
{
    m_framingDistance = 0.0f;
    m_yaw   += mouseDelta.x * settings.mouseSensitivity;
    m_pitch -= mouseDelta.y * settings.mouseSensitivity;
    m_pitch = std::clamp(m_pitch, settings.minPitch, settings.maxPitch);

    m_distance = std::clamp(m_distance - scrollDelta, settings.minDistance, settings.maxDistance);

    const glm::vec3 desired = characterPosition + glm::vec3(0.0f, settings.heightOffset, 0.0f);
    const float t = 1.0f - std::exp(-settings.followSmoothness * deltaTime);
    m_target = glm::mix(m_target, desired, t);

    computePosition(world, deltaTime);

    m_shakeTime += deltaTime;
    m_shake *= std::exp(-7.0f * deltaTime);
    m_shakeOffset = glm::vec3(std::sin(m_shakeTime * 53.0f), std::sin(m_shakeTime * 41.0f + 1.0f),
                              std::sin(m_shakeTime * 47.0f + 2.0f)) * m_shake;
}

void ThirdPersonCamera::frame(const glm::vec3& a, const glm::vec3& b, const World& world, float deltaTime)
{
    const float t = 1.0f - std::exp(-2.5f * deltaTime);
    const glm::vec3 middle = (a + b) * 0.5f + glm::vec3(0.0f, settings.heightOffset * 0.85f, 0.0f);
    m_target = glm::mix(m_target, middle, t);

    glm::vec2 line(b.x - a.x, b.z - a.z);
    if (glm::length(line) < 1e-3f)
        line = glm::vec2(1.0f, 0.0f);
    line = glm::normalize(line);
    glm::vec2 look(-line.y, line.x);
    const float yawNow = glm::radians(m_yaw);
    if (glm::dot(look, glm::vec2(std::cos(yawNow), std::sin(yawNow))) < 0.0f)
        look = -look;
    const float wantedYaw = glm::degrees(std::atan2(look.y, look.x)) - 25.0f;
    float difference = std::fmod(wantedYaw - m_yaw + 540.0f, 360.0f) - 180.0f;
    m_yaw += difference * t;
    m_pitch += (-10.0f - m_pitch) * t;

    const float wantedDistance = glm::clamp(glm::length(b - a) * 1.3f + 2.5f, 4.0f, 8.0f);
    m_framingDistance = m_framingDistance <= 0.0f ? m_distance : m_framingDistance;
    m_framingDistance += (wantedDistance - m_framingDistance) * t;
    const float saved = m_distance;
    m_distance = m_framingDistance;
    computePosition(world, deltaTime);
    m_distance = saved;
}

void ThirdPersonCamera::snapTo(const glm::vec3& characterPosition, const World& world)
{
    m_target = characterPosition + glm::vec3(0.0f, settings.heightOffset, 0.0f);
    computePosition(world, 0.0f);
}

void ThirdPersonCamera::snapBehind(const glm::vec3& characterPosition, float characterFacing,
                                   const World& world)
{
    const float yaw = glm::degrees(std::atan2(std::cos(characterFacing), std::sin(characterFacing)));
    setAngles(yaw, -15.0f);
    snapTo(characterPosition, world);
}

void ThirdPersonCamera::computePosition(const World& world, float deltaTime)
{
    const float yaw   = glm::radians(m_yaw);
    const float pitch = glm::radians(m_pitch);
    const glm::vec3 lookDirection(std::cos(yaw) * std::cos(pitch),
                                  std::sin(pitch),
                                  std::sin(yaw) * std::cos(pitch));

    float allowed = m_distance;
    if (world.needsSolidCheck(m_target)) {
        for (float t = 0.3f; t <= m_distance; t += 0.2f) {
            if (world.isSolid(m_target - lookDirection * t)) {
                allowed = std::max(0.5f, t - 0.35f);
                break;
            }
        }
    }
    if (deltaTime <= 0.0f || allowed < m_currentDistance)
        m_currentDistance = allowed;
    else
        m_currentDistance += (allowed - m_currentDistance) * (1.0f - std::exp(-3.0f * deltaTime));

    m_position = m_target - lookDirection * m_currentDistance;

    const GroundHit ground = world.groundBelow(glm::vec3(m_position.x, m_target.y, m_position.z), 0.5f);
    if (ground.found)
        m_position.y = std::max(m_position.y, ground.height + settings.groundClearance);
}

glm::mat4 ThirdPersonCamera::getViewMatrix() const
{
    return glm::lookAt(m_position + m_shakeOffset, m_target + m_shakeOffset, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 ThirdPersonCamera::getProjectionMatrix(float aspectRatio) const
{
    return glm::perspective(glm::radians(settings.fov), aspectRatio, 0.1f, 500.0f);
}

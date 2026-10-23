#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

static const glm::vec3 WORLD_UP(0.0f, 1.0f, 0.0f);

Camera::Camera(const glm::vec3& position, float yaw, float pitch)
    : m_position(position), m_yaw(yaw), m_pitch(pitch)
{
    updateVectors();
}

void Camera::move(CameraMove direction, float deltaTime)
{
    const float distance = movementSpeed * deltaTime;
    switch (direction) {
        case CameraMove::Forward:  m_position += m_front  * distance; break;
        case CameraMove::Backward: m_position -= m_front  * distance; break;
        case CameraMove::Right:    m_position += m_right  * distance; break;
        case CameraMove::Left:     m_position -= m_right  * distance; break;
        case CameraMove::Up:       m_position += WORLD_UP * distance; break;
        case CameraMove::Down:     m_position -= WORLD_UP * distance; break;
    }
}

void Camera::rotate(float mouseDeltaX, float mouseDeltaY)
{
    m_yaw   += mouseDeltaX * mouseSensitivity;
    m_pitch -= mouseDeltaY * mouseSensitivity;

    m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
    updateVectors();
}

void Camera::zoom(float scrollDelta)
{
    fov = std::clamp(fov - scrollDelta * 2.0f, 20.0f, 90.0f);
}

void Camera::lookAt(const glm::vec3& target)
{
    const glm::vec3 direction = glm::normalize(target - m_position);
    m_pitch = glm::degrees(std::asin(direction.y));
    m_yaw   = glm::degrees(std::atan2(direction.z, direction.x));
    m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
    updateVectors();
}

glm::mat4 Camera::getViewMatrix() const
{
    return glm::lookAt(m_position, m_position + m_front, m_up);
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio) const
{
    return glm::perspective(glm::radians(fov), aspectRatio, 0.1f, 500.0f);
}

void Camera::updateVectors()
{
    const float yaw   = glm::radians(m_yaw);
    const float pitch = glm::radians(m_pitch);
    m_front = glm::normalize(glm::vec3(std::cos(yaw) * std::cos(pitch),
                                       std::sin(pitch),
                                       std::sin(yaw) * std::cos(pitch)));
    m_right = glm::normalize(glm::cross(m_front, WORLD_UP));
    m_up    = glm::normalize(glm::cross(m_right, m_front));
}

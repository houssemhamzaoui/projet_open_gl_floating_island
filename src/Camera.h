#pragma once

#include <glm/glm.hpp>

enum class CameraMove { Forward, Backward, Left, Right, Up, Down };

class Camera {
public:
    explicit Camera(const glm::vec3& position = {0.0f, 0.0f, 3.0f},
                    float yaw = -90.0f, float pitch = 0.0f);

    void move(CameraMove direction, float deltaTime);

    void rotate(float mouseDeltaX, float mouseDeltaY);

    void zoom(float scrollDelta);

    void lookAt(const glm::vec3& target);

    void setPosition(const glm::vec3& position) { m_position = position; }

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio) const;
    const glm::vec3& getPosition() const { return m_position; }

    float movementSpeed    = 5.0f;
    float mouseSensitivity = 0.1f;
    float fov              = 60.0f;

private:
    void updateVectors();

    glm::vec3 m_position;
    glm::vec3 m_front{0.0f, 0.0f, -1.0f};
    glm::vec3 m_right{1.0f, 0.0f, 0.0f};
    glm::vec3 m_up   {0.0f, 1.0f, 0.0f};
    float m_yaw;
    float m_pitch;
};

#pragma once

#include "Shader.h"

#include <glm/glm.hpp>

class ScreenFade {
public:
    ScreenFade();
    ~ScreenFade();

    ScreenFade(const ScreenFade&) = delete;
    ScreenFade& operator=(const ScreenFade&) = delete;

    void start(const glm::vec3& color, float duration);

    bool update(float deltaTime);

    bool isActive() const { return m_time >= 0.0f; }

    void draw() const;

private:
    float alpha() const;

    Shader    m_shader;
    GLuint    m_emptyVao = 0;
    glm::vec3 m_color{1.0f};
    float     m_duration = 0.5f;
    float     m_time = -1.0f;
};

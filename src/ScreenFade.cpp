#include "ScreenFade.h"

ScreenFade::ScreenFade()
    : m_shader("shaders/fade.vert", "shaders/fade.frag")
{
    glGenVertexArrays(1, &m_emptyVao);
}

ScreenFade::~ScreenFade()
{
    glDeleteVertexArrays(1, &m_emptyVao);
}

void ScreenFade::start(const glm::vec3& color, float duration)
{
    m_color = color;
    m_duration = duration;
    m_time = 0.0f;
}

bool ScreenFade::update(float deltaTime)
{
    if (!isActive())
        return false;

    const float half = m_duration * 0.5f;
    const float before = m_time;
    m_time += deltaTime;

    const bool justCovered = before < half && m_time >= half;

    if (m_time >= m_duration)
        m_time = -1.0f;
    return justCovered;
}

float ScreenFade::alpha() const
{
    if (!isActive())
        return 0.0f;
    const float half = m_duration * 0.5f;
    const float linear = m_time < half ? m_time / half : 1.0f - (m_time - half) / half;
    return glm::smoothstep(0.0f, 1.0f, linear);
}

void ScreenFade::draw() const
{
    if (!isActive())
        return;

    m_shader.use();
    m_shader.setVec3("uColor", m_color);
    m_shader.setFloat("uAlpha", alpha());

    GLint polygonMode[2];
    glGetIntegerv(GL_POLYGON_MODE, polygonMode);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(m_emptyVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, polygonMode[0]);
}

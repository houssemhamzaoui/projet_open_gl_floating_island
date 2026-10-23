#include "ParticleSystem.h"

#include <cstddef>

ParticleSystem::ParticleSystem(int capacity, ParticleShape shape)
    : m_particles(capacity), m_shape(shape)
{
    m_instances.reserve(capacity);

    const float corners[8] = {-1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f};
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_quadBuffer);
    glGenBuffers(1, &m_instanceBuffer);
    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_quadBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(corners), corners, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, m_instanceBuffer);
    glBufferData(GL_ARRAY_BUFFER, capacity * sizeof(Instance), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Instance), reinterpret_cast<void*>(offsetof(Instance, positionSize)));
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Instance), reinterpret_cast<void*>(offsetof(Instance, params)));
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);
    glBindVertexArray(0);
}

ParticleSystem::~ParticleSystem()
{
    glDeleteBuffers(1, &m_instanceBuffer);
    glDeleteBuffers(1, &m_quadBuffer);
    glDeleteVertexArrays(1, &m_vao);
}

int ParticleSystem::draw()
{
    m_instances.clear();
    const float shape = static_cast<float>(m_shape);
    for (const Particle& p : m_particles)
        if (p.active && p.alpha > 0.01f)
            m_instances.push_back({glm::vec4(p.position, p.size), glm::vec4(p.rotation, p.stretch, p.alpha, shape)});
    if (m_instances.empty())
        return 0;

    glBindBuffer(GL_ARRAY_BUFFER, m_instanceBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_instances.size() * sizeof(Instance), m_instances.data());
    glBindVertexArray(m_vao);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, static_cast<GLsizei>(m_instances.size()));
    glBindVertexArray(0);
    return static_cast<int>(m_instances.size());
}

#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <vector>

enum class ParticleShape { Streak = 0, Flake = 1, Petal = 2, Ring = 3 };

struct Particle {
    glm::vec3 position{0.0f};
    glm::vec3 velocity{0.0f};
    float size = 0.1f;
    float stretch = 1.0f;
    float rotation = 0.0f;
    float spin = 0.0f;
    float age = 0.0f, life = 1.0f;
    float seed = 0.0f;
    float alpha = 1.0f;
    bool  active = true;
};

class ParticleSystem {
public:
    ParticleSystem(int capacity, ParticleShape shape);
    ~ParticleSystem();

    ParticleSystem(const ParticleSystem&) = delete;
    ParticleSystem& operator=(const ParticleSystem&) = delete;

    std::vector<Particle>& particles() { return m_particles; }
    const std::vector<Particle>& particles() const { return m_particles; }
    ParticleShape shape() const { return m_shape; }

    int draw();

private:
    struct Instance {
        glm::vec4 positionSize;
        glm::vec4 params;
    };

    std::vector<Particle> m_particles;
    std::vector<Instance> m_instances;
    ParticleShape m_shape;
    GLuint m_vao = 0, m_quadBuffer = 0, m_instanceBuffer = 0;
};

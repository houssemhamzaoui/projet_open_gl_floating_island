#pragma once

#include "InstancedBatch.h"
#include "PointLight.h"
#include "Shader.h"
#include "World.h"

#include <glm/glm.hpp>

#include <memory>
#include <vector>

struct NightSettings {
    int   firefliesMeadow  = 140;
    int   firefliesSakura  = 90;
    float fireflyWander    = 1.6f;
    float fireflyDrift     = 0.12f;
    int   fireflyLights    = 6;
    float fireflyLightRadius = 2.2f;
    float fireflyLightIntensity = 0.3f;
    glm::vec3 fireflyColor {0.80f, 1.00f, 0.45f};

    glm::vec3 lanternColor {1.00f, 0.72f, 0.38f};
    float lanternIntensity = 0.85f;
    float lanternRadius    = 6.5f;
};

class NightLife {
public:
    explicit NightLife(World& world, const NightSettings& settings = {});

    NightSettings settings;

    void generate(World& world);

    void update(float darkness, float time, float fireflyAllowed = 1.0f);

    void appendLights(std::vector<PointLight>& lights, const glm::vec3& player) const;
    void drawLanterns() const;
    void drawGlow(const Shader& gemShader) const;

private:
    struct Firefly { glm::vec3 home; float seed; float brightness = 0.0f; glm::vec3 position{0.0f}; };

    std::vector<Firefly>   m_fireflies;
    std::vector<glm::vec3> m_lanterns;
    float m_on = 0.0f;
    float m_fireflyAmount = 0.0f;
    float m_time = 0.0f;

    std::unique_ptr<InstancedBatch> m_lanternFrames;
    std::unique_ptr<InstancedBatch> m_lanternGlass;
    std::unique_ptr<InstancedBatch> m_fireflyBatch;
};

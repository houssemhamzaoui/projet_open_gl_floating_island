#pragma once

#include <glm/glm.hpp>

#include <vector>

struct PointLight {
    glm::vec3 position{0.0f};
    glm::vec3 color{1.0f};
    float     intensity = 1.0f;
    float     radius = 7.5f;
    bool      pulses = false;
    float     phase = 0.0f;
};

struct PointLightSettings {
    float defaultRadius    = 7.0f;
    float defaultIntensity = 0.9f;
    float pulseSpeed       = 1.6f;
    float pulseAmount      = 0.35f;
    int   maxActive        = 16;
};

struct CaveLightingInfo {
    std::vector<glm::vec3> path;
    glm::vec4 bounds{0.0f};
    glm::vec3 chamber{0.0f};
    glm::vec2 chamberSize{0.0f};
    glm::vec2 fade{0.0f, 1.0f};
    glm::vec3 ambient{0.0f};
    glm::vec3 fogColor{0.0f};
    float     fogDensity = 0.0f;
};

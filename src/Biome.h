#pragma once

#include <glm/glm.hpp>

#include <string>

struct BiomePalette {
    glm::vec3 shore;
    glm::vec3 groundDark;
    glm::vec3 groundLight;
    glm::vec3 groundHigh;
    glm::vec3 rock;
    glm::vec3 soil;
    glm::vec3 undersideTop;
    glm::vec3 undersideBottom;
};

enum class BiomeKind { Meadow, Crystal, Sakura, Frost, Rocky, SkyRock, Cave };

inline const char* biomeId(BiomeKind kind)
{
    switch (kind) {
    case BiomeKind::Meadow:  return "meadow";
    case BiomeKind::Crystal: return "crystal";
    case BiomeKind::Sakura:  return "sakura";
    case BiomeKind::Frost:   return "frost";
    case BiomeKind::Rocky:   return "rocky";
    case BiomeKind::SkyRock: return "skyrock";
    case BiomeKind::Cave:    return "cave";
    }
    return "";
}

struct BiomeSettings {
    std::string  name;
    BiomeKind    kind = BiomeKind::Meadow;
    BiomePalette colors;
    glm::vec3    portalColor{1.0f};

    float plateauHeight = 2.0f;
    float bumpHeight    = 6.0f;
    float noiseScale    = 0.06f;
    int   octaves       = 5;
    float persistence   = 0.5f;
    float lacunarity    = 2.0f;
    float falloffStart  = 0.55f;
    float coastNoise    = 0.2f;
    float ridged        = 0.0f;
    float peakSharpness = 1.0f;
    float rockSlope     = 0.25f;

    float undersideDepthRatio = 0.9f;
    float undersideRough      = 0.3f;

    static BiomeSettings meadow();
    static BiomeSettings crystal();
    static BiomeSettings sakura();
    static BiomeSettings frost();
    static BiomeSettings rocky();
    static BiomeSettings skyRock();
    static BiomeSettings cave();
};

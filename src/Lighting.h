#pragma once

#include "PointLight.h"
#include "Shader.h"
#include "Sky.h"

#include <glm/glm.hpp>

#include <string>
#include <vector>

struct DirectionalLight {
    glm::vec3 direction;
    glm::vec3 color;
    float     intensity;
};

struct Material {
    float specularStrength;
    float shininess;
    float rimStrength;
};

struct LightingSettings {
    glm::vec3 sunColor      {1.00f, 0.95f, 0.85f};
    float     sunIntensity  = 1.0f;

    float     ambientStrength = 0.45f;

    int       toonBands          = 2;
    float     shadowThreshold    = 0.0f;
    float     highlightThreshold = 0.5f;
    float     midBandLevel       = 0.5f;
    float     bandSoftness       = 0.03f;
    glm::vec3 shadowTint         {0.88f, 0.85f, 1.00f};
    float     rimWidth           = 0.35f;
    float     rimSoftness        = 0.05f;

    Material terrainMaterial {0.03f, 16.0f, 0.06f};
    Material objectMaterial  {0.40f, 48.0f, 0.40f};
    Material characterMaterial {0.20f, 32.0f, 0.55f};
    Material stoneMaterial     {0.10f, 16.0f, 0.30f};
};

class Lighting {
public:
    LightingSettings settings;
    bool  toonShading  = true;
    float sunAzimuth   = 120.0f;
    float sunElevation = 35.0f;

    glm::vec3 getDirectionToSun() const;
    DirectionalLight getSun(const SkyState& sky) const;

    void rotateSun(float deltaAzimuth, float deltaElevation);

    void toggleToonShading() { toonShading = !toonShading; }
    void toggleToonBands()   { settings.toonBands = settings.toonBands == 2 ? 3 : 2; }
    std::string describeMode() const;

    void apply(const Shader& shader, const glm::vec3& cameraPosition, const SkyState& sky) const;

    PointLightSettings pointLightSettings;
    void setPointLights(const std::vector<PointLight>& lights, const glm::vec3& viewer, float time);
    int activePointLights() const { return static_cast<int>(m_activeLights.size()); }

    void setCave(const CaveLightingInfo* cave) { m_cave = cave ? *cave : CaveLightingInfo{}; m_hasCave = cave != nullptr; }
    void setViewerCaveDepth(float depth) { m_viewerCaveDepth = depth; }
    static void applyMaterial(const Shader& shader, const Material& material);

private:
    std::vector<PointLight> m_activeLights;
    CaveLightingInfo m_cave;
    bool  m_hasCave = false;
    float m_viewerCaveDepth = 0.0f;
};

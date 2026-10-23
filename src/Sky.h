#pragma once

#include "Mesh.h"
#include "Shader.h"

#include <glm/glm.hpp>

struct SkyColors {
    glm::vec3 zenith;
    glm::vec3 horizon;
};

struct SkySettings {
    SkyColors day    {{0.36f, 0.62f, 0.92f}, {0.80f, 0.90f, 0.98f}};
    SkyColors sunset {{0.33f, 0.36f, 0.68f}, {0.74f, 0.60f, 0.80f}};
    SkyColors night  {{0.06f, 0.08f, 0.20f}, {0.18f, 0.21f, 0.40f}};
    glm::vec3 sunsetGlow    {1.00f, 0.62f, 0.45f};
    glm::vec3 sunsetSunTint {1.00f, 0.70f, 0.50f};

    glm::vec3 moonColor      {0.90f, 0.93f, 1.00f};
    glm::vec3 moonLightColor {0.45f, 0.55f, 0.85f};
    float     moonLight      = 0.45f;
    float     moonSize       = 2.2f;
    glm::vec3 nightAmbientBoost{0.05f, 0.06f, 0.11f};
    float     starDensity    = 0.012f;
    float     starBrightness = 1.0f;

    float sunSize       = 2.5f;
    float glowStrength  = 0.35f;
    float glowSharpness = 24.0f;

    float     cloudCoverage = 0.45f;
    float     cloudSoftness = 0.04f;
    float     cloudScale    = 1.6f;
    float     cloudSpeed    = 0.02f;
    glm::vec3 cloudLight    {1.00f, 1.00f, 1.00f};
    glm::vec3 cloudShadow   {0.74f, 0.77f, 0.92f};

    float cloudSeaScale = 3.0f;

    float     ambientHorizonMix = 0.6f;
    glm::vec3 groundAmbientTint {0.70f, 0.62f, 0.72f};
    float     fogDensity = 0.005f;
};

struct SkyState {
    glm::vec3 zenith{0.0f};
    glm::vec3 horizon{0.0f};
    float     sunsetAmount = 0.0f;
    float     nightAmount  = 0.0f;
    float     darkness     = 0.0f;
    float     starAmount   = 0.0f;
    glm::vec3 directionToLight{0.0f, 1.0f, 0.0f};
    glm::vec3 lightColor{1.0f};

    glm::vec3 sunDiscColor{1.0f};
    glm::vec3 sunLightColor{1.0f};
    glm::vec3 ambientSky{0.0f};
    glm::vec3 ambientGround{0.0f};
    glm::vec3 fogColor{0.0f};
    float     fogDensity = 0.0f;
};

class Sky {
public:
    explicit Sky(const SkySettings& settings = {});

    SkySettings settings;
    bool cloudsEnabled = true;
    bool fogEnabled    = true;

    void update(const glm::vec3& directionToSun, const glm::vec3& sunColor, float dayAngle = 0.0f);
    const SkyState& getState() const { return m_state; }

    void setWeather(float clouds, float rain, float cloudBoost, float lightDim, float fogBoost)
    {
        m_weatherClouds = clouds; m_weatherRain = rain;
        m_cloudBoost = cloudBoost; m_lightDim = lightDim; m_fogBoost = fogBoost;
    }

    void draw(const glm::mat4& view, const glm::mat4& projection, float time) const;

private:
    Shader    m_shader;
    Mesh      m_cube;
    SkyState  m_state;
    glm::vec3 m_directionToSun{0.0f, 1.0f, 0.0f};
    float     m_dayAngle = 0.0f;
    float     m_weatherClouds = 0.0f, m_weatherRain = 0.0f;
    float     m_cloudBoost = 0.0f, m_lightDim = 0.0f, m_fogBoost = 0.0f;
};

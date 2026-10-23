#include "Sky.h"

#include <algorithm>
#include <cmath>

Sky::Sky(const SkySettings& settings)
    : settings(settings),
      m_shader("shaders/sky.vert", "shaders/sky.frag"),
      m_cube(Mesh::createCube())
{
}

void Sky::update(const glm::vec3& directionToSun, const glm::vec3& sunColor, float dayAngle)
{
    m_directionToSun = directionToSun;
    m_dayAngle = dayAngle;
    const SkySettings& s = settings;

    const float height = directionToSun.y;

    const float day   = glm::smoothstep(0.0f, 0.35f, height);
    const float night = 1.0f - glm::smoothstep(-0.25f, 0.0f, height);
    m_state.sunsetAmount = (1.0f - day) * (1.0f - night);
    m_state.nightAmount  = night;

    m_state.zenith  = glm::mix(glm::mix(s.sunset.zenith,  s.day.zenith,  day), s.night.zenith,  night);
    m_state.horizon = glm::mix(glm::mix(s.sunset.horizon, s.day.horizon, day), s.night.horizon, night);

    const auto gray = [&](glm::vec3 c) {
        const float luminance = glm::dot(c, glm::vec3(0.3f, 0.59f, 0.11f));
        return glm::mix(c, glm::vec3(luminance) * glm::vec3(0.92f, 0.95f, 1.05f), 0.75f * m_weatherRain)
             * (1.0f - 0.3f * m_weatherRain);
    };
    m_state.zenith  = gray(m_state.zenith);
    m_state.horizon = gray(m_state.horizon);

    m_state.sunDiscColor  = glm::mix(sunColor, sunColor * s.sunsetSunTint, m_state.sunsetAmount);
    m_state.sunLightColor = m_state.sunDiscColor * (1.0f - night);

    const float sunUp  = glm::smoothstep(0.0f, 0.1f, height);
    const float moonUp = 1.0f - glm::smoothstep(-0.12f, 0.0f, height);
    m_state.directionToLight = height >= 0.0f ? directionToSun : -directionToSun;
    m_state.lightColor = (m_state.sunDiscColor * sunUp + s.moonLightColor * s.moonLight * moonUp)
                       * (1.0f - m_lightDim * m_weatherRain);
    m_state.darkness   = moonUp;
    m_state.starAmount = 1.0f - glm::smoothstep(-0.2f, 0.02f, height);

    m_state.ambientSky    = glm::mix(m_state.zenith, m_state.horizon, s.ambientHorizonMix);
    m_state.ambientSky   += s.nightAmbientBoost * night;
    m_state.ambientGround = m_state.ambientSky * s.groundAmbientTint;

    m_state.fogColor   = m_state.horizon;
    m_state.fogDensity = fogEnabled ? s.fogDensity + m_fogBoost * m_weatherRain : 0.0f;
}

void Sky::draw(const glm::mat4& view, const glm::mat4& projection, float time) const
{
    const SkySettings& s = settings;

    const glm::mat4 skyView = glm::mat4(glm::mat3(view));

    m_shader.use();
    m_shader.setMat4("uView", skyView);
    m_shader.setMat4("uProjection", projection);
    m_shader.setFloat("uTime", time);

    m_shader.setVec3 ("uSunDirection", m_directionToSun);
    m_shader.setVec3 ("uSunDiscColor", m_state.sunDiscColor);
    m_shader.setFloat("uSunCos",       std::cos(glm::radians(s.sunSize)));
    m_shader.setFloat("uSunEdgeCos",   std::cos(glm::radians(s.sunSize * 1.25f)));
    m_shader.setFloat("uGlowStrength", s.glowStrength);
    m_shader.setFloat("uGlowSharpness", s.glowSharpness);

    m_shader.setVec3 ("uZenith",       m_state.zenith);
    m_shader.setVec3 ("uHorizon",      m_state.horizon);
    m_shader.setVec3 ("uSunsetGlow",   s.sunsetGlow);
    m_shader.setFloat("uSunsetAmount", m_state.sunsetAmount);
    m_shader.setFloat("uNightAmount",  m_state.nightAmount);

    m_shader.setVec3 ("uMoonDirection", -m_directionToSun);
    m_shader.setVec3 ("uMoonColor",     s.moonColor);
    m_shader.setFloat("uMoonCos",       std::cos(glm::radians(s.moonSize)));
    m_shader.setFloat("uMoonEdgeCos",   std::cos(glm::radians(s.moonSize * 1.15f)));
    m_shader.setFloat("uStarAmount",    m_state.starAmount * s.starBrightness);
    m_shader.setFloat("uStarDensity",   s.starDensity);
    m_shader.setFloat("uDayAngle",      m_dayAngle);

    m_shader.setInt  ("uCloudsEnabled", cloudsEnabled ? 1 : 0);
    m_shader.setFloat("uCloudCoverage", std::min(0.95f, s.cloudCoverage + m_cloudBoost * m_weatherClouds));
    m_shader.setFloat("uCloudSoftness", s.cloudSoftness);
    m_shader.setFloat("uCloudScale",    s.cloudScale);
    m_shader.setFloat("uCloudSpeed",    s.cloudSpeed);
    m_shader.setVec3 ("uCloudLight",    s.cloudLight * (1.0f - 0.4f * m_weatherRain));
    m_shader.setVec3 ("uCloudShadow",   s.cloudShadow * (1.0f - 0.35f * m_weatherRain));
    m_shader.setFloat("uCloudSeaScale", s.cloudSeaScale);

    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    GLint polygonMode[2];
    glGetIntegerv(GL_POLYGON_MODE, polygonMode);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    m_cube.draw();

    glPolygonMode(GL_FRONT_AND_BACK, polygonMode[0]);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
}

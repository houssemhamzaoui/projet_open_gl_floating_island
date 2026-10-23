#include "Lighting.h"

#include <algorithm>
#include <cstdio>
#include <cmath>

glm::vec3 Lighting::getDirectionToSun() const
{
    const float azimuth   = glm::radians(sunAzimuth);
    const float elevation = glm::radians(sunElevation);
    return glm::normalize(glm::vec3(std::cos(elevation) * std::cos(azimuth),
                                    std::sin(elevation),
                                    std::cos(elevation) * std::sin(azimuth)));
}

DirectionalLight Lighting::getSun(const SkyState& sky) const
{
    return {-sky.directionToLight, sky.lightColor, settings.sunIntensity};
}

void Lighting::rotateSun(float deltaAzimuth, float deltaElevation)
{
    sunAzimuth   = std::fmod(sunAzimuth + deltaAzimuth, 360.0f);
    sunElevation = std::clamp(sunElevation + deltaElevation, -15.0f, 89.0f);
}

std::string Lighting::describeMode() const
{
    if (!toonShading)
        return "realistic (Blinn-Phong)";
    return "toon (" + std::to_string(settings.toonBands) + " bands)";
}

void Lighting::apply(const Shader& shader, const glm::vec3& cameraPosition,
                     const SkyState& sky) const
{
    const DirectionalLight sun = getSun(sky);
    const LightingSettings& s = settings;

    shader.setVec3 ("uSun.direction", sun.direction);
    shader.setVec3 ("uSun.color",     sun.color);
    shader.setFloat("uSun.intensity", sun.intensity);
    shader.setVec3 ("uCameraPos",     cameraPosition);

    shader.setVec3 ("uSkyAmbient",      sky.ambientSky);
    shader.setVec3 ("uGroundAmbient",   sky.ambientGround);
    shader.setFloat("uAmbientStrength", s.ambientStrength);
    const float inCave = m_hasCave ? glm::smoothstep(0.05f, 0.5f, m_viewerCaveDepth) : 0.0f;
    shader.setVec3 ("uFogColor",   glm::mix(sky.fogColor, m_cave.fogColor, inCave));
    shader.setFloat("uFogDensity", glm::mix(sky.fogDensity, m_cave.fogDensity, inCave));

    shader.setInt("uPointLightCount", static_cast<int>(m_activeLights.size()));
    char name[64];
    for (size_t i = 0; i < m_activeLights.size(); ++i) {
        const PointLight& light = m_activeLights[i];
        std::snprintf(name, sizeof(name), "uPointLights[%d].position", static_cast<int>(i));
        shader.setVec3(name, light.position);
        std::snprintf(name, sizeof(name), "uPointLights[%d].color", static_cast<int>(i));
        shader.setVec3(name, light.color);
        std::snprintf(name, sizeof(name), "uPointLights[%d].intensity", static_cast<int>(i));
        shader.setFloat(name, light.intensity);
        std::snprintf(name, sizeof(name), "uPointLights[%d].radius", static_cast<int>(i));
        shader.setFloat(name, light.radius);
    }

    shader.setInt("uCavePathCount", m_hasCave ? static_cast<int>(m_cave.path.size()) : 0);
    if (m_hasCave) {
        for (size_t i = 0; i < m_cave.path.size(); ++i) {
            std::snprintf(name, sizeof(name), "uCavePath[%d]", static_cast<int>(i));
            shader.setVec3(name, m_cave.path[i]);
        }
        glUniform4f(glGetUniformLocation(shader.getId(), "uCaveBounds"),
                    m_cave.bounds.x, m_cave.bounds.y, m_cave.bounds.z, m_cave.bounds.w);
        shader.setVec3("uCaveChamber", m_cave.chamber);
        glUniform2f(glGetUniformLocation(shader.getId(), "uCaveChamberSize"), m_cave.chamberSize.x, m_cave.chamberSize.y);
        glUniform2f(glGetUniformLocation(shader.getId(), "uCaveFade"), m_cave.fade.x, m_cave.fade.y);
        shader.setVec3("uCaveAmbient", m_cave.ambient);
    }

    shader.setInt  ("uToon",               toonShading ? 1 : 0);
    shader.setInt  ("uToonBands",          s.toonBands);
    shader.setFloat("uShadowThreshold",    s.shadowThreshold);
    shader.setFloat("uHighlightThreshold", s.highlightThreshold);
    shader.setFloat("uMidBandLevel",       s.midBandLevel);
    shader.setFloat("uBandSoftness",       s.bandSoftness);
    shader.setVec3 ("uShadowTint",         s.shadowTint);
    shader.setFloat("uRimWidth",           s.rimWidth);
    shader.setFloat("uRimSoftness",        s.rimSoftness);
}

void Lighting::setPointLights(const std::vector<PointLight>& lights, const glm::vec3& viewer, float time)
{
    const PointLightSettings& s = pointLightSettings;

    std::vector<std::pair<float, const PointLight*>> sorted;
    for (const PointLight& light : lights)
        sorted.push_back({glm::length(light.position - viewer) - light.radius, &light});
    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    m_activeLights.clear();
    for (const auto& entry : sorted) {
        if (static_cast<int>(m_activeLights.size()) >= s.maxActive)
            break;
        PointLight light = *entry.second;
        if (light.pulses)
            light.intensity *= 1.0f + s.pulseAmount * std::sin(time * s.pulseSpeed + light.phase);
        m_activeLights.push_back(light);
    }
}

void Lighting::applyMaterial(const Shader& shader, const Material& material)
{
    shader.setFloat("uSpecularStrength", material.specularStrength);
    shader.setFloat("uShininess",        material.shininess);
    shader.setFloat("uRimStrength",      material.rimStrength);
}

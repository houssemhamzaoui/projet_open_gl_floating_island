#include "NightLife.h"
#include "MeshBuilder.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/noise.hpp>

#include <algorithm>
#include <cmath>
#include <random>

namespace {

const glm::mat4 I(1.0f);
constexpr float LAMP_HEIGHT = 1.75f;

Mesh buildLanternFrame()
{
    MeshBuilder b;
    const glm::vec3 wood{0.30f, 0.22f, 0.20f}, metal{0.22f, 0.20f, 0.24f}, gold{0.85f, 0.68f, 0.35f};
    b.addPrism(I, 6, 0.08f, 0.06f, LAMP_HEIGHT - 0.25f, 0.0f, wood);
    b.addPrism(glm::translate(I, glm::vec3(0.0f, LAMP_HEIGHT - 0.27f, 0.0f)), 4, 0.2f, 0.2f, 0.05f, 0.0f, metal);
    for (int i = 0; i < 4; ++i) {
        const float a = glm::quarter_pi<float>() + i * glm::half_pi<float>();
        b.addBox(glm::translate(I, glm::vec3(std::cos(a) * 0.15f, LAMP_HEIGHT, std::sin(a) * 0.15f)),
                 {0.02f, 0.24f, 0.02f}, metal);
    }
    b.addCone(glm::translate(I, glm::vec3(0.0f, LAMP_HEIGHT + 0.24f, 0.0f)), 4, 0.26f, 0.22f, metal);
    b.addSphere(glm::translate(I, glm::vec3(0.0f, LAMP_HEIGHT + 0.5f, 0.0f)) * glm::scale(I, glm::vec3(0.04f)), gold, 6, 4);
    return b.build();
}

}

NightLife::NightLife(World& world, const NightSettings& settings)
    : settings(settings)
{
    m_lanternFrames = std::make_unique<InstancedBatch>(buildLanternFrame());
    MeshBuilder glass;
    glass.addPrism(glm::translate(I, glm::vec3(0.0f, -0.2f, 0.0f)), 4, 0.13f, 0.13f, 0.4f, 0.0f, {1.0f, 0.85f, 0.55f});
    m_lanternGlass = std::make_unique<InstancedBatch>(glass.build());
    MeshBuilder dot;
    dot.addSphere(I, settings.fireflyColor, 6, 4);
    m_fireflyBatch = std::make_unique<InstancedBatch>(dot.build());
    generate(world);
}

void NightLife::generate(World& world)
{
    m_fireflies.clear();
    m_lanterns.clear();
    std::mt19937 rng(world.getSeed() * 5u + 1u);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);

    for (int i = 0; i < static_cast<int>(world.islands().size()); ++i) {
        const Island& island = world.islands()[i];
        const BiomeKind kind = island.settings().biome.kind;
        const int count = kind == BiomeKind::Meadow ? settings.firefliesMeadow
                        : kind == BiomeKind::Sakura ? settings.firefliesSakura : 0;
        for (int k = 0; k < count; ++k) {
            const float r = island.settings().radius * 0.85f * std::sqrt(unit(rng));
            const float a = unit(rng) * glm::two_pi<float>();
            const float x = island.center().x + r * std::cos(a), z = island.center().z + r * std::sin(a);
            if (!island.isOnIsland(x, z))
                continue;
            m_fireflies.push_back({{x, island.heightAt(x, z) + 0.5f + 1.4f * unit(rng), z}, unit(rng) * 100.0f});
        }
    }

    std::vector<Collider> posts;
    auto addLantern = [&](int islandIndex, glm::vec2 xz) {
        const Island& island = world.islands()[islandIndex];
        const glm::vec3 ground(xz.x, island.heightAt(xz.x, xz.y), xz.y);
        m_lanterns.push_back(ground + glm::vec3(0.0f, LAMP_HEIGHT, 0.0f));
        posts.push_back({xz, 0.18f, ground.y - 1.0f, ground.y + 1000.0f, islandIndex});
    };
    std::vector<const PortalSite*> plaza;
    for (const PortalSite& site : world.portalSites())
        if (site.island == World::MAIN_ISLAND)
            plaza.push_back(&site);
    for (const PortalSite* site : plaza) {
        const glm::vec2 p(site->position.x, site->position.z);
        const glm::vec2 forward(std::sin(site->facing), std::cos(site->facing));
        const glm::vec2 right(std::cos(site->facing), -std::sin(site->facing));
        addLantern(World::MAIN_ISLAND, p - forward * 2.6f + right * 3.15f);
    }
    if (!plaza.empty()) {
        const PortalSite* end = plaza.front();
        float best = 1e9f;
        for (const PortalSite* site : plaza) {
            const glm::vec2 right(std::cos(site->facing), -std::sin(site->facing));
            const float side = glm::dot(glm::vec2(site->position.x, site->position.z), right);
            if (side < best) { best = side; end = site; }
        }
        const glm::vec2 p(end->position.x, end->position.z);
        const glm::vec2 forward(std::sin(end->facing), std::cos(end->facing));
        const glm::vec2 right(std::cos(end->facing), -std::sin(end->facing));
        addLantern(World::MAIN_ISLAND, p - forward * 2.6f - right * 3.15f);
    }
    for (const NpcSite& npc : world.npcSites()) {
        const glm::vec2 forward(std::sin(npc.facing), std::cos(npc.facing));
        const glm::vec2 right(std::cos(npc.facing), -std::sin(npc.facing));
        addLantern(npc.island, glm::vec2(npc.position.x, npc.position.z) - right * 1.4f - forward * 0.6f);
    }
    world.addColliders(posts);

    std::vector<glm::mat4> frames;
    for (const glm::vec3& lamp : m_lanterns)
        frames.push_back(glm::translate(I, lamp - glm::vec3(0.0f, LAMP_HEIGHT, 0.0f)));
    m_lanternFrames->setInstances(frames);
}

void NightLife::update(float darkness, float time, float fireflyAllowed)
{
    const float deltaTime = std::clamp(time - m_time, 0.0f, 0.1f);
    m_time = time;
    m_on += (glm::smoothstep(0.2f, 0.7f, darkness) - m_on) * (1.0f - std::exp(-3.0f * deltaTime));
    m_fireflyAmount = glm::smoothstep(0.5f, 1.0f, darkness) * fireflyAllowed;

    for (Firefly& f : m_fireflies) {
        const float t = time * settings.fireflyDrift;
        const glm::vec3 offset(glm::simplex(glm::vec2(f.seed, t)), 0.5f * glm::simplex(glm::vec2(f.seed + 31.0f, t)),
                               glm::simplex(glm::vec2(f.seed + 67.0f, t)));
        f.position = f.home + offset * settings.fireflyWander;
        const float wave = 0.5f + 0.5f * std::sin(time * (1.2f + 0.8f * std::fmod(f.seed, 1.0f)) + f.seed);
        f.brightness = glm::smoothstep(0.35f, 0.75f, wave) * m_fireflyAmount;
    }
}

void NightLife::appendLights(std::vector<PointLight>& lights, const glm::vec3& player) const
{
    if (m_on > 0.02f) {
        for (size_t i = 0; i < m_lanterns.size(); ++i) {
            PointLight light;
            light.position = m_lanterns[i];
            light.color = settings.lanternColor;
            light.intensity = settings.lanternIntensity * m_on
                            * (0.92f + 0.08f * std::sin(m_time * 9.0f + i * 1.7f) * std::sin(m_time * 5.3f + i));
            light.radius = settings.lanternRadius;
            lights.push_back(light);
        }
    }
    if (m_fireflyAmount <= 0.02f)
        return;
    std::vector<std::pair<float, const Firefly*>> near;
    for (const Firefly& f : m_fireflies) {
        const float d = glm::length(f.position - player);
        if (d < 25.0f && f.brightness > 0.2f)
            near.push_back({d, &f});
    }
    std::sort(near.begin(), near.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    for (int i = 0; i < std::min(settings.fireflyLights, static_cast<int>(near.size())); ++i) {
        PointLight light;
        light.position = near[i].second->position;
        light.color = settings.fireflyColor;
        light.intensity = settings.fireflyLightIntensity * near[i].second->brightness;
        light.radius = settings.fireflyLightRadius;
        lights.push_back(light);
    }
}

void NightLife::drawLanterns() const
{
    m_lanternFrames->draw();
}

void NightLife::drawGlow(const Shader& gemShader) const
{
    gemShader.setInt("uRainbow", 0);
    gemShader.setFloat("uAlpha", 1.0f);
    std::vector<glm::mat4> glass;
    for (const glm::vec3& lamp : m_lanterns)
        glass.push_back(glm::translate(I, lamp));
    m_lanternGlass->setInstances(glass, true);
    gemShader.setVec4("uTint", glm::vec4(0.28f, 0.24f, 0.22f, 1.0f - m_on));
    m_lanternGlass->draw();
    gemShader.setVec4("uTint", glm::vec4(0.0f));

    if (m_fireflyAmount <= 0.01f)
        return;
    std::vector<glm::mat4> dots;
    for (const Firefly& f : m_fireflies)
        if (f.brightness > 0.02f)
            dots.push_back(glm::scale(glm::translate(I, f.position), glm::vec3(0.06f * f.brightness + 0.01f)));
    m_fireflyBatch->setInstances(dots, true);
    m_fireflyBatch->draw();
}

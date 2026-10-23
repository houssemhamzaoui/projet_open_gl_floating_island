#include "CaveDecor.h"
#include "Lighting.h"
#include "MeshBuilder.h"
#include "PropMeshes.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <random>

namespace {

Mesh mushroomGroup(const glm::vec3& capColor)
{
    MeshBuilder b;
    const glm::vec3 stem{0.85f, 0.90f, 0.95f};
    const float group[3][4] = {{0.0f, 0.0f, 1.0f, 0.0f}, {0.28f, 0.12f, 0.7f, 1.0f}, {-0.2f, 0.22f, 0.55f, 2.0f}};
    for (const auto& m : group) {
        const glm::mat4 base = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(m[0], 0.0f, m[1])), glm::vec3(m[2]));
        b.addPrism(base, 6, 0.06f, 0.045f, 0.32f, 0.0f, stem);
        b.addSphere(base * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.34f, 0.0f))
                    * glm::scale(glm::mat4(1.0f), glm::vec3(0.22f, 0.12f, 0.22f)), capColor, 10, 6);
    }
    return b.build();
}

Mesh sphereMesh(const glm::vec3& color)
{
    MeshBuilder b;
    b.addSphere(glm::mat4(1.0f), color, 6, 4);
    return b.build();
}

}

CaveDecor::CaveDecor()
{
    m_glowBatches.push_back(std::make_unique<InstancedBatch>(PropMeshes::crystalCluster({0.30f, 0.90f, 1.00f}, {0.45f, 0.70f, 1.00f})));
    m_glowBatches.push_back(std::make_unique<InstancedBatch>(PropMeshes::crystalCluster({0.70f, 0.45f, 1.00f}, {0.55f, 0.50f, 1.00f})));
    m_glowBatches.push_back(std::make_unique<InstancedBatch>(mushroomGroup({0.35f, 1.00f, 0.85f})));
    m_glowBatches.push_back(std::make_unique<InstancedBatch>(mushroomGroup({0.85f, 0.45f, 1.00f})));
    m_sporesCyan   = std::make_unique<InstancedBatch>(sphereMesh({0.55f, 1.00f, 0.95f}));
    m_sporesPurple = std::make_unique<InstancedBatch>(sphereMesh({0.90f, 0.65f, 1.00f}));
    m_pool = Mesh::createSphere(1.0f, {0.05f, 0.09f, 0.18f}, 32, 8);
}

void CaveDecor::generate(const CaveIsland* cave)
{
    m_cave = cave;
    m_spores.clear();
    std::vector<std::vector<glm::mat4>> matrices(m_glowBatches.size());
    if (cave) {
        for (const GlowSpot& spot : cave->glowSpots()) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), spot.position - glm::vec3(0.0f, 0.05f, 0.0f));
            model = glm::rotate(model, spot.yaw, glm::vec3(0.0f, 1.0f, 0.0f));
            matrices[static_cast<int>(spot.kind)].push_back(glm::scale(model, glm::vec3(spot.scale)));
        }
        std::mt19937 rng(7u);
        std::uniform_real_distribution<float> unit(0.0f, 1.0f);
        const glm::vec3 center = cave->chamberFloorCenter();
        const float radius = cave->chamberRadius() - 1.5f;
        for (int i = 0; i < 160; ++i) {
            const float r = radius * std::sqrt(unit(rng));
            const float a = unit(rng) * glm::two_pi<float>();
            m_spores.push_back({center + glm::vec3(std::cos(a) * r, 0.6f + unit(rng) * 5.5f, std::sin(a) * r),
                                unit(rng) * 6.28f, 0.2f + unit(rng) * 0.3f});
        }
    }
    for (size_t k = 0; k < m_glowBatches.size(); ++k)
        m_glowBatches[k]->setInstances(matrices[k]);
}

void CaveDecor::appendLights(std::vector<PointLight>& lights) const
{
    if (!m_cave)
        return;
    for (const GlowSpot& spot : m_cave->glowSpots())
        if (spot.hasLight)
            lights.push_back(spot.light);
}

void CaveDecor::drawGlow(const Shader& gemShader, float time) const
{
    if (!m_cave)
        return;
    gemShader.setInt("uRainbow", 0);
    gemShader.setFloat("uAlpha", 1.0f);
    for (const auto& batch : m_glowBatches)
        batch->draw();

    std::vector<glm::mat4> cyan, purple;
    for (size_t i = 0; i < m_spores.size(); ++i) {
        const Spore& s = m_spores[i];
        const float t = time * s.speed + s.phase;
        const glm::vec3 p = s.base + glm::vec3(std::sin(t) * 0.8f, std::sin(t * 0.7f + 1.3f) * 0.6f, std::cos(t * 0.9f) * 0.8f);
        const float size = 0.035f + 0.025f * (0.5f + 0.5f * std::sin(time * 2.0f + s.phase * 3.0f));
        (i % 3 == 0 ? purple : cyan).push_back(glm::scale(glm::translate(glm::mat4(1.0f), p), glm::vec3(size)));
    }
    m_sporesCyan->setInstances(cyan, true);
    m_sporesCyan->draw();
    m_sporesPurple->setInstances(purple, true);
    m_sporesPurple->draw();
}

void CaveDecor::drawPool(const Shader& objectShader) const
{
    if (!m_cave)
        return;
    Lighting::applyMaterial(objectShader, {0.8f, 90.0f, 0.4f});
    const float r = m_cave->poolRadius();
    objectShader.setMat4("uModel", glm::scale(glm::translate(glm::mat4(1.0f), m_cave->poolCenter()),
                                              glm::vec3(r, 0.04f, r)));
    m_pool.draw();
}

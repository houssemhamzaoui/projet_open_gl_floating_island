#include "PortalSystem.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace {

const glm::vec3 STONE_COLOR{0.80f, 0.78f, 0.85f};
const float PILLAR_RADIUS = 0.45f;

glm::vec3 forwardOf(float facing) { return {std::sin(facing), 0.0f, std::cos(facing)}; }
glm::vec3 rightOf(float facing)   { return {std::cos(facing), 0.0f, -std::sin(facing)}; }

}

PortalSystem::PortalSystem(const World& world, const PortalSettings& settings)
    : settings(settings),
      m_ring(Mesh::createTorus(settings.size, 0.26f, STONE_COLOR, 16, 6)),
      m_pillar(Mesh::createBox({0.28f, 0.45f, 0.32f}, STONE_COLOR)),
      m_swirlQuad(Mesh::createStandingQuad()),
      m_glowQuad(Mesh::createGroundQuad()),
      m_swirlShader("shaders/portal.vert", "shaders/portal.frag")
{
    rebuild(world);
}

void PortalSystem::rebuild(const World& world)
{
    m_portals.clear();
    const std::vector<PortalSite>& sites = world.portalSites();
    for (const PortalSite& site : sites) {
        const int destinationIsland = sites[site.destination].island;
        const BiomeSettings& biome = world.islands()[destinationIsland].settings().biome;
        const bool isIslet = biome.name.find("Islet") != std::string::npos;
        const std::string label = isIslet ? biome.name : biome.name + " Island";

        m_portals.push_back({site, biome.portalColor, label,
                             Mesh::createOctahedron(0.28f, 0.45f, biome.portalColor)});
    }
    m_pendingDestination = -1;
}

glm::mat4 PortalSystem::portalMatrix(const Portal& portal) const
{
    glm::mat4 model = glm::translate(glm::mat4(1.0f), portal.site.position);
    return glm::rotate(model, portal.site.facing, glm::vec3(0.0f, 1.0f, 0.0f));
}

bool PortalSystem::isInsideSwirl(const Portal& portal, const glm::vec3& position) const
{
    const glm::vec3 offset = position - portal.site.position;
    const float lx = glm::dot(offset, rightOf(portal.site.facing));
    const float lz = glm::dot(offset, forwardOf(portal.site.facing));
    return std::abs(lz) < 0.45f
        && std::abs(lx) < settings.size * 0.7f
        && offset.y > -0.5f && offset.y < settings.size * 1.8f;
}

bool PortalSystem::update(Player& player, const World& world, float deltaTime)
{
    m_cooldownLeft = std::max(0.0f, m_cooldownLeft - deltaTime);

    for (const Portal& portal : m_portals) {
        for (float side : {-1.0f, 1.0f}) {
            const glm::vec3 pillar = portal.site.position
                                   + rightOf(portal.site.facing) * side * (settings.size + 0.05f);
            player.pushOutOfCircle(glm::vec2(pillar.x, pillar.z), PILLAR_RADIUS);
        }
    }

    const bool covered = m_fade.update(deltaTime);
    if (m_pendingDestination >= 0 && covered) {
        const Portal& destination = m_portals[m_pendingDestination];
        glm::vec3 arrival = destination.site.position
                          + forwardOf(destination.site.facing) * settings.arrivalDistance;
        const GroundHit ground = world.groundBelow(arrival + glm::vec3(0.0f, 20.0f, 0.0f));
        arrival.y = ground.found ? ground.height : destination.site.position.y;

        player.teleport(arrival, destination.site.facing, destination.site.island);
        m_pendingDestination = -1;
        m_cooldownLeft = settings.cooldown;
        return true;
    }

    if (m_pendingDestination < 0 && m_cooldownLeft <= 0.0f && !m_fade.isActive()) {
        for (const Portal& portal : m_portals) {
            if (isInsideSwirl(portal, player.getPosition())) {
                m_pendingDestination = portal.site.destination;
                m_fade.start(glm::mix(portal.color, glm::vec3(1.0f), 0.4f), settings.fadeDuration);
                break;
            }
        }
    }
    return false;
}

std::string PortalSystem::nearbyDestination(const glm::vec3& position) const
{
    for (const Portal& portal : m_portals) {
        if (glm::length(position - portal.site.position) < settings.labelDistance)
            return portal.label;
    }
    return "";
}

void PortalSystem::drawFrames(const Shader& objectShader, float time) const
{
    const float ringCenterY = settings.size - 0.2f;
    for (const Portal& portal : m_portals) {
        const glm::mat4 base = portalMatrix(portal);

        objectShader.setMat4("uModel", glm::translate(base, glm::vec3(0.0f, ringCenterY, 0.0f)));
        m_ring.draw();

        for (float side : {-1.0f, 1.0f}) {
            objectShader.setMat4("uModel",
                glm::translate(base, glm::vec3(side * (settings.size + 0.05f), 0.35f, 0.0f)));
            m_pillar.draw();
        }

        glm::mat4 keystone = glm::translate(base, glm::vec3(0.0f, ringCenterY + settings.size + 0.55f, 0.0f));
        keystone = glm::rotate(keystone, time * 1.5f, glm::vec3(0.0f, 1.0f, 0.0f));
        objectShader.setMat4("uModel", keystone);
        portal.keystone.draw();
    }
}

void PortalSystem::drawEffects(const glm::mat4& view, const glm::mat4& projection,
                               const Shader& blobShader, float time) const
{
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);
    blobShader.use();
    blobShader.setMat4("uView", view);
    blobShader.setMat4("uProjection", projection);
    const float pulse = 0.85f + 0.15f * std::sin(time * 2.0f);
    for (const Portal& portal : m_portals) {
        glm::mat4 model = glm::translate(portalMatrix(portal), glm::vec3(0.0f, 0.06f, 1.0f));
        model = glm::scale(model, glm::vec3(2.4f, 1.0f, 2.0f));
        blobShader.setMat4("uModel", model);
        blobShader.setVec3("uColor", portal.color);
        blobShader.setFloat("uOpacity", settings.glowStrength * pulse);
        m_glowQuad.draw();
    }
    glDisable(GL_POLYGON_OFFSET_FILL);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_swirlShader.use();
    m_swirlShader.setMat4("uView", view);
    m_swirlShader.setMat4("uProjection", projection);
    m_swirlShader.setFloat("uTime", time);
    m_swirlShader.setFloat("uSpeed", settings.swirlSpeed);
    const float opening = settings.size - 0.2f;
    for (const Portal& portal : m_portals) {
        glm::mat4 model = glm::translate(portalMatrix(portal), glm::vec3(0.0f, settings.size - 0.2f, 0.0f));
        model = glm::scale(model, glm::vec3(opening, opening, 1.0f));
        m_swirlShader.setMat4("uModel", model);
        m_swirlShader.setVec3("uColor", portal.color);
        m_swirlQuad.draw();
    }

    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

#pragma once

#include "Mesh.h"
#include "Player.h"
#include "ScreenFade.h"
#include "Shader.h"
#include "World.h"

#include <glm/glm.hpp>

#include <string>
#include <vector>

struct PortalSettings {
    float size            = 1.7f;
    float swirlSpeed      = 1.2f;
    float fadeDuration    = 0.5f;
    float cooldown        = 1.5f;
    float arrivalDistance = 8.5f;
    float glowStrength    = 0.5f;
    float labelDistance   = 8.0f;
};

struct Portal {
    PortalSite  site;
    glm::vec3   color;
    std::string label;
    Mesh        keystone;
};

class PortalSystem {
public:
    explicit PortalSystem(const World& world, const PortalSettings& settings = {});

    PortalSettings settings;

    void rebuild(const World& world);

    bool update(Player& player, const World& world, float deltaTime);

    bool isTeleporting() const { return m_pendingDestination >= 0; }

    std::string nearbyDestination(const glm::vec3& position) const;

    void drawFrames(const Shader& objectShader, float time) const;
    void drawEffects(const glm::mat4& view, const glm::mat4& projection,
                     const Shader& blobShader, float time) const;
    void drawFade() const { m_fade.draw(); }

private:
    glm::mat4 portalMatrix(const Portal& portal) const;
    bool isInsideSwirl(const Portal& portal, const glm::vec3& position) const;

    std::vector<Portal> m_portals;
    Mesh   m_ring;
    Mesh   m_pillar;
    Mesh   m_swirlQuad;
    Mesh   m_glowQuad;
    Shader m_swirlShader;
    ScreenFade m_fade;

    int   m_pendingDestination = -1;
    float m_cooldownLeft = 0.0f;
};

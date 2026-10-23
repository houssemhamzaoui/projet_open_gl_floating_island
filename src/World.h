#pragma once

#include "CaveIsland.h"
#include "Island.h"
#include "Shader.h"

#include <glm/glm.hpp>

#include <memory>
#include <utility>
#include <vector>

struct GroundHit {
    bool  found  = false;
    float height = 0.0f;
    int   island = -1;
    bool  onProp = false;
    bool  inCave = false;
};

struct Collider {
    glm::vec2 center{0.0f};
    float     radius = 0.5f;
    float     bottom = 0.0f;
    float     top    = 0.0f;
    int       island = 0;
};

struct ClearZone {
    glm::vec2 a{0.0f}, b{0.0f};
    float     radius = 0.0f;
    int       island = 0;

    glm::vec2 closestPoint(const glm::vec2& p) const
    {
        const glm::vec2 ab = b - a;
        const float lengthSquared = glm::dot(ab, ab);
        const float t = lengthSquared > 0.0f ? glm::clamp(glm::dot(p - a, ab) / lengthSquared, 0.0f, 1.0f) : 0.0f;
        return a + ab * t;
    }
    bool contains(const glm::vec2& p, float margin = 0.0f) const
    {
        return glm::length(p - closestPoint(p)) < radius + margin;
    }
};

struct PortalSite {
    int       island = 0;
    glm::vec3 position{0.0f};
    float     facing = 0.0f;
    int       destination = 0;
};

enum class NpcRole { Elder, Gardener, Explorer, Scholar };

struct NpcSite {
    NpcRole   role = NpcRole::Elder;
    int       island = 0;
    glm::vec3 position{0.0f};
    float     facing = 0.0f;
};

class World {
public:
    static constexpr int MAIN_ISLAND = 0;

    explicit World(unsigned seed = 1337);

    void generate(unsigned seed);
    unsigned getSeed() const { return m_seed; }

    GroundHit groundBelow(const glm::vec3& position, float stepUp = 0.0f) const;

    glm::vec3 spawnPoint(int island) const;

    struct Arrival { glm::vec3 position; float facing; int island; };
    Arrival nearestArrival(const glm::vec3& position) const;

    float lowestPoint() const;

    void setColliders(std::vector<Collider> colliders);
    const std::vector<Collider>& colliders() const { return m_colliders; }
    void addColliders(const std::vector<Collider>& more) { m_colliders.insert(m_colliders.end(), more.begin(), more.end()); }

    void setClearZones(std::vector<ClearZone> zones) { m_clearZones = std::move(zones); }
    const std::vector<ClearZone>& clearZones() const { return m_clearZones; }

    void draw(const Shader& terrainShader) const;

    const CaveIsland* cave() const { return m_cave.get(); }
    void setCaveSealed(bool sealed) { if (m_cave) m_cave->setSealed(sealed); }
    bool isInCave(const glm::vec3& position) const { return m_cave && m_cave->isInCave(position); }
    void constrainToCave(glm::vec3& position, glm::vec3& velocity, float radius, float height) const;
    bool isSolid(const glm::vec3& point) const;
    bool needsSolidCheck(const glm::vec3& point) const { return m_cave && m_cave->isNear(point); }

    const std::vector<Island>&     islands()     const { return m_islands; }
    const std::vector<PortalSite>& portalSites() const { return m_portalSites; }
    const std::vector<NpcSite>&    npcSites()    const { return m_npcSites; }

private:
    std::vector<Island>     m_islands;
    std::vector<PortalSite> m_portalSites;
    std::vector<NpcSite>    m_npcSites;
    std::vector<Collider>   m_colliders;
    std::vector<ClearZone>  m_clearZones;
    std::unique_ptr<CaveIsland> m_cave;
    unsigned m_seed = 0;
};

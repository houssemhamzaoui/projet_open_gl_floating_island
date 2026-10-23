#include "World.h"

#include <algorithm>
#include <cmath>

#include <glm/gtc/constants.hpp>

namespace {

std::vector<IslandSettings> worldLayout()
{
    std::vector<IslandSettings> layout;

    IslandSettings main;
    main.position   = {0.0f, 0.0f, 0.0f};
    main.radius     = 40.0f;
    main.resolution = 220;
    main.biome      = BiomeSettings::meadow();
    main.hasPortal  = false;
    layout.push_back(main);

    IslandSettings crystal;
    crystal.position   = {105.0f, -12.0f, 20.0f};
    crystal.radius     = 30.0f;
    crystal.resolution = 150;
    crystal.biome      = BiomeSettings::crystal();
    crystal.landmarkClearing = 12.0f;
    layout.push_back(crystal);

    IslandSettings sakura;
    sakura.position   = {-80.0f, -16.0f, -75.0f};
    sakura.radius     = 30.0f;
    sakura.resolution = 150;
    sakura.biome      = BiomeSettings::sakura();
    sakura.landmarkClearing = 9.0f;
    layout.push_back(sakura);

    IslandSettings frost;
    frost.position   = {-15.0f, -22.0f, 112.0f};
    frost.radius     = 30.0f;
    frost.resolution = 150;
    frost.biome      = BiomeSettings::frost();
    frost.landmarkClearing = 10.0f;
    layout.push_back(frost);

    IslandSettings islet;
    islet.position   = {62.0f, -14.0f, 82.0f};
    islet.radius     = 14.0f;
    islet.resolution = 80;
    islet.biome      = BiomeSettings::rocky();
    islet.landmarkClearing = 7.0f;
    layout.push_back(islet);

    IslandSettings skyRock;
    skyRock.radius     = 4.5f;
    skyRock.resolution = 40;
    skyRock.biome      = BiomeSettings::skyRock();
    skyRock.hasPortal  = false;
    skyRock.isRespawnPoint = false;
    layout.push_back(skyRock);

    IslandSettings cave;
    cave.position   = {60.0f, -8.0f, -95.0f};
    cave.radius     = 30.0f;
    cave.resolution = 160;
    cave.biome      = BiomeSettings::cave();
    cave.portalDistance = 0.8f;
    layout.push_back(cave);

    return layout;
}

float facingOf(const glm::vec2& direction)
{
    return std::atan2(direction.x, direction.y);
}

}

World::World(unsigned seed)
{
    generate(seed);
}

void World::generate(unsigned seed)
{
    m_seed = seed;
    std::vector<IslandSettings> layout = worldLayout();

    std::vector<int> portalIslands;
    for (int i = 1; i < static_cast<int>(layout.size()); ++i)
        if (layout[i].hasPortal)
            portalIslands.push_back(i);
    const int sideCount = static_cast<int>(portalIslands.size());

    struct LocalSite { int island; glm::vec2 local; int destination; };
    std::vector<LocalSite> sites;
    const float arcAngles[] = {40.0f, -40.0f, -20.0f, 20.0f, 0.0f};
    for (int k = 0; k < sideCount; ++k) {
        const float a = glm::radians(arcAngles[k]);
        sites.push_back({MAIN_ISLAND, glm::vec2(std::sin(a), std::cos(a)) * 18.0f, sideCount + k});
    }
    layout[MAIN_ISLAND].flatSpots.push_back({{0.0f, 16.0f}, 20.0f});

    for (int k = 0; k < sideCount; ++k) {
        const int i = portalIslands[k];
        const glm::vec2 toMain = glm::normalize(-glm::vec2(layout[i].position.x, layout[i].position.z));
        const glm::vec2 local = toMain * layout[i].radius * layout[i].portalDistance;
        sites.push_back({i, local, k});
        layout[i].flatSpots.push_back({local, 7.0f});
    }

    CaveSettings caveSettings;
    for (IslandSettings& settings : layout) {
        if (settings.biome.kind == BiomeKind::Cave) {
            const glm::vec2 away = glm::normalize(glm::vec2(settings.position.x, settings.position.z));
            settings.flatSpots.push_back({away * caveSettings.mountainOffset,
                                          caveSettings.mountainRadius / 0.6f + 1.0f});
        }
    }

    for (IslandSettings& settings : layout)
        if (settings.landmarkClearing > 0.0f)
            settings.flatSpots.push_back({{0.0f, 0.0f}, settings.landmarkClearing});

    m_islands.clear();
    m_colliders.clear();
    m_clearZones.clear();
    m_islands.reserve(layout.size());
    for (size_t i = 0; i < layout.size(); ++i) {
        layout[i].seed = seed + static_cast<unsigned>(i) * 7919u;

        if (layout[i].biome.kind == BiomeKind::SkyRock) {
            const Island& frost = m_islands[3];
            const float south = glm::half_pi<float>();
            const float gap = 10.0f;
            const float distance = frost.coastDistance(south) + gap + layout[i].radius;
            layout[i].position = frost.center() + glm::vec3(0.0f, -3.5f, distance);
        }
        m_islands.emplace_back(layout[i]);
    }

    m_cave.reset();
    for (size_t i = 0; i < m_islands.size(); ++i)
        if (m_islands[i].settings().biome.kind == BiomeKind::Cave)
            m_cave = std::make_unique<CaveIsland>(m_islands[i], static_cast<int>(i), caveSettings);
    m_colliders = m_cave ? m_cave->colliders() : std::vector<Collider>{};

    m_portalSites.clear();
    for (const LocalSite& site : sites) {
        const Island& island = m_islands[site.island];
        const glm::vec3 center = island.settings().position;
        const float x = center.x + site.local.x;
        const float z = center.z + site.local.y;
        m_portalSites.push_back({site.island, {x, island.heightAt(x, z), z},
                                 facingOf(-site.local),
                                 site.destination});
    }

    m_npcSites.clear();
    {
        const float a = glm::radians(62.0f);
        const glm::vec2 local = glm::vec2(std::sin(a), std::cos(a)) * 12.0f;
        const Island& main = m_islands[MAIN_ISLAND];
        m_npcSites.push_back({NpcRole::Elder, MAIN_ISLAND,
                              {local.x, main.heightAt(local.x, local.y), local.y},
                              facingOf(glm::vec2(0.0f, 9.0f) - local)});
    }
    for (const PortalSite& site : m_portalSites) {
        NpcRole role;
        switch (m_islands[site.island].settings().biome.kind) {
        case BiomeKind::Sakura:  role = NpcRole::Gardener; break;
        case BiomeKind::Frost:   role = NpcRole::Explorer; break;
        case BiomeKind::Crystal: role = NpcRole::Scholar;  break;
        default: continue;
        }
        const glm::vec2 forward(std::sin(site.facing), std::cos(site.facing));
        const glm::vec2 right(std::cos(site.facing), -std::sin(site.facing));
        const glm::vec2 portal(site.position.x, site.position.z);
        const glm::vec2 spot = portal + forward * 4.0f + right * 6.0f;
        const glm::vec2 arrival = portal + forward * 8.5f;
        const Island& island = m_islands[site.island];
        m_npcSites.push_back({role, site.island, {spot.x, island.heightAt(spot.x, spot.y), spot.y},
                              facingOf(arrival - spot)});
    }
}

GroundHit World::groundBelow(const glm::vec3& position, float stepUp) const
{
    GroundHit best;
    for (size_t i = 0; i < m_islands.size(); ++i) {
        const Island& island = m_islands[i];
        if (!island.isOnIsland(position.x, position.z))
            continue;
        if (m_cave && m_cave->islandIndex() == static_cast<int>(i) && m_cave->hidesTerrain(position.x, position.z))
            continue;
        const float height = island.heightAt(position.x, position.z);
        if (height <= position.y + stepUp && (!best.found || height > best.height)) {
            best.found  = true;
            best.height = height;
            best.island = static_cast<int>(i);
        }
    }

    float caveFloor = 0.0f;
    if (m_cave && m_cave->floorBelow(position, stepUp, caveFloor) && (!best.found || caveFloor > best.height)) {
        best.found  = true;
        best.height = caveFloor;
        best.island = m_cave->islandIndex();
        best.inCave = true;
    }

    for (const Collider& collider : m_colliders) {
        if (collider.top > position.y + stepUp || (best.found && collider.top <= best.height))
            continue;
        if (glm::length(glm::vec2(position.x, position.z) - collider.center) < collider.radius) {
            best.found  = true;
            best.height = collider.top;
            best.island = collider.island;
            best.onProp = true;
        }
    }
    return best;
}

glm::vec3 World::spawnPoint(int island) const
{
    island = std::clamp(island, 0, int(m_islands.size()) - 1);
    const Island& target = m_islands[island];
    glm::vec3 point = target.center();

    if (target.settings().hasPortal) {
        for (const PortalSite& site : m_portalSites) {
            if (site.island == island) {
                point = site.position + glm::vec3(std::sin(site.facing), 0.0f, std::cos(site.facing)) * 8.5f;
                break;
            }
        }
    }
    point.y = target.heightAt(point.x, point.z);
    return point;
}

World::Arrival World::nearestArrival(const glm::vec3& position) const
{
    Arrival best{spawnPoint(MAIN_ISLAND), 0.0f, MAIN_ISLAND};
    float bestDistance = 1e9f;
    for (const PortalSite& site : m_portalSites) {
        const glm::vec3 forward(std::sin(site.facing), 0.0f, std::cos(site.facing));
        glm::vec3 p = site.position + forward * 8.5f;
        p.y = m_islands[site.island].heightAt(p.x, p.z);
        const float d = glm::length(p - position);
        if (d < bestDistance) {
            bestDistance = d;
            best = {p, site.facing, site.island};
        }
    }
    return best;
}

float World::lowestPoint() const
{
    float lowest = 0.0f;
    for (const Island& island : m_islands)
        lowest = std::min(lowest, island.bottomY());
    return lowest;
}

void World::setColliders(std::vector<Collider> colliders)
{
    m_colliders = std::move(colliders);
    if (m_cave)
        m_colliders.insert(m_colliders.end(), m_cave->colliders().begin(), m_cave->colliders().end());
}

void World::constrainToCave(glm::vec3& position, glm::vec3& velocity, float radius, float height) const
{
    if (m_cave)
        m_cave->constrain(position, velocity, radius, height);
}

bool World::isSolid(const glm::vec3& point) const
{
    return m_cave && m_cave->isSolid(point, m_islands[m_cave->islandIndex()]);
}

void World::draw(const Shader& terrainShader) const
{
    for (size_t i = 0; i < m_islands.size(); ++i) {
        const bool hasHole = m_cave && m_cave->islandIndex() == static_cast<int>(i);
        terrainShader.setVec3("uHole", hasHole ? glm::vec3(m_cave->mountainBase().x, m_cave->mountainBase().z,
                                                          m_cave->mountainRadius() - 0.05f)
                                               : glm::vec3(0.0f));
        terrainShader.setFloat("uHoleMinY", hasHole ? m_cave->mountainBase().y - 6.0f : 0.0f);
        terrainShader.setMat4("uModel", m_islands[i].getModelMatrix());
        m_islands[i].draw();
    }
}

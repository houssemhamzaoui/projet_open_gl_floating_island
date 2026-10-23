#include "PropSystem.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace {

constexpr int KIND_COUNT = static_cast<int>(PropKind::Count);
int index(PropKind kind) { return static_cast<int>(kind); }

const Material FOLIAGE {0.05f, 16.0f, 0.25f};
const Material STONE   {0.05f, 16.0f, 0.15f};
const Material CRYSTAL {0.60f, 48.0f, 0.60f};
const Material SMALL   {0.00f,  8.0f, 0.00f};

struct ScatterRule { PropKind kind; int count; float minScale; float maxScale; };

std::vector<ScatterRule> decorationPlan(BiomeKind biome)
{
    switch (biome) {
    case BiomeKind::Meadow:
        return {{PropKind::RoundTree, 45, 0.9f, 1.4f}, {PropKind::Rock, 30, 0.5f, 1.4f},
                {PropKind::GrassMeadow, 900, 0.8f, 1.3f}, {PropKind::FlowerWhite, 110, 0.8f, 1.2f},
                {PropKind::FlowerYellow, 110, 0.8f, 1.2f}, {PropKind::FlowerPink, 70, 0.8f, 1.2f}};
    case BiomeKind::Crystal:
        return {{PropKind::CrystalCluster, 70, 0.7f, 1.6f}, {PropKind::CrystalRock, 30, 0.6f, 1.4f},
                {PropKind::SmallCrystal, 120, 0.3f, 0.6f}, {PropKind::GrassLavender, 350, 0.8f, 1.2f}};
    case BiomeKind::Sakura:
        return {{PropKind::SakuraTree, 40, 0.9f, 1.4f}, {PropKind::Rock, 12, 0.5f, 1.2f},
                {PropKind::PetalPatch, 120, 0.8f, 1.4f}, {PropKind::GrassSakura, 400, 0.8f, 1.2f},
                {PropKind::FlowerPink, 120, 0.8f, 1.2f}, {PropKind::FlowerWhite, 80, 0.8f, 1.2f}};
    case BiomeKind::Frost:
        return {{PropKind::SnowyPine, 60, 0.8f, 1.6f}, {PropKind::IceRock, 35, 0.5f, 1.4f},
                {PropKind::IceCluster, 30, 0.6f, 1.2f}};
    case BiomeKind::Rocky:
        return {{PropKind::MossRock, 14, 0.6f, 1.6f}, {PropKind::RoundTree, 4, 0.8f, 1.1f},
                {PropKind::GrassMoss, 160, 0.8f, 1.2f}, {PropKind::FlowerYellow, 30, 0.8f, 1.1f}};
    case BiomeKind::SkyRock:
        return {{PropKind::GrassMoss, 25, 0.8f, 1.1f}};
    case BiomeKind::Cave:
        return {{PropKind::DeadTree, 14, 0.8f, 1.3f}, {PropKind::DarkRock, 22, 0.6f, 1.6f},
                {PropKind::GrassCave, 260, 0.8f, 1.2f}, {PropKind::FlowerWhite, 20, 0.7f, 1.0f}};
    }
    return {};
}

glm::vec2 xz(const glm::vec3& v) { return {v.x, v.z}; }

float flatness(const Island& island, float x, float z)
{
    const float e = 0.5f;
    const glm::vec3 normal = glm::normalize(glm::vec3(
        island.heightAt(x - e, z) - island.heightAt(x + e, z), 2.0f * e,
        island.heightAt(x, z - e) - island.heightAt(x, z + e)));
    return normal.y;
}

float facingOf(const glm::vec2& direction) { return std::atan2(direction.x, direction.y); }

}

PropSystem::PropSystem(World& world, const PropSettings& settings)
    : settings(settings),
      m_info(KIND_COUNT), m_instances(KIND_COUNT), m_landmarkColliders(KIND_COUNT)
{
    std::vector<Mesh> meshes(KIND_COUNT);
    meshes[index(PropKind::RoundTree)]      = PropMeshes::roundTree();
    meshes[index(PropKind::SakuraTree)]     = PropMeshes::sakuraTree();
    meshes[index(PropKind::SnowyPine)]      = PropMeshes::snowyPine();
    meshes[index(PropKind::Rock)]           = PropMeshes::rock({0.66f, 0.63f, 0.62f}, 1.0f);
    meshes[index(PropKind::IceRock)]        = PropMeshes::rock({0.74f, 0.88f, 0.98f}, 2.0f);
    meshes[index(PropKind::CrystalRock)]    = PropMeshes::rock({0.56f, 0.50f, 0.72f}, 3.0f);
    meshes[index(PropKind::MossRock)]       = PropMeshes::rock({0.60f, 0.64f, 0.52f}, 4.0f);
    meshes[index(PropKind::CrystalCluster)] = PropMeshes::crystalCluster({0.60f, 0.44f, 0.95f}, {0.46f, 0.60f, 1.00f});
    meshes[index(PropKind::IceCluster)]     = PropMeshes::crystalCluster({0.72f, 0.92f, 1.00f}, {0.55f, 0.80f, 0.98f});
    meshes[index(PropKind::SmallCrystal)]   = PropMeshes::crystalCluster({0.78f, 0.66f, 1.00f}, {0.60f, 0.72f, 1.00f});
    meshes[index(PropKind::GrassMeadow)]    = PropMeshes::grassTuft({0.42f, 0.72f, 0.36f});
    meshes[index(PropKind::GrassLavender)]  = PropMeshes::grassTuft({0.68f, 0.62f, 0.90f});
    meshes[index(PropKind::GrassSakura)]    = PropMeshes::grassTuft({0.55f, 0.76f, 0.48f});
    meshes[index(PropKind::GrassMoss)]      = PropMeshes::grassTuft({0.52f, 0.66f, 0.42f});
    meshes[index(PropKind::FlowerWhite)]    = PropMeshes::flowers({1.00f, 0.98f, 0.95f});
    meshes[index(PropKind::FlowerYellow)]   = PropMeshes::flowers({1.00f, 0.88f, 0.40f});
    meshes[index(PropKind::FlowerPink)]     = PropMeshes::flowers({1.00f, 0.62f, 0.78f});
    meshes[index(PropKind::PetalPatch)]     = PropMeshes::petalPatch();
    meshes[index(PropKind::DeadTree)]       = PropMeshes::deadTree();
    meshes[index(PropKind::DarkRock)]       = PropMeshes::rock({0.40f, 0.43f, 0.52f}, 5.0f);
    meshes[index(PropKind::GrassCave)]      = PropMeshes::grassTuft({0.26f, 0.42f, 0.30f});
    meshes[index(PropKind::LandmarkCrystal)] = PropMeshes::landmarkCrystal(m_landmarkColliders[index(PropKind::LandmarkCrystal)]);
    meshes[index(PropKind::LandmarkSakura)]  = PropMeshes::landmarkSakura(m_landmarkColliders[index(PropKind::LandmarkSakura)]);
    meshes[index(PropKind::LandmarkFrost)]   = PropMeshes::landmarkFrost(m_landmarkColliders[index(PropKind::LandmarkFrost)]);
    meshes[index(PropKind::LandmarkArch)]    = PropMeshes::landmarkArch(m_landmarkColliders[index(PropKind::LandmarkArch)]);
    for (Mesh& mesh : meshes)
        m_batches.push_back(std::make_unique<InstancedBatch>(std::move(mesh)));

    auto set = [&](PropKind kind, float radius, float height, bool standable, const Material& material) {
        m_info[index(kind)] = {radius, height, standable, material};
    };
    set(PropKind::RoundTree,      0.45f, 0.0f,  false, FOLIAGE);
    set(PropKind::SakuraTree,     0.45f, 0.0f,  false, FOLIAGE);
    set(PropKind::SnowyPine,      0.60f, 0.0f,  false, FOLIAGE);
    set(PropKind::Rock,           0.85f, 0.75f, true,  STONE);
    set(PropKind::IceRock,        0.85f, 0.75f, true,  CRYSTAL);
    set(PropKind::CrystalRock,    0.85f, 0.75f, true,  STONE);
    set(PropKind::MossRock,       0.85f, 0.75f, true,  STONE);
    set(PropKind::DarkRock,       0.85f, 0.75f, true,  STONE);
    set(PropKind::DeadTree,       0.30f, 0.0f,  false, STONE);
    set(PropKind::CrystalCluster, 0.75f, 0.0f,  false, CRYSTAL);
    set(PropKind::IceCluster,     0.75f, 0.0f,  false, CRYSTAL);
    set(PropKind::SmallCrystal,   0.0f,  0.0f,  false, CRYSTAL);
    for (PropKind k : {PropKind::GrassMeadow, PropKind::GrassLavender, PropKind::GrassSakura, PropKind::GrassMoss,
                       PropKind::GrassCave,
                       PropKind::FlowerWhite, PropKind::FlowerYellow, PropKind::FlowerPink, PropKind::PetalPatch})
        set(k, 0.0f, 0.0f, false, SMALL);
    set(PropKind::LandmarkCrystal, 0.0f, 0.0f, false, CRYSTAL);
    set(PropKind::LandmarkSakura,  0.0f, 0.0f, false, FOLIAGE);
    set(PropKind::LandmarkFrost,   0.0f, 0.0f, false, CRYSTAL);
    set(PropKind::LandmarkArch,    0.0f, 0.0f, false, STONE);

    generate(world);
}

int PropSystem::totalInstances() const
{
    int total = 0;
    for (const auto& list : m_instances)
        total += static_cast<int>(list.size());
    return total;
}

void PropSystem::generate(World& world)
{
    for (auto& list : m_instances)
        list.clear();
    m_rareSpots.clear();
    m_allZones.clear();

    std::vector<Collider> colliders;
    for (int i = 0; i < static_cast<int>(world.islands().size()); ++i)
        decorateIsland(world, i, colliders);

    for (int k = 0; k < KIND_COUNT; ++k)
        m_batches[k]->setInstances(m_instances[k]);
    world.setColliders(std::move(colliders));
    world.setClearZones(m_allZones);
}

void PropSystem::decorateIsland(World& world, int islandIndex, std::vector<Collider>& colliders)
{
    const Island& island = world.islands()[islandIndex];
    const IslandSettings& is = island.settings();
    const glm::vec2 center = xz(island.center());
    std::mt19937 rng(is.seed * 31u + 17u);

    std::vector<ClearZone> zones;
    std::vector<Footprint> footprints;

    float landmarkFacing = 0.0f;
    for (const PortalSite& site : world.portalSites()) {
        if (site.island != islandIndex)
            continue;
        const glm::vec2 portal = xz(site.position);
        zones.push_back({portal, portal, settings.portalClear});
        zones.push_back({portal, center, settings.pathClear});
        landmarkFacing = facingOf(portal - center);
    }
    if (islandIndex == World::MAIN_ISLAND) {
        zones.push_back({center + glm::vec2(0.0f, 16.0f), center + glm::vec2(0.0f, 16.0f), 20.0f});
        zones.push_back({center, center, 6.0f});
    }
    if (is.landmarkClearing > 0.0f)
        zones.push_back({center, center, is.landmarkClearing * 0.85f});
    for (const NpcSite& npc : world.npcSites()) {
        if (npc.island == islandIndex) {
            const glm::vec2 p = xz(npc.position);
            zones.push_back({p, p, 3.5f});
        }
    }

    switch (is.biome.kind) {
    case BiomeKind::Crystal: {
        placeLandmark(world, islandIndex, PropKind::LandmarkCrystal, landmarkFacing, footprints, colliders);
        m_rareSpots.push_back(island.center() + glm::vec3(0.0f, island.heightAt(center.x, center.y)
                              - island.center().y + PropMeshes::CRYSTAL_COLUMN_HEIGHT + 1.0f, 0.0f));
        break;
    }
    case BiomeKind::Sakura:
        placeLandmark(world, islandIndex, PropKind::LandmarkSakura, landmarkFacing, footprints, colliders);
        placeHiddenNook(world, islandIndex, zones, footprints, colliders);
        break;
    case BiomeKind::Frost:
        placeLandmark(world, islandIndex, PropKind::LandmarkFrost, landmarkFacing, footprints, colliders);
        break;
    case BiomeKind::Rocky:
        placeLandmark(world, islandIndex, PropKind::LandmarkArch, landmarkFacing, footprints, colliders);
        break;
    case BiomeKind::SkyRock:
        m_rareSpots.push_back({center.x, island.heightAt(center.x, center.y) + 1.0f, center.y});
        zones.push_back({center, center, 1.5f});
        break;
    case BiomeKind::Cave:
        if (const CaveIsland* cave = world.cave()) {
            const glm::vec2 mountain = xz(cave->mountainBase());
            zones.push_back({mountain, mountain, cave->mountainRadius() + 2.5f});
        }
        break;
    case BiomeKind::Meadow:
        break;
    }

    for (const ScatterRule& rule : decorationPlan(is.biome.kind)) {
        const int count = static_cast<int>(std::round(rule.count * settings.density));
        scatter(world, islandIndex, rule.kind, count, rule.minScale, rule.maxScale, zones, footprints, colliders, rng);
    }

    for (ClearZone zone : zones) {
        zone.island = islandIndex;
        m_allZones.push_back(zone);
    }

    if (is.biome.kind == BiomeKind::Sakura) {
        std::uniform_real_distribution<float> unit(0.0f, 1.0f);
        for (int i = 0; i < 14; ++i) {
            const float angle = unit(rng) * glm::two_pi<float>();
            const float distance = 2.0f + unit(rng) * 5.0f;
            const glm::vec2 p = center + distance * glm::vec2(std::cos(angle), std::sin(angle));
            addProp(PropKind::PetalPatch, {p.x, island.heightAt(p.x, p.y), p.y}, unit(rng) * 6.3f,
                    glm::vec3(1.0f + unit(rng)), islandIndex, footprints, colliders);
        }
    }
}

void PropSystem::placeLandmark(const World& world, int islandIndex, PropKind kind, float facing,
                               std::vector<Footprint>& footprints, std::vector<Collider>& colliders)
{
    const Island& island = world.islands()[islandIndex];
    const glm::vec3 base(island.center().x, island.heightAt(island.center().x, island.center().z), island.center().z);

    m_instances[index(kind)].push_back(glm::rotate(glm::translate(glm::mat4(1.0f), base), facing,
                                                   glm::vec3(0.0f, 1.0f, 0.0f)));

    const float c = std::cos(facing), s = std::sin(facing);
    for (const LocalCollider& local : m_landmarkColliders[index(kind)]) {
        const glm::vec2 offset(local.offset.x * c + local.offset.y * s, -local.offset.x * s + local.offset.y * c);
        const glm::vec2 position = xz(base) + offset;
        colliders.push_back({position, local.radius, base.y - 1.0f, base.y + local.top, islandIndex});
        footprints.push_back({position, local.radius});
    }
}

void PropSystem::placeHiddenNook(const World& world, int islandIndex, std::vector<ClearZone>& zones,
                                 std::vector<Footprint>& footprints, std::vector<Collider>& colliders)
{
    const Island& island = world.islands()[islandIndex];
    const glm::vec2 center = xz(island.center());

    const glm::vec2 farSide = glm::normalize(center);
    const float angle = std::atan2(farSide.y, farSide.x);
    const glm::vec2 nook = center + farSide * island.coastDistance(angle) * 0.7f;
    const float ground = island.heightAt(nook.x, nook.y);

    m_rareSpots.push_back({nook.x, ground + 1.0f, nook.y});
    const glm::vec2 toCenter = glm::normalize(center - nook);
    for (float degrees : {-95.0f, -40.0f, 40.0f, 95.0f}) {
        const float a = glm::radians(degrees);
        const glm::vec2 dir(toCenter.x * std::cos(a) - toCenter.y * std::sin(a),
                            toCenter.x * std::sin(a) + toCenter.y * std::cos(a));
        const glm::vec2 p = nook + dir * 2.2f;
        addProp(PropKind::Rock, {p.x, island.heightAt(p.x, p.y), p.y}, degrees, glm::vec3(1.5f, 1.3f, 1.5f),
                islandIndex, footprints, colliders);
    }
    zones.push_back({nook, nook, 4.5f});
}

void PropSystem::scatter(const World& world, int islandIndex, PropKind kind, int count, float minScale,
                         float maxScale, const std::vector<ClearZone>& zones, std::vector<Footprint>& footprints,
                         std::vector<Collider>& colliders, std::mt19937& rng)
{
    const Island& island = world.islands()[islandIndex];
    const IslandSettings& is = island.settings();
    const KindInfo& info = m_info[index(kind)];
    const bool big = info.colliderRadius > 0.0f;
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);

    int placed = 0;
    for (int attempt = 0; attempt < count * 25 && placed < count; ++attempt) {
        const float r = is.radius * std::sqrt(unit(rng));
        const float a = unit(rng) * glm::two_pi<float>();
        const float x = is.position.x + r * std::cos(a);
        const float z = is.position.z + r * std::sin(a);
        const glm::vec2 p(x, z);

        if (!island.isOnIsland(x, z))
            continue;
        const float ground = island.heightAt(x, z);
        if (ground - is.position.y < is.biome.plateauHeight * 0.6f)
            continue;
        if (flatness(island, x, z) < (big ? 0.9f : 0.8f))
            continue;

        bool blocked = false;
        for (const ClearZone& zone : zones)
            if (zone.contains(p)) { blocked = true; break; }
        if (blocked)
            continue;

        const float scale = minScale + (maxScale - minScale) * unit(rng);
        const float radius = big ? info.colliderRadius * scale : 0.0f;
        for (const Footprint& other : footprints) {
            const float minimum = big ? radius + other.radius + settings.spacing : other.radius;
            if (glm::length(p - other.center) < minimum) { blocked = true; break; }
        }
        if (blocked)
            continue;

        glm::vec3 scale3(scale);
        float tilt = 0.0f;
        if (info.standable) {
            scale3 = scale * glm::vec3(0.9f + 0.3f * unit(rng), 0.6f + 0.4f * unit(rng), 0.9f + 0.3f * unit(rng));
            tilt = (unit(rng) - 0.5f) * 0.3f;
        }
        addProp(kind, {x, ground, z}, unit(rng) * glm::two_pi<float>(), scale3, islandIndex, footprints, colliders, tilt);
        ++placed;

        if (kind == PropKind::SakuraTree) {
            for (int i = 0; i < 2; ++i) {
                const float pa = unit(rng) * glm::two_pi<float>();
                const glm::vec2 q = p + (1.2f + unit(rng) * 1.5f) * glm::vec2(std::cos(pa), std::sin(pa));
                if (island.isOnIsland(q.x, q.y))
                    addProp(PropKind::PetalPatch, {q.x, island.heightAt(q.x, q.y), q.y}, pa,
                            glm::vec3(0.8f + 0.5f * unit(rng)), islandIndex, footprints, colliders);
            }
        }
    }
}

void PropSystem::addProp(PropKind kind, const glm::vec3& position, float yaw, const glm::vec3& scale,
                         int islandIndex, std::vector<Footprint>& footprints, std::vector<Collider>& colliders,
                         float tilt)
{
    const KindInfo& info = m_info[index(kind)];

    const float sink = info.standable ? 0.25f * scale.y : 0.05f;
    glm::mat4 model = glm::translate(glm::mat4(1.0f), position - glm::vec3(0.0f, sink, 0.0f));
    model = glm::rotate(model, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
    if (tilt != 0.0f)
        model = glm::rotate(model, tilt, glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::scale(model, scale);
    m_instances[index(kind)].push_back(model);

    if (info.colliderRadius > 0.0f) {
        const float radius = info.colliderRadius * std::max(scale.x, scale.z);
        const float top = info.standable ? position.y + info.colliderHeight * scale.y : position.y + 1000.0f;
        colliders.push_back({glm::vec2(position.x, position.z), radius, position.y - 1.0f, top, islandIndex});
        footprints.push_back({glm::vec2(position.x, position.z), radius});
    }
}

void PropSystem::draw(const Shader& instancedShader) const
{
    for (int k = 0; k < KIND_COUNT; ++k) {
        if (m_batches[k]->count() == 0)
            continue;
        Lighting::applyMaterial(instancedShader, m_info[k].material);
        m_batches[k]->draw();
    }
}

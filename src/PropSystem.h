#pragma once

#include "InstancedBatch.h"
#include "Lighting.h"
#include "PropMeshes.h"
#include "Shader.h"
#include "World.h"

#include <glm/glm.hpp>

#include <memory>
#include <random>
#include <vector>

enum class PropKind {
    RoundTree, SakuraTree, SnowyPine,
    Rock, IceRock, CrystalRock, MossRock,
    CrystalCluster, IceCluster, SmallCrystal,
    GrassMeadow, GrassLavender, GrassSakura, GrassMoss,
    FlowerWhite, FlowerYellow, FlowerPink, PetalPatch,
    DeadTree, DarkRock, GrassCave,
    LandmarkCrystal, LandmarkSakura, LandmarkFrost, LandmarkArch,
    Count
};

struct PropSettings {
    float density       = 1.0f;
    float spacing       = 0.8f;
    float portalClear   = 5.0f;
    float pathClear     = 4.0f;
};

class PropSystem {
public:
    explicit PropSystem(World& world, const PropSettings& settings = {});

    PropSettings settings;

    void generate(World& world);

    void draw(const Shader& instancedShader) const;

    const std::vector<glm::vec3>& rareDiamondSpots() const { return m_rareSpots; }

    int totalInstances() const;

private:
    struct Footprint { glm::vec2 center; float radius; };

    struct KindInfo {
        float    colliderRadius = 0.0f;
        float    colliderHeight = 0.0f;
        bool     standable = false;
        Material material;
    };

    void decorateIsland(World& world, int islandIndex, std::vector<Collider>& colliders);
    void placeLandmark(const World& world, int islandIndex, PropKind kind, float facing,
                       std::vector<Footprint>& footprints, std::vector<Collider>& colliders);
    void placeHiddenNook(const World& world, int islandIndex, std::vector<ClearZone>& zones,
                         std::vector<Footprint>& footprints, std::vector<Collider>& colliders);
    void scatter(const World& world, int islandIndex, PropKind kind, int count, float minScale, float maxScale,
                 const std::vector<ClearZone>& zones, std::vector<Footprint>& footprints,
                 std::vector<Collider>& colliders, std::mt19937& rng);
    void addProp(PropKind kind, const glm::vec3& position, float yaw, const glm::vec3& scale, int islandIndex,
                 std::vector<Footprint>& footprints, std::vector<Collider>& colliders, float tilt = 0.0f);

    std::vector<std::unique_ptr<InstancedBatch>> m_batches;
    std::vector<KindInfo>                        m_info;
    std::vector<std::vector<glm::mat4>>          m_instances;
    std::vector<std::vector<LocalCollider>>      m_landmarkColliders;
    std::vector<glm::vec3>                       m_rareSpots;
    std::vector<ClearZone>                       m_allZones;
};

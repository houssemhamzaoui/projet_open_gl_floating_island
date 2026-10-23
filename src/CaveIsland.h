#pragma once

#include "Island.h"
#include "Mesh.h"
#include "PointLight.h"

#include <glm/glm.hpp>

#include <vector>

struct Collider;
class MeshBuilder;

struct CaveSettings {
    float mountainRadius = 12.0f;
    float mountainHeight = 22.0f;
    float mountainOffset = 6.0f;

    float tunnelHalfWidth = 2.1f;
    float tunnelHeight    = 3.0f;
    float wallRoughness   = 0.3f;

    float chamberRadius = 10.0f;
    float chamberHeight = 8.0f;
    float chamberDepth  = 14.0f;

    float     sunFadeStart   = 0.04f;
    float     sunFadeEnd     = 0.40f;
    glm::vec3 caveAmbient    {0.07f, 0.06f, 0.13f};
    glm::vec3 caveFogColor   {0.02f, 0.02f, 0.05f};
    float     caveFogDensity = 0.035f;
    float     glowSpacing    = 5.0f;
};

enum class GlowKind { CrystalTeal, CrystalPurple, MushroomsCyan, MushroomsPurple };
struct GlowSpot {
    glm::vec3  position{0.0f};
    float      yaw = 0.0f;
    float      scale = 1.0f;
    GlowKind   kind = GlowKind::CrystalTeal;
    bool       hasLight = true;
    PointLight light;
};

struct TunnelQuery {
    bool      hit = false;
    float     floorY = 0.0f;
    float     along = 0.0f;
    float     lateral = 0.0f;
    glm::vec3 side{1.0f, 0.0f, 0.0f};
};

class CaveIsland {
public:
    CaveIsland(const Island& island, int islandIndex, const CaveSettings& settings = {});

    void draw() const { m_mesh.draw(); m_floorMesh.draw(); }

    bool isInCave(const glm::vec3& p) const;
    bool floorBelow(const glm::vec3& p, float stepUp, float& height) const;
    bool isSolid(const glm::vec3& p, const Island& island) const;
    bool hidesTerrain(float x, float z) const
    {
        return glm::length(glm::vec2(x - m_mountainBase.x, z - m_mountainBase.z)) < m_settings.mountainRadius;
    }
    bool isNear(const glm::vec3& p) const;

    void constrain(glm::vec3& position, glm::vec3& velocity, float radius, float height) const;

    float depthAt(const glm::vec3& p) const;
    float chamberFloorAt(float x, float z) const;

    const std::vector<Collider>& colliders() const { return m_colliders; }

    struct TunnelPoint {
        glm::vec3 floor;
        glm::vec3 side;
        float     distance;
    };
    const std::vector<TunnelPoint>& path() const { return m_samples; }
    const std::vector<GlowSpot>& glowSpots() const { return m_glowSpots; }
    const std::vector<glm::vec3>& gemSpots() const { return m_gemSpots; }
    const glm::vec3& heartPosition() const { return m_heartPosition; }
    const PointLight& heartLight() const { return m_heartLight; }
    const glm::vec3& poolCenter() const { return m_poolCenter; }
    float poolRadius() const { return 2.4f; }
    const CaveSettings& settings() const { return m_settings; }
    CaveLightingInfo lightingInfo() const;

    int islandIndex() const { return m_island; }
    float chamberRadius() const { return m_settings.chamberRadius; }
    glm::vec2 exitDirection() const { return m_exitDirection; }
    void setSealed(bool sealed) { m_sealed = sealed; }
    bool isSealed() const { return m_sealed; }
    const glm::vec3& mountainBase() const { return m_mountainBase; }
    float mountainRadius() const { return m_settings.mountainRadius; }
    const glm::vec3& chamberFloorCenter() const { return m_chamberFloor; }
    const glm::vec3& entrance() const { return m_entrance; }

private:
    using Sample = TunnelPoint;

    void build(const Island& island);
    void placeDecoration(MeshBuilder& builder);
    TunnelQuery queryTunnel(const glm::vec3& p, float below, float above) const;
    bool inChamber(const glm::vec3& p) const;
    bool inChamberExit(const glm::vec3& p, float margin) const;
    float mountainRadiusAt(float heightAboveBase) const;

    CaveSettings m_settings;
    int          m_island;
    Mesh         m_mesh;
    Mesh         m_floorMesh;
    std::vector<Sample>   m_samples;
    std::vector<Collider> m_colliders;

    glm::vec3 m_mountainBase{0.0f};
    glm::vec3 m_chamberFloor{0.0f};
    glm::vec3 m_entrance{0.0f};
    glm::vec3 m_entranceForward{0.0f};
    glm::vec3 m_entranceSide{0.0f};
    glm::vec2 m_exitDirection{0.0f};
    bool      m_sealed = false;

    std::vector<GlowSpot>  m_glowSpots;
    std::vector<glm::vec3> m_gemSpots;
    glm::vec3 m_heartPosition{0.0f};
    glm::vec3 m_poolCenter{0.0f};
    PointLight m_heartLight;
};

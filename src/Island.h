#pragma once

#include "Biome.h"
#include "Mesh.h"

#include <glm/glm.hpp>

#include <vector>

struct FlatSpot {
    glm::vec2 center{0.0f};
    float     radius = 5.0f;
};

struct IslandSettings {
    glm::vec3     position{0.0f};
    float         radius     = 20.0f;
    int           resolution = 120;
    unsigned      seed       = 1337;
    BiomeSettings biome = BiomeSettings::meadow();
    std::vector<FlatSpot> flatSpots;
    bool  hasPortal = true;
    bool  isRespawnPoint = true;
    float portalDistance = 0.45f;
    float landmarkClearing = 0.0f;
};

class Island {
public:
    explicit Island(const IslandSettings& settings);

    void generate();

    glm::mat4 getModelMatrix() const;
    void draw() const { m_mesh.draw(); }

    bool  isOnIsland(float x, float z) const;
    float heightAt(float x, float z) const;
    float bottomY() const;
    float coastDistance(float angle) const { return m_settings.radius * coastline(angle); }
    glm::vec3 center() const { return m_settings.position; }

    IslandSettings&       settings()       { return m_settings; }
    const IslandSettings& settings() const { return m_settings; }

private:
    float localHeight(float x, float z) const;
    float naturalHeight(float x, float z) const;
    float fbm(glm::vec2 position) const;
    float coastline(float angle) const;
    float undersideDepth() const;

    void buildTop(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) const;
    void buildUnderside(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) const;
    void colorTop(std::vector<Vertex>& vertices, size_t count) const;
    void colorUnderside(std::vector<Vertex>& vertices, size_t first) const;

    IslandSettings m_settings;
    Mesh m_mesh;

    glm::vec2 m_hillOffset{0.0f};
    glm::vec2 m_coastOffset{0.0f};
    glm::vec3 m_detailOffset{0.0f};
};

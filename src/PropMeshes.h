#pragma once

#include "Mesh.h"

#include <glm/glm.hpp>

#include <vector>

struct LocalCollider {
    glm::vec2 offset{0.0f};
    float     radius = 0.5f;
    float     top = 0.0f;
    bool      standable = false;
};

namespace PropMeshes {

Mesh roundTree();
Mesh sakuraTree();
Mesh snowyPine();
Mesh rock(const glm::vec3& color, float seed);
Mesh crystalCluster(const glm::vec3& colorA, const glm::vec3& colorB);
Mesh grassTuft(const glm::vec3& color);
Mesh flowers(const glm::vec3& petalColor);
Mesh petalPatch();
Mesh deadTree();

Mesh landmarkCrystal(std::vector<LocalCollider>& colliders);
Mesh landmarkSakura(std::vector<LocalCollider>& colliders);
Mesh landmarkFrost(std::vector<LocalCollider>& colliders);
Mesh landmarkArch(std::vector<LocalCollider>& colliders);

constexpr float CRYSTAL_COLUMN_HEIGHT = 7.0f;

}

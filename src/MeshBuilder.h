#pragma once

#include "Mesh.h"

#include <glm/glm.hpp>

#include <vector>

class MeshBuilder {
public:
    void addTriangle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& color);

    void addSphere(const glm::mat4& transform, const glm::vec3& color, int segments = 10, int rings = 7);

    void addPrism(const glm::mat4& transform, int sides, float bottomRadius, float topRadius,
                  float height, float tipHeight, const glm::vec3& color);

    void addCone(const glm::mat4& transform, int sides, float radius, float height, const glm::vec3& color);

    void addBox(const glm::mat4& transform, const glm::vec3& halfSize, const glm::vec3& color);

    void addRock(const glm::mat4& transform, const glm::vec3& color, float seed);

    Mesh build() const { return Mesh(m_vertices, m_indices); }

private:
    std::vector<Vertex>       m_vertices;
    std::vector<unsigned int> m_indices;
};

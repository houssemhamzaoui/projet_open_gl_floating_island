#include "MeshBuilder.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/noise.hpp>

#include <cmath>

namespace {

glm::vec3 transformPoint(const glm::mat4& m, const glm::vec3& p)
{
    return glm::vec3(m * glm::vec4(p, 1.0f));
}

glm::vec3 circlePoint(int i, int sides, float radius, float y)
{
    const float angle = i / float(sides) * glm::two_pi<float>();
    return {std::cos(angle) * radius, y, std::sin(angle) * radius};
}

}

void MeshBuilder::addTriangle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c,
                              const glm::vec3& color)
{
    const glm::vec3 cross = glm::cross(b - a, c - a);
    const float length = glm::length(cross);
    if (length < 1e-8f)
        return;

    const glm::vec3 normal = cross / length;
    const auto first = static_cast<unsigned int>(m_vertices.size());
    m_vertices.push_back({a, normal, color});
    m_vertices.push_back({b, normal, color});
    m_vertices.push_back({c, normal, color});
    m_indices.insert(m_indices.end(), {first, first + 1, first + 2});
}

void MeshBuilder::addSphere(const glm::mat4& transform, const glm::vec3& color, int segments, int rings)
{
    const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));
    const auto first = static_cast<unsigned int>(m_vertices.size());

    for (int r = 0; r <= rings; ++r) {
        const float polar = r / float(rings) * glm::pi<float>();
        for (int s = 0; s <= segments; ++s) {
            const float azimuth = s / float(segments) * glm::two_pi<float>();
            const glm::vec3 n(std::sin(polar) * std::cos(azimuth), std::cos(polar),
                              std::sin(polar) * std::sin(azimuth));
            m_vertices.push_back({transformPoint(transform, n), glm::normalize(normalMatrix * n), color});
        }
    }
    const unsigned rowLength = segments + 1;
    for (unsigned r = 0; r < unsigned(rings); ++r) {
        for (unsigned s = 0; s < unsigned(segments); ++s) {
            const unsigned a = first + r * rowLength + s, b = a + rowLength;
            m_indices.insert(m_indices.end(), {a, a + 1, b, a + 1, b + 1, b});
        }
    }
}

void MeshBuilder::addPrism(const glm::mat4& transform, int sides, float bottomRadius, float topRadius,
                           float height, float tipHeight, const glm::vec3& color)
{
    const glm::vec3 bottomCenter = transformPoint(transform, {0.0f, 0.0f, 0.0f});
    const glm::vec3 topCenter    = transformPoint(transform, {0.0f, height, 0.0f});
    const glm::vec3 tip          = transformPoint(transform, {0.0f, height + tipHeight, 0.0f});

    for (int i = 0; i < sides; ++i) {
        const glm::vec3 b0 = transformPoint(transform, circlePoint(i,     sides, bottomRadius, 0.0f));
        const glm::vec3 b1 = transformPoint(transform, circlePoint(i + 1, sides, bottomRadius, 0.0f));
        const glm::vec3 t0 = transformPoint(transform, circlePoint(i,     sides, topRadius, height));
        const glm::vec3 t1 = transformPoint(transform, circlePoint(i + 1, sides, topRadius, height));

        addTriangle(b0, t0, b1, color);
        addTriangle(b1, t0, t1, color);
        if (tipHeight > 0.0f)
            addTriangle(t0, tip, t1, color);
        else
            addTriangle(t0, topCenter, t1, color);
        addTriangle(bottomCenter, b0, b1, color);
    }
}

void MeshBuilder::addCone(const glm::mat4& transform, int sides, float radius, float height,
                          const glm::vec3& color)
{
    const glm::vec3 base = transformPoint(transform, {0.0f, 0.0f, 0.0f});
    const glm::vec3 apex = transformPoint(transform, {0.0f, height, 0.0f});
    for (int i = 0; i < sides; ++i) {
        const glm::vec3 b0 = transformPoint(transform, circlePoint(i,     sides, radius, 0.0f));
        const glm::vec3 b1 = transformPoint(transform, circlePoint(i + 1, sides, radius, 0.0f));
        addTriangle(b0, apex, b1, color);
        addTriangle(base, b0, b1, color);
    }
}

void MeshBuilder::addBox(const glm::mat4& transform, const glm::vec3& halfSize, const glm::vec3& color)
{
    const glm::vec3 normals[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    const glm::vec3 sides[6]   = {{0, 0, -1}, {0, 0, 1}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {-1, 0, 0}};
    for (int f = 0; f < 6; ++f) {
        const glm::vec3 n = normals[f], u = sides[f], v = glm::cross(n, u);
        const glm::vec3 c0 = transformPoint(transform, (n - u - v) * halfSize);
        const glm::vec3 c1 = transformPoint(transform, (n + u - v) * halfSize);
        const glm::vec3 c2 = transformPoint(transform, (n + u + v) * halfSize);
        const glm::vec3 c3 = transformPoint(transform, (n - u + v) * halfSize);
        addTriangle(c0, c1, c2, color);
        addTriangle(c0, c2, c3, color);
    }
}

void MeshBuilder::addRock(const glm::mat4& transform, const glm::vec3& color, float seed)
{
    const int segments = 7, rings = 5;
    std::vector<glm::vec3> points;
    for (int r = 0; r <= rings; ++r) {
        const float polar = r / float(rings) * glm::pi<float>();
        for (int s = 0; s <= segments; ++s) {
            const float azimuth = (s % segments) / float(segments) * glm::two_pi<float>();
            const glm::vec3 direction(std::sin(polar) * std::cos(azimuth), std::cos(polar),
                                      std::sin(polar) * std::sin(azimuth));
            const float bump = 1.0f + 0.28f * glm::simplex(direction * 1.6f + glm::vec3(seed));
            points.push_back(transformPoint(transform, direction * bump));
        }
    }
    const int rowLength = segments + 1;
    for (int r = 0; r < rings; ++r) {
        for (int s = 0; s < segments; ++s) {
            const int a = r * rowLength + s, b = a + rowLength;
            addTriangle(points[a], points[a + 1], points[b], color);
            addTriangle(points[a + 1], points[b + 1], points[b], color);
        }
    }
}

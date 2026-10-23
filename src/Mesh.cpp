#include "Mesh.h"

#include <glm/gtc/constants.hpp>

#include <cmath>
#include <cstddef>
#include <utility>

namespace {

struct Row {
    float polarAngle;
    float yOffset;
};

Mesh buildRows(const std::vector<Row>& rows, float radius, const glm::vec3& color, int segments)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    for (const Row& row : rows) {
        for (int s = 0; s <= segments; ++s) {
            const float azimuth = s / float(segments) * glm::two_pi<float>();
            const glm::vec3 normal(std::sin(row.polarAngle) * std::cos(azimuth),
                                   std::cos(row.polarAngle),
                                   std::sin(row.polarAngle) * std::sin(azimuth));
            const glm::vec3 position = normal * radius + glm::vec3(0.0f, row.yOffset, 0.0f);
            vertices.push_back({position, normal, color});
        }
    }

    const unsigned rowLength = segments + 1;
    for (unsigned r = 0; r + 1 < rows.size(); ++r) {
        for (unsigned s = 0; s < unsigned(segments); ++s) {
            const unsigned a = r * rowLength + s;
            const unsigned b = a + rowLength;
            indices.insert(indices.end(), {a, a + 1, b, a + 1, b + 1, b});
        }
    }
    return Mesh(vertices, indices);
}

}

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
    : m_indexCount(static_cast<GLsizei>(indices.size()))
{
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
                 vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                 indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, color)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void Mesh::attachInstanceBuffer(GLuint instanceBuffer)
{
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
    for (int column = 0; column < 4; ++column) {
        const GLuint location = 3 + column;
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(location, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                              reinterpret_cast<void*>(sizeof(glm::vec4) * column));
        glVertexAttribDivisor(location, 1);
    }
    glBindVertexArray(0);
}

void Mesh::drawInstanced(GLsizei count) const
{
    glBindVertexArray(m_vao);
    glDrawElementsInstanced(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr, count);
}

Mesh::~Mesh()
{
    release();
}

Mesh::Mesh(Mesh&& other) noexcept
{
    *this = std::move(other);
}

Mesh& Mesh::operator=(Mesh&& other) noexcept
{
    if (this != &other) {
        release();
        std::swap(m_vao, other.m_vao);
        std::swap(m_vbo, other.m_vbo);
        std::swap(m_ebo, other.m_ebo);
        std::swap(m_indexCount, other.m_indexCount);
    }
    return *this;
}

void Mesh::release()
{
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_ebo);
    m_vao = m_vbo = m_ebo = 0;
    m_indexCount = 0;
}

void Mesh::draw() const
{
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
}

void Mesh::computeNormals(std::vector<Vertex>& vertices,
                          const std::vector<unsigned int>& indices)
{
    for (Vertex& v : vertices)
        v.normal = glm::vec3(0.0f);

    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        Vertex& v0 = vertices[indices[i]];
        Vertex& v1 = vertices[indices[i + 1]];
        Vertex& v2 = vertices[indices[i + 2]];
        const glm::vec3 faceNormal = glm::cross(v1.position - v0.position,
                                                v2.position - v0.position);
        v0.normal += faceNormal;
        v1.normal += faceNormal;
        v2.normal += faceNormal;
    }

    for (Vertex& v : vertices) {
        const float length = glm::length(v.normal);
        v.normal = length > 0.0f ? v.normal / length : glm::vec3(0.0f, 1.0f, 0.0f);
    }
}

Mesh Mesh::createCube()
{
    struct Face { glm::vec3 normal, u, color; };
    const Face faces[6] = {
        {{ 1, 0, 0}, { 0, 0,-1}, {0.98f, 0.60f, 0.60f}},
        {{-1, 0, 0}, { 0, 0, 1}, {0.45f, 0.80f, 0.80f}},
        {{ 0, 1, 0}, { 1, 0, 0}, {0.60f, 0.88f, 0.55f}},
        {{ 0,-1, 0}, { 1, 0, 0}, {0.65f, 0.50f, 0.40f}},
        {{ 0, 0, 1}, { 1, 0, 0}, {0.98f, 0.85f, 0.50f}},
        {{ 0, 0,-1}, {-1, 0, 0}, {0.70f, 0.60f, 0.90f}},
    };

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    for (const Face& f : faces) {
        const glm::vec3 v = glm::cross(f.normal, f.u);
        const glm::vec3 center = f.normal * 0.5f;
        const auto first = static_cast<unsigned int>(vertices.size());

        vertices.push_back({center - 0.5f * f.u - 0.5f * v, f.normal, f.color});
        vertices.push_back({center + 0.5f * f.u - 0.5f * v, f.normal, f.color});
        vertices.push_back({center + 0.5f * f.u + 0.5f * v, f.normal, f.color});
        vertices.push_back({center - 0.5f * f.u + 0.5f * v, f.normal, f.color});

        indices.insert(indices.end(), {first, first + 1, first + 2,
                                       first + 2, first + 3, first});
    }
    return Mesh(vertices, indices);
}

Mesh Mesh::createSphere(float radius, const glm::vec3& color, int segments, int rings)
{
    std::vector<Row> rows;
    for (int r = 0; r <= rings; ++r)
        rows.push_back({r / float(rings) * glm::pi<float>(), 0.0f});
    return buildRows(rows, radius, color, segments);
}

Mesh Mesh::createCapsule(float radius, float cylinderHeight, const glm::vec3& color,
                         int segments, int rings)
{
    const int half = rings / 2;
    const float halfHeight = cylinderHeight * 0.5f;
    std::vector<Row> rows;
    for (int r = 0; r <= half; ++r)
        rows.push_back({r / float(half) * glm::half_pi<float>(), halfHeight});
    for (int r = 0; r <= half; ++r)
        rows.push_back({glm::half_pi<float>() + r / float(half) * glm::half_pi<float>(), -halfHeight});
    return buildRows(rows, radius, color, segments);
}

Mesh Mesh::createBox(const glm::vec3& halfSize, const glm::vec3& color)
{
    const glm::vec3 normals[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    const glm::vec3 sides[6]   = {{0, 0, -1}, {0, 0, 1}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {-1, 0, 0}};

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    for (int f = 0; f < 6; ++f) {
        const glm::vec3 n = normals[f], u = sides[f], v = glm::cross(n, u);
        const auto first = static_cast<unsigned int>(vertices.size());
        const glm::vec3 corners[4] = {n - u - v, n + u - v, n + u + v, n - u + v};
        for (const glm::vec3& corner : corners)
            vertices.push_back({corner * halfSize, n, color});
        indices.insert(indices.end(), {first, first + 1, first + 2, first + 2, first + 3, first});
    }
    return Mesh(vertices, indices);
}

Mesh Mesh::createTorus(float majorRadius, float minorRadius, const glm::vec3& color,
                       int ringSegments, int tubeSegments)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    for (int i = 0; i <= ringSegments; ++i) {
        const float u = i / float(ringSegments) * glm::two_pi<float>();
        const glm::vec3 ringCenter(std::cos(u) * majorRadius, std::sin(u) * majorRadius, 0.0f);
        for (int j = 0; j <= tubeSegments; ++j) {
            const float v = j / float(tubeSegments) * glm::two_pi<float>();
            const glm::vec3 normal(std::cos(u) * std::cos(v), std::sin(u) * std::cos(v), std::sin(v));
            vertices.push_back({ringCenter + normal * minorRadius, normal, color});
        }
    }

    const unsigned rowLength = tubeSegments + 1;
    for (unsigned i = 0; i < unsigned(ringSegments); ++i) {
        for (unsigned j = 0; j < unsigned(tubeSegments); ++j) {
            const unsigned a = i * rowLength + j;
            const unsigned b = a + rowLength;
            indices.insert(indices.end(), {a, b, a + 1, a + 1, b, b + 1});
        }
    }
    return Mesh(vertices, indices);
}

Mesh Mesh::createOctahedron(float radius, float height, const glm::vec3& color)
{
    const glm::vec3 top(0.0f, height, 0.0f), bottom(0.0f, -height, 0.0f);
    const glm::vec3 ring[4] = {{radius, 0, 0}, {0, 0, radius}, {-radius, 0, 0}, {0, 0, -radius}};

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    auto addFace = [&](const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
        const glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
        const auto first = static_cast<unsigned int>(vertices.size());
        vertices.push_back({a, normal, color});
        vertices.push_back({b, normal, color});
        vertices.push_back({c, normal, color});
        indices.insert(indices.end(), {first, first + 1, first + 2});
    };
    for (int i = 0; i < 4; ++i) {
        const glm::vec3& p0 = ring[i];
        const glm::vec3& p1 = ring[(i + 1) % 4];
        addFace(top, p1, p0);
        addFace(bottom, p0, p1);
    }
    return Mesh(vertices, indices);
}

Mesh Mesh::createGroundQuad()
{
    const glm::vec3 up(0.0f, 1.0f, 0.0f), white(1.0f);
    const std::vector<Vertex> vertices = {
        {{-1.0f, 0.0f, -1.0f}, up, white}, {{1.0f, 0.0f, -1.0f}, up, white},
        {{ 1.0f, 0.0f,  1.0f}, up, white}, {{-1.0f, 0.0f, 1.0f}, up, white},
    };
    return Mesh(vertices, {0, 3, 1, 1, 3, 2});
}

Mesh Mesh::createStandingQuad()
{
    const glm::vec3 front(0.0f, 0.0f, 1.0f), white(1.0f);
    const std::vector<Vertex> vertices = {
        {{-1.0f, -1.0f, 0.0f}, front, white}, {{1.0f, -1.0f, 0.0f}, front, white},
        {{ 1.0f,  1.0f, 0.0f}, front, white}, {{-1.0f, 1.0f, 0.0f}, front, white},
    };
    return Mesh(vertices, {0, 1, 2, 2, 3, 0});
}

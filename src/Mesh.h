#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <vector>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
};

class Mesh {
public:
    Mesh() = default;
    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    void draw() const;

    void attachInstanceBuffer(GLuint instanceBuffer);
    void drawInstanced(GLsizei count) const;

    static Mesh createCube();

    static Mesh createSphere(float radius, const glm::vec3& color,
                             int segments = 32, int rings = 16);

    static Mesh createCapsule(float radius, float cylinderHeight, const glm::vec3& color,
                              int segments = 32, int rings = 16);

    static Mesh createGroundQuad();
    static Mesh createStandingQuad();

    static Mesh createBox(const glm::vec3& halfSize, const glm::vec3& color);

    static Mesh createTorus(float majorRadius, float minorRadius, const glm::vec3& color,
                            int ringSegments = 16, int tubeSegments = 6);

    static Mesh createOctahedron(float radius, float height, const glm::vec3& color);

    static void computeNormals(std::vector<Vertex>& vertices,
                               const std::vector<unsigned int>& indices);

private:
    void release();

    GLuint  m_vao = 0;
    GLuint  m_vbo = 0;
    GLuint  m_ebo = 0;
    GLsizei m_indexCount = 0;
};

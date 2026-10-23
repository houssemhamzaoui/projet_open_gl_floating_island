#pragma once

#include "Mesh.h"

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <vector>

class InstancedBatch {
public:
    explicit InstancedBatch(Mesh mesh);
    ~InstancedBatch();

    InstancedBatch(const InstancedBatch&) = delete;
    InstancedBatch& operator=(const InstancedBatch&) = delete;

    void setInstances(const std::vector<glm::mat4>& modelMatrices, bool dynamic = false);

    void draw() const;

    GLsizei count() const { return m_count; }

private:
    Mesh    m_mesh;
    GLuint  m_instanceBuffer = 0;
    GLsizei m_count = 0;
};

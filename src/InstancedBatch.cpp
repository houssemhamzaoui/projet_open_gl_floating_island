#include "InstancedBatch.h"

#include <utility>

InstancedBatch::InstancedBatch(Mesh mesh)
    : m_mesh(std::move(mesh))
{
    glGenBuffers(1, &m_instanceBuffer);
    m_mesh.attachInstanceBuffer(m_instanceBuffer);
}

InstancedBatch::~InstancedBatch()
{
    glDeleteBuffers(1, &m_instanceBuffer);
}

void InstancedBatch::setInstances(const std::vector<glm::mat4>& modelMatrices, bool dynamic)
{
    m_count = static_cast<GLsizei>(modelMatrices.size());
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceBuffer);
    glBufferData(GL_ARRAY_BUFFER, modelMatrices.size() * sizeof(glm::mat4), modelMatrices.data(),
                 dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
}

void InstancedBatch::draw() const
{
    if (m_count > 0)
        m_mesh.drawInstanced(m_count);
}

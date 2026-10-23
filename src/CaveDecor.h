#pragma once

#include "CaveIsland.h"
#include "InstancedBatch.h"
#include "Mesh.h"
#include "PointLight.h"
#include "Shader.h"

#include <glm/glm.hpp>

#include <memory>
#include <vector>

class CaveDecor {
public:
    CaveDecor();

    void generate(const CaveIsland* cave);

    void appendLights(std::vector<PointLight>& lights) const;

    void drawGlow(const Shader& gemShader, float time) const;
    void drawPool(const Shader& objectShader) const;

private:
    const CaveIsland* m_cave = nullptr;
    std::vector<std::unique_ptr<InstancedBatch>> m_glowBatches;
    std::unique_ptr<InstancedBatch> m_sporesCyan;
    std::unique_ptr<InstancedBatch> m_sporesPurple;
    Mesh m_pool;

    struct Spore { glm::vec3 base; float phase; float speed; };
    std::vector<Spore> m_spores;
};

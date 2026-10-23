#pragma once

#include "Mesh.h"
#include "Player.h"
#include "Shader.h"
#include "World.h"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <vector>

struct NpcSettings {
    float talkDistance  = 3.0f;
    float lookDistance  = 7.0f;
    float turnSpeed     = 4.0f;
    float bodyRadius    = 0.55f;
};

class NpcSystem {
public:
    struct Npc {
        std::string id;
        std::string name;
        NpcRole     role;
        int         island = 0;
        glm::vec3   position{0.0f};
        float       restFacing = 0.0f;
        float       facing = 0.0f;
        float       height = 2.0f;
        float       seed = 0.0f;
        int         look = 0;
    };

    explicit NpcSystem(const World& world, const NpcSettings& settings = {});

    NpcSettings settings;

    void generate(const World& world);

    void update(Player& player, float time, float deltaTime, int talkingTo);

    int nearestTalkable(const glm::vec3& playerPosition) const;

    void draw(const Shader& objectShader) const;

    const std::vector<Npc>& npcs() const { return m_npcs; }
    glm::vec3 headTop(int index) const;

private:
    struct Look {
        std::unique_ptr<Mesh> body;
        std::unique_ptr<Mesh> floating;
        glm::vec3 floatOffset{0.0f};
        float     height = 2.0f;
    };

    std::vector<Look> m_looks;
    std::vector<Npc>  m_npcs;
    float m_time = 0.0f;
};

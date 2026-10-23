#pragma once

#include "Collectibles.h"
#include "InstancedBatch.h"
#include "Mesh.h"
#include "Player.h"
#include "PointLight.h"
#include "Shader.h"
#include "World.h"

#include <glm/glm.hpp>

#include <deque>
#include <memory>
#include <random>
#include <vector>

enum class Reaction { Follow, Flee };

struct CreatureType {
    int       count;
    Reaction  reaction;
    float     noticeDistance;
    float     loseDistance;
    float     followDistance;
    float     speed;
    float     hopHeight;
    float     hopDuration;
    glm::vec3 color;
};

struct CreatureSettings {
    CreatureType meadow  {9, Reaction::Follow,  7.0f, 14.0f, 2.2f, 2.2f, 0.60f, 0.55f, {0.55f, 0.88f, 0.45f}};
    CreatureType crystal {7, Reaction::Flee,    5.0f, 10.0f, 0.0f, 2.4f, 0.50f, 0.50f, {0.72f, 0.56f, 1.00f}};
    CreatureType sakura  {8, Reaction::Follow,  8.0f, 16.0f, 2.8f, 2.6f, 1.00f, 0.65f, {1.00f, 0.66f, 0.80f}};
    CreatureType frost   {7, Reaction::Flee,    9.0f, 16.0f, 0.0f, 3.4f, 0.70f, 0.42f, {0.62f, 0.88f, 1.00f}};
    CreatureType cave    {5, Reaction::Follow,  6.0f, 12.0f, 1.8f, 2.0f, 0.45f, 0.50f, {0.45f, 1.00f, 0.90f}};
    float caveSlimeSize  = 0.65f;
    float caveSlimeLight = 0.6f;

    float wanderRange = 8.0f;
    float edgeMargin  = 0.8f;

    float spiritOrbitRadius   = 7.5f;
    float spiritOrbitHeight   = 7.0f;
    float spiritOrbitSpeed    = 0.7f;
    float spiritVisitDistance = 14.0f;
};

class Creatures {
public:
    explicit Creatures(const World& world, const CreatureSettings& settings = {});

    CreatureSettings settings;

    void generate(const World& world);

    void update(Player& player, const World& world, Collectibles& gems, float time, float deltaTime,
                const std::vector<glm::vec3>& dangers = {});

    void draw(const Shader& instancedShader, float time) const;
    void appendLights(std::vector<PointLight>& lights) const;
    void drawGlow(const Shader& gemShader, float time) const;
    void drawShadows(const Shader& blobShader) const;

    int count() const { return static_cast<int>(m_slimes.size()); }
    void setSpiritGiftGiven() { m_spirit.giftGiven = true; }
    void setCaveSlimesHidden(bool hidden) { m_caveHidden = hidden; }

private:
    enum class State { Idle, Wander, Notice, Follow, Flee };

    struct Slime {
        int       island = 0;
        int       type = 0;
        glm::vec2 position{0.0f};
        float     ground = 0.0f;
        float     yaw = 0.0f;
        float     size = 1.0f;
        State     state = State::Idle;
        float     timer = 0.0f;
        glm::vec2 target{0.0f};
        float     noticeCooldown = 0.0f;
        bool      hopping = false;
        float     hopPhase = 0.0f;
        float     hopScale = 1.0f;
        glm::vec2 hopDirection{0.0f};
        float     lift = 0.0f;
        float     squash = 1.0f;
        float     seed = 0.0f;
    };

    struct Spirit {
        bool      exists = false;
        int       island = -1;
        glm::vec3 treeBase{0.0f};
        glm::vec3 position{0.0f};
        float     angle = 0.0f;
        bool      visiting = false;
        float     visitTime = 0.0f;
        bool      giftGiven = false;
        float     trailTimer = 0.0f;
        std::deque<glm::vec3> trail;
    };

    const CreatureType& typeOf(const Slime& slime) const;
    void updateSlime(Slime& slime, Player& player, const World& world, int playerIsland, float deltaTime);
    void updateSpirit(Player& player, const World& world, Collectibles& gems, float time, float deltaTime);
    bool isValidSpot(int island, const glm::vec2& p, float radius, const World& world) const;
    void resolveConstraints(Slime& slime, const World& world) const;
    bool pickWanderTarget(Slime& slime, const World& world);
    glm::vec2 chooseHopDirection(const Slime& slime, const glm::vec2& wanted, const World& world) const;

    std::vector<Slime> m_slimes;
    Spirit             m_spirit;
    std::mt19937       m_rng;
    float              m_time = 0.0f;
    std::vector<glm::vec3> m_dangers;
    bool m_caveHidden = false;

    std::vector<std::unique_ptr<InstancedBatch>> m_bodies;
    std::vector<std::unique_ptr<InstancedBatch>> m_accessories;
    std::unique_ptr<InstancedBatch> m_eyes;
    std::unique_ptr<InstancedBatch> m_eyeShines;
    std::unique_ptr<InstancedBatch> m_spiritCore;
    std::unique_ptr<InstancedBatch> m_spiritSparkles;
    std::unique_ptr<InstancedBatch> m_spiritTrail;
    Mesh m_shadowQuad;
};

#pragma once

#include "EventBus.h"
#include "InstancedBatch.h"
#include "Mesh.h"
#include "Player.h"
#include "Shader.h"
#include "UIRenderer.h"
#include "World.h"

#include <glm/glm.hpp>

#include <memory>
#include <random>
#include <vector>

struct EnemySettings {
    int   perIsland       = 4;
    int   health          = 3;
    float radius          = 0.55f;
    float noticeDistance  = 9.0f;
    float giveUpDistance  = 15.0f;
    float leashRadius     = 13.0f;
    float patrolRadius    = 6.0f;
    float patrolSpeed     = 2.2f;
    float chaseSpeed      = 4.4f;
    float attackRange     = 2.6f;
    float windupTime      = 0.65f;
    float lungeSpeed      = 11.0f;
    float lungeTime       = 0.28f;
    float cooldownTime    = 1.0f;
    float hurtTime        = 0.35f;
    int   damage          = 1;
    float safeMargin      = 3.0f;
    float respawnTime     = 60.0f;
    float healDropChance  = 0.4f;
};

struct EnemyEffect {
    enum class Kind { Death, PlayerHit, HealDrop };
    Kind      kind;
    glm::vec3 position;
};

class EnemySystem {
public:
    enum class State { Patrol, Chase, Windup, Attack, Cooldown, Hurt, Return, Dead, Gone };

    struct Enemy {
        int       island = 0;
        glm::vec2 home{0.0f};
        glm::vec2 position{0.0f};
        float     ground = 0.0f;
        glm::vec2 velocity{0.0f};
        float     yaw = 0.0f;
        State     state = State::Gone;
        float     timer = 0.0f;
        int       health = 3;
        glm::vec2 target{0.0f};
        glm::vec2 lungeDirection{0.0f};
        bool      hitThisLunge = false;
        float     flash = 0.0f;
        bool      barShown = false;
        float     seed = 0.0f;
    };

    EnemySystem(const World& world, EventBus& events, const EnemySettings& settings = {});

    EnemySettings settings;

    void generate(const World& world);
    bool isActive() const { return m_active; }
    void activate();

    void update(Player& player, const World& world, bool targetable, float time, float deltaTime);

    bool damage(int index, int amount, const glm::vec3& from, float knockback);
    bool isHittable(int index) const;
    glm::vec3 center(int index) const;

    const std::vector<Enemy>& enemies() const { return m_enemies; }
    std::vector<EnemyEffect> takeEffects() { return std::move(m_effects); }
    std::vector<glm::vec3> dangerPositions() const;

    void draw(const Shader& objectShader, float time) const;
    void drawGlow(const Shader& gemShader, float time) const;
    void drawShadows(const Shader& blobShader) const;
    void drawHealthBars(UIRenderer& ui, const glm::mat4& viewProjection) const;

private:
    void spawn(Enemy& enemy);
    bool isValidSpot(int island, const glm::vec2& p, const World& world, float zoneMargin) const;
    bool playerIsProtected(int island, const glm::vec3& p, const World& world) const;
    bool moveTo(Enemy& enemy, const glm::vec2& next, const World& world, bool hunting) const;
    void face(Enemy& enemy, const glm::vec2& direction, float deltaTime) const;

    std::vector<Enemy> m_enemies;
    std::vector<EnemyEffect> m_effects;
    EventBus& m_events;
    bool m_active = false;
    std::mt19937 m_rng;

    Mesh m_body;
    std::unique_ptr<InstancedBatch> m_eyes;
    Mesh m_shadowQuad;
};

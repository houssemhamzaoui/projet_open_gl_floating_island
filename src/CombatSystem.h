#pragma once

#include "EnemySystem.h"
#include "InstancedBatch.h"
#include "Mesh.h"
#include "Player.h"
#include "Shader.h"
#include "World.h"

#include <glm/glm.hpp>

#include <memory>
#include <vector>

class BossSystem;

struct CombatSettings {
    float swingRange      = 2.3f;
    float swingHalfAngle  = 75.0f;
    int   swingDamage     = 1;
    float swingKnockback  = 8.0f;
    float orbCooldown     = 0.8f;
    float orbSpeed        = 18.0f;
    float orbLifetime     = 1.1f;
    float orbRadius       = 0.25f;
    int   orbDamage       = 1;
    float orbKnockback    = 5.0f;
    float healDropLifetime = 30.0f;
    float shakeOnHit      = 0.06f;
    float shakeOnHurt     = 0.18f;
};

class CombatSystem {
public:
    explicit CombatSystem(const CombatSettings& settings = {});

    CombatSettings settings;

    void update(Player& player, EnemySystem& enemies, BossSystem* boss, const World& world, bool swingPressed,
                bool orbPressed, float aimYaw, float deltaTime);

    float takeShake() { const float s = m_shake; m_shake = 0.0f; return s; }

    void draw(const Shader& gemShader, float time) const;
    void drawSwing(const Shader& gemShader, const Player& player) const;

private:
    struct Orb { glm::vec3 position, velocity; float age = 0.0f; };
    struct Particle { glm::vec3 position, velocity; float age = 0.0f, life = 0.5f, size = 0.1f; int color = 0; };
    struct HealDrop { glm::vec3 position; float age = 0.0f; };

    void burst(const glm::vec3& at, int color, int count, float speed);
    void swingHits(const Player& player, EnemySystem& enemies, BossSystem* boss);

    std::vector<Orb>      m_orbs;
    std::vector<Particle> m_particles;
    std::vector<HealDrop> m_drops;
    float m_orbCooldown = 0.0f;
    float m_shake = 0.0f;
    unsigned m_noise = 12345u;

    std::vector<std::unique_ptr<InstancedBatch>> m_particleBatches;
    std::unique_ptr<InstancedBatch> m_orbBatch;
    std::unique_ptr<InstancedBatch> m_dropBatch;
    std::unique_ptr<InstancedBatch> m_arcBatch;
};

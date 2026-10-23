#pragma once

#include "InstancedBatch.h"
#include "Mesh.h"
#include "Shader.h"
#include "World.h"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <vector>

struct PlayerSettings {
    float walkSpeed     = 4.5f;
    float sprintSpeed   = 8.5f;
    float acceleration  = 30.0f;
    float turnSpeed     = 12.0f;
    float jumpStrength  = 7.5f;
    float gravity       = 22.0f;
    float respawnMargin = 5.0f;
};

struct GlideSettings {
    float fallSpeed     = 2.5f;
    float forwardSpeed  = 10.0f;
    float turnSpeed     = 2.5f;
    float acceleration  = 12.0f;
    float minHeight     = 1.5f;
};

struct HealthSettings {
    int   baseHearts       = 5;
    float invulnerableTime = 1.2f;
    float knockbackSpeed   = 9.0f;
    float knockbackUp      = 4.5f;
    float regenDelay       = 6.0f;
    float regenInterval    = 3.0f;
    float respawnSafety    = 3.5f;
};

struct SwingSettings {
    float duration = 0.32f;
    float cooldown = 0.45f;
    float hitMoment = 0.35f;
};

struct PlayerRewards {
    bool glideTrail   = false;
    bool sakuraGlider = false;
    bool swiftBoots   = false;
    bool crystalCrown = false;
    int  extraHearts  = 0;
};

struct PlayerInput {
    glm::vec2 move{0.0f};
    bool sprint = false;
    bool jump   = false;
    bool glide  = false;
};

class Player {
public:
    explicit Player(const PlayerSettings& settings = {}, const GlideSettings& glide = {});

    PlayerSettings settings;
    GlideSettings  glide;
    HealthSettings health;
    SwingSettings  swing;

    int  hearts() const { return m_hearts; }
    int  maxHearts() const { return health.baseHearts + m_rewards.extraHearts; }
    bool isDead() const { return m_hearts <= 0; }
    bool isInvulnerable() const { return m_invulnerable > 0.0f; }
    bool isSafe() const { return m_safety > 0.0f; }
    bool takeHit(int damage, const glm::vec3& from);
    void heal(int amount) { m_hearts = std::min(maxHearts(), m_hearts + amount); }
    void revive(const glm::vec3& position, float facing, int island);

    bool startSwing();
    float swingProgress() const;
    bool swingHitsNow() const { return m_swingHitNow; }
    void face(float facing) { m_facing = facing; }

    void update(const PlayerInput& input, float cameraYaw, const World& world, float deltaTime);

    void draw(const Shader& shader) const;
    void drawEffects(const Shader& gemShader) const;

    void applyReward(const std::string& rewardId);
    const PlayerRewards& rewards() const { return m_rewards; }
    void setExtraHearts(int extra) { m_rewards.extraHearts = std::max(0, extra); m_hearts = maxHearts(); }

    void drawShadow(const Shader& blobShader, const World& world) const;

    void respawn(const World& world, int island = -1);

    void teleport(const glm::vec3& position, float facing, int island);

    void pushOutOfCircle(const glm::vec2& center, float radius);

    const glm::vec3& getPosition() const { return m_position; }
    float getFacing() const { return m_facing; }
    bool isGliding() const { return m_gliding; }
    int getRespawnCount() const { return m_respawnCount; }
    int getLastIsland() const { return m_lastIsland; }

private:
    void updateGroundMovement(const glm::vec3& wishDirection, bool sprint, float deltaTime);
    void updateGlideMovement(const glm::vec3& wishDirection, float deltaTime);
    void updateAnimation(float deltaTime);
    void updateParticles(bool sprinting, float deltaTime);

    glm::vec3 m_position{0.0f};
    glm::vec3 m_velocity{0.0f};
    float     m_facing = 0.0f;
    bool      m_grounded = false;
    bool      m_gliding  = false;
    int       m_lastIsland = World::MAIN_ISLAND;
    int       m_respawnCount = 0;

    int   m_hearts = 5;
    float m_invulnerable = 0.0f;
    float m_safety = 0.0f;
    float m_sinceHit = 100.0f;
    float m_regenTimer = 0.0f;
    float m_swingTime = -1.0f;
    float m_swingCooldown = 0.0f;
    bool  m_swingHitNow = false;

    float m_walkPhase   = 0.0f;
    float m_walkAmount  = 0.0f;
    float m_glideAmount = 0.0f;

    Mesh m_body;
    Mesh m_head;
    Mesh m_hair;
    Mesh m_eye;
    Mesh m_scarf;
    Mesh m_scarfTail;
    Mesh m_arm;
    Mesh m_glider;
    Mesh m_crown;
    Mesh m_staff;
    Mesh m_shadowQuad;

    PlayerRewards m_rewards;
    struct Particle {
        glm::vec3 position, velocity;
        float age = 0.0f, life = 1.0f, size = 0.1f;
        bool  gold = false;
    };
    std::vector<Particle> m_particles;
    float m_spawnTimer = 0.0f;
    std::unique_ptr<InstancedBatch> m_trailBatch;
    std::unique_ptr<InstancedBatch> m_sparkleBatch;
};

#pragma once

#include "EventBus.h"
#include "InstancedBatch.h"
#include "Mesh.h"
#include "Player.h"
#include "PointLight.h"
#include "Shader.h"
#include "UIRenderer.h"
#include "World.h"

#include <glm/glm.hpp>

#include <memory>
#include <random>
#include <string>
#include <vector>

struct BossSettings {
    std::string name     = "Korvath, Guardian of the Cave Heart";
    int   health         = 30;
    float phase2At       = 0.5f;
    float phase2Speed    = 1.35f;
    float radius         = 1.5f;
    float moveSpeed      = 1.7f;
    float idleTime       = 1.5f;
    float introTime      = 2.8f;
    int   damage         = 1;

    float slamWindup     = 1.1f;
    float shockwaveSpeed = 6.5f;
    float shockwaveHeight = 0.55f;
    float shockwaveWidth = 0.45f;

    float chargeWindup   = 1.1f;
    float chargeLock     = 0.3f;
    float chargeSpeed    = 13.0f;
    float stunTime       = 2.4f;
    float stunDamageBonus = 2.0f;

    int   rainCount      = 4;
    float rainWindup     = 1.4f;
    float rainRadius     = 1.4f;
    float rainFallTime   = 0.35f;
};

class BossSystem {
public:
    enum class State { Dormant, Intro, Idle, SlamWindup, Slam, ChargeWindup, Charge, Stunned,
                       RainWindup, Rain, PhaseShift, Dying, Defeated };

    BossSystem(const World& world, EventBus& events, const BossSettings& settings = {});

    BossSettings settings;

    void generate(const World& world);

    void update(Player& player, World& world, bool questActive, bool targetable, float deltaTime);

    void onPlayerDefeated(World& world);

    void restoreDefeated();

    State state() const { return m_state; }
    bool isIntro() const { return m_state == State::Intro; }
    bool isFighting() const { return m_state != State::Dormant && m_state != State::Defeated; }
    bool isDefeated() const { return m_state == State::Defeated; }
    bool isPhase2() const { return m_phase2; }
    glm::vec3 center() const { return m_position + glm::vec3(0.0f, 1.6f, 0.0f); }
    float radius() const { return settings.radius; }
    bool isHittable() const;
    bool damage(int amount, const glm::vec3& from);
    float takeShake() { const float s = m_shake; m_shake = 0.0f; return s; }

    float lightBrightness() const { return m_moodBrightness; }
    float lightRed() const { return m_moodRed; }
    void appendLights(std::vector<PointLight>& lights) const;

    void draw(const Shader& objectShader, float time) const;
    void drawGlow(const Shader& gemShader, float time) const;
    void drawFloorMarkers(const Shader& blobShader, float time) const;
    void drawTransparent(const Shader& gemShader, float time) const;
    void drawHud(UIRenderer& ui, float time) const;
    bool hudVisible() const { return isFighting() && m_state != State::Intro; }

private:
    struct Shockwave { float radius = 0.0f; bool hitPlayer = false; float delay = 0.0f; };
    struct Drop { glm::vec3 spot; float warning; float fall = 0.0f; bool landed = false; };
    struct Shard { glm::vec3 position, velocity; float age = 0.0f, life = 0.8f, size = 0.12f; int kind = 0; };

    void enter(State state, float time);
    void chooseAttack(const glm::vec3& playerPosition);
    void burst(const glm::vec3& at, int kind, int count, float speed);
    void clampToArena(bool charging, World& world);
    void setSealed(World& world, bool sealed);
    float floorAt(float x, float z) const;
    float speedScale() const { return m_phase2 ? settings.phase2Speed : 1.0f; }
    glm::vec3 playerFeet() const { return m_playerPosition; }

    EventBus& m_events;
    const CaveIsland* m_cave = nullptr;
    int m_caveIsland = -1;
    std::mt19937 m_rng{42u};

    State     m_state = State::Dormant;
    float     m_timer = 0.0f;
    float     m_stateTime = 0.0f;
    glm::vec3 m_home{0.0f};
    glm::vec3 m_position{0.0f};
    float     m_yaw = 0.0f;
    float     m_restYaw = 0.0f;
    int       m_health = 24;
    bool      m_phase2 = false;
    float     m_flash = 0.0f;
    float     m_glow = 0.0f;
    float     m_shake = 0.0f;
    State     m_lastAttack = State::Idle;
    glm::vec3 m_playerPosition{0.0f};

    std::vector<Shockwave> m_waves;
    std::vector<Drop>      m_drops;
    std::vector<Shard>     m_shards;
    glm::vec2 m_chargeDirection{0.0f, 1.0f};
    bool      m_chargeHit = false;
    float     m_barrier = 0.0f;
    float     m_moodBrightness = 1.0f, m_moodRed = 0.0f;

    Mesh m_body, m_arm, m_quad;
    std::unique_ptr<InstancedBatch> m_crystals;
    std::unique_ptr<InstancedBatch> m_barrierWall;
    std::unique_ptr<InstancedBatch> m_seal;
    std::unique_ptr<InstancedBatch> m_crystalShards;
    std::unique_ptr<InstancedBatch> m_goldShards;
    std::unique_ptr<InstancedBatch> m_rockShards;
    std::unique_ptr<InstancedBatch> m_rainCrystals;
};

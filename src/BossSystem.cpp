#include "BossSystem.h"
#include "Lighting.h"
#include "MathUtils.h"
#include "MeshBuilder.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace {

const glm::vec3 UP(0.0f, 1.0f, 0.0f);
const glm::mat4 I(1.0f);
const Material ROCK_MATERIAL{0.10f, 16.0f, 0.35f};
const glm::vec3 CRYSTAL_CYAN{0.40f, 0.90f, 1.00f};
const glm::vec4 RED_TINT{1.00f, 0.22f, 0.18f, 0.65f};
const glm::vec4 STONE_TINT{0.42f, 0.44f, 0.50f, 0.9f};
const glm::vec3 SHOULDER(1.25f, 2.15f, 0.0f);

glm::mat4 T(float x, float y, float z) { return glm::translate(I, glm::vec3(x, y, z)); }
glm::mat4 S(float x, float y, float z) { return glm::scale(I, glm::vec3(x, y, z)); }
float smoothFactor(float speed, float deltaTime) { return 1.0f - std::exp(-speed * deltaTime); }
glm::mat4 tilt(float dx, float dz, float degrees)
{
    const glm::vec3 axis = glm::cross(UP, glm::vec3(dx, 0.0f, dz));
    return glm::length(axis) < 1e-5f ? I : glm::rotate(I, glm::radians(degrees), glm::normalize(axis));
}

Mesh buildBody()
{
    MeshBuilder b;
    const glm::vec3 rock{0.50f, 0.52f, 0.62f}, rockLight{0.58f, 0.59f, 0.68f}, moss{0.34f, 0.50f, 0.38f};
    for (float side : {-1.0f, 1.0f})
        b.addRock(T(0.55f * side, 0.45f, 0.0f) * S(0.45f, 0.5f, 0.5f), rock, 3.0f + side);
    b.addRock(T(0.0f, 1.65f, 0.0f) * S(1.15f, 1.0f, 0.9f), rock, 7.0f);
    b.addRock(T(0.0f, 2.2f, -0.2f) * S(0.9f, 0.35f, 0.65f), moss, 9.0f);
    b.addRock(T(0.0f, 2.75f, 0.15f) * S(0.55f, 0.48f, 0.5f), rockLight, 11.0f);
    b.addRock(T(0.0f, 2.55f, 0.48f) * S(0.42f, 0.12f, 0.15f), rock, 13.0f);
    return b.build();
}

Mesh buildArm()
{
    MeshBuilder b;
    const glm::vec3 rock{0.52f, 0.54f, 0.64f};
    b.addRock(T(0.0f, -0.55f, 0.0f) * S(0.32f, 0.6f, 0.32f), rock, 2.0f);
    b.addRock(T(0.0f, -1.25f, 0.05f) * S(0.48f, 0.42f, 0.48f), rock, 4.0f);
    return b.build();
}

Mesh buildCrystals()
{
    MeshBuilder b;
    const glm::vec3 a = CRYSTAL_CYAN, c{0.55f, 0.70f, 1.00f};
    const float spine[5][4] = {{-0.55f, 1.9f, 0.9f, 25.0f}, {0.0f, 2.25f, 1.3f, 15.0f}, {0.55f, 1.95f, 1.0f, 25.0f},
                               {-0.3f, 1.45f, 0.7f, 35.0f}, {0.35f, 1.4f, 0.75f, 35.0f}};
    for (const auto& s : spine) {
        const glm::vec2 out = glm::normalize(glm::vec2(s[0], -1.0f));
        b.addPrism(T(s[0], s[1], -0.65f) * tilt(out.x, out.y, s[3]), 6, 0.16f, 0.14f, s[2], 0.35f, s[0] > 0 ? a : c);
    }
    for (float side : {-1.0f, 1.0f})
        b.addPrism(T(1.05f * side, 2.35f, -0.05f) * tilt(side, 0.0f, 30.0f), 6, 0.14f, 0.12f, 0.5f, 0.25f, a);
    for (float side : {-1.0f, 1.0f})
        b.addSphere(T(0.18f * side, 2.82f, 0.6f) * S(0.09f, 0.06f, 0.05f), {0.85f, 1.0f, 1.0f}, 8, 5);
    return b.build();
}

Mesh crystalShape(const glm::vec3& color) { return Mesh::createOctahedron(1.0f, 1.6f, color); }

}

BossSystem::BossSystem(const World& world, EventBus& events, const BossSettings& settings)
    : settings(settings), m_events(events), m_body(buildBody()), m_arm(buildArm()), m_quad(Mesh::createGroundQuad())
{
    m_crystals = std::make_unique<InstancedBatch>(buildCrystals());
    MeshBuilder seal;
    seal.addSphere(I, {0.45f, 0.85f, 1.0f}, 24, 14);
    m_seal = std::make_unique<InstancedBatch>(seal.build());
    m_crystalShards = std::make_unique<InstancedBatch>(crystalShape(CRYSTAL_CYAN));
    m_goldShards = std::make_unique<InstancedBatch>(crystalShape({1.0f, 0.85f, 0.35f}));
    m_rockShards = std::make_unique<InstancedBatch>(crystalShape({0.42f, 0.42f, 0.50f}));
    MeshBuilder rain;
    rain.addPrism(I, 6, 0.28f, 0.2f, 0.9f, 0.6f, CRYSTAL_CYAN);
    m_rainCrystals = std::make_unique<InstancedBatch>(rain.build());
    m_barrierWall = std::make_unique<InstancedBatch>(crystalShape(CRYSTAL_CYAN));
    generate(world);
}

void BossSystem::generate(const World& world)
{
    m_cave = world.cave();
    m_caveIsland = m_cave ? m_cave->islandIndex() : -1;
    m_waves.clear();
    m_drops.clear();
    m_shards.clear();
    m_state = State::Dormant;
    m_health = settings.health;
    m_phase2 = false;
    m_glow = 0.0f;
    m_barrier = 0.0f;
    if (!m_cave)
        return;

    const glm::vec3 c = m_cave->chamberFloorCenter();
    m_home = {c.x, floorAt(c.x, c.z), c.z};
    m_position = m_home;
    m_restYaw = m_yaw = std::atan2(m_cave->exitDirection().x, m_cave->exitDirection().y);

    const glm::vec2 out = m_cave->exitDirection();
    const glm::vec2 side(-out.y, out.x);
    MeshBuilder wall;
    for (int i = 0; i < 5; ++i) {
        const float t = -1.9f + i * 0.95f;
        const float h = 2.4f + 0.5f * std::sin(i * 2.1f);
        wall.addPrism(T(side.x * t, 0.0f, side.y * t) * tilt(-out.x, -out.y, 10.0f), 6, 0.42f, 0.36f, h, 0.6f,
                      i % 2 ? CRYSTAL_CYAN : glm::vec3(0.6f, 0.75f, 1.0f));
    }
    m_barrierWall = std::make_unique<InstancedBatch>(wall.build());
}

float BossSystem::floorAt(float x, float z) const
{
    return m_cave ? m_cave->chamberFloorAt(x, z) : 0.0f;
}

bool BossSystem::isHittable() const
{
    switch (m_state) {
    case State::Idle: case State::SlamWindup: case State::Slam: case State::ChargeWindup: case State::Charge:
    case State::Stunned: case State::RainWindup: case State::Rain:
        return true;
    default:
        return false;
    }
}

void BossSystem::enter(State state, float time)
{
    m_state = state;
    m_timer = time;
    m_stateTime = 0.0f;
}

void BossSystem::setSealed(World& world, bool sealed)
{
    world.setCaveSealed(sealed);
}

void BossSystem::burst(const glm::vec3& at, int kind, int count, float speed)
{
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    for (int i = 0; i < count; ++i) {
        const float a = unit(m_rng) * glm::two_pi<float>();
        const glm::vec3 dir = glm::normalize(glm::vec3(std::cos(a), 0.3f + unit(m_rng), std::sin(a)));
        m_shards.push_back({at, dir * speed * (0.5f + unit(m_rng)), 0.0f, 0.6f + 0.5f * unit(m_rng),
                            0.08f + 0.1f * unit(m_rng), kind});
    }
}

bool BossSystem::damage(int amount, const glm::vec3& from)
{
    (void)from;
    if (!isHittable())
        return false;
    const float bonus = m_state == State::Stunned ? settings.stunDamageBonus : 1.0f;
    m_health -= static_cast<int>(std::round(amount * bonus));
    m_flash = 1.0f;
    burst(center(), 0, 6, 4.0f);

    if (m_health <= 0) {
        m_health = 0;
        enter(State::Dying, 2.6f);
        m_waves.clear();
        m_drops.clear();
        return true;
    }
    if (!m_phase2 && m_health <= settings.health * settings.phase2At) {
        m_phase2 = true;
        enter(State::PhaseShift, 1.8f);
        m_waves.clear();
        m_drops.clear();
    }
    return true;
}

void BossSystem::onPlayerDefeated(World& world)
{
    m_waves.clear();
    m_drops.clear();
    m_state = State::Dormant;
    m_health = settings.health;
    m_phase2 = false;
    m_position = m_home;
    setSealed(world, false);
}

void BossSystem::restoreDefeated()
{
    m_waves.clear();
    m_drops.clear();
    m_shards.clear();
    m_state = State::Defeated;
    m_glow = 0.0f;
    m_barrier = 0.0f;
    m_moodBrightness = 1.3f;
    m_moodRed = 0.0f;
}

void BossSystem::chooseAttack(const glm::vec3& p)
{
    const float distance = glm::length(glm::vec2(p.x - m_position.x, p.z - m_position.z));
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    State choice;
    if (distance > 5.5f)      choice = unit(m_rng) < 0.6f ? State::ChargeWindup : State::RainWindup;
    else if (distance < 3.5f) choice = unit(m_rng) < 0.65f ? State::SlamWindup : State::RainWindup;
    else {
        const float r = unit(m_rng);
        choice = r < 0.34f ? State::SlamWindup : (r < 0.67f ? State::ChargeWindup : State::RainWindup);
    }
    if (choice == m_lastAttack)
        choice = choice == State::SlamWindup ? State::ChargeWindup
               : (choice == State::ChargeWindup ? State::RainWindup : State::SlamWindup);
    m_lastAttack = choice;

    const float k = 1.0f / speedScale();
    if (choice == State::SlamWindup) {
        enter(choice, settings.slamWindup * k);
    } else if (choice == State::ChargeWindup) {
        enter(choice, settings.chargeWindup * k);
    } else {
        enter(choice, settings.rainWindup * k);
        m_drops.clear();
        const int count = settings.rainCount + (m_phase2 ? 2 : 0);
        const glm::vec3 c = m_cave->chamberFloorCenter();
        m_drops.push_back({{p.x, floorAt(p.x, p.z), p.z}, settings.rainWindup * k});
        for (int i = 1; i < count; ++i) {
            const float a = unit(m_rng) * glm::two_pi<float>();
            const float r = (m_cave->chamberRadius() - 2.0f) * std::sqrt(unit(m_rng));
            const glm::vec2 q(c.x + std::cos(a) * r, c.z + std::sin(a) * r);
            m_drops.push_back({{q.x, floorAt(q.x, q.y), q.y}, (settings.rainWindup + 0.25f * i) * k});
        }
    }
}

void BossSystem::clampToArena(bool charging, World& world)
{
    (void)world;
    const glm::vec3 c = m_cave->chamberFloorCenter();
    glm::vec2 offset(m_position.x - c.x, m_position.z - c.z);
    const float limit = m_cave->chamberRadius() - settings.radius - 0.6f;
    bool crashed = false;
    if (glm::length(offset) > limit) {
        offset = glm::normalize(offset) * limit;
        m_position.x = c.x + offset.x;
        m_position.z = c.z + offset.y;
        crashed = true;
    }
    for (const Collider& col : world.colliders()) {
        if (col.island != m_caveIsland || glm::length(col.center - glm::vec2(c.x, c.z)) > m_cave->chamberRadius() + 1.0f)
            continue;
        const glm::vec2 d(m_position.x - col.center.x, m_position.z - col.center.y);
        const float minimum = col.radius + settings.radius;
        if (glm::length(d) < minimum && glm::length(d) > 1e-4f) {
            const glm::vec2 pushed = col.center + glm::normalize(d) * minimum;
            m_position.x = pushed.x;
            m_position.z = pushed.y;
            crashed = true;
        }
    }
    if (charging && crashed) {
        enter(State::Stunned, settings.stunTime);
        m_shake = std::max(m_shake, 0.35f);
        burst(center() + glm::vec3(m_chargeDirection.x, 0.0f, m_chargeDirection.y) * settings.radius, 2, 18, 5.0f);
    }
}

void BossSystem::update(Player& player, World& world, bool questActive, bool targetable, float deltaTime)
{
    if (!m_cave)
        return;
    const BossSettings& s = settings;
    const glm::vec3 p = player.getPosition();
    m_playerPosition = p;
    m_timer -= deltaTime;
    m_stateTime += deltaTime;
    m_flash = std::max(0.0f, m_flash - deltaTime * 4.0f);
    const float dt = deltaTime * speedScale();
    const glm::vec3 chamber = m_cave->chamberFloorCenter();
    const float playerDistanceToCenter = glm::length(glm::vec2(p.x - chamber.x, p.z - chamber.z));
    const bool playerInArena = playerDistanceToCenter < m_cave->chamberRadius() + 0.5f
                            && p.y > chamber.y - 1.0f && p.y < chamber.y + 6.0f;
    const glm::vec2 toPlayer(p.x - m_position.x, p.z - m_position.z);
    const float playerDistance = glm::length(toPlayer);
    const glm::vec2 dirToPlayer = playerDistance > 1e-3f ? toPlayer / playerDistance : glm::vec2(0.0f, 1.0f);
    auto turnTo = [&](const glm::vec2& d, float speed) {
        m_yaw += shortestAngle(m_yaw, std::atan2(d.x, d.y)) * smoothFactor(speed, deltaTime);
    };

    switch (m_state) {
    case State::Dormant:
        m_glow = std::max(0.0f, m_glow - deltaTime);
        m_yaw += shortestAngle(m_yaw, m_restYaw) * smoothFactor(2.0f, deltaTime);
        if (questActive && !player.isDead() && playerDistanceToCenter < m_cave->chamberRadius() - 2.5f && playerInArena) {
            enter(State::Intro, s.introTime);
            setSealed(world, true);
            m_shake = 0.25f;
        }
        break;

    case State::Intro:
        m_glow = std::min(1.0f, m_stateTime / (s.introTime * 0.7f));
        turnTo(dirToPlayer, 2.0f);
        if (std::fmod(m_stateTime, 0.5f) < deltaTime)
            m_shake = std::max(m_shake, 0.08f);
        if (m_timer <= 0.0f)
            enter(State::Idle, s.idleTime);
        break;

    case State::Idle:
        turnTo(dirToPlayer, 4.0f);
        if (playerDistance > 3.2f) {
            m_position.x += dirToPlayer.x * s.moveSpeed * dt;
            m_position.z += dirToPlayer.y * s.moveSpeed * dt;
        }
        if (m_timer <= 0.0f && targetable)
            chooseAttack(p);
        break;

    case State::SlamWindup:
        turnTo(dirToPlayer, 3.0f);
        m_shake = std::max(m_shake, 0.03f);
        if (m_timer <= 0.0f) {
            enter(State::Slam, 0.9f / speedScale());
            m_waves.push_back({});
            if (m_phase2)
                m_waves.push_back({0.0f, false, 0.55f});
            m_shake = std::max(m_shake, 0.4f);
            burst(m_position + glm::vec3(0.0f, 0.2f, 0.0f), 2, 20, 6.0f);
        }
        break;

    case State::Slam:
        if (m_timer <= 0.0f)
            enter(State::Idle, s.idleTime / speedScale());
        break;

    case State::ChargeWindup: {
        const float k = 1.0f / speedScale();
        if (m_timer > s.chargeLock * k) {
            m_chargeDirection = dirToPlayer;
            turnTo(dirToPlayer, 8.0f);
        }
        m_shake = std::max(m_shake, 0.02f);
        if (m_timer <= 0.0f) {
            enter(State::Charge, 3.0f);
            m_chargeHit = false;
        }
        break;
    }
    case State::Charge:
        m_position.x += m_chargeDirection.x * s.chargeSpeed * speedScale() * deltaTime;
        m_position.z += m_chargeDirection.y * s.chargeSpeed * speedScale() * deltaTime;
        if (!m_chargeHit && targetable && playerDistance < s.radius + 0.6f) {
            m_chargeHit = true;
            if (player.takeHit(s.damage, m_position))
                m_shake = std::max(m_shake, 0.25f);
        }
        if (std::fmod(m_stateTime, 0.05f) < deltaTime)
            burst(m_position + glm::vec3(0.0f, 0.1f, 0.0f), 2, 2, 2.0f);
        clampToArena(true, world);
        if (m_state == State::Charge && m_timer <= 0.0f)
            enter(State::Idle, s.idleTime);
        break;

    case State::Stunned:
        if (std::fmod(m_stateTime, 0.15f) < deltaTime)
            m_shards.push_back({center() + glm::vec3(std::cos(m_stateTime * 6.0f), 1.5f, std::sin(m_stateTime * 6.0f)) * 0.8f,
                                glm::vec3(0.0f, 0.5f, 0.0f), 0.0f, 0.5f, 0.12f, 1});
        if (m_timer <= 0.0f)
            enter(State::Idle, s.idleTime * 0.5f);
        break;

    case State::RainWindup:
    case State::Rain: {
        bool allDone = true;
        for (Drop& d : m_drops) {
            d.warning -= deltaTime;
            if (d.warning > 0.0f) { allDone = false; continue; }
            if (d.landed) continue;
            d.fall += deltaTime;
            allDone = false;
            if (d.fall >= s.rainFallTime) {
                d.landed = true;
                burst(d.spot + glm::vec3(0.0f, 0.3f, 0.0f), 0, 12, 4.5f);
                m_shake = std::max(m_shake, 0.12f);
                const float dist = glm::length(glm::vec2(p.x - d.spot.x, p.z - d.spot.z));
                if (targetable && dist < s.rainRadius && p.y - d.spot.y < 2.0f)
                    player.takeHit(s.damage, d.spot);
            }
        }
        if (m_state == State::RainWindup && m_timer <= 0.0f)
            m_state = State::Rain;
        if (allDone) {
            m_drops.clear();
            enter(State::Idle, s.idleTime / speedScale());
        }
        break;
    }
    case State::PhaseShift:
        m_shake = std::max(m_shake, 0.12f);
        if (std::fmod(m_stateTime, 0.2f) < deltaTime)
            burst(center() + glm::vec3(0.0f, 0.6f, -0.5f), 0, 4, 5.0f);
        if (m_timer <= 0.0f)
            enter(State::Idle, 0.6f);
        break;

    case State::Dying:
        m_shake = std::max(m_shake, 0.1f);
        if (std::fmod(m_stateTime, 0.22f) < deltaTime) {
            std::uniform_real_distribution<float> unit(-1.0f, 1.0f);
            burst(center() + glm::vec3(unit(m_rng), unit(m_rng) + 0.5f, unit(m_rng)), m_stateTime > 1.0f ? 2 : 0, 8, 4.0f);
        }
        if (m_timer <= 0.0f) {
            burst(center(), 0, 50, 8.0f);
            burst(center(), 2, 40, 6.0f);
            m_shake = 0.6f;
            m_state = State::Defeated;
            setSealed(world, false);
            m_events.emit({EventType::BossDefeated, "guardian"});
        }
        break;

    case State::Defeated:
        break;
    }

    for (Shockwave& w : m_waves) {
        if (w.delay > 0.0f) { w.delay -= deltaTime; continue; }
        w.radius += s.shockwaveSpeed * speedScale() * deltaTime;
        const float d = glm::length(glm::vec2(p.x - m_position.x, p.z - m_position.z));
        const float feet = p.y - floorAt(p.x, p.z);
        if (!w.hitPlayer && targetable && std::abs(d - w.radius) < s.shockwaveWidth + 0.35f && feet < s.shockwaveHeight) {
            w.hitPlayer = true;
            player.takeHit(s.damage, m_position);
        }
    }
    m_waves.erase(std::remove_if(m_waves.begin(), m_waves.end(),
                                 [this](const Shockwave& w) { return w.radius > m_cave->chamberRadius() + 2.0f; }),
                  m_waves.end());

    if (m_state != State::Charge)
        clampToArena(false, world);
    m_position.y = floorAt(m_position.x, m_position.z);
    if (m_state != State::Defeated && std::abs(p.y - m_position.y) < 3.0f)
        player.pushOutOfCircle({m_position.x, m_position.z}, s.radius);

    for (Shard& sh : m_shards) {
        sh.age += deltaTime;
        if (sh.kind != 1)
            sh.velocity.y -= 9.0f * deltaTime;
        sh.position += sh.velocity * deltaTime;
    }
    m_shards.erase(std::remove_if(m_shards.begin(), m_shards.end(), [](const Shard& sh) { return sh.age >= sh.life; }),
                   m_shards.end());

    const bool sealedNow = m_cave->isSealed();
    m_barrier += ((sealedNow ? 1.0f : 0.0f) - m_barrier) * smoothFactor(4.0f, deltaTime);
    float brightness = 1.0f, red = 0.0f;
    if (m_state == State::Defeated)      { brightness = 1.3f; }
    else if (isFighting() && m_phase2)   { brightness = 0.8f; red = 0.75f; }
    else if (isFighting())               { brightness = 0.5f; }
    m_moodBrightness += (brightness - m_moodBrightness) * smoothFactor(1.5f, deltaTime);
    m_moodRed += (red - m_moodRed) * smoothFactor(1.5f, deltaTime);
}

void BossSystem::appendLights(std::vector<PointLight>& lights) const
{
    if (!m_cave || m_glow <= 0.01f || m_state == State::Defeated)
        return;
    PointLight light;
    light.position = center() + glm::vec3(0.0f, 0.8f, 0.0f);
    light.color = m_phase2 ? glm::vec3(1.0f, 0.3f, 0.2f) : CRYSTAL_CYAN;
    light.intensity = 1.6f * m_glow * (1.0f + 0.5f * m_flash);
    light.radius = 9.0f;
    lights.push_back(light);
}

void BossSystem::draw(const Shader& objectShader, float time) const
{
    if (!m_cave || m_state == State::Defeated)
        return;
    float armAngle = -12.0f + 4.0f * std::sin(time * 1.5f);
    float lean = 0.0f, wobble = 0.0f, crouch = 1.0f, scale = 1.0f, sink = 0.0f;
    glm::vec3 jitter(0.0f);
    glm::vec4 flash(1.0f, 1.0f, 1.0f, m_flash * 0.8f);
    const float k = 1.0f / speedScale();
    switch (m_state) {
    case State::Dormant: armAngle = -8.0f; crouch = 0.92f; break;
    case State::Intro: {
        const float t = std::min(1.0f, m_stateTime / settings.introTime);
        crouch = 0.92f + 0.08f * t;
        armAngle = -8.0f - 100.0f * std::sin(glm::pi<float>() * t);
        break;
    }
    case State::SlamWindup: {
        const float t = 1.0f - std::max(m_timer, 0.0f) / (settings.slamWindup * k);
        armAngle = -12.0f - 155.0f * (1.0f - (1.0f - t) * (1.0f - t));
        lean = -10.0f * t;
        jitter = glm::vec3(std::sin(time * 50.0f), 0.0f, std::cos(time * 43.0f)) * 0.03f * t;
        flash = glm::max(flash, glm::vec4(0.6f, 0.95f, 1.0f, 0.35f * t * (0.6f + 0.4f * std::sin(time * 20.0f))));
        break;
    }
    case State::Slam: armAngle = glm::mix(-167.0f, -55.0f, std::min(1.0f, m_stateTime / 0.12f)); lean = 15.0f; break;
    case State::ChargeWindup: {
        const float t = 1.0f - std::max(m_timer, 0.0f) / (settings.chargeWindup * k);
        lean = 22.0f * t;
        armAngle = 35.0f * t;
        crouch = 1.0f - 0.12f * t;
        jitter = glm::vec3(std::sin(time * 60.0f), 0.0f, 0.0f) * 0.04f * t;
        flash = glm::max(flash, glm::vec4(1.0f, 0.3f, 0.2f, 0.35f * t * (0.5f + 0.5f * std::sin(time * 25.0f))));
        break;
    }
    case State::Charge: lean = 28.0f; armAngle = 45.0f; break;
    case State::Stunned: wobble = 12.0f * std::sin(time * 6.0f); armAngle = 5.0f; lean = -8.0f; break;
    case State::RainWindup: case State::Rain: {
        armAngle = -175.0f;
        flash = glm::max(flash, glm::vec4(0.6f, 0.95f, 1.0f, 0.2f + 0.15f * std::sin(time * 15.0f)));
        break;
    }
    case State::PhaseShift:
        armAngle = -165.0f;
        jitter = glm::vec3(std::sin(time * 70.0f), 0.0f, std::cos(time * 61.0f)) * 0.06f;
        flash = glm::vec4(1.0f, 0.25f, 0.2f, 0.35f + 0.25f * std::sin(time * 30.0f));
        break;
    case State::Dying: {
        const float t = 1.0f - std::max(m_timer, 0.0f) / 2.6f;
        jitter = glm::vec3(std::sin(time * 80.0f), 0.0f, std::cos(time * 71.0f)) * 0.08f;
        sink = 0.6f * t * t;
        scale = 1.0f - 0.3f * t * t;
        armAngle = -20.0f + 30.0f * t;
        flash = glm::vec4(1.0f, 1.0f, 1.0f, 0.2f + 0.5f * t * (0.5f + 0.5f * std::sin(time * 25.0f)));
        break;
    }
    default: break;
    }

    glm::mat4 base = glm::translate(I, m_position + jitter - glm::vec3(0.0f, 0.05f + sink, 0.0f));
    base = glm::rotate(base, m_yaw, UP);
    base = glm::rotate(base, glm::radians(lean), glm::vec3(1.0f, 0.0f, 0.0f));
    base = glm::rotate(base, glm::radians(wobble), glm::vec3(0.0f, 0.0f, 1.0f));
    base = glm::scale(base, glm::vec3(scale, scale * crouch, scale));

    Lighting::applyMaterial(objectShader, ROCK_MATERIAL);
    objectShader.setVec4("uFlash", flash);
    objectShader.setMat4("uModel", base);
    m_body.draw();
    for (float side : {-1.0f, 1.0f}) {
        glm::mat4 arm = glm::translate(base, glm::vec3(SHOULDER.x * side, SHOULDER.y, SHOULDER.z));
        arm = glm::rotate(arm, glm::radians(armAngle), glm::vec3(1.0f, 0.0f, 0.0f));
        arm = glm::rotate(arm, glm::radians(8.0f * side), glm::vec3(0.0f, 0.0f, 1.0f));
        objectShader.setMat4("uModel", arm);
        m_arm.draw();
    }
    objectShader.setVec4("uFlash", glm::vec4(0.0f));

    m_crystals->setInstances({base}, true);
}

void BossSystem::drawGlow(const Shader& gemShader, float time) const
{
    if (!m_cave)
        return;
    gemShader.setInt("uRainbow", 0);
    gemShader.setFloat("uAlpha", 1.0f);
    glm::vec4 tint = glm::mix(STONE_TINT, glm::vec4(0.0f), m_glow);
    if (m_phase2)
        tint = RED_TINT;
    if (m_state != State::Defeated) {
        gemShader.setVec4("uTint", tint);
        m_crystals->draw();
    }

    gemShader.setVec4("uTint", m_phase2 ? RED_TINT : glm::vec4(0.0f));
    std::vector<glm::mat4> crystal, gold, rock;
    for (const Shard& sh : m_shards) {
        glm::mat4 m = glm::translate(I, sh.position);
        m = glm::rotate(m, sh.age * 9.0f, glm::vec3(0.3f, 1.0f, 0.4f));
        m = glm::scale(m, glm::vec3(sh.size * (1.0f - sh.age / sh.life)));
        (sh.kind == 0 ? crystal : sh.kind == 1 ? gold : rock).push_back(m);
    }
    for (const Shockwave& w : m_waves) {
        if (w.delay > 0.0f)
            continue;
        const int spikes = 56;
        for (int i = 0; i < spikes; ++i) {
            const float a = i * glm::two_pi<float>() / spikes;
            const glm::vec3 q = m_position + glm::vec3(std::cos(a), 0.0f, std::sin(a)) * w.radius;
            const float h = 0.35f + 0.12f * std::sin(a * 5.0f + time * 20.0f);
            crystal.push_back(glm::scale(glm::translate(I, glm::vec3(q.x, floorAt(q.x, q.z) + h * 0.6f, q.z)),
                                         glm::vec3(0.16f, h, 0.16f)));
        }
    }
    m_crystalShards->setInstances(crystal, true);
    m_crystalShards->draw();
    gemShader.setVec4("uTint", glm::vec4(0.0f));
    m_goldShards->setInstances(gold, true);
    m_goldShards->draw();
    m_rockShards->setInstances(rock, true);
    m_rockShards->draw();

    std::vector<glm::mat4> rain;
    const float ceiling = 6.5f;
    for (const Drop& d : m_drops) {
        if (d.landed)
            continue;
        const float grow = d.warning > 0.0f ? std::max(0.0f, 1.0f - d.warning / settings.rainWindup) : 1.0f;
        const float t = d.fall / settings.rainFallTime;
        const float y = d.spot.y + ceiling * (1.0f - t * t);
        glm::mat4 m = glm::translate(I, glm::vec3(d.spot.x, y, d.spot.z));
        m = glm::rotate(m, glm::pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
        rain.push_back(glm::scale(m, glm::vec3(0.4f + 0.6f * grow)));
    }
    gemShader.setVec4("uTint", m_phase2 ? RED_TINT : glm::vec4(0.0f));
    m_rainCrystals->setInstances(rain, true);
    m_rainCrystals->draw();
    gemShader.setVec4("uTint", glm::vec4(0.0f));

    if (m_barrier > 0.02f) {
        const glm::vec2 out = m_cave->exitDirection();
        const glm::vec3 c = m_cave->chamberFloorCenter();
        const float r = m_cave->chamberRadius() - 0.5f;
        const glm::vec3 mouth(c.x + out.x * r, floorAt(c.x + out.x * r, c.z + out.y * r), c.z + out.y * r);
        m_barrierWall->setInstances({glm::scale(glm::translate(I, mouth), glm::vec3(1.0f, m_barrier, 1.0f))}, true);
        m_barrierWall->draw();
    }
}

void BossSystem::drawFloorMarkers(const Shader& blobShader, float time) const
{
    if (!m_cave)
        return;
    std::vector<std::pair<glm::mat4, glm::vec4>> marks;
    auto disc = [&](const glm::vec3& at, float radius, const glm::vec3& color, float opacity) {
        glm::mat4 m = glm::translate(I, glm::vec3(at.x, floorAt(at.x, at.z) + 0.06f, at.z));
        marks.push_back({glm::scale(m, glm::vec3(radius)), glm::vec4(color, opacity)});
    };
    const float k = 1.0f / speedScale();
    if (m_state == State::SlamWindup) {
        const float t = 1.0f - std::max(m_timer, 0.0f) / (settings.slamWindup * k);
        disc(m_position, 1.5f + 2.5f * t, {0.4f, 0.9f, 1.0f}, 0.5f + 0.3f * std::sin(time * 18.0f));
    }
    if (m_state == State::ChargeWindup) {
        const glm::vec2 d = m_chargeDirection;
        for (float step = 1.5f; step < 2.0f * m_cave->chamberRadius(); step += 1.0f) {
            const glm::vec3 q = m_position + glm::vec3(d.x, 0.0f, d.y) * step;
            const glm::vec3 c = m_cave->chamberFloorCenter();
            if (glm::length(glm::vec2(q.x - c.x, q.z - c.z)) > m_cave->chamberRadius() - 0.5f)
                break;
            disc(q, 0.55f, {1.0f, 0.25f, 0.15f}, 0.55f + 0.3f * std::sin(time * 20.0f - step));
        }
    }
    for (const Drop& d : m_drops) {
        if (d.landed)
            continue;
        const float urgency = 1.0f - std::clamp(d.warning / settings.rainWindup, 0.0f, 1.0f);
        disc(d.spot, settings.rainRadius, {1.0f, 0.45f, 0.15f},
             0.35f + 0.45f * urgency * (0.5f + 0.5f * std::sin(time * (8.0f + 20.0f * urgency))));
    }
    for (const Shockwave& w : m_waves)
        if (w.delay <= 0.0f)
            disc(m_position, w.radius + 0.6f, {0.4f, 0.9f, 1.0f}, 0.12f);
    if (marks.empty())
        return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);
    for (const auto& [model, color] : marks) {
        blobShader.setMat4("uModel", model);
        blobShader.setVec3("uColor", glm::vec3(color));
        blobShader.setFloat("uOpacity", color.a);
        m_quad.draw();
    }
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void BossSystem::drawTransparent(const Shader& gemShader, float time) const
{
    if (!m_cave || m_state == State::Defeated)
        return;
    const float pulse = 1.0f + 0.04f * std::sin(time * 3.0f);
    m_seal->setInstances({glm::scale(glm::translate(I, m_cave->heartPosition()), glm::vec3(1.25f * pulse))}, true);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    gemShader.setInt("uRainbow", 0);
    gemShader.setVec4("uTint", glm::vec4(0.0f));
    gemShader.setFloat("uAlpha", 0.28f + 0.06f * std::sin(time * 2.0f));
    m_seal->draw();
    gemShader.setFloat("uAlpha", 1.0f);
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void BossSystem::drawHud(UIRenderer& ui, float time) const
{
    const float s = ui.scale();
    if (m_state == State::Intro) {
        const float t = m_stateTime / settings.introTime;
        const float alpha = std::clamp((t - 0.25f) * 4.0f, 0.0f, 1.0f) * std::clamp((1.0f - t) * 6.0f, 0.0f, 1.0f);
        const float size = 40.0f * s;
        const float y = ui.height() * 0.22f;
        ui.text("The Cave Guardian", ui.width() * 0.5f, y - 30.0f * s, 22.0f * s, {1.0f, 0.84f, 0.46f, alpha},
                TextAlign::Center);
        ui.text(settings.name, ui.width() * 0.5f, y, size, {1.0f, 0.96f, 0.86f, alpha}, TextAlign::Center);
        return;
    }
    if (!hudVisible())
        return;
    const float w = std::min(560.0f * s, ui.width() * 0.45f), h = 16.0f * s;
    const float x = (ui.width() - w) * 0.5f, y = 26.0f * s;
    ui.panel(x - 16.0f * s, y - 8.0f * s, w + 32.0f * s, h + 46.0f * s, 12.0f * s);
    ui.text(settings.name, ui.width() * 0.5f, y - 2.0f * s, 19.0f * s, UIColors::GOLD, TextAlign::Center);
    const glm::vec4 fill = m_phase2 ? glm::vec4(1.0f, 0.32f, 0.25f, 1.0f) : glm::vec4(0.45f, 0.85f, 1.0f, 1.0f);
    const float barY = y + 26.0f * s;
    ui.bar(x, barY, w, h, m_health / static_cast<float>(settings.health), fill);
    ui.rect(x + w * settings.phase2At - 1.5f * s, barY - 3.0f * s, 3.0f * s, h + 6.0f * s, {1.0f, 0.84f, 0.46f, 0.8f});
    if (m_state == State::Stunned)
        ui.text("Stunned!", ui.width() * 0.5f, barY + h + 12.0f * s, 18.0f * s,
                {1.0f, 0.85f, 0.35f, 0.7f + 0.3f * std::sin(time * 10.0f)}, TextAlign::Center);
}

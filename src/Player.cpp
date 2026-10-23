#include "Player.h"
#include "MathUtils.h"
#include "MeshBuilder.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace {

const glm::vec3 BODY_COLOR   {0.58f, 0.68f, 0.98f};
const glm::vec3 SKIN_COLOR   {1.00f, 0.87f, 0.78f};
const glm::vec3 HAIR_COLOR   {0.98f, 0.70f, 0.80f};
const glm::vec3 EYE_COLOR    {0.16f, 0.16f, 0.28f};
const glm::vec3 SCARF_COLOR  {1.00f, 0.55f, 0.48f};
const glm::vec3 GLIDER_COLOR {1.00f, 0.95f, 0.75f};
const glm::vec3 SAKURA_GLIDER{1.00f, 0.72f, 0.84f};

const float BODY_RADIUS = 0.32f;
const float BODY_CYLINDER = 0.45f;
const float BODY_HEIGHT = BODY_CYLINDER + 2.0f * BODY_RADIUS;
const float HEAD_RADIUS = 0.30f;
const float HEAD_Y = BODY_HEIGHT + 0.22f;

float smoothFactor(float speed, float deltaTime)
{
    return 1.0f - std::exp(-speed * deltaTime);
}

glm::vec3 moveTowards(const glm::vec3& current, const glm::vec3& target, float maxStep)
{
    const glm::vec3 change = target - current;
    const float length = glm::length(change);
    return length <= maxStep ? target : current + change / length * maxStep;
}

}

Player::Player(const PlayerSettings& settings, const GlideSettings& glide)
    : settings(settings), glide(glide),
      m_body      (Mesh::createCapsule(BODY_RADIUS, BODY_CYLINDER, BODY_COLOR)),
      m_head      (Mesh::createSphere(HEAD_RADIUS, SKIN_COLOR)),
      m_hair      (Mesh::createSphere(0.32f, HAIR_COLOR)),
      m_eye       (Mesh::createSphere(0.05f, EYE_COLOR, 12, 8)),
      m_scarf     (Mesh::createSphere(0.30f, SCARF_COLOR)),
      m_scarfTail (Mesh::createCapsule(0.07f, 0.30f, SCARF_COLOR, 12, 8)),
      m_arm       (Mesh::createCapsule(0.08f, 0.30f, BODY_COLOR, 12, 8)),
      m_glider    (Mesh::createSphere(1.0f, GLIDER_COLOR, 24, 12)),
      m_shadowQuad(Mesh::createGroundQuad())
{
    MeshBuilder crown;
    crown.addPrism(glm::mat4(1.0f), 10, 0.25f, 0.25f, 0.05f, 0.0f, {1.0f, 0.82f, 0.45f});
    for (int i = 0; i < 5; ++i) {
        const float a = glm::half_pi<float>() + i * glm::two_pi<float>() / 5.0f;
        const float big = i == 0 ? 1.5f : 1.0f;
        glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(std::cos(a) * 0.22f, 0.03f, std::sin(a) * 0.22f));
        m = glm::rotate(m, glm::radians(12.0f), glm::vec3(std::sin(a), 0.0f, -std::cos(a)));
        crown.addPrism(m, 6, 0.04f * big, 0.035f * big, 0.08f * big, 0.07f * big,
                       i % 2 == 0 ? glm::vec3(0.55f, 0.85f, 1.0f) : glm::vec3(0.80f, 0.60f, 1.0f));
    }
    m_crown = crown.build();

    MeshBuilder staff;
    staff.addPrism(glm::mat4(1.0f), 6, 0.035f, 0.03f, 1.15f, 0.0f, {0.52f, 0.36f, 0.26f});
    staff.addPrism(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.12f, 0.0f)), 6, 0.06f, 0.05f, 0.05f, 0.0f,
                   {1.0f, 0.82f, 0.45f});
    staff.addPrism(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.16f, 0.0f)), 6, 0.07f, 0.06f, 0.1f, 0.12f,
                   {0.55f, 0.90f, 1.00f});
    m_staff = staff.build();
    m_hearts = maxHearts();

    MeshBuilder dot;
    dot.addSphere(glm::mat4(1.0f), {0.80f, 0.95f, 1.00f}, 6, 4);
    m_trailBatch = std::make_unique<InstancedBatch>(dot.build());
    MeshBuilder spark;
    spark.addSphere(glm::mat4(1.0f), {1.00f, 0.82f, 0.40f}, 6, 4);
    m_sparkleBatch = std::make_unique<InstancedBatch>(spark.build());
}

bool Player::takeHit(int damage, const glm::vec3& from)
{
    if (isDead() || isInvulnerable() || isSafe())
        return false;
    m_hearts = std::max(0, m_hearts - damage);
    m_invulnerable = health.invulnerableTime;
    m_sinceHit = 0.0f;

    glm::vec2 away(m_position.x - from.x, m_position.z - from.z);
    away = glm::length(away) > 1e-3f ? glm::normalize(away) : glm::vec2(std::sin(m_facing), std::cos(m_facing)) * -1.0f;
    m_velocity = glm::vec3(away.x, 0.0f, away.y) * health.knockbackSpeed + glm::vec3(0.0f, health.knockbackUp, 0.0f);
    m_grounded = false;
    m_gliding = false;
    return true;
}

void Player::revive(const glm::vec3& position, float facing, int island)
{
    teleport(position, facing, island);
    m_hearts = maxHearts();
    m_invulnerable = 0.0f;
    m_safety = health.respawnSafety;
    m_sinceHit = 100.0f;
    m_swingTime = -1.0f;
    ++m_respawnCount;
}

bool Player::startSwing()
{
    if (m_swingCooldown > 0.0f || isDead())
        return false;
    m_swingTime = 0.0f;
    m_swingCooldown = swing.cooldown;
    return true;
}

float Player::swingProgress() const
{
    return m_swingTime < 0.0f ? -1.0f : std::min(1.0f, m_swingTime / swing.duration);
}

void Player::applyReward(const std::string& rewardId)
{
    if (rewardId == "glide_trail") {
        m_rewards.glideTrail = true;
    } else if (rewardId == "sakura_glider") {
        m_rewards.sakuraGlider = true;
        m_glider = Mesh::createSphere(1.0f, SAKURA_GLIDER, 24, 12);
    } else if (rewardId == "swift_boots" && !m_rewards.swiftBoots) {
        m_rewards.swiftBoots = true;
        settings.sprintSpeed *= 1.3f;
    } else if (rewardId == "extra_heart") {
        ++m_rewards.extraHearts;
        m_hearts = maxHearts();
    } else if (rewardId == "crystal_crown") {
        m_rewards.crystalCrown = true;
    }
}

void Player::respawn(const World& world, int island)
{
    if (island >= 0)
        m_lastIsland = island;
    m_position = world.spawnPoint(m_lastIsland);
    m_velocity = glm::vec3(0.0f);
    m_grounded = true;
    m_gliding  = false;
    ++m_respawnCount;
}

void Player::teleport(const glm::vec3& position, float facing, int island)
{
    m_position = position;
    m_velocity = glm::vec3(0.0f);
    m_facing   = facing;
    m_grounded = true;
    m_gliding  = false;
    m_glideAmount = 0.0f;
    m_lastIsland = island;
}

void Player::pushOutOfCircle(const glm::vec2& center, float radius)
{
    const float characterRadius = BODY_RADIUS + 0.05f;
    const glm::vec2 offset = glm::vec2(m_position.x, m_position.z) - center;
    const float distance = glm::length(offset);
    const float minimum = radius + characterRadius;
    if (distance >= minimum || distance < 1e-5f)
        return;

    const glm::vec2 away = offset / distance;
    const glm::vec2 corrected = center + away * minimum;
    m_position.x = corrected.x;
    m_position.z = corrected.y;

    const float into = glm::dot(glm::vec2(m_velocity.x, m_velocity.z), away);
    if (into < 0.0f) {
        m_velocity.x -= away.x * into;
        m_velocity.z -= away.y * into;
    }
}

void Player::update(const PlayerInput& input, float cameraYaw, const World& world, float deltaTime)
{
    m_invulnerable = std::max(0.0f, m_invulnerable - deltaTime);
    m_safety = std::max(0.0f, m_safety - deltaTime);
    m_swingCooldown = std::max(0.0f, m_swingCooldown - deltaTime);
    m_sinceHit += deltaTime;
    if (!isDead() && m_sinceHit > health.regenDelay && m_hearts < maxHearts()) {
        m_regenTimer += deltaTime;
        if (m_regenTimer >= health.regenInterval) {
            m_regenTimer = 0.0f;
            heal(1);
        }
    } else {
        m_regenTimer = 0.0f;
    }
    m_swingHitNow = false;
    if (m_swingTime >= 0.0f) {
        const float before = m_swingTime / swing.duration;
        m_swingTime += deltaTime;
        const float after = m_swingTime / swing.duration;
        m_swingHitNow = before < swing.hitMoment && after >= swing.hitMoment;
        if (after >= 1.0f)
            m_swingTime = -1.0f;
    }

    const float yaw = glm::radians(cameraYaw);
    const glm::vec3 forward(std::cos(yaw), 0.0f, std::sin(yaw));
    const glm::vec3 right(-std::sin(yaw), 0.0f, std::cos(yaw));

    glm::vec3 wishDirection = forward * input.move.y + right * input.move.x;
    if (glm::length(wishDirection) > 1.0f)
        wishDirection = glm::normalize(wishDirection);

    const GroundHit below = world.groundBelow(m_position);
    const float heightAboveGround = below.found ? m_position.y - below.height : 1e9f;
    if (!input.glide || m_grounded || world.isInCave(m_position))
        m_gliding = false;
    else if (!m_gliding && m_velocity.y < 0.0f && heightAboveGround > glide.minHeight)
        m_gliding = true;

    if (m_gliding) {
        updateGlideMovement(wishDirection, deltaTime);
    } else {
        updateGroundMovement(wishDirection, input.sprint, deltaTime);

        if (input.jump && m_grounded) {
            m_velocity.y = settings.jumpStrength;
            m_grounded = false;
        }
        m_velocity.y -= settings.gravity * deltaTime;
    }
    m_position += m_velocity * deltaTime;

    for (const Collider& collider : world.colliders()) {
        if (m_position.y >= collider.top - 0.05f || m_position.y + 1.6f < collider.bottom)
            continue;
        pushOutOfCircle(collider.center, collider.radius);
    }

    world.constrainToCave(m_position, m_velocity, BODY_RADIUS + 0.05f, 1.6f);

    const GroundHit ground = world.groundBelow(m_position, 0.3f);
    if (ground.found) {
        const bool stickDownhill = m_grounded && m_velocity.y <= 0.0f && m_position.y - ground.height < 0.3f;
        if (m_position.y <= ground.height || stickDownhill) {
            m_position.y = ground.height;
            m_velocity.y = 0.0f;
            m_grounded = true;
            m_gliding = false;
            if (world.islands()[ground.island].settings().isRespawnPoint)
                m_lastIsland = ground.island;
        } else {
            m_grounded = false;
        }
    } else {
        m_grounded = false;
    }

    if (m_position.y < world.lowestPoint() - settings.respawnMargin)
        respawn(world);

    updateAnimation(deltaTime);
    updateParticles(input.sprint, deltaTime);
}

void Player::updateParticles(bool sprinting, float deltaTime)
{
    for (Particle& p : m_particles) {
        p.age += deltaTime;
        p.position += p.velocity * deltaTime;
        p.velocity *= 1.0f - std::min(1.0f, 2.0f * deltaTime);
    }
    m_particles.erase(std::remove_if(m_particles.begin(), m_particles.end(),
                                     [](const Particle& p) { return p.age >= p.life; }),
                      m_particles.end());

    m_spawnTimer -= deltaTime;
    if (m_spawnTimer > 0.0f)
        return;
    m_spawnTimer = 0.025f;
    const glm::vec3 right(std::cos(m_facing), 0.0f, -std::sin(m_facing));
    const float speed = glm::length(glm::vec2(m_velocity.x, m_velocity.z));

    if (m_rewards.glideTrail && m_glideAmount > 0.5f) {
        for (float side : {-1.0f, 1.0f}) {
            const glm::vec3 tip = m_position + glm::vec3(0.0f, HEAD_Y + 0.6f, 0.0f) + right * side * 1.45f;
            m_particles.push_back({tip, glm::vec3(0.0f), 0.0f, 0.9f, 0.09f, false});
        }
    }
    if (m_rewards.swiftBoots && sprinting && m_grounded && speed > settings.walkSpeed + 0.5f) {
        const float jitter = std::sin(m_walkPhase * 7.3f);
        const glm::vec3 kick = glm::vec3(-m_velocity.x, 0.0f, -m_velocity.z) * 0.15f;
        m_particles.push_back({m_position + right * 0.2f * jitter + glm::vec3(0.0f, 0.1f, 0.0f),
                               kick + glm::vec3(0.0f, 1.2f, 0.0f) + right * jitter, 0.0f, 0.5f, 0.06f, true});
    }
}

void Player::drawEffects(const Shader& gemShader) const
{
    if (m_particles.empty())
        return;
    std::vector<glm::mat4> trail, sparkles;
    for (const Particle& p : m_particles) {
        const float size = p.size * (1.0f - p.age / p.life);
        (p.gold ? sparkles : trail).push_back(glm::scale(glm::translate(glm::mat4(1.0f), p.position), glm::vec3(size)));
    }
    gemShader.setInt("uRainbow", 0);
    gemShader.setFloat("uAlpha", 1.0f);
    m_trailBatch->setInstances(trail, true);
    m_trailBatch->draw();
    m_sparkleBatch->setInstances(sparkles, true);
    m_sparkleBatch->draw();
}

void Player::updateGroundMovement(const glm::vec3& wishDirection, bool sprint, float deltaTime)
{
    const float speed = sprint ? settings.sprintSpeed : settings.walkSpeed;
    const glm::vec3 horizontal = moveTowards(glm::vec3(m_velocity.x, 0.0f, m_velocity.z),
                                             wishDirection * speed,
                                             settings.acceleration * deltaTime);
    m_velocity.x = horizontal.x;
    m_velocity.z = horizontal.z;

    if (glm::length(wishDirection) > 0.01f) {
        const float targetFacing = std::atan2(wishDirection.x, wishDirection.z);
        m_facing += shortestAngle(m_facing, targetFacing) * smoothFactor(settings.turnSpeed, deltaTime);
    }
}

void Player::updateGlideMovement(const glm::vec3& wishDirection, float deltaTime)
{
    if (glm::length(wishDirection) > 0.01f) {
        const float targetFacing = std::atan2(wishDirection.x, wishDirection.z);
        m_facing += shortestAngle(m_facing, targetFacing) * smoothFactor(glide.turnSpeed, deltaTime);
    }

    const glm::vec3 facingDirection(std::sin(m_facing), 0.0f, std::cos(m_facing));
    const glm::vec3 horizontal = moveTowards(glm::vec3(m_velocity.x, 0.0f, m_velocity.z),
                                             facingDirection * glide.forwardSpeed,
                                             glide.acceleration * deltaTime);
    m_velocity.x = horizontal.x;
    m_velocity.z = horizontal.z;

    m_velocity.y += (-glide.fallSpeed - m_velocity.y) * smoothFactor(4.0f, deltaTime);
}

void Player::updateAnimation(float deltaTime)
{
    const float horizontalSpeed = glm::length(glm::vec2(m_velocity.x, m_velocity.z));
    const float walkTarget = m_grounded ? std::min(horizontalSpeed / settings.walkSpeed, 1.0f) : 0.0f;
    m_walkAmount += (walkTarget - m_walkAmount) * smoothFactor(10.0f, deltaTime);
    if (m_grounded)
        m_walkPhase += horizontalSpeed * deltaTime * 4.5f;
    m_glideAmount += ((m_gliding ? 1.0f : 0.0f) - m_glideAmount) * smoothFactor(12.0f, deltaTime);
}

void Player::draw(const Shader& shader) const
{
    if (m_invulnerable > 0.0f && static_cast<int>(m_invulnerable * 16.0f) % 2 == 0)
        return;

    const float bounce = std::abs(std::sin(m_walkPhase)) * 0.08f * m_walkAmount;
    float stretch = 1.0f + 0.05f * std::sin(m_walkPhase * 2.0f) * m_walkAmount;
    if (!m_grounded && !m_gliding)
        stretch += std::clamp(m_velocity.y * 0.015f, -0.08f, 0.12f);
    const float width = 1.0f / std::sqrt(stretch);
    const float lean = 0.12f * m_walkAmount + 0.35f * m_glideAmount;

    glm::mat4 base = glm::translate(glm::mat4(1.0f), m_position + glm::vec3(0.0f, bounce, 0.0f));
    base = glm::rotate(base, m_facing, glm::vec3(0.0f, 1.0f, 0.0f));
    base = glm::rotate(base, lean, glm::vec3(1.0f, 0.0f, 0.0f));
    base = glm::scale(base, glm::vec3(width, stretch, width));

    auto drawPart = [&](const Mesh& mesh, const glm::mat4& local) {
        shader.setMat4("uModel", base * local);
        mesh.draw();
    };
    const glm::mat4 identity(1.0f);

    drawPart(m_body, glm::translate(identity, glm::vec3(0.0f, BODY_HEIGHT * 0.5f, 0.0f)));

    drawPart(m_head, glm::translate(identity, glm::vec3(0.0f, HEAD_Y, 0.0f)));
    drawPart(m_hair, glm::scale(glm::translate(identity, glm::vec3(0.0f, HEAD_Y + 0.06f, -0.06f)),
                                glm::vec3(1.0f, 0.85f, 0.95f)));

    if (m_rewards.crystalCrown)
        drawPart(m_crown, glm::translate(identity, glm::vec3(0.0f, HEAD_Y + 0.27f, -0.03f)));

    for (float side : {-1.0f, 1.0f}) {
        glm::mat4 eye = glm::translate(identity, glm::vec3(0.10f * side, HEAD_Y - 0.02f, HEAD_RADIUS - 0.03f));
        drawPart(m_eye, glm::scale(eye, glm::vec3(1.0f, 1.3f, 0.6f)));
    }

    drawPart(m_scarf, glm::scale(glm::translate(identity, glm::vec3(0.0f, BODY_HEIGHT - 0.06f, 0.0f)),
                                 glm::vec3(1.15f, 0.32f, 1.15f)));
    const float swing = glm::radians(35.0f) * m_walkAmount + 0.15f * std::sin(m_walkPhase) * m_walkAmount
                      + glm::radians(70.0f) * m_glideAmount;
    glm::mat4 tail = glm::translate(identity, glm::vec3(0.12f, BODY_HEIGHT - 0.12f, -BODY_RADIUS - 0.02f));
    tail = glm::rotate(tail, swing, glm::vec3(1.0f, 0.0f, 0.0f));
    tail = glm::translate(tail, glm::vec3(0.0f, -0.20f, 0.0f));
    drawPart(m_scarfTail, tail);

    glm::mat4 staff = glm::translate(identity, glm::vec3(-0.36f, BODY_HEIGHT * 0.55f, 0.05f));
    const float p = swingProgress();
    if (p >= 0.0f) {
        const float eased = 1.0f - (1.0f - p) * (1.0f - p);
        staff = glm::rotate(staff, glm::radians(-100.0f + 220.0f * eased), glm::vec3(0.0f, 1.0f, 0.0f));
        staff = glm::rotate(staff, glm::radians(80.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    } else {
        staff = glm::rotate(staff, glm::radians(-12.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        staff = glm::rotate(staff, glm::radians(10.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        staff = glm::translate(staff, glm::vec3(0.0f, -0.45f, 0.0f));
    }
    drawPart(m_staff, staff);

    if (m_glideAmount > 0.01f) {
        const float open = m_glideAmount;
        for (float side : {-1.0f, 1.0f}) {
            glm::mat4 arm = glm::translate(identity, glm::vec3(0.30f * side, BODY_HEIGHT - 0.15f, 0.0f));
            arm = glm::rotate(arm, side * glm::radians(140.0f) * open, glm::vec3(0.0f, 0.0f, 1.0f));
            arm = glm::translate(arm, glm::vec3(0.0f, -0.25f, 0.0f));
            drawPart(m_arm, arm);
        }
        for (float side : {-1.0f, 1.0f}) {
            glm::mat4 wing = glm::translate(identity, glm::vec3(0.75f * side * open, HEAD_Y + 0.55f, 0.0f));
            wing = glm::rotate(wing, side * glm::radians(15.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            drawPart(m_glider, glm::scale(wing, glm::vec3(0.85f * open, 0.10f, 0.75f)));
        }
    }
}

void Player::drawShadow(const Shader& blobShader, const World& world) const
{
    const GroundHit hit = world.groundBelow(m_position, 0.3f);
    if (!hit.found)
        return;

    const float height = std::max(m_position.y - hit.height, 0.0f);
    const float fade = 1.0f - std::clamp(height / 20.0f, 0.0f, 1.0f);
    if (fade <= 0.0f)
        return;
    const float size = 0.55f * (0.5f + 0.5f * fade);

    const float x = m_position.x, z = m_position.z;
    glm::vec3 normal(0.0f, 1.0f, 0.0f);
    if (!hit.onProp && !hit.inCave) {
        const Island& island = world.islands()[hit.island];
        const float e = 0.3f;
        normal = glm::normalize(glm::vec3(
            island.heightAt(x - e, z) - island.heightAt(x + e, z),
            2.0f * e,
            island.heightAt(x, z - e) - island.heightAt(x, z + e)));
    }

    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, hit.height + 0.04f, z));
    const glm::vec3 axis = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), normal);
    if (glm::length(axis) > 1e-4f)
        model = glm::rotate(model, std::acos(glm::clamp(normal.y, -1.0f, 1.0f)), glm::normalize(axis));
    model = glm::scale(model, glm::vec3(size));

    blobShader.setMat4("uModel", model);
    blobShader.setFloat("uOpacity", 0.45f * fade);
    blobShader.setVec3("uColor", glm::vec3(0.16f, 0.13f, 0.28f));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);

    m_shadowQuad.draw();

    glDisable(GL_POLYGON_OFFSET_FILL);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

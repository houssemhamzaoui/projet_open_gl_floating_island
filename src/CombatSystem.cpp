#include "CombatSystem.h"
#include "BossSystem.h"
#include "MeshBuilder.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace {

const glm::vec3 UP(0.0f, 1.0f, 0.0f);
const glm::vec3 PARTICLE_COLORS[5] = {
    {0.35f, 0.15f, 0.45f},
    {1.00f, 0.65f, 0.25f},
    {1.00f, 0.30f, 0.30f},
    {0.50f, 0.95f, 1.00f},
    {1.00f, 0.55f, 0.70f},
};

Mesh sphereMesh(const glm::vec3& color)
{
    MeshBuilder b;
    b.addSphere(glm::mat4(1.0f), color, 8, 5);
    return b.build();
}

Mesh buildArc(float halfAngleDegrees, float inner, float outer)
{
    MeshBuilder b;
    const int segments = 18;
    const glm::vec3 color{1.0f, 0.93f, 0.70f};
    for (int i = 0; i < segments; ++i) {
        const float a0 = glm::radians(-halfAngleDegrees + 2.0f * halfAngleDegrees * i / segments);
        const float a1 = glm::radians(-halfAngleDegrees + 2.0f * halfAngleDegrees * (i + 1) / segments);
        const float w0 = std::sin(glm::pi<float>() * i / segments), w1 = std::sin(glm::pi<float>() * (i + 1) / segments);
        const float i0 = outer - (outer - inner) * w0, i1 = outer - (outer - inner) * w1;
        const glm::vec3 o0(std::sin(a0) * outer, 0.0f, std::cos(a0) * outer), o1(std::sin(a1) * outer, 0.0f, std::cos(a1) * outer);
        const glm::vec3 n0(std::sin(a0) * i0, 0.0f, std::cos(a0) * i0), n1(std::sin(a1) * i1, 0.0f, std::cos(a1) * i1);
        b.addTriangle(n0, o0, o1, color);
        b.addTriangle(n0, o1, n1, color);
    }
    return b.build();
}

}

CombatSystem::CombatSystem(const CombatSettings& settings)
    : settings(settings)
{
    m_arcBatch = std::make_unique<InstancedBatch>(buildArc(settings.swingHalfAngle, 0.6f, settings.swingRange));
    for (const glm::vec3& color : PARTICLE_COLORS)
        m_particleBatches.push_back(std::make_unique<InstancedBatch>(Mesh::createOctahedron(1.0f, 1.4f, color)));
    m_orbBatch = std::make_unique<InstancedBatch>(sphereMesh({0.55f, 0.95f, 1.00f}));
    m_dropBatch = std::make_unique<InstancedBatch>(Mesh::createOctahedron(1.0f, 1.4f, {1.00f, 0.42f, 0.58f}));
}

void CombatSystem::burst(const glm::vec3& at, int color, int count, float speed)
{
    for (int i = 0; i < count; ++i) {
        auto random = [this]() { m_noise = m_noise * 1664525u + 1013904223u; return (m_noise >> 8) / 16777216.0f; };
        const float a = random() * glm::two_pi<float>();
        const float up = 0.3f + random() * 0.9f;
        const glm::vec3 dir = glm::normalize(glm::vec3(std::cos(a), up, std::sin(a)));
        m_particles.push_back({at, dir * speed * (0.5f + random()), 0.0f, 0.4f + random() * 0.35f,
                               0.06f + random() * 0.06f, color});
    }
}

void CombatSystem::swingHits(const Player& player, EnemySystem& enemies, BossSystem* boss)
{
    const glm::vec3 p = player.getPosition();
    const glm::vec2 facing(std::sin(player.getFacing()), std::cos(player.getFacing()));
    const float cosHalfAngle = std::cos(glm::radians(settings.swingHalfAngle));

    if (boss && boss->isHittable()) {
        const glm::vec3 c = boss->center();
        const glm::vec2 toBoss(c.x - p.x, c.z - p.z);
        const float distance = glm::length(toBoss);
        const bool close = distance <= settings.swingRange + boss->radius();
        const bool inFront = distance < boss->radius() + 0.5f || glm::dot(facing, toBoss / distance) >= cosHalfAngle;
        if (close && inFront && boss->damage(settings.swingDamage, p)) {
            burst(c, 1, 8, 4.0f);
            m_shake = std::max(m_shake, settings.shakeOnHit);
        }
    }
    for (int i = 0; i < static_cast<int>(enemies.enemies().size()); ++i) {
        if (!enemies.isHittable(i))
            continue;
        const glm::vec3 c = enemies.center(i);
        const glm::vec2 toEnemy(c.x - p.x, c.z - p.z);
        const float distance = glm::length(toEnemy);
        if (distance > settings.swingRange + enemies.settings.radius || std::abs(c.y - (p.y + 0.6f)) > 1.8f)
            continue;
        if (distance > 0.8f && glm::dot(facing, toEnemy / distance) < cosHalfAngle)
            continue;
        enemies.damage(i, settings.swingDamage, p, settings.swingKnockback);
        burst(c, 1, 8, 4.0f);
        m_shake = std::max(m_shake, settings.shakeOnHit);
    }
}

void CombatSystem::update(Player& player, EnemySystem& enemies, BossSystem* boss, const World& world,
                          bool swingPressed, bool orbPressed, float aimYaw, float deltaTime)
{
    m_orbCooldown = std::max(0.0f, m_orbCooldown - deltaTime);

    if (swingPressed)
        player.startSwing();
    if (player.swingHitsNow())
        swingHits(player, enemies, boss);

    if (orbPressed && m_orbCooldown <= 0.0f && !player.isDead()) {
        const float yaw = glm::radians(aimYaw);
        const glm::vec3 dir(std::cos(yaw), 0.0f, std::sin(yaw));
        player.face(std::atan2(dir.x, dir.z));
        m_orbs.push_back({player.getPosition() + glm::vec3(0.0f, 1.0f, 0.0f) + dir * 0.6f, dir * settings.orbSpeed});
        m_orbCooldown = settings.orbCooldown;
    }

    for (Orb& orb : m_orbs) {
        orb.age += deltaTime;
        orb.position += orb.velocity * deltaTime;
        if (std::fmod(orb.age, 0.04f) < deltaTime)
            m_particles.push_back({orb.position, glm::vec3(0.0f), 0.0f, 0.3f, 0.07f, 3});
        bool hit = false;
        for (int i = 0; i < static_cast<int>(enemies.enemies().size()) && !hit; ++i) {
            if (enemies.isHittable(i) && glm::length(enemies.center(i) - orb.position) < enemies.settings.radius + settings.orbRadius) {
                enemies.damage(i, settings.orbDamage, orb.position - orb.velocity, settings.orbKnockback);
                m_shake = std::max(m_shake, settings.shakeOnHit);
                hit = true;
            }
        }
        if (!hit && boss && boss->isHittable()
            && glm::length(boss->center() - orb.position) < boss->radius() + settings.orbRadius) {
            boss->damage(settings.orbDamage, orb.position);
            m_shake = std::max(m_shake, settings.shakeOnHit);
            hit = true;
        }
        const GroundHit ground = world.groundBelow(orb.position + UP * 0.5f, 0.0f);
        if (ground.found && ground.height > orb.position.y)
            hit = true;
        if (world.needsSolidCheck(orb.position) && world.isSolid(orb.position))
            hit = true;
        if (hit) {
            burst(orb.position, 3, 10, 4.0f);
            orb.age = settings.orbLifetime;
        }
    }
    m_orbs.erase(std::remove_if(m_orbs.begin(), m_orbs.end(),
                                [this](const Orb& o) { return o.age >= settings.orbLifetime; }), m_orbs.end());

    for (const EnemyEffect& effect : enemies.takeEffects()) {
        switch (effect.kind) {
        case EnemyEffect::Kind::Death:     burst(effect.position, 0, 22, 5.0f); burst(effect.position, 1, 8, 3.0f); break;
        case EnemyEffect::Kind::PlayerHit: burst(effect.position, 2, 12, 4.0f);
                                           m_shake = std::max(m_shake, settings.shakeOnHurt); break;
        case EnemyEffect::Kind::HealDrop:  m_drops.push_back({effect.position}); break;
        }
    }

    for (HealDrop& drop : m_drops) {
        drop.age += deltaTime;
        const glm::vec3 body = player.getPosition() + glm::vec3(0.0f, 0.8f, 0.0f);
        if (player.hearts() < player.maxHearts() && glm::length(body - drop.position) < 1.3f) {
            player.heal(1);
            burst(drop.position, 4, 14, 3.0f);
            drop.age = settings.healDropLifetime;
        }
    }
    m_drops.erase(std::remove_if(m_drops.begin(), m_drops.end(),
                                 [this](const HealDrop& d) { return d.age >= settings.healDropLifetime; }), m_drops.end());

    for (Particle& particle : m_particles) {
        particle.age += deltaTime;
        particle.velocity.y -= 9.0f * deltaTime;
        particle.position += particle.velocity * deltaTime;
    }
    m_particles.erase(std::remove_if(m_particles.begin(), m_particles.end(),
                                     [](const Particle& p) { return p.age >= p.life; }), m_particles.end());
}

void CombatSystem::draw(const Shader& gemShader, float time) const
{
    gemShader.setInt("uRainbow", 0);
    gemShader.setFloat("uAlpha", 1.0f);

    std::vector<std::vector<glm::mat4>> particles(m_particleBatches.size());
    for (const Particle& p : m_particles) {
        const float size = p.size * (1.0f - p.age / p.life);
        glm::mat4 m = glm::translate(glm::mat4(1.0f), p.position);
        m = glm::rotate(m, p.age * 10.0f, glm::vec3(0.3f, 1.0f, 0.2f));
        particles[p.color].push_back(glm::scale(m, glm::vec3(size)));
    }
    for (size_t i = 0; i < m_particleBatches.size(); ++i) {
        m_particleBatches[i]->setInstances(particles[i], true);
        m_particleBatches[i]->draw();
    }

    std::vector<glm::mat4> orbs;
    for (const Orb& o : m_orbs)
        orbs.push_back(glm::scale(glm::translate(glm::mat4(1.0f), o.position),
                                  glm::vec3(settings.orbRadius * (1.0f + 0.15f * std::sin(o.age * 40.0f)))));
    m_orbBatch->setInstances(orbs, true);
    m_orbBatch->draw();

    std::vector<glm::mat4> drops;
    for (const HealDrop& d : m_drops) {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), d.position + UP * (0.12f * std::sin(time * 3.0f + d.age)));
        m = glm::rotate(m, time * 2.0f, UP);
        drops.push_back(glm::scale(m, glm::vec3(0.22f)));
    }
    m_dropBatch->setInstances(drops, true);
    m_dropBatch->draw();
}

void CombatSystem::drawSwing(const Shader& gemShader, const Player& player) const
{
    const float p = player.swingProgress();
    if (p < 0.0f)
        return;
    const float alpha = 0.75f * std::sin(glm::pi<float>() * p);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), player.getPosition() + glm::vec3(0.0f, 0.75f, 0.0f));
    model = glm::rotate(model, player.getFacing(), UP);
    model = glm::rotate(model, glm::radians(-8.0f + 16.0f * p), glm::vec3(0.0f, 0.0f, 1.0f));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    gemShader.setInt("uRainbow", 0);
    gemShader.setFloat("uAlpha", alpha);
    m_arcBatch->setInstances({model}, true);
    m_arcBatch->draw();
    gemShader.setFloat("uAlpha", 1.0f);
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

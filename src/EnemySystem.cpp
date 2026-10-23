#include "EnemySystem.h"
#include "Lighting.h"
#include "MathUtils.h"
#include "MeshBuilder.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace {

const glm::vec3 UP(0.0f, 1.0f, 0.0f);
const Material ENEMY_MATERIAL{0.35f, 32.0f, 0.45f};

float smoothFactor(float speed, float deltaTime) { return 1.0f - std::exp(-speed * deltaTime); }
Mesh buildBody()
{
    MeshBuilder b;
    const glm::vec3 dark{0.13f, 0.07f, 0.18f}, plum{0.32f, 0.12f, 0.34f}, ember{0.55f, 0.16f, 0.22f};
    b.addSphere(glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.45f, 0.0f)), glm::vec3(0.5f, 0.43f, 0.5f)),
                dark, 16, 10);
    const int spikes = 16;
    for (int i = 0; i < spikes; ++i) {
        const float t = (i + 0.5f) / spikes;
        const float polar = std::acos(1.0f - t * 1.15f);
        const float azimuth = i * 2.39996f;
        const glm::vec3 dir(std::sin(polar) * std::cos(azimuth), std::cos(polar), std::sin(polar) * std::sin(azimuth));
        if (dir.z > 0.55f && dir.y < 0.5f)
            continue;
        const glm::vec3 base = glm::vec3(0.0f, 0.45f, 0.0f) + dir * glm::vec3(0.46f, 0.39f, 0.46f);
        const glm::vec3 axis = glm::cross(UP, dir);
        glm::mat4 m = glm::translate(glm::mat4(1.0f), base);
        if (glm::length(axis) > 1e-4f)
            m = glm::rotate(m, std::acos(glm::clamp(dir.y, -1.0f, 1.0f)), glm::normalize(axis));
        const float length = 0.28f + 0.12f * std::sin(i * 3.1f);
        b.addCone(m, 5, 0.09f, length, i % 3 == 0 ? ember : (i % 2 == 0 ? plum : dark));
    }
    for (int i = 0; i < 4; ++i) {
        const float x = -0.12f + i * 0.08f;
        glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(x, 0.36f, 0.43f));
        m = glm::rotate(m, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        b.addCone(m, 3, 0.03f, 0.07f, {0.85f, 0.80f, 0.75f});
    }
    return b.build();
}

Mesh buildEye()
{
    MeshBuilder b;
    b.addSphere(glm::mat4(1.0f), {1.00f, 0.38f, 0.12f}, 10, 6);
    return b.build();
}

}

EnemySystem::EnemySystem(const World& world, EventBus& events, const EnemySettings& settings)
    : settings(settings), m_events(events), m_body(buildBody()), m_shadowQuad(Mesh::createGroundQuad())
{
    m_eyes = std::make_unique<InstancedBatch>(buildEye());
    generate(world);
    events.subscribe([this](const GameEvent& event) {
        if (event.type == EventType::QuestAccepted && event.id == "shadows")
            activate();
    });
}

bool EnemySystem::isValidSpot(int island, const glm::vec2& p, const World& world, float zoneMargin) const
{
    const Island& is = world.islands()[island];
    const glm::vec2 c(is.center().x, is.center().z);
    const glm::vec2 offset = p - c;
    if (glm::length(offset) > 0.82f * is.coastDistance(std::atan2(offset.y, offset.x)))
        return false;
    for (const ClearZone& zone : world.clearZones())
        if (zone.island == island && zone.contains(p, zoneMargin + settings.radius))
            return false;
    for (const Collider& collider : world.colliders())
        if (collider.island == island && glm::length(p - collider.center) < collider.radius + settings.radius)
            return false;
    return true;
}

bool EnemySystem::playerIsProtected(int island, const glm::vec3& p, const World& world) const
{
    for (const ClearZone& zone : world.clearZones())
        if (zone.island == island && zone.contains({p.x, p.z}, 1.5f))
            return true;
    return false;
}

void EnemySystem::generate(const World& world)
{
    m_enemies.clear();
    m_effects.clear();
    m_rng.seed(world.getSeed() * 11u + 7u);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);

    for (int i = 0; i < static_cast<int>(world.islands().size()); ++i) {
        const BiomeKind kind = world.islands()[i].settings().biome.kind;
        if (kind != BiomeKind::Meadow && kind != BiomeKind::Crystal && kind != BiomeKind::Frost)
            continue;
        const Island& island = world.islands()[i];
        int placed = 0;
        for (int attempt = 0; attempt < 800 && placed < settings.perIsland; ++attempt) {
            const float r = island.settings().radius * (0.45f + 0.35f * unit(m_rng));
            const float a = unit(m_rng) * glm::two_pi<float>();
            const glm::vec2 p(island.center().x + r * std::cos(a), island.center().z + r * std::sin(a));
            if (!isValidSpot(i, p, world, settings.safeMargin))
                continue;
            bool crowded = false;
            for (const Enemy& other : m_enemies)
                if (glm::length(other.home - p) < 10.0f) crowded = true;
            if (crowded || (i == World::MAIN_ISLAND && glm::length(p) < 20.0f))
                continue;
            Enemy enemy;
            enemy.island = i;
            enemy.home = p;
            enemy.seed = unit(m_rng) * 10.0f;
            m_enemies.push_back(enemy);
            ++placed;
        }
    }
    if (m_active)
        for (Enemy& enemy : m_enemies)
            spawn(enemy);
}

void EnemySystem::activate()
{
    if (m_active)
        return;
    m_active = true;
    for (Enemy& enemy : m_enemies)
        spawn(enemy);
}

void EnemySystem::spawn(Enemy& enemy)
{
    enemy.position = enemy.home;
    enemy.target = enemy.home;
    enemy.velocity = glm::vec2(0.0f);
    enemy.health = settings.health;
    enemy.state = State::Patrol;
    enemy.timer = 0.0f;
    enemy.flash = 0.0f;
    enemy.barShown = false;
}

bool EnemySystem::isHittable(int index) const
{
    const State s = m_enemies[index].state;
    return s != State::Dead && s != State::Gone;
}

glm::vec3 EnemySystem::center(int index) const
{
    const Enemy& e = m_enemies[index];
    return {e.position.x, e.ground + 0.45f, e.position.y};
}

std::vector<glm::vec3> EnemySystem::dangerPositions() const
{
    std::vector<glm::vec3> result;
    for (size_t i = 0; i < m_enemies.size(); ++i)
        if (isHittable(static_cast<int>(i)))
            result.push_back(center(static_cast<int>(i)));
    return result;
}

bool EnemySystem::moveTo(Enemy& enemy, const glm::vec2& next, const World& world, bool hunting) const
{
    if (!isValidSpot(enemy.island, next, world, hunting ? 0.0f : settings.safeMargin))
        return false;
    enemy.position = next;
    return true;
}

void EnemySystem::face(Enemy& enemy, const glm::vec2& direction, float deltaTime) const
{
    if (glm::length(direction) < 1e-4f)
        return;
    enemy.yaw += shortestAngle(enemy.yaw, std::atan2(direction.x, direction.y)) * smoothFactor(10.0f, deltaTime);
}

bool EnemySystem::damage(int index, int amount, const glm::vec3& from, float knockback)
{
    Enemy& e = m_enemies[index];
    if (!isHittable(index))
        return false;
    e.health -= amount;
    e.flash = 1.0f;
    e.barShown = true;
    glm::vec2 away(e.position.x - from.x, e.position.y - from.z);
    away = glm::length(away) > 1e-3f ? glm::normalize(away) : glm::vec2(0.0f, 1.0f);
    e.velocity = away * knockback;
    if (e.health <= 0) {
        e.state = State::Dead;
        e.timer = 0.6f;
        m_effects.push_back({EnemyEffect::Kind::Death, center(index)});
        if (std::uniform_real_distribution<float>(0.0f, 1.0f)(m_rng) < settings.healDropChance)
            m_effects.push_back({EnemyEffect::Kind::HealDrop, center(index) + glm::vec3(0.0f, 0.5f, 0.0f)});
        m_events.emit({EventType::EnemyDefeated, "shadow"});
        return true;
    }
    e.state = State::Hurt;
    e.timer = settings.hurtTime;
    return false;
}

void EnemySystem::update(Player& player, const World& world, bool targetable, float time, float deltaTime)
{
    (void)time;
    if (!m_active)
        return;
    const EnemySettings& s = settings;
    const glm::vec3 p = player.getPosition();
    const GroundHit playerGround = world.groundBelow(p, 0.3f);

    for (size_t i = 0; i < m_enemies.size(); ++i) {
        Enemy& e = m_enemies[i];
        e.flash = std::max(0.0f, e.flash - deltaTime * 5.0f);
        e.timer -= deltaTime;

        if (e.state == State::Gone) {
            if (e.timer <= 0.0f && glm::length(glm::vec2(p.x, p.z) - e.home) > 25.0f)
                spawn(e);
            continue;
        }
        if (e.state == State::Dead) {
            if (e.timer <= 0.0f) {
                e.state = State::Gone;
                e.timer = s.respawnTime;
            }
            continue;
        }

        const glm::vec2 toPlayer(p.x - e.position.x, p.z - e.position.y);
        const float distance = glm::length(toPlayer);
        const bool sameIsland = playerGround.found && playerGround.island == e.island && !playerGround.inCave
                             && std::abs(p.y - e.ground) < 3.0f;
        const bool canTarget = targetable && sameIsland && !player.isDead()
                            && !playerIsProtected(e.island, p, world)
                            && glm::length(glm::vec2(p.x, p.z) - e.home) < s.leashRadius + 3.0f;
        const glm::vec2 dirToPlayer = distance > 1e-3f ? toPlayer / distance : glm::vec2(0.0f, 1.0f);

        switch (e.state) {
        case State::Patrol: {
            const glm::vec2 toTarget = e.target - e.position;
            if (glm::length(toTarget) < 0.4f || e.timer < -6.0f) {
                std::uniform_real_distribution<float> unit(0.0f, 1.0f);
                const float a = unit(m_rng) * glm::two_pi<float>();
                const glm::vec2 candidate = e.home + glm::vec2(std::cos(a), std::sin(a)) * s.patrolRadius * unit(m_rng);
                if (isValidSpot(e.island, candidate, world, settings.safeMargin))
                    e.target = candidate;
                e.timer = 0.0f;
            } else {
                const glm::vec2 step = glm::normalize(toTarget) * s.patrolSpeed * deltaTime;
                if (!moveTo(e, e.position + step, world, false))
                    e.target = e.position;
                face(e, toTarget, deltaTime);
            }
            if (canTarget && distance < s.noticeDistance)
                e.state = State::Chase;
            break;
        }
        case State::Chase:
            if (!canTarget || distance > s.giveUpDistance || glm::length(e.position - e.home) > s.leashRadius) {
                e.state = State::Return;
                break;
            }
            face(e, dirToPlayer, deltaTime);
            if (distance < s.attackRange) {
                e.state = State::Windup;
                e.timer = s.windupTime;
            } else if (!moveTo(e, e.position + dirToPlayer * s.chaseSpeed * deltaTime, world, true)) {
                e.state = State::Return;
            }
            break;

        case State::Windup:
            face(e, dirToPlayer, deltaTime);
            if (!canTarget) {
                e.state = State::Return;
            } else if (e.timer <= 0.0f) {
                e.state = State::Attack;
                e.timer = s.lungeTime;
                e.lungeDirection = dirToPlayer;
                e.hitThisLunge = false;
            }
            break;

        case State::Attack:
            if (!moveTo(e, e.position + e.lungeDirection * s.lungeSpeed * deltaTime, world, true))
                e.timer = 0.0f;
            if (!e.hitThisLunge && distance < s.radius + 0.6f && canTarget) {
                e.hitThisLunge = true;
                if (player.takeHit(s.damage, center(static_cast<int>(i))))
                    m_effects.push_back({EnemyEffect::Kind::PlayerHit, p + glm::vec3(0.0f, 0.9f, 0.0f)});
            }
            if (e.timer <= 0.0f) {
                e.state = State::Cooldown;
                e.timer = s.cooldownTime;
            }
            break;

        case State::Cooldown:
            moveTo(e, e.position - dirToPlayer * s.patrolSpeed * 0.5f * deltaTime, world, true);
            face(e, dirToPlayer, deltaTime);
            if (e.timer <= 0.0f)
                e.state = canTarget ? State::Chase : State::Return;
            break;

        case State::Hurt:
            if (!moveTo(e, e.position + e.velocity * deltaTime, world, true))
                e.velocity = glm::vec2(0.0f);
            e.velocity *= 1.0f - smoothFactor(8.0f, deltaTime);
            if (e.timer <= 0.0f)
                e.state = canTarget ? State::Chase : State::Return;
            break;

        case State::Return: {
            const glm::vec2 toHome = e.home - e.position;
            if (glm::length(toHome) < 0.6f) {
                e.health = s.health;
                e.barShown = false;
                e.state = State::Patrol;
                e.target = e.home;
            } else {
                const glm::vec2 next = e.position + glm::normalize(toHome) * s.patrolSpeed * 1.3f * deltaTime;
                if (!moveTo(e, next, world, true))
                    e.position = next;
                face(e, toHome, deltaTime);
            }
            break;
        }
        default:
            break;
        }

        e.ground = world.islands()[e.island].heightAt(e.position.x, e.position.y);
        if (std::abs(p.y - e.ground) < 2.0f)
            player.pushOutOfCircle(e.position, s.radius);
    }

    for (size_t a = 0; a < m_enemies.size(); ++a) {
        for (size_t b = a + 1; b < m_enemies.size(); ++b) {
            Enemy& ea = m_enemies[a];
            Enemy& eb = m_enemies[b];
            if (ea.island != eb.island || !isHittable(int(a)) || !isHittable(int(b)))
                continue;
            const glm::vec2 d = eb.position - ea.position;
            const float length = glm::length(d);
            if (length < 2.0f * s.radius && length > 1e-4f) {
                const glm::vec2 push = d / length * (2.0f * s.radius - length) * 0.5f;
                ea.position -= push;
                eb.position += push;
            }
        }
    }
}

void EnemySystem::draw(const Shader& objectShader, float time) const
{
    if (!m_active)
        return;
    Lighting::applyMaterial(objectShader, ENEMY_MATERIAL);
    for (const Enemy& e : m_enemies) {
        if (e.state == State::Gone)
            continue;
        glm::vec3 scale(1.0f);
        glm::vec3 offset(0.0f);
        glm::vec4 flash(1.0f, 1.0f, 1.0f, e.flash * 0.85f);
        const float bob = 0.04f * std::sin(time * 6.0f + e.seed);
        switch (e.state) {
        case State::Windup: {
            const float t = 1.0f - std::max(e.timer, 0.0f) / settings.windupTime;
            scale = glm::vec3(1.0f + 0.15f * t, 1.0f - 0.25f * t, 1.0f + 0.15f * t);
            offset = glm::vec3(std::sin(time * 60.0f) * 0.04f * t, 0.0f, 0.0f);
            flash = glm::vec4(1.0f, 0.25f, 0.1f, 0.25f + 0.35f * t * (0.5f + 0.5f * std::sin(time * 30.0f)));
            break;
        }
        case State::Attack:
            scale = glm::vec3(0.85f, 0.9f, 1.35f);
            offset.y = 0.25f;
            break;
        case State::Dead: {
            const float t = std::max(e.timer, 0.0f) / 0.6f;
            scale = glm::vec3(t);
            flash = glm::vec4(1.0f, 1.0f, 1.0f, 0.6f);
            break;
        }
        default:
            scale.y += bob;
            break;
        }
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(e.position.x, e.ground - 0.02f, e.position.y) + offset);
        model = glm::rotate(model, e.yaw + (e.state == State::Dead ? (0.6f - e.timer) * 12.0f : 0.0f), UP);
        model = glm::scale(model, scale);
        objectShader.setMat4("uModel", model);
        objectShader.setVec4("uFlash", flash);
        m_body.draw();
    }
    objectShader.setVec4("uFlash", glm::vec4(0.0f));
}

void EnemySystem::drawGlow(const Shader& gemShader, float time) const
{
    if (!m_active)
        return;
    std::vector<glm::mat4> eyes;
    for (const Enemy& e : m_enemies) {
        if (e.state == State::Gone || e.state == State::Dead)
            continue;
        const float big = e.state == State::Windup ? 1.35f + 0.15f * std::sin(time * 30.0f) : 1.0f;
        glm::mat4 base = glm::translate(glm::mat4(1.0f), glm::vec3(e.position.x, e.ground, e.position.y));
        base = glm::rotate(base, e.yaw, UP);
        for (float side : {-1.0f, 1.0f}) {
            glm::mat4 eye = glm::translate(base, glm::vec3(0.15f * side, 0.55f, 0.42f));
            eye = glm::rotate(eye, glm::radians(-22.0f * side), glm::vec3(0.0f, 0.0f, 1.0f));
            eyes.push_back(glm::scale(eye, glm::vec3(0.1f, 0.05f, 0.05f) * big));
        }
    }
    gemShader.setInt("uRainbow", 0);
    gemShader.setFloat("uAlpha", 1.0f);
    m_eyes->setInstances(eyes, true);
    m_eyes->draw();
}

void EnemySystem::drawShadows(const Shader& blobShader) const
{
    if (!m_active)
        return;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);
    blobShader.setVec3("uColor", glm::vec3(0.10f, 0.04f, 0.12f));
    for (const Enemy& e : m_enemies) {
        if (e.state == State::Gone)
            continue;
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(e.position.x, e.ground + 0.04f, e.position.y));
        blobShader.setMat4("uModel", glm::scale(model, glm::vec3(settings.radius * 1.3f)));
        blobShader.setFloat("uOpacity", 0.5f);
        m_shadowQuad.draw();
    }
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void EnemySystem::drawHealthBars(UIRenderer& ui, const glm::mat4& viewProjection) const
{
    if (!m_active)
        return;
    const float s = ui.scale();
    for (size_t i = 0; i < m_enemies.size(); ++i) {
        const Enemy& e = m_enemies[i];
        if (!e.barShown || !isHittable(static_cast<int>(i)))
            continue;
        glm::vec2 screen;
        const glm::vec3 above(e.position.x, e.ground + 1.45f, e.position.y);
        if (!UIRenderer::worldToScreen(above, viewProjection, ui.width(), ui.height(), screen))
            continue;
        const float w = 64.0f * s, h = 10.0f * s;
        ui.rect(screen.x - w * 0.5f - 2.0f * s, screen.y - 2.0f * s, w + 4.0f * s, h + 4.0f * s,
                {0.10f, 0.06f, 0.10f, 0.8f}, (h + 4.0f * s) * 0.5f);
        ui.bar(screen.x - w * 0.5f, screen.y, w, h, e.health / static_cast<float>(settings.health),
               {0.95f, 0.30f, 0.30f, 1.0f});
    }
}

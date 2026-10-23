#include "Creatures.h"
#include "Lighting.h"
#include "MathUtils.h"
#include "MeshBuilder.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace {

constexpr float BODY_RADIUS = 0.55f;
constexpr float BODY_HEIGHT = 0.9f;

const Material SLIME_MATERIAL {0.45f, 40.0f, 0.55f};
const Material EYE_MATERIAL   {0.80f, 64.0f, 0.00f};

int slimeTypeFor(BiomeKind kind)
{
    switch (kind) {
    case BiomeKind::Meadow:  return 0;
    case BiomeKind::Crystal: return 1;
    case BiomeKind::Sakura:  return 2;
    case BiomeKind::Frost:   return 3;
    default:                 return -1;
    }
}

float smoothFactor(float speed, float deltaTime) { return 1.0f - std::exp(-speed * deltaTime); }

float facingOf(const glm::vec2& direction) { return std::atan2(direction.x, direction.y); }

glm::vec2 rotate2D(const glm::vec2& v, float angle)
{
    const float c = std::cos(angle), s = std::sin(angle);
    return {v.x * c - v.y * s, v.x * s + v.y * c};
}

Mesh accessoryMesh(int type)
{
    MeshBuilder b;
    const glm::mat4 I(1.0f);
    auto T = [](float x, float y, float z) { return glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z)); };
    auto S = [](float x, float y, float z) { return glm::scale(glm::mat4(1.0f), glm::vec3(x, y, z)); };
    auto rotZ = [](float degrees) { return glm::rotate(glm::mat4(1.0f), glm::radians(degrees), glm::vec3(0, 0, 1)); };

    switch (type) {
    case 0:
        b.addCone(I, 5, 0.035f, 0.22f, {0.30f, 0.58f, 0.26f});
        b.addSphere(T(0.09f, 0.18f, 0.0f) * rotZ(-30.0f) * S(0.11f, 0.03f, 0.06f), {0.45f, 0.80f, 0.35f}, 8, 5);
        b.addSphere(T(-0.09f, 0.18f, 0.0f) * rotZ(30.0f) * S(0.11f, 0.03f, 0.06f), {0.45f, 0.80f, 0.35f}, 8, 5);
        break;
    case 1:
        b.addPrism(T(0.0f, -0.03f, 0.0f), 6, 0.07f, 0.06f, 0.16f, 0.08f, {0.55f, 0.65f, 1.00f});
        b.addPrism(T(0.08f, -0.03f, 0.02f) * rotZ(-25.0f), 6, 0.045f, 0.04f, 0.09f, 0.05f, {0.80f, 0.65f, 1.00f});
        break;
    case 2:
        for (int i = 0; i < 5; ++i) {
            const float a = i / 5.0f * glm::two_pi<float>();
            b.addSphere(T(std::cos(a) * 0.08f, 0.02f, std::sin(a) * 0.08f)
                        * glm::rotate(I, -a, glm::vec3(0, 1, 0)) * S(0.075f, 0.022f, 0.05f),
                        {1.00f, 0.88f, 0.93f}, 8, 4);
        }
        b.addSphere(T(0.0f, 0.04f, 0.0f) * S(0.035f, 0.035f, 0.035f), {1.00f, 0.85f, 0.35f}, 6, 4);
        break;
    case 4:
        b.addPrism(I, 5, 0.035f, 0.03f, 0.12f, 0.0f, {0.85f, 0.95f, 1.00f});
        b.addSphere(T(0.0f, 0.13f, 0.0f) * S(0.11f, 0.06f, 0.11f), {0.85f, 0.50f, 1.00f}, 8, 5);
        break;
    default:
        b.addSphere(T(0.0f, 0.0f, 0.0f) * S(0.27f, 0.07f, 0.27f), {0.96f, 0.98f, 1.00f}, 12, 6);
        b.addPrism(T(0.0f, 0.04f, 0.0f), 6, 0.05f, 0.045f, 0.10f, 0.06f, {0.70f, 0.92f, 1.00f});
        break;
    }
    return b.build();
}

Mesh sphereMesh(const glm::vec3& color, int segments, int rings)
{
    MeshBuilder b;
    b.addSphere(glm::mat4(1.0f), color, segments, rings);
    return b.build();
}

}

Creatures::Creatures(const World& world, const CreatureSettings& settings)
    : settings(settings), m_shadowQuad(Mesh::createGroundQuad())
{
    const CreatureType* types[5] = {&settings.meadow, &settings.crystal, &settings.sakura, &settings.frost,
                                    &settings.cave};
    for (int t = 0; t < 5; ++t) {
        m_bodies.push_back(std::make_unique<InstancedBatch>(Mesh::createSphere(1.0f, types[t]->color, 20, 12)));
        m_accessories.push_back(std::make_unique<InstancedBatch>(accessoryMesh(t)));
    }
    m_eyes      = std::make_unique<InstancedBatch>(Mesh::createSphere(1.0f, {0.13f, 0.12f, 0.24f}, 10, 6));
    m_eyeShines = std::make_unique<InstancedBatch>(Mesh::createSphere(1.0f, {1.0f, 1.0f, 1.0f}, 8, 5));
    m_spiritCore     = std::make_unique<InstancedBatch>(sphereMesh({1.00f, 0.95f, 0.78f}, 14, 9));
    m_spiritSparkles = std::make_unique<InstancedBatch>(Mesh::createOctahedron(1.0f, 1.4f, {1.00f, 0.72f, 0.86f}));
    m_spiritTrail    = std::make_unique<InstancedBatch>(sphereMesh({1.00f, 0.86f, 0.95f}, 8, 5));
    generate(world);
}

const CreatureType& Creatures::typeOf(const Slime& slime) const
{
    switch (slime.type) {
    case 0:  return settings.meadow;
    case 1:  return settings.crystal;
    case 2:  return settings.sakura;
    case 3:  return settings.frost;
    default: return settings.cave;
    }
}

bool Creatures::isValidSpot(int island, const glm::vec2& p, float radius, const World& world) const
{
    if (const CaveIsland* cave = world.cave(); cave && cave->islandIndex() == island) {
        const glm::vec2 center(cave->chamberFloorCenter().x, cave->chamberFloorCenter().z);
        if (glm::length(p - center) > cave->chamberRadius() - 1.2f - radius)
            return false;
        if (glm::length(p - glm::vec2(cave->poolCenter().x, cave->poolCenter().z)) < cave->poolRadius() + radius)
            return false;
        for (const Collider& collider : world.colliders())
            if (collider.island == island && glm::length(p - collider.center) < collider.radius + radius)
                return false;
        return true;
    }

    const Island& is = world.islands()[island];
    const glm::vec2 center(is.center().x, is.center().z);
    const glm::vec2 offset = p - center;

    if (glm::length(offset) > settings.edgeMargin * is.coastDistance(std::atan2(offset.y, offset.x)))
        return false;
    for (const Collider& collider : world.colliders())
        if (collider.island == island && glm::length(p - collider.center) < collider.radius + radius)
            return false;
    for (const ClearZone& zone : world.clearZones())
        if (zone.island == island && zone.contains(p, radius))
            return false;
    return true;
}

void Creatures::generate(const World& world)
{
    m_slimes.clear();
    m_rng.seed(world.getSeed() * 7u + 3u);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);

    for (int i = 0; i < static_cast<int>(world.islands().size()); ++i) {
        const Island& island = world.islands()[i];
        const int type = slimeTypeFor(island.settings().biome.kind);
        if (type < 0)
            continue;

        Slime prototype;
        prototype.type = type;
        const int count = typeOf(prototype).count;
        int placed = 0;
        for (int attempt = 0; attempt < 500 && placed < count; ++attempt) {
            const float r = island.settings().radius * std::sqrt(unit(m_rng));
            const float a = unit(m_rng) * glm::two_pi<float>();
            const glm::vec2 p(island.center().x + r * std::cos(a), island.center().z + r * std::sin(a));
            const float size = 0.8f + 0.4f * unit(m_rng);
            if (!isValidSpot(i, p, BODY_RADIUS * size + 0.3f, world))
                continue;

            Slime slime;
            slime.island = i;
            slime.type = type;
            slime.position = p;
            slime.ground = island.heightAt(p.x, p.y);
            slime.yaw = unit(m_rng) * glm::two_pi<float>();
            slime.size = size;
            slime.timer = 1.0f + 3.0f * unit(m_rng);
            slime.seed = unit(m_rng) * 100.0f;
            m_slimes.push_back(slime);
            ++placed;
        }
    }

    if (const CaveIsland* cave = world.cave()) {
        const glm::vec3 c = cave->chamberFloorCenter();
        int placed = 0;
        for (int attempt = 0; attempt < 300 && placed < settings.cave.count; ++attempt) {
            const float r = cave->chamberRadius() * std::sqrt(unit(m_rng));
            const float a = unit(m_rng) * glm::two_pi<float>();
            const glm::vec2 p(c.x + r * std::cos(a), c.z + r * std::sin(a));
            const float size = settings.caveSlimeSize * (0.9f + 0.2f * unit(m_rng));
            if (!isValidSpot(cave->islandIndex(), p, BODY_RADIUS * size + 0.3f, world))
                continue;
            Slime slime;
            slime.island = cave->islandIndex();
            slime.type = 4;
            slime.position = p;
            slime.ground = cave->chamberFloorAt(p.x, p.y);
            slime.yaw = unit(m_rng) * glm::two_pi<float>();
            slime.size = size;
            slime.timer = 1.0f + 3.0f * unit(m_rng);
            slime.seed = unit(m_rng) * 100.0f;
            m_slimes.push_back(slime);
            ++placed;
        }
    }

    m_spirit = Spirit{};
    for (int i = 0; i < static_cast<int>(world.islands().size()); ++i) {
        const Island& island = world.islands()[i];
        if (island.settings().biome.kind == BiomeKind::Sakura) {
            m_spirit.exists = true;
            m_spirit.island = i;
            const glm::vec3 c = island.center();
            m_spirit.treeBase = {c.x, island.heightAt(c.x, c.z), c.z};
            m_spirit.position = m_spirit.treeBase + glm::vec3(settings.spiritOrbitRadius, settings.spiritOrbitHeight, 0.0f);
        }
    }
}

void Creatures::update(Player& player, const World& world, Collectibles& gems, float time, float deltaTime,
                       const std::vector<glm::vec3>& dangers)
{
    m_time = time;
    m_dangers = dangers;

    const glm::vec3 p = player.getPosition();
    const GroundHit ground = world.groundBelow(p, 0.3f);
    const int playerIsland = (ground.found && p.y - ground.height < 4.0f) ? ground.island : -1;

    for (Slime& slime : m_slimes)
        if (!(slime.type == 4 && m_caveHidden))
            updateSlime(slime, player, world, playerIsland, deltaTime);

    for (size_t i = 0; i < m_slimes.size(); ++i) {
        for (size_t j = i + 1; j < m_slimes.size(); ++j) {
            Slime& a = m_slimes[i];
            Slime& b = m_slimes[j];
            if (a.island != b.island)
                continue;
            const glm::vec2 offset = b.position - a.position;
            const float distance = glm::length(offset);
            const float minimum = BODY_RADIUS * (a.size + b.size);
            if (distance < minimum && distance > 1e-4f) {
                const glm::vec2 push = offset / distance * (minimum - distance) * 0.5f;
                a.position -= push;
                b.position += push;
            }
        }
    }

    for (const Slime& slime : m_slimes) {
        if (slime.type == 4 && m_caveHidden)
            continue;
        const float top = slime.ground + slime.lift + BODY_HEIGHT * slime.size;
        if (p.y < top && p.y > slime.ground - 2.0f)
            player.pushOutOfCircle(slime.position, BODY_RADIUS * slime.size);
    }

    updateSpirit(player, world, gems, time, deltaTime);
}

void Creatures::updateSlime(Slime& slime, Player& player, const World& world, int playerIsland, float deltaTime)
{
    const CreatureType& type = typeOf(slime);
    const glm::vec2 playerXZ(player.getPosition().x, player.getPosition().z);
    const glm::vec2 toPlayer = playerXZ - slime.position;
    const float playerDistance = glm::length(toPlayer);
    const bool playerHere = playerIsland == slime.island
                         && (slime.type != 4 || world.isInCave(player.getPosition()));

    slime.noticeCooldown = std::max(0.0f, slime.noticeCooldown - deltaTime);
    slime.timer -= deltaTime;
    const bool noticesPlayer = playerHere && playerDistance < type.noticeDistance && slime.noticeCooldown <= 0.0f;

    glm::vec2 wanted(0.0f);
    bool facePlayer = false;

    switch (slime.state) {
    case State::Idle:
        if (noticesPlayer) {
            slime.state = State::Notice;
            slime.timer = 0.6f;
        } else if (slime.timer <= 0.0f) {
            if (pickWanderTarget(slime, world)) {
                slime.state = State::Wander;
                slime.timer = 6.0f;
            } else {
                slime.timer = 1.0f;
            }
        }
        break;

    case State::Wander:
        if (noticesPlayer) {
            slime.state = State::Notice;
            slime.timer = 0.6f;
        } else if (glm::length(slime.target - slime.position) < 0.6f || slime.timer <= 0.0f) {
            slime.state = State::Idle;
            slime.timer = 1.5f + 3.0f * std::uniform_real_distribution<float>(0.0f, 1.0f)(m_rng);
        } else {
            wanted = glm::normalize(slime.target - slime.position);
        }
        break;

    case State::Notice:
        facePlayer = true;
        if (!slime.hopping && slime.timer > 0.45f) {
            slime.hopping = true;
            slime.hopPhase = 0.0f;
            slime.hopScale = 0.6f;
            slime.hopDirection = glm::vec2(0.0f);
        }
        if (slime.timer <= 0.0f)
            slime.state = type.reaction == Reaction::Follow ? State::Follow : State::Flee;
        break;

    case State::Follow:
        facePlayer = true;
        if (!playerHere || playerDistance > type.loseDistance) {
            slime.state = State::Idle;
            slime.timer = 2.0f;
            slime.noticeCooldown = 3.0f;
        } else if (playerDistance > type.followDistance) {
            wanted = toPlayer / playerDistance;
        }
        break;

    case State::Flee:
        if (!playerHere || playerDistance > type.loseDistance) {
            slime.state = State::Idle;
            slime.timer = 2.0f;
            slime.noticeCooldown = 3.0f;
        } else if (playerDistance > 1e-3f) {
            wanted = -toPlayer / playerDistance;
        }
        break;
    }

    for (const glm::vec3& danger : m_dangers) {
        const glm::vec2 away = slime.position - glm::vec2(danger.x, danger.z);
        if (glm::length(away) < 5.0f && glm::length(away) > 1e-3f && std::abs(danger.y - slime.ground) < 3.0f) {
            wanted = glm::normalize(away);
            facePlayer = false;
        }
    }

    const bool wantsToMove = glm::length(wanted) > 0.0f;
    if (!slime.hopping && wantsToMove) {
        slime.hopping = true;
        slime.hopPhase = 0.0f;
        slime.hopScale = 1.0f;
        slime.hopDirection = chooseHopDirection(slime, wanted, world);
    }

    if (slime.hopping) {
        slime.hopPhase += deltaTime / type.hopDuration;
        if (slime.hopPhase < 0.2f) {
            slime.lift = 0.0f;
            slime.squash = 1.0f - 0.25f * std::sin(glm::pi<float>() * slime.hopPhase / 0.2f);
        } else if (slime.hopPhase < 1.0f) {
            const float air = (slime.hopPhase - 0.2f) / 0.8f;
            slime.lift = type.hopHeight * slime.hopScale * std::sin(glm::pi<float>() * air);
            slime.squash = 1.0f + 0.18f * std::sin(glm::pi<float>() * air);
            slime.position += slime.hopDirection * (type.speed / 0.8f) * deltaTime;
        } else {
            slime.hopping = false;
            slime.lift = 0.0f;
            slime.squash = 0.85f;
        }
    } else {
        const float breathe = 1.0f + 0.04f * std::sin(slime.seed + 2.5f * m_time);
        slime.squash += (breathe - slime.squash) * smoothFactor(10.0f, deltaTime);
    }

    resolveConstraints(slime, world);
    if (slime.type == 4)
        slime.ground = world.cave()->chamberFloorAt(slime.position.x, slime.position.y);
    else
        slime.ground = world.islands()[slime.island].heightAt(slime.position.x, slime.position.y);

    float targetYaw = slime.yaw;
    if (facePlayer && playerDistance > 1e-3f)
        targetYaw = facingOf(toPlayer);
    else if (glm::length(slime.hopDirection) > 0.0f && slime.hopping)
        targetYaw = facingOf(slime.hopDirection);
    slime.yaw += shortestAngle(slime.yaw, targetYaw) * smoothFactor(8.0f, deltaTime);
}

bool Creatures::pickWanderTarget(Slime& slime, const World& world)
{
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    for (int attempt = 0; attempt < 12; ++attempt) {
        const float angle = unit(m_rng) * glm::two_pi<float>();
        const float distance = 2.0f + unit(m_rng) * settings.wanderRange;
        const glm::vec2 target = slime.position + distance * glm::vec2(std::cos(angle), std::sin(angle));
        if (isValidSpot(slime.island, target, BODY_RADIUS * slime.size + 0.2f, world)) {
            slime.target = target;
            return true;
        }
    }
    return false;
}

glm::vec2 Creatures::chooseHopDirection(const Slime& slime, const glm::vec2& wanted, const World& world) const
{
    const CreatureType& type = typeOf(slime);
    const float hopLength = type.speed * type.hopDuration;
    const float radius = BODY_RADIUS * slime.size;
    for (float degrees : {0.0f, 35.0f, -35.0f, 70.0f, -70.0f, 110.0f, -110.0f}) {
        const glm::vec2 direction = rotate2D(wanted, glm::radians(degrees));
        if (isValidSpot(slime.island, slime.position + direction * hopLength, radius, world))
            return direction;
    }
    return glm::vec2(0.0f);
}

void Creatures::resolveConstraints(Slime& slime, const World& world) const
{
    const float radius = BODY_RADIUS * slime.size;

    for (const Collider& collider : world.colliders()) {
        if (collider.island != slime.island)
            continue;
        const glm::vec2 offset = slime.position - collider.center;
        const float distance = glm::length(offset);
        const float minimum = collider.radius + radius;
        if (distance < minimum && distance > 1e-4f)
            slime.position = collider.center + offset / distance * minimum;
    }

    for (const ClearZone& zone : world.clearZones()) {
        if (zone.island != slime.island || !zone.contains(slime.position, radius))
            continue;
        const glm::vec2 closest = zone.closestPoint(slime.position);
        glm::vec2 away = slime.position - closest;
        away = glm::length(away) > 1e-4f ? glm::normalize(away) : glm::vec2(1.0f, 0.0f);
        slime.position = closest + away * (zone.radius + radius);
    }

    if (slime.type == 4) {
        const CaveIsland* cave = world.cave();
        const glm::vec2 center(cave->chamberFloorCenter().x, cave->chamberFloorCenter().z);
        const glm::vec2 offset = slime.position - center;
        const float limit = cave->chamberRadius() - 1.2f - radius;
        if (glm::length(offset) > limit)
            slime.position = center + glm::normalize(offset) * limit;
        return;
    }

    const Island& island = world.islands()[slime.island];
    const glm::vec2 center(island.center().x, island.center().z);
    const glm::vec2 offset = slime.position - center;
    const float distance = glm::length(offset);
    const float safe = settings.edgeMargin * island.coastDistance(std::atan2(offset.y, offset.x));
    if (distance > safe)
        slime.position = center + offset / distance * safe;
}

void Creatures::updateSpirit(Player& player, const World& world, Collectibles& gems, float time, float deltaTime)
{
    Spirit& s = m_spirit;
    if (!s.exists)
        return;

    const glm::vec3 p = player.getPosition();
    const float distanceToTree = glm::length(glm::vec2(p.x - s.treeBase.x, p.z - s.treeBase.z));
    const bool playerOnIsland = world.islands()[s.island].isOnIsland(p.x, p.z) && std::abs(p.y - s.treeBase.y) < 8.0f;

    if (!s.visiting && playerOnIsland && distanceToTree < settings.spiritVisitDistance) {
        s.visiting = true;
        s.visitTime = 0.0f;
    } else if (s.visiting && (!playerOnIsland || distanceToTree > settings.spiritVisitDistance + 6.0f)) {
        s.visiting = false;
    }

    glm::vec3 target;
    if (s.visiting) {
        s.angle += 1.8f * deltaTime;
        s.visitTime += deltaTime;
        target = p + glm::vec3(std::cos(s.angle) * 2.2f, 2.0f + 0.3f * std::sin(time * 3.0f), std::sin(s.angle) * 2.2f);

        if (!s.giftGiven && s.visitTime > 2.5f) {
            s.giftGiven = true;
            const float f = player.getFacing();
            glm::vec2 spot(p.x - std::sin(f) * 1.8f, p.z - std::cos(f) * 1.8f);
            bool blocked = !world.islands()[s.island].isOnIsland(spot.x, spot.y);
            for (const Collider& collider : world.colliders())
                if (glm::length(spot - collider.center) < collider.radius + 0.6f) blocked = true;
            if (blocked)
                spot = glm::vec2(p.x, p.z);
            const float ground = world.islands()[s.island].heightAt(spot.x, spot.y);
            gems.addGem(GemType::SpiritGift, {spot.x, ground + 1.0f, spot.y});
        }
    } else {
        s.angle += settings.spiritOrbitSpeed * deltaTime;
        target = s.treeBase + glm::vec3(std::cos(s.angle) * settings.spiritOrbitRadius,
                                        settings.spiritOrbitHeight + 0.6f * std::sin(time * 1.7f),
                                        std::sin(s.angle) * settings.spiritOrbitRadius);
    }
    s.position += (target - s.position) * smoothFactor(3.0f, deltaTime);

    s.trailTimer -= deltaTime;
    if (s.trailTimer <= 0.0f) {
        s.trailTimer = 0.03f;
        s.trail.push_front(s.position);
        if (s.trail.size() > 28)
            s.trail.pop_back();
    }
}

void Creatures::draw(const Shader& instancedShader, float time) const
{
    std::vector<std::vector<glm::mat4>> bodies(5), accessories(5);
    std::vector<glm::mat4> eyes, shines;

    for (const Slime& slime : m_slimes) {
        if (slime.type == 4 && m_caveHidden)
            continue;
        const float sy = slime.squash;
        const float sxz = 1.0f / std::sqrt(sy);

        glm::mat4 base = glm::translate(glm::mat4(1.0f), glm::vec3(slime.position.x, slime.ground + slime.lift - 0.03f,
                                                                   slime.position.y));
        base = glm::rotate(base, slime.yaw, glm::vec3(0.0f, 1.0f, 0.0f));
        base = glm::scale(base, glm::vec3(slime.size));

        const float a = BODY_RADIUS * sxz;
        const float b = BODY_HEIGHT * 0.5f * sy;
        bodies[slime.type].push_back(glm::scale(base, glm::vec3(a, b, a)) * glm::translate(glm::mat4(1.0f), {0, 1, 0}));

        const bool blink = std::fmod(time + slime.seed, 4.0f) < 0.12f;
        for (float side : {-1.0f, 1.0f}) {
            const float x = 0.17f * side * sxz;
            const float y = b + 0.11f * sy;
            const float inside = 1.0f - (x * x) / (a * a) - ((y - b) * (y - b)) / (b * b);
            const float z = a * std::sqrt(std::max(inside, 0.0f)) - 0.015f;
            glm::mat4 eye = glm::translate(base, glm::vec3(x, y, z));
            eyes.push_back(glm::scale(eye, glm::vec3(0.065f, blink ? 0.012f : 0.095f, 0.045f)));
            if (!blink)
                shines.push_back(glm::scale(glm::translate(eye, glm::vec3(0.02f, 0.035f, 0.03f)), glm::vec3(0.024f)));
        }

        accessories[slime.type].push_back(glm::translate(base, glm::vec3(0.0f, 2.0f * b - 0.04f, 0.0f)));
    }

    Lighting::applyMaterial(instancedShader, SLIME_MATERIAL);
    for (int t = 0; t < 5; ++t) {
        m_bodies[t]->setInstances(bodies[t], true);
        m_accessories[t]->setInstances(accessories[t], true);
        if (t == 4)
            continue;
        m_bodies[t]->draw();
        m_accessories[t]->draw();
    }
    Lighting::applyMaterial(instancedShader, EYE_MATERIAL);
    m_eyes->setInstances(eyes, true);
    m_eyes->draw();
    m_eyeShines->setInstances(shines, true);
    m_eyeShines->draw();
}

void Creatures::appendLights(std::vector<PointLight>& lights) const
{
    for (const Slime& slime : m_slimes) {
        if (slime.type != 4 || m_caveHidden)
            continue;
        PointLight light;
        light.position = glm::vec3(slime.position.x, slime.ground + slime.lift + 0.5f, slime.position.y);
        light.color = settings.cave.color;
        light.intensity = settings.caveSlimeLight;
        light.radius = 3.5f;
        lights.push_back(light);
    }
}

void Creatures::drawGlow(const Shader& gemShader, float time) const
{
    gemShader.setInt("uRainbow", 0);
    gemShader.setFloat("uAlpha", 1.0f);
    m_bodies[4]->draw();
    m_accessories[4]->draw();

    const Spirit& s = m_spirit;
    if (!s.exists)
        return;
    gemShader.setInt("uRainbow", 0);
    gemShader.setFloat("uAlpha", 1.0f);

    const float pulse = 1.0f + 0.12f * std::sin(time * 6.0f);
    const std::vector<glm::mat4> core = {glm::scale(glm::translate(glm::mat4(1.0f), s.position), glm::vec3(0.28f * pulse))};
    m_spiritCore->setInstances(core, true);
    m_spiritCore->draw();

    std::vector<glm::mat4> sparkles;
    for (int i = 0; i < 3; ++i) {
        const float a = time * 3.0f + i * glm::two_pi<float>() / 3.0f;
        const glm::vec3 offset(std::cos(a) * 0.5f, 0.15f * std::sin(a * 2.0f), std::sin(a) * 0.5f);
        glm::mat4 m = glm::translate(glm::mat4(1.0f), s.position + offset);
        sparkles.push_back(glm::scale(glm::rotate(m, a * 2.0f, glm::vec3(0, 1, 0)), glm::vec3(0.07f)));
    }
    m_spiritSparkles->setInstances(sparkles, true);
    m_spiritSparkles->draw();

    std::vector<glm::mat4> trail;
    for (size_t i = 1; i < s.trail.size(); ++i) {
        const float age = i / static_cast<float>(s.trail.size());
        trail.push_back(glm::scale(glm::translate(glm::mat4(1.0f), s.trail[i]), glm::vec3(0.16f * (1.0f - age))));
    }
    m_spiritTrail->setInstances(trail, true);
    m_spiritTrail->draw();
}

void Creatures::drawShadows(const Shader& blobShader) const
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);

    blobShader.setVec3("uColor", glm::vec3(0.16f, 0.13f, 0.28f));
    for (const Slime& slime : m_slimes) {
        if (slime.type == 4 && m_caveHidden)
            continue;
        const float shrink = 1.0f - std::min(slime.lift, 1.0f) * 0.35f;
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(slime.position.x, slime.ground + 0.04f, slime.position.y));
        model = glm::scale(model, glm::vec3(BODY_RADIUS * slime.size * 1.15f * shrink));
        blobShader.setMat4("uModel", model);
        blobShader.setFloat("uOpacity", 0.38f * shrink);
        m_shadowQuad.draw();
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

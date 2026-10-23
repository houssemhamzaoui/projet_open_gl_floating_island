#include "Collectibles.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <random>

namespace {

constexpr int TYPE_COUNT = static_cast<int>(GemType::Count);
int index(GemType type) { return static_cast<int>(type); }

struct GemLook { glm::vec3 color; float size; };
const GemLook LOOKS[TYPE_COUNT] = {
    {{0.35f, 0.62f, 1.00f}, 0.32f},
    {{0.96f, 0.97f, 1.00f}, 0.50f},
    {{0.70f, 0.40f, 1.00f}, 0.38f},
    {{1.00f, 0.52f, 0.74f}, 0.38f},
    {{0.60f, 0.95f, 1.00f}, 0.38f},
    {{1.00f, 0.82f, 0.40f}, 0.45f},
    {{1.00f, 0.35f, 0.60f}, 0.90f},
};

}

Collectibles::Collectibles(const World& world, const std::vector<glm::vec3>& rareSpots,
                           const CollectibleSettings& settings)
    : settings(settings),
      m_shader("shaders/instanced.vert", "shaders/gem.frag")
{
    for (int t = 0; t < TYPE_COUNT; ++t) {
        m_batches.push_back(std::make_unique<InstancedBatch>(Mesh::createOctahedron(1.0f, 1.4f, LOOKS[t].color)));
        m_effectBatches.push_back(std::make_unique<InstancedBatch>(Mesh::createOctahedron(1.0f, 1.4f, LOOKS[t].color)));
    }
    generate(world, rareSpots);
}

void Collectibles::generate(const World& world, const std::vector<glm::vec3>& rareSpots)
{
    m_gems.clear();
    m_effects.clear();
    std::vector<glm::vec3> taken;

    for (const glm::vec3& spot : rareSpots) {
        m_gems.push_back({GemType::Rare, spot, 0.0f});
        taken.push_back(spot);
    }

    for (int i = 0; i < static_cast<int>(world.islands().size()); ++i) {
        switch (world.islands()[i].settings().biome.kind) {
        case BiomeKind::Meadow:
            scatterGems(world, i, GemType::Common, settings.commonOnMain, taken);
            break;
        case BiomeKind::Crystal:
            scatterGems(world, i, GemType::Common, settings.commonPerSide, taken);
            scatterGems(world, i, GemType::Amethyst, settings.biomeGems, taken);
            break;
        case BiomeKind::Sakura:
            scatterGems(world, i, GemType::Common, settings.commonPerSide, taken);
            scatterGems(world, i, GemType::SakuraGem, settings.biomeGems, taken);
            break;
        case BiomeKind::Frost:
            scatterGems(world, i, GemType::Common, settings.commonPerSide, taken);
            scatterGems(world, i, GemType::IceGem, settings.biomeGems, taken);
            break;
        case BiomeKind::Rocky:
            scatterGems(world, i, GemType::Common, settings.commonPerSide, taken);
            break;
        case BiomeKind::SkyRock:
            break;
        case BiomeKind::Cave:
            if (const CaveIsland* cave = world.cave()) {
                for (const glm::vec3& spot : cave->gemSpots()) {
                    m_gems.push_back({GemType::Common, spot, static_cast<float>(m_gems.size())});
                    taken.push_back(spot);
                }
                m_gems.push_back({GemType::CaveHeart, cave->heartPosition(), 0.0f});
            }
            break;
        }
    }
}

void Collectibles::scatterGems(const World& world, int islandIndex, GemType type, int count,
                               std::vector<glm::vec3>& taken)
{
    const Island& island = world.islands()[islandIndex];
    const IslandSettings& is = island.settings();
    std::mt19937 rng(is.seed * 13u + static_cast<unsigned>(type) * 101u + 5u);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);

    int placed = 0;
    for (int attempt = 0; attempt < 2000 && placed < count; ++attempt) {
        const float r = is.radius * 0.85f * std::sqrt(unit(rng));
        const float a = unit(rng) * glm::two_pi<float>();
        const float x = is.position.x + r * std::cos(a);
        const float z = is.position.z + r * std::sin(a);
        if (!island.isOnIsland(x, z))
            continue;
        const float ground = island.heightAt(x, z);
        if (ground - is.position.y < is.biome.plateauHeight * 0.6f)
            continue;

        bool blocked = false;
        for (const Collider& collider : world.colliders())
            if (glm::length(glm::vec2(x, z) - collider.center) < collider.radius + 1.0f) { blocked = true; break; }
        const glm::vec3 position(x, ground + settings.floatHeight, z);
        for (const glm::vec3& other : taken)
            if (glm::length(position - other) < 5.0f) { blocked = true; break; }
        if (blocked)
            continue;

        m_gems.push_back({type, position, unit(rng) * 6.28f});
        taken.push_back(position);
        ++placed;
    }
}

glm::mat4 Collectibles::gemMatrix(GemType type, const glm::vec3& position, float angle, float scale) const
{
    glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
    model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
    return glm::scale(model, glm::vec3(LOOKS[index(type)].size * scale));
}

void Collectibles::addGem(GemType type, const glm::vec3& position)
{
    m_gems.push_back({type, position, 0.0f});
}

namespace {
const char* gemId(GemType type)
{
    switch (type) {
    case GemType::Common:     return "common";
    case GemType::Rare:       return "rare";
    case GemType::Amethyst:   return "amethyst";
    case GemType::SakuraGem:  return "sakura";
    case GemType::IceGem:     return "ice";
    case GemType::SpiritGift: return "spirit";
    case GemType::CaveHeart:  return "caveheart";
    default:                  return "";
    }
}
}

bool Collectibles::update(const glm::vec3& playerPosition, float time, float deltaTime, const EventBus& events)
{
    bool collectedSomething = false;
    const glm::vec3 body = playerPosition + glm::vec3(0.0f, 0.8f, 0.0f);
    for (Gem& gem : m_gems) {
        const float reach = gem.type == GemType::CaveHeart ? 2.0f : settings.collectRadius;
        if (gem.type == GemType::CaveHeart && m_heartLocked)
            continue;
        if (!gem.collected && glm::length(body - gem.position) < reach) {
            gem.collected = true;
            m_effects.push_back({gem.type, gem.position});
            if (gem.type == GemType::CaveHeart)
                events.emit({EventType::ItemCollected, "caveheart"});
            else
                events.emit({EventType::GemCollected, gemId(gem.type)});
            collectedSomething = true;
        }
    }

    for (Effect& effect : m_effects)
        effect.age += deltaTime;
    m_effects.erase(std::remove_if(m_effects.begin(), m_effects.end(),
                                   [&](const Effect& e) { return e.age >= settings.effectDuration; }),
                    m_effects.end());

    std::vector<std::vector<glm::mat4>> matrices(TYPE_COUNT);
    for (const Gem& gem : m_gems) {
        if (gem.collected)
            continue;
        const float bob = std::sin(time * 2.0f + gem.phase) * settings.bobHeight;
        matrices[index(gem.type)].push_back(
            gemMatrix(gem.type, gem.position + glm::vec3(0.0f, bob, 0.0f), time * settings.spinSpeed + gem.phase, 1.0f));
    }
    for (int t = 0; t < TYPE_COUNT; ++t)
        m_batches[t]->setInstances(matrices[t], true);

    return collectedSomething;
}

void Collectibles::draw(float time) const
{
    m_shader.setFloat("uTime", time);
    m_shader.setFloat("uAlpha", 1.0f);
    for (int t = 0; t < TYPE_COUNT; ++t) {
        m_shader.setInt("uRainbow", t == index(GemType::Rare) ? 1 : 0);
        m_batches[t]->draw();
    }
}

void Collectibles::drawEffects(float time) const
{
    if (m_effects.empty())
        return;

    m_shader.use();
    m_shader.setFloat("uTime", time);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    for (const Effect& effect : m_effects) {
        const float t = effect.age / settings.effectDuration;
        const bool big = effect.type == GemType::CaveHeart;
        const int shards = big ? 18 : 6;
        const float reach = big ? 6.0f : 2.5f;
        std::vector<glm::mat4> pieces;
        pieces.push_back(gemMatrix(effect.type, effect.position, t * 6.0f, 1.0f + (big ? 2.5f : 1.5f) * t));
        for (int i = 0; i < shards; ++i) {
            const float a = i / static_cast<float>(shards) * glm::two_pi<float>();
            const glm::vec3 direction(std::cos(a), big ? 0.3f + 0.6f * (i % 3) : 0.6f, std::sin(a));
            pieces.push_back(gemMatrix(effect.type, effect.position + direction * t * reach, a + t * 8.0f,
                                       0.35f * (1.0f - t)));
        }
        m_effectBatches[index(effect.type)]->setInstances(pieces, true);
        m_shader.setInt("uRainbow", effect.type == GemType::Rare ? 1 : 0);
        m_shader.setFloat("uAlpha", 1.0f - t);
        m_effectBatches[index(effect.type)]->draw();
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

bool Collectibles::caveHeartCollected() const
{
    for (const Gem& gem : m_gems)
        if (gem.type == GemType::CaveHeart)
            return gem.collected;
    return false;
}

bool Collectibles::spiritGiftCollected() const
{
    for (const Gem& gem : m_gems)
        if (gem.type == GemType::SpiritGift && gem.collected)
            return true;
    return false;
}

std::string Collectibles::collectedMask() const
{
    std::string mask;
    for (const Gem& gem : m_gems)
        if (gem.type != GemType::SpiritGift)
            mask += gem.collected ? '1' : '0';
    return mask;
}

bool Collectibles::restoreCollected(const std::string& mask)
{
    size_t placed = 0;
    for (const Gem& gem : m_gems)
        if (gem.type != GemType::SpiritGift)
            ++placed;
    if (mask.size() != placed)
        return false;
    size_t i = 0;
    for (Gem& gem : m_gems)
        if (gem.type != GemType::SpiritGift)
            gem.collected = (mask[i++] == '1');
    return true;
}

void Collectibles::restoreCaveHeart()
{
    for (Gem& gem : m_gems)
        if (gem.type == GemType::CaveHeart)
            gem.collected = true;
}

void Collectibles::restoreSpiritGift()
{
    if (spiritGiftCollected())
        return;
    m_gems.push_back({GemType::SpiritGift, glm::vec3(0.0f), 0.0f, true});
}

GemCounts Collectibles::counts() const
{
    GemCounts c;
    for (const Gem& gem : m_gems) {
        const int got = gem.collected ? 1 : 0;
        switch (gem.type) {
        case GemType::Common:     c.common += got; ++c.commonTotal; break;
        case GemType::Rare:       c.rare += got;   ++c.rareTotal;   break;
        case GemType::SpiritGift: c.spirit += got;                  break;
        case GemType::CaveHeart:  c.heart = gem.collected;          break;
        default:                  c.biome += got;  ++c.biomeTotal;  break;
        }
    }
    return c;
}

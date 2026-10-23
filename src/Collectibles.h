#pragma once

#include "EventBus.h"
#include "InstancedBatch.h"
#include "Shader.h"
#include "World.h"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <vector>

enum class GemType { Common, Rare, Amethyst, SakuraGem, IceGem, SpiritGift, CaveHeart, Count };

struct GemCounts {
    int common = 0, commonTotal = 0;
    int biome = 0, biomeTotal = 0;
    int rare = 0, rareTotal = 0;
    int spirit = 0;
    bool heart = false;
};

struct CollectibleSettings {
    float collectRadius  = 1.3f;
    float floatHeight    = 1.0f;
    float spinSpeed      = 1.5f;
    float bobHeight      = 0.15f;
    float effectDuration = 0.5f;
    int   commonOnMain   = 8;
    int   commonPerSide  = 3;
    int   biomeGems      = 5;
};

class Collectibles {
public:
    Collectibles(const World& world, const std::vector<glm::vec3>& rareSpots,
                 const CollectibleSettings& settings = {});

    CollectibleSettings settings;

    void generate(const World& world, const std::vector<glm::vec3>& rareSpots);

    void setHeartLocked(bool locked) { m_heartLocked = locked; }

    void addGem(GemType type, const glm::vec3& position);

    bool update(const glm::vec3& playerPosition, float time, float deltaTime, const EventBus& events);

    const Shader& shader() const { return m_shader; }

    void draw(float time) const;
    void drawEffects(float time) const;

    bool caveHeartCollected() const;
    bool spiritGiftCollected() const;

    std::string collectedMask() const;
    bool restoreCollected(const std::string& mask);
    void restoreCaveHeart();
    void restoreSpiritGift();

    GemCounts counts() const;

    struct Gem {
        GemType   type;
        glm::vec3 position;
        float     phase;
        bool      collected = false;
    };

private:
    struct Effect {
        GemType   type;
        glm::vec3 position;
        float     age = 0.0f;
    };

    void scatterGems(const World& world, int islandIndex, GemType type, int count, std::vector<glm::vec3>& taken);
    glm::mat4 gemMatrix(GemType type, const glm::vec3& position, float angle, float scale) const;

    Shader m_shader;
    std::vector<std::unique_ptr<InstancedBatch>> m_batches;
    std::vector<std::unique_ptr<InstancedBatch>> m_effectBatches;
    std::vector<Gem>    m_gems;
    std::vector<Effect> m_effects;
    bool m_heartLocked = false;
};

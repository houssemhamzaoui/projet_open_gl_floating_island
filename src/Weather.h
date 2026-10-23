#pragma once

#include "ParticleSystem.h"
#include "Shader.h"
#include "World.h"

#include <glm/glm.hpp>

#include <memory>
#include <random>
#include <string>
#include <vector>

enum class WeatherState { Clear, Cloudy, Rain, Clearing, Snow, Blizzard, Petals };

struct WeatherSettings {
    int   rainDrops   = 2200;
    int   snowFlakes  = 3000;
    int   petals      = 700;
    int   splashes    = 250;
    glm::vec3 boxSize{30.0f, 20.0f, 30.0f};
    float boxBelow     = 7.0f;
    float clearTime    = 100.0f;
    float cloudyTime   = 25.0f;
    float rainTime     = 60.0f;
    float clearingTime = 25.0f;
    float transitionRate = 0.35f;
    float rainCloudBoost  = 0.40f;
    float rainLightDim    = 0.45f;
    float rainFogBoost    = 0.006f;
    float wetSpecular     = 0.30f;
    float rainSpeed    = 22.0f;
    float snowSpeed    = 1.4f;
    float petalSpeed   = 1.0f;
    glm::vec2 wind{1.6f, 0.6f};
};

class Weather {
public:
    explicit Weather(const World& world, const WeatherSettings& settings = {});

    WeatherSettings settings;

    void generate(const World& world);

    void update(const World& world, const glm::vec3& camera, bool enabled, float deltaTime);

    void cycle(int island);
    WeatherState stateOf(int island) const { return m_zones[island].state; }

    float clouds() const { return m_clouds; }
    float rain() const { return m_rain; }
    WeatherState stateNear() const { return m_stateNear; }
    static std::string name(WeatherState state);
    static std::string id(WeatherState state);
    static bool fromId(const std::string& text, WeatherState& state);

    void setState(int island, WeatherState state);
    int  zoneCount() const { return static_cast<int>(m_zones.size()); }

    void draw(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& camera,
              const glm::vec3& light, const glm::vec3& fogColor, float fogDensity);
    int drawnLastFrame() const { return m_drawn; }

private:
    struct Zone {
        WeatherState state = WeatherState::Clear;
        float timer = 0.0f;
        bool  cycles = false;
        std::vector<WeatherState> choices;
        float clouds = 0.0f, rain = 0.0f, snow = 0.0f, petals = 0.0f;
        glm::vec2 center{0.0f};
        float radius = 10.0f, topY = 0.0f;
    };
    enum class Kind { Rain, Snow, Petal };

    float amountAt(Kind kind, float x, float z) const;
    float zoneWeight(const Zone& zone, float x, float z, float inner, float outer) const;
    void  updatePool(ParticleSystem& pool, Kind kind, const World& world, const glm::vec3& camera, float deltaTime);
    void  respawnAtTop(Particle& p, const glm::vec3& camera);
    void  splash(const glm::vec3& at);

    std::vector<Zone> m_zones;
    float m_clouds = 0.0f, m_rain = 0.0f, m_visibility = 1.0f;
    WeatherState m_stateNear = WeatherState::Clear;
    float m_time = 0.0f;
    int   m_drawn = 0;
    std::mt19937 m_rng{99u};

    Shader m_shader;
    std::unique_ptr<ParticleSystem> m_rainPool, m_snowPool, m_petalPool, m_splashPool;
    size_t m_nextSplash = 0;
};

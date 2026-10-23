#include "Weather.h"
#include "MathUtils.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/noise.hpp>

#include <algorithm>
#include <cmath>

namespace {
float ease(float current, float target, float rate, float deltaTime)
{
    return current + (target - current) * (1.0f - std::exp(-rate * deltaTime));
}
}

Weather::Weather(const World& world, const WeatherSettings& settings)
    : settings(settings), m_shader("shaders/particle.vert", "shaders/particle.frag")
{
    m_rainPool   = std::make_unique<ParticleSystem>(settings.rainDrops, ParticleShape::Streak);
    m_snowPool   = std::make_unique<ParticleSystem>(settings.snowFlakes, ParticleShape::Flake);
    m_petalPool  = std::make_unique<ParticleSystem>(settings.petals, ParticleShape::Petal);
    m_splashPool = std::make_unique<ParticleSystem>(settings.splashes, ParticleShape::Ring);

    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    auto scatter = [&](ParticleSystem& pool, float size, float stretch, float spin) {
        for (Particle& p : pool.particles()) {
            p.position = (glm::vec3(unit(m_rng), unit(m_rng), unit(m_rng)) - 0.5f) * settings.boxSize;
            p.size = size * (0.6f + 0.8f * unit(m_rng));
            p.stretch = stretch;
            p.seed = unit(m_rng) * 100.0f;
            p.rotation = unit(m_rng) * glm::two_pi<float>();
            p.spin = (unit(m_rng) - 0.5f) * spin;
        }
    };
    scatter(*m_rainPool, 0.022f, 22.0f, 0.0f);
    scatter(*m_snowPool, 0.10f, 1.0f, 0.0f);
    scatter(*m_petalPool, 0.14f, 0.75f, 5.0f);
    for (Particle& p : m_splashPool->particles())
        p.active = false;
    generate(world);
}

void Weather::generate(const World& world)
{
    m_zones.clear();
    for (const Island& island : world.islands()) {
        Zone zone;
        zone.center = {island.center().x, island.center().z};
        zone.radius = island.settings().radius;
        zone.topY = island.center().y + island.settings().biome.plateauHeight + island.settings().biome.bumpHeight + 1.0f;
        switch (island.settings().biome.kind) {
        case BiomeKind::Meadow:
            zone.state = WeatherState::Clear;
            zone.timer = settings.clearTime * 0.5f;
            zone.cycles = true;
            zone.choices = std::vector<WeatherState>{WeatherState::Clear, WeatherState::Cloudy, WeatherState::Rain, WeatherState::Clearing};
            break;
        case BiomeKind::Frost:
            zone.state = WeatherState::Snow;
            zone.choices = std::vector<WeatherState>{WeatherState::Snow, WeatherState::Blizzard, WeatherState::Clear};
            break;
        case BiomeKind::Sakura:
            zone.state = WeatherState::Petals;
            zone.choices = std::vector<WeatherState>{WeatherState::Petals, WeatherState::Clear};
            break;
        default:
            zone.choices = std::vector<WeatherState>{WeatherState::Clear, WeatherState::Cloudy};
            break;
        }
        m_zones.push_back(zone);
    }
}

void Weather::cycle(int island)
{
    if (island < 0 || island >= static_cast<int>(m_zones.size()))
        return;
    Zone& zone = m_zones[island];
    const auto it = std::find(zone.choices.begin(), zone.choices.end(), zone.state);
    const size_t next = it == zone.choices.end() ? 0 : (it - zone.choices.begin() + 1) % zone.choices.size();
    zone.state = zone.choices[next];
    zone.timer = zone.state == WeatherState::Rain ? settings.rainTime : settings.cloudyTime;
}

void Weather::setState(int island, WeatherState state)
{
    if (island < 0 || island >= static_cast<int>(m_zones.size()))
        return;
    Zone& zone = m_zones[island];
    if (std::find(zone.choices.begin(), zone.choices.end(), state) == zone.choices.end())
        return;
    zone.state = state;
    zone.timer = state == WeatherState::Rain ? settings.rainTime : settings.cloudyTime;
}

std::string Weather::id(WeatherState state)
{
    const std::string text = name(state);
    return text == "heavy snow" ? "blizzard" : text;
}

bool Weather::fromId(const std::string& text, WeatherState& state)
{
    const WeatherState all[] = {WeatherState::Clear, WeatherState::Cloudy, WeatherState::Rain, WeatherState::Clearing,
                                WeatherState::Snow, WeatherState::Blizzard, WeatherState::Petals};
    for (WeatherState candidate : all) {
        if (id(candidate) == text) {
            state = candidate;
            return true;
        }
    }
    return false;
}

std::string Weather::name(WeatherState state)
{
    switch (state) {
    case WeatherState::Clear:    return "clear";
    case WeatherState::Cloudy:   return "cloudy";
    case WeatherState::Rain:     return "rain";
    case WeatherState::Clearing: return "clearing";
    case WeatherState::Snow:     return "snow";
    case WeatherState::Blizzard: return "heavy snow";
    case WeatherState::Petals:   return "petals";
    }
    return "";
}

float Weather::zoneWeight(const Zone& zone, float x, float z, float inner, float outer) const
{
    const float d = glm::length(glm::vec2(x, z) - zone.center) / zone.radius;
    return 1.0f - glm::smoothstep(inner, outer, d);
}

float Weather::amountAt(Kind kind, float x, float z) const
{
    float best = 0.0f;
    for (const Zone& zone : m_zones) {
        const float amount = kind == Kind::Rain ? zone.rain : kind == Kind::Snow ? zone.snow : zone.petals;
        if (amount > 0.0f)
            best = std::max(best, amount * zoneWeight(zone, x, z, 1.0f, 1.3f));
    }
    return best;
}

void Weather::update(const World& world, const glm::vec3& camera, bool enabled, float deltaTime)
{
    const WeatherSettings& s = settings;
    m_time += deltaTime;
    m_visibility = ease(m_visibility, enabled ? 1.0f : 0.0f, 4.0f, deltaTime);

    for (Zone& zone : m_zones) {
        if (zone.cycles) {
            zone.timer -= deltaTime;
            if (zone.timer <= 0.0f) {
                switch (zone.state) {
                case WeatherState::Clear:    zone.state = WeatherState::Cloudy;   zone.timer = s.cloudyTime;   break;
                case WeatherState::Cloudy:   zone.state = WeatherState::Rain;     zone.timer = s.rainTime;     break;
                case WeatherState::Rain:     zone.state = WeatherState::Clearing; zone.timer = s.clearingTime; break;
                default:                     zone.state = WeatherState::Clear;    zone.timer = s.clearTime;    break;
                }
            }
        }
        float clouds = 0.0f, rain = 0.0f, snow = 0.0f, petals = 0.0f;
        switch (zone.state) {
        case WeatherState::Clear:    break;
        case WeatherState::Cloudy:   clouds = 1.0f; break;
        case WeatherState::Rain:     clouds = 1.0f; rain = 1.0f; break;
        case WeatherState::Clearing: clouds = 0.5f; break;
        case WeatherState::Snow:     clouds = 0.5f; snow = 0.7f; break;
        case WeatherState::Blizzard: clouds = 0.9f; snow = 1.0f; break;
        case WeatherState::Petals:   clouds = 0.15f; petals = 1.0f; break;
        }
        zone.clouds = ease(zone.clouds, clouds, s.transitionRate, deltaTime);
        zone.rain   = ease(zone.rain, rain * glm::smoothstep(0.6f, 0.95f, zone.clouds), s.transitionRate, deltaTime);
        zone.snow   = ease(zone.snow, snow, s.transitionRate, deltaTime);
        zone.petals = ease(zone.petals, petals, s.transitionRate, deltaTime);
    }

    float clouds = 0.0f, rain = 0.0f, bestWeight = 0.0f;
    for (const Zone& zone : m_zones) {
        const float w = zoneWeight(zone, camera.x, camera.z, 1.0f, 1.8f);
        clouds = std::max(clouds, w * zone.clouds);
        rain = std::max(rain, w * zone.rain);
        if (w > bestWeight) { bestWeight = w; m_stateNear = zone.state; }
    }
    if (bestWeight <= 0.0f)
        m_stateNear = WeatherState::Clear;
    m_clouds = ease(m_clouds, clouds * m_visibility, 1.0f, deltaTime);
    m_rain = ease(m_rain, rain * m_visibility, 1.0f, deltaTime);

    updatePool(*m_rainPool, Kind::Rain, world, camera, deltaTime);
    updatePool(*m_snowPool, Kind::Snow, world, camera, deltaTime);
    updatePool(*m_petalPool, Kind::Petal, world, camera, deltaTime);
    for (Particle& p : m_splashPool->particles()) {
        if (!p.active)
            continue;
        p.age += deltaTime;
        p.size = 0.05f + 0.25f * (p.age / p.life);
        p.alpha = (1.0f - p.age / p.life) * m_visibility;
        if (p.age >= p.life)
            p.active = false;
    }
}

void Weather::respawnAtTop(Particle& p, const glm::vec3& camera)
{
    p.position.y = camera.y - settings.boxBelow + settings.boxSize.y - 0.5f;
}

void Weather::splash(const glm::vec3& at)
{
    Particle& p = m_splashPool->particles()[m_nextSplash];
    m_nextSplash = (m_nextSplash + 1) % m_splashPool->particles().size();
    p.active = true;
    p.position = at + glm::vec3(0.0f, 0.05f, 0.0f);
    p.age = 0.0f;
    p.life = 0.28f;
    p.stretch = 0.35f;
    p.rotation = 0.0f;
}

void Weather::updatePool(ParticleSystem& pool, Kind kind, const World& world, const glm::vec3& camera, float deltaTime)
{
    const WeatherSettings& s = settings;
    bool any = false;
    for (const Zone& zone : m_zones) {
        const float amount = kind == Kind::Rain ? zone.rain : kind == Kind::Snow ? zone.snow : zone.petals;
        if (amount > 0.01f && glm::length(glm::vec2(camera.x, camera.z) - zone.center) < zone.radius * 1.3f + s.boxSize.x)
            any = true;
    }
    if (!any || m_visibility < 0.01f) {
        for (Particle& p : pool.particles())
            p.alpha = 0.0f;
        return;
    }

    const glm::vec3 boxMin = camera - glm::vec3(s.boxSize.x * 0.5f, s.boxBelow, s.boxSize.z * 0.5f);
    const int count = static_cast<int>(pool.particles().size());
    for (int i = 0; i < count; ++i) {
        Particle& p = pool.particles()[i];

        glm::vec3 velocity(0.0f);
        switch (kind) {
        case Kind::Rain:
            velocity = glm::vec3(s.wind.x * 0.3f, -s.rainSpeed, s.wind.y * 0.3f);
            break;
        case Kind::Snow: {
            const float sway = std::sin(m_time * 1.3f + p.seed) * 0.6f;
            velocity = glm::vec3(s.wind.x * 0.35f + sway, -s.snowSpeed * (0.7f + 0.3f * std::sin(p.seed)),
                                 s.wind.y * 0.35f + std::cos(m_time * 1.1f + p.seed) * 0.4f);
            break;
        }
        case Kind::Petal: {
            const float flutter = glm::simplex(glm::vec2(p.seed, m_time * 0.6f));
            velocity = glm::vec3(s.wind.x * (1.0f + 0.5f * flutter), -s.petalSpeed * (0.6f + 0.4f * flutter),
                                 s.wind.y + 0.8f * glm::simplex(glm::vec2(p.seed + 9.0f, m_time * 0.5f)));
            p.rotation += p.spin * deltaTime;
            p.stretch = 0.25f + 0.6f * std::abs(std::sin(m_time * 2.0f + p.seed));
            break;
        }
        }
        p.position += velocity * deltaTime;

        p.position = boxMin + wrapPeriodic(p.position - boxMin, s.boxSize);

        for (const Zone& zone : m_zones) {
            if (p.position.y > zone.topY || glm::length(glm::vec2(p.position.x, p.position.z) - zone.center) > zone.radius * 1.3f)
                continue;
            const Island& island = world.islands()[&zone - m_zones.data()];
            if (island.isOnIsland(p.position.x, p.position.z)
                && p.position.y < island.heightAt(p.position.x, p.position.z)) {
                if (kind == Kind::Rain && (i % 3) == 0 && p.alpha > 0.2f)
                    splash({p.position.x, island.heightAt(p.position.x, p.position.z), p.position.z});
                respawnAtTop(p, camera);
                break;
            }
        }

        float alpha = amountAt(kind, p.position.x, p.position.z) * m_visibility;
        const glm::vec3 local = p.position - boxMin;
        const glm::vec3 edge = glm::min(local, s.boxSize - local);
        alpha *= glm::smoothstep(0.0f, 3.0f, std::min(edge.x, std::min(edge.y, edge.z)));
        p.alpha = alpha;
    }
}

void Weather::draw(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& camera,
                   const glm::vec3& light, const glm::vec3& fogColor, float fogDensity)
{
    m_drawn = 0;
    if (m_visibility < 0.01f)
        return;
    m_shader.use();
    m_shader.setMat4("uView", view);
    m_shader.setMat4("uProjection", projection);
    m_shader.setVec3("uCameraRight", glm::vec3(view[0][0], view[1][0], view[2][0]));
    m_shader.setVec3("uCameraUp",    glm::vec3(view[0][1], view[1][1], view[2][1]));
    m_shader.setVec3("uLight", light);
    m_shader.setVec3("uCameraPos", camera);
    m_shader.setVec3("uFogColor", fogColor);
    m_shader.setFloat("uFogDensity", fogDensity);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    m_drawn += m_rainPool->draw();
    m_drawn += m_snowPool->draw();
    m_drawn += m_petalPool->draw();
    m_drawn += m_splashPool->draw();
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

#include "Island.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/noise.hpp>

#include <algorithm>
#include <cmath>
#include <random>

namespace {

glm::vec2 squareToDisc(glm::vec2 s)
{
    return { s.x * std::sqrt(1.0f - s.y * s.y * 0.5f),
             s.y * std::sqrt(1.0f - s.x * s.x * 0.5f) };
}

float to01(float n) { return glm::clamp(n * 0.5f + 0.5f, 0.0f, 1.0f); }

}

Island::Island(const IslandSettings& settings)
    : m_settings(settings)
{
    generate();
}

glm::mat4 Island::getModelMatrix() const
{
    return glm::translate(glm::mat4(1.0f), m_settings.position);
}

bool Island::isOnIsland(float x, float z) const
{
    const float lx = x - m_settings.position.x;
    const float lz = z - m_settings.position.z;
    const float coastRadius = m_settings.radius * coastline(std::atan2(lz, lx));
    return lx * lx + lz * lz < coastRadius * coastRadius;
}

float Island::heightAt(float x, float z) const
{
    return m_settings.position.y
         + localHeight(x - m_settings.position.x, z - m_settings.position.z);
}

float Island::bottomY() const
{
    return m_settings.position.y - undersideDepth();
}

float Island::undersideDepth() const
{
    return m_settings.radius * m_settings.biome.undersideDepthRatio;
}

float Island::fbm(glm::vec2 position) const
{
    const BiomeSettings& b = m_settings.biome;
    float sum = 0.0f, amplitude = 1.0f, frequency = 1.0f, total = 0.0f;
    for (int octave = 0; octave < b.octaves; ++octave) {
        sum   += amplitude * glm::simplex(position * frequency + m_hillOffset
                                          + glm::vec2(octave * 17.3f));
        total += amplitude;
        amplitude *= b.persistence;
        frequency *= b.lacunarity;
    }
    return sum / total;
}

float Island::coastline(float angle) const
{
    const glm::vec2 onCircle(std::cos(angle), std::sin(angle));
    return 1.0f + m_settings.biome.coastNoise * glm::simplex(onCircle * 1.5f + m_coastOffset);
}

float Island::naturalHeight(float x, float z) const
{
    const BiomeSettings& b = m_settings.biome;

    const float distance = std::sqrt(x * x + z * z) / (m_settings.radius * coastline(std::atan2(z, x)));

    const float falloff = 1.0f - glm::smoothstep(b.falloffStart, 1.0f, distance);
    if (falloff <= 0.0f)
        return 0.0f;

    const float noise = fbm(glm::vec2(x, z) * b.noiseScale);
    float hills = glm::mix(to01(noise), 1.0f - std::abs(noise), b.ridged);
    hills = std::pow(glm::clamp(hills, 0.0f, 1.0f), b.peakSharpness);

    return falloff * (b.plateauHeight + b.bumpHeight * hills);
}

float Island::localHeight(float x, float z) const
{
    float height = naturalHeight(x, z);

    for (const FlatSpot& spot : m_settings.flatSpots) {
        const float distance = glm::length(glm::vec2(x, z) - spot.center);
        const float flatten = 1.0f - glm::smoothstep(spot.radius * 0.6f, spot.radius, distance);
        if (flatten > 0.0f)
            height = glm::mix(height, naturalHeight(spot.center.x, spot.center.y), flatten);
    }
    return height;
}

void Island::generate()
{
    std::mt19937 rng(m_settings.seed);
    std::uniform_real_distribution<float> random(-1000.0f, 1000.0f);
    m_hillOffset   = {random(rng), random(rng)};
    m_coastOffset  = {random(rng), random(rng)};
    m_detailOffset = {random(rng), random(rng), random(rng)};

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    buildTop(vertices, indices);
    const size_t topCount = vertices.size();
    buildUnderside(vertices, indices);

    Mesh::computeNormals(vertices, indices);
    colorTop(vertices, topCount);
    colorUnderside(vertices, topCount);

    m_mesh = Mesh(vertices, indices);
}

void Island::buildTop(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) const
{
    const int n = m_settings.resolution;
    const int rowLength = n + 1;

    for (int row = 0; row <= n; ++row) {
        for (int col = 0; col <= n; ++col) {
            const glm::vec2 square(col / float(n) * 2.0f - 1.0f, row / float(n) * 2.0f - 1.0f);
            const glm::vec2 disc = squareToDisc(square);
            const float angle = std::atan2(disc.y, disc.x);
            const glm::vec2 xz = disc * m_settings.radius * coastline(angle);

            vertices.push_back({{xz.x, localHeight(xz.x, xz.y), xz.y}, {}, {}});
        }
    }

    for (int row = 0; row < n; ++row) {
        for (int col = 0; col < n; ++col) {
            const unsigned bottomLeft  = row * rowLength + col;
            const unsigned bottomRight = bottomLeft + 1;
            const unsigned topLeft     = bottomLeft + rowLength;
            const unsigned topRight    = topLeft + 1;

            indices.insert(indices.end(), {bottomLeft, topLeft, bottomRight,
                                           bottomRight, topLeft, topRight});
        }
    }
}

void Island::buildUnderside(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) const
{
    const int n = m_settings.resolution;
    const int rowLength = n + 1;
    const float depth = undersideDepth();
    const float rough = m_settings.biome.undersideRough;

    std::vector<unsigned int> edge;
    for (int col = 0; col < n; ++col) edge.push_back(col);
    for (int row = 0; row < n; ++row) edge.push_back(row * rowLength + n);
    for (int col = n; col > 0; --col) edge.push_back(n * rowLength + col);
    for (int row = n; row > 0; --row) edge.push_back(row * rowLength);

    const unsigned ringSize = static_cast<unsigned>(edge.size());
    const unsigned first = static_cast<unsigned>(vertices.size());
    const int rings = 32;

    for (int ring = 0; ring <= rings; ++ring) {
        const float t = ring / float(rings);
        const float y = -t * depth;
        const float shrink = std::pow(std::cos(t * glm::half_pi<float>()), 1.5f);

        const float roughness = rough * glm::smoothstep(0.0f, 0.15f, t);

        for (unsigned i = 0; i < ringSize; ++i) {
            const glm::vec3 shore = vertices[edge[i]].position;
            glm::vec3 position = shore;

            if (ring > 0) {
                const glm::vec3 p(shore.x * 0.25f, y * 0.08f, shore.z * 0.25f);
                const float bump = to01((glm::simplex(p + m_detailOffset)
                                         + 0.5f * glm::simplex(p * 2.3f + m_detailOffset)) / 1.5f);
                const float scale = shrink * (1.0f - roughness * bump);
                position = glm::vec3(shore.x * scale, y, shore.z * scale);
            }
            vertices.push_back({position, {}, {}});
        }
    }

    for (int ring = 0; ring < rings; ++ring) {
        for (unsigned i = 0; i < ringSize; ++i) {
            const unsigned a = first + ring * ringSize + i;
            const unsigned b = first + ring * ringSize + (i + 1) % ringSize;
            const unsigned c = a + ringSize;
            const unsigned d = b + ringSize;
            indices.insert(indices.end(), {a, b, c, b, d, c});
        }
    }
}

void Island::colorTop(std::vector<Vertex>& vertices, size_t count) const
{
    const BiomeSettings& b = m_settings.biome;
    const BiomePalette& c = b.colors;

    const float peakRock = glm::clamp(b.bumpHeight / 6.0f, 0.0f, 1.0f);

    for (size_t i = 0; i < count; ++i) {
        Vertex& v = vertices[i];
        const float relief = glm::clamp((v.position.y - b.plateauHeight) / std::max(b.bumpHeight, 0.001f),
                                        0.0f, 1.0f);
        const float slope  = 1.0f - v.normal.y;
        const glm::vec2 xz(v.position.x, v.position.z);

        const float patches = to01(glm::simplex(xz * 0.12f + glm::vec2(m_detailOffset))
                                   + 0.5f * glm::simplex(xz * 0.5f + glm::vec2(m_detailOffset)));
        glm::vec3 color = glm::mix(c.groundDark, c.groundLight, patches);

        color = glm::mix(color, c.groundHigh, glm::smoothstep(0.55f, 0.85f, relief));
        color = glm::mix(color, c.rock, 0.5f * peakRock * glm::smoothstep(0.85f, 1.0f, relief));
        color = glm::mix(c.shore, color, glm::smoothstep(0.25f, 0.7f, v.position.y));
        color = glm::mix(color, c.rock,
                         glm::smoothstep(b.rockSlope, b.rockSlope + 0.2f, slope));

        v.color = color;
    }
}

void Island::colorUnderside(std::vector<Vertex>& vertices, size_t first) const
{
    const BiomePalette& c = m_settings.biome.colors;
    const float depth = undersideDepth();

    for (size_t i = first; i < vertices.size(); ++i) {
        Vertex& v = vertices[i];
        const float t = glm::clamp(-v.position.y / depth, 0.0f, 1.0f);

        const float streaks = to01(glm::simplex(v.position * 0.3f + m_detailOffset));
        glm::vec3 color = glm::mix(c.undersideTop, c.undersideBottom, glm::smoothstep(0.1f, 1.0f, t));
        color *= 0.9f + 0.2f * streaks;
        color = glm::mix(c.soil, color, glm::smoothstep(0.02f, 0.10f, t));

        v.color = color;
    }
}

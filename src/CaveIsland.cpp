#include "CaveIsland.h"
#include "MeshBuilder.h"
#include "World.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/noise.hpp>

#include <algorithm>
#include <cmath>

namespace {

const glm::vec3 UP(0.0f, 1.0f, 0.0f);

constexpr int HOLE_COLUMNS = 4;
constexpr int HOLE_ROWS    = 4;
constexpr int LOOP_SIZE    = 2 * (HOLE_COLUMNS + HOLE_ROWS);

glm::vec3 horizontal(const glm::vec3& v) { return {v.x, 0.0f, v.z}; }

glm::vec3 catmullRom(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, float t)
{
    const float t2 = t * t, t3 = t2 * t;
    return 0.5f * ((2.0f * p1) + (-p0 + p2) * t + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2
                   + (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

struct RevolutionGrid {
    std::vector<std::vector<glm::vec3>> points;
    int columns = 0;
    const glm::vec3& at(int row, int column) const
    {
        return points[row][((column % columns) + columns) % columns];
    }
};

std::vector<glm::vec3> holeLoop(const RevolutionGrid& grid, int c0)
{
    std::vector<glm::vec3> loop;
    for (int i = 0; i < HOLE_COLUMNS; ++i) loop.push_back(grid.at(0, c0 + i));
    for (int j = 0; j < HOLE_ROWS; ++j)    loop.push_back(grid.at(j, c0 + HOLE_COLUMNS));
    for (int i = HOLE_COLUMNS; i > 0; --i) loop.push_back(grid.at(HOLE_ROWS, c0 + i));
    for (int j = HOLE_ROWS; j > 0; --j)    loop.push_back(grid.at(j, c0));
    return loop;
}

bool isHoleQuad(int row, int column, int columns, int c0)
{
    const int offset = ((column - c0) % columns + columns) % columns;
    return row < HOLE_ROWS && offset < HOLE_COLUMNS;
}

template <typename ColorFn>
void addFacing(MeshBuilder& b, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, const glm::vec3& wanted, ColorFn color)
{
    glm::vec3 normal = glm::cross(p1 - p0, p2 - p0);
    if (glm::dot(normal, wanted) < 0.0f) {
        std::swap(p1, p2);
        normal = -normal;
    }
    const float length = glm::length(normal);
    if (length < 1e-8f)
        return;
    b.addTriangle(p0, p1, p2, color(normal / length, (p0 + p1 + p2) / 3.0f));
}

glm::vec3 vary(const glm::vec3& color, const glm::vec3& p)
{
    return color * (0.9f + 0.2f * (0.5f + 0.5f * glm::simplex(p * 0.35f)));
}

}

CaveIsland::CaveIsland(const Island& island, int islandIndex, const CaveSettings& settings)
    : m_settings(settings), m_island(islandIndex)
{
    build(island);
}

float CaveIsland::mountainRadiusAt(float h) const
{
    const float t = glm::clamp(h / m_settings.mountainHeight, 0.0f, 1.0f);
    return m_settings.mountainRadius * std::pow(1.0f - std::pow(t, 1.6f), 0.8f)
         * (1.0f - 0.35f * glm::smoothstep(0.3f, 1.0f, t));
}

float CaveIsland::chamberFloorAt(float x, float z) const
{
    const float d = glm::length(glm::vec2(x - m_chamberFloor.x, z - m_chamberFloor.z)) / m_settings.chamberRadius;
    const float fade = glm::smoothstep(0.0f, 0.35f, 1.0f - d);
    float height = m_chamberFloor.y + 0.45f * glm::simplex(glm::vec2(x, z) * 0.22f) * fade;

    const float toPool = glm::length(glm::vec2(x - m_poolCenter.x, z - m_poolCenter.z));
    const float dip = 1.0f - glm::smoothstep(poolRadius() - 0.3f, poolRadius() + 0.8f, toPool);
    return glm::mix(height, m_chamberFloor.y - 0.3f, dip);
}

void CaveIsland::build(const Island& island)
{
    const CaveSettings& s = m_settings;
    const glm::vec3 center = island.center();
    const glm::vec3 away = glm::normalize(horizontal(center));
    const glm::vec3 side = glm::normalize(glm::cross(UP, away));
    const glm::vec3 mountainXZ = center + away * s.mountainOffset;
    const float groundY = island.heightAt(mountainXZ.x, mountainXZ.z);
    auto local = [&](float u, float v, float y) { return horizontal(center) + away * u + side * v + glm::vec3(0.0f, groundY + y, 0.0f); };

    m_mountainBase = {mountainXZ.x, groundY, mountainXZ.z};
    m_chamberFloor = local(4.0f, 0.0f, -s.chamberDepth);

    MeshBuilder b;
    const glm::vec3 rockColor  {0.40f, 0.43f, 0.52f};
    const glm::vec3 mossColor  {0.28f, 0.40f, 0.30f};
    const glm::vec3 wallColor  {0.36f, 0.37f, 0.45f};
    const glm::vec3 floorColor {0.38f, 0.34f, 0.34f};

    const float holeRow = s.tunnelHeight / HOLE_ROWS;
    std::vector<float> mountainRows = {0.0f, holeRow, 2 * holeRow, 3 * holeRow, 4 * holeRow};
    for (float h = 4.5f; h < s.mountainHeight - 0.8f; h += 2.0f)
        mountainRows.push_back(h);
    RevolutionGrid mountain;
    mountain.columns = static_cast<int>(std::round(glm::two_pi<float>() * s.mountainRadius / (2.0f * s.tunnelHalfWidth / HOLE_COLUMNS)));
    const float mountainStep = glm::two_pi<float>() / mountain.columns;
    for (float h : mountainRows) {
        std::vector<glm::vec3> row;
        for (int i = 0; i < mountain.columns; ++i) {
            const float angle = i * mountainStep;
            const glm::vec3 dir(std::cos(angle), 0.0f, std::sin(angle));
            const glm::vec3 q = dir * 1.6f + glm::vec3(0.0f, h * 0.18f, 0.0f);
            const float n = glm::simplex(q) + 0.5f * glm::simplex(q * 2.7f)
                          + 0.8f * std::abs(glm::simplex(dir * 3.0f + glm::vec3(4.0f)));
            const float amount = 0.06f * glm::smoothstep(1.0f, 4.0f, h) + 0.14f * glm::smoothstep(5.0f, 10.0f, h);
            const float radius = mountainRadiusAt(h) * (1.0f + amount * n);
            row.push_back(m_mountainBase + dir * radius + glm::vec3(0.0f, h, 0.0f));
        }
        mountain.points.push_back(row);
    }
    const float entranceAngle = std::atan2(-away.z, -away.x);
    int entranceColumn = static_cast<int>(std::round(entranceAngle / mountainStep)) - HOLE_COLUMNS / 2;
    entranceColumn = (entranceColumn % mountain.columns + mountain.columns) % mountain.columns;

    auto mountainColor = [&](const glm::vec3& n, const glm::vec3& p) {
        return vary(glm::mix(rockColor, mossColor, glm::smoothstep(0.55f, 0.75f, n.y)), p);
    };
    for (size_t r = 0; r + 1 < mountainRows.size(); ++r) {
        for (int i = 0; i < mountain.columns; ++i) {
            if (isHoleQuad(static_cast<int>(r), i, mountain.columns, entranceColumn))
                continue;
            const glm::vec3 a = mountain.at(r, i), bb = mountain.at(r, i + 1);
            const glm::vec3 c = mountain.at(r + 1, i), d = mountain.at(r + 1, i + 1);
            const glm::vec3 out = horizontal((a + d) * 0.5f - m_mountainBase);
            addFacing(b, a, bb, c, out, mountainColor);
            addFacing(b, bb, d, c, out, mountainColor);
        }
    }
    const glm::vec3 peak = m_mountainBase + glm::vec3(0.0f, s.mountainHeight, 0.0f);
    for (int i = 0; i < mountain.columns; ++i)
        addFacing(b, mountain.at(mountainRows.size() - 1, i), mountain.at(mountainRows.size() - 1, i + 1), peak, UP,
                  mountainColor);

    const std::vector<glm::vec3> entranceLoop = holeLoop(mountain, entranceColumn);
    m_entrance = (mountain.at(0, entranceColumn) + mountain.at(0, entranceColumn + HOLE_COLUMNS)) * 0.5f;
    m_entranceForward = glm::normalize(horizontal(m_mountainBase - m_entrance));
    m_entranceSide = glm::normalize(glm::cross(UP, m_entranceForward));

    const float frameAngle = (entranceColumn + HOLE_COLUMNS * 0.5f) * mountainStep;
    for (float sideSign : {-1.0f, 1.0f}) {
        const glm::vec3 p = m_entrance + m_entranceSide * sideSign * (s.tunnelHalfWidth + 1.0f)
                          - m_entranceForward * 0.4f + glm::vec3(0.0f, 0.9f, 0.0f);
        b.addRock(glm::scale(glm::translate(glm::mat4(1.0f), p), glm::vec3(1.0f, 1.8f, 1.0f)), rockColor * 0.9f,
                  sideSign * 5.0f);
        m_colliders.push_back({glm::vec2(p.x, p.z), 0.9f, groundY - 1.0f, groundY + 1000.0f, m_island});
    }
    glm::mat4 lintel = glm::translate(glm::mat4(1.0f), m_entrance - m_entranceForward * 0.3f
                                      + glm::vec3(0.0f, s.tunnelHeight + 0.8f, 0.0f));
    lintel = glm::rotate(lintel, -frameAngle, UP);
    b.addRock(glm::scale(lintel, glm::vec3(0.9f, 0.8f, 3.4f)), rockColor * 0.85f, 9.0f);

    const glm::vec3 lastControl = local(2.0f, -13.0f, -s.chamberDepth);
    const glm::vec3 toLast = horizontal(lastControl - m_chamberFloor);
    const std::vector<float> chamberRows = {0.0f, holeRow, 2 * holeRow, 3 * holeRow, 4 * holeRow,
                                            4.2f, 5.4f, 6.4f, 7.2f, 7.7f};
    RevolutionGrid chamber;
    chamber.columns = static_cast<int>(std::round(glm::two_pi<float>() * s.chamberRadius / (2.0f * s.tunnelHalfWidth / HOLE_COLUMNS)));
    const float chamberStep = glm::two_pi<float>() / chamber.columns;
    for (float h : chamberRows) {
        std::vector<glm::vec3> row;
        const float profile = std::sqrt(std::max(0.0f, 1.0f - (h / s.chamberHeight) * (h / s.chamberHeight)));
        for (int i = 0; i < chamber.columns; ++i) {
            const float angle = i * chamberStep;
            const glm::vec3 dir(std::cos(angle), 0.0f, std::sin(angle));
            const float n = glm::simplex(dir * 2.0f + glm::vec3(0.0f, h * 0.3f, 7.0f));
            const float radius = s.chamberRadius * profile + 0.4f * n;
            row.push_back(m_chamberFloor + dir * radius + glm::vec3(0.0f, h, 0.0f));
        }
        chamber.points.push_back(row);
    }
    int exitColumn = static_cast<int>(std::round(std::atan2(toLast.z, toLast.x) / chamberStep)) - HOLE_COLUMNS / 2;
    exitColumn = (exitColumn % chamber.columns + chamber.columns) % chamber.columns;

    const glm::vec3 chamberMiddle = m_chamberFloor + glm::vec3(0.0f, s.chamberHeight * 0.35f, 0.0f);
    auto caveColor = [&](const glm::vec3& n, const glm::vec3& p) {
        return vary(n.y > 0.6f ? floorColor : wallColor, p);
    };
    for (size_t r = 0; r + 1 < chamberRows.size(); ++r) {
        for (int i = 0; i < chamber.columns; ++i) {
            if (isHoleQuad(static_cast<int>(r), i, chamber.columns, exitColumn))
                continue;
            const glm::vec3 a = chamber.at(r, i), bb = chamber.at(r, i + 1);
            const glm::vec3 c = chamber.at(r + 1, i), d = chamber.at(r + 1, i + 1);
            const glm::vec3 in = chamberMiddle - (a + d) * 0.5f;
            addFacing(b, a, bb, c, in, caveColor);
            addFacing(b, bb, d, c, in, caveColor);
        }
    }
    const glm::vec3 apex = m_chamberFloor + glm::vec3(0.0f, s.chamberHeight, 0.0f);
    for (int i = 0; i < chamber.columns; ++i)
        addFacing(b, chamber.at(chamberRows.size() - 1, i), chamber.at(chamberRows.size() - 1, i + 1), apex, -UP, caveColor);

    const glm::vec3 exitPoint = (chamber.at(0, exitColumn) + chamber.at(0, exitColumn + HOLE_COLUMNS)) * 0.5f;
    m_exitDirection = glm::normalize(glm::vec2(exitPoint.x - m_chamberFloor.x, exitPoint.z - m_chamberFloor.z));
    const glm::vec2 poolOffset = glm::vec2(-m_exitDirection.y, m_exitDirection.x) * 4.2f;
    m_poolCenter = m_chamberFloor + glm::vec3(poolOffset.x, -0.05f, poolOffset.y);

    const int floorRings = 8;
    std::vector<Vertex> floorVertices;
    std::vector<unsigned int> floorIndices;
    for (int k = 0; k < floorRings; ++k) {
        const float f = 1.0f - k / static_cast<float>(floorRings);
        for (int i = 0; i < chamber.columns; ++i) {
            glm::vec3 p = chamber.at(0, i);
            if (k > 0) {
                p = m_chamberFloor + (p - m_chamberFloor) * f;
                p.y = chamberFloorAt(p.x, p.z);
            }
            floorVertices.push_back({p, UP, vary(floorColor, p)});
        }
    }
    const auto centerIndex = static_cast<unsigned int>(floorVertices.size());
    floorVertices.push_back({{m_chamberFloor.x, chamberFloorAt(m_chamberFloor.x, m_chamberFloor.z), m_chamberFloor.z},
                             UP, floorColor});
    const auto C = static_cast<unsigned int>(chamber.columns);
    for (unsigned k = 0; k < static_cast<unsigned>(floorRings); ++k) {
        for (unsigned i = 0; i < C; ++i) {
            const unsigned i1 = (i + 1) % C;
            const unsigned a = k * C + i, b2 = k * C + i1;
            if (k + 1 == static_cast<unsigned>(floorRings)) {
                floorIndices.insert(floorIndices.end(), {a, centerIndex, b2});
            } else {
                const unsigned c = a + C, d = b2 + C;
                floorIndices.insert(floorIndices.end(), {a, c, b2, b2, c, d});
            }
        }
    }
    Mesh::computeNormals(floorVertices, floorIndices);
    m_floorMesh = Mesh(floorVertices, floorIndices);
    const std::vector<glm::vec3> exitLoop = holeLoop(chamber, exitColumn);

    std::vector<glm::vec3> control = {
        m_entrance,
        local(-3.0f, 0.0f, -0.4f), local(2.0f, 4.0f, -1.5f), local(8.0f, 8.0f, -3.5f),
        local(14.0f, 4.0f, -6.0f), local(15.0f, -5.0f, -9.0f), local(9.0f, -12.0f, -12.0f),
        lastControl, exitPoint};
    const int count = static_cast<int>(control.size());
    auto controlAt = [&](int i) {
        if (i < 0)      return 2.0f * control[0] - control[1];
        if (i >= count) return 2.0f * control[count - 1] - control[count - 2];
        return control[i];
    };

    std::vector<glm::vec3> path = {control[0]};
    for (int j = 0; j + 1 < count; ++j) {
        const int steps = std::max(2, static_cast<int>(std::ceil(glm::length(control[j + 1] - control[j]))));
        for (int k = 1; k <= steps; ++k)
            path.push_back(catmullRom(controlAt(j - 1), control[j], control[j + 1], controlAt(j + 2), k / float(steps)));
    }

    float distance = 0.0f;
    for (size_t k = 0; k < path.size(); ++k) {
        const glm::vec3 next = path[std::min(k + 1, path.size() - 1)];
        const glm::vec3 previous = path[k > 0 ? k - 1 : 0];
        const glm::vec3 forward = glm::normalize(horizontal(next - previous));
        if (k > 0)
            distance += glm::length(path[k] - path[k - 1]);
        m_samples.push_back({path[k], glm::normalize(glm::cross(UP, forward)), distance});
    }
    const float tunnelLength = distance;

    std::vector<glm::vec2> section;
    const float w = s.tunnelHalfWidth, H = s.tunnelHeight;
    for (int i = 0; i < HOLE_COLUMNS; ++i) section.push_back({-w + 2.0f * w * i / HOLE_COLUMNS, 0.0f});
    for (int j = 0; j < HOLE_ROWS; ++j)    section.push_back({w, H * j / HOLE_ROWS});
    for (int i = 0; i < HOLE_COLUMNS; ++i) {
        const float x = w - 2.0f * w * i / HOLE_COLUMNS;
        section.push_back({x, H + 0.5f * (1.0f - (x / w) * (x / w))});
    }
    for (int j = 0; j < HOLE_ROWS; ++j)    section.push_back({-w, H - H * j / HOLE_ROWS});

    std::vector<std::vector<glm::vec3>> rings;
    for (const Sample& sample : m_samples) {
        const float toEntrance = 1.0f - glm::smoothstep(0.0f, 3.0f, sample.distance);
        const float toExit     = 1.0f - glm::smoothstep(0.0f, 3.0f, tunnelLength - sample.distance);
        const float roughness  = s.wallRoughness * (1.0f - toEntrance) * (1.0f - toExit);

        std::vector<glm::vec3> ring;
        for (int i = 0; i < LOOP_SIZE; ++i) {
            glm::vec2 xy = section[i];
            if (xy.y > 0.01f) {
                xy.x *= 1.0f + 0.12f * std::sin(glm::pi<float>() * std::min(xy.y / H, 1.0f));
                const glm::vec2 out = glm::normalize(xy - glm::vec2(0.0f, H * 0.5f));
                const glm::vec3 p = sample.floor + sample.side * xy.x + UP * xy.y;
                xy += out * roughness * glm::simplex(p * 0.45f);
            }
            glm::vec3 point = sample.floor + sample.side * xy.x + UP * xy.y;
            point = glm::mix(point, entranceLoop[i], toEntrance);
            point = glm::mix(point, exitLoop[i], toExit);
            ring.push_back(point);
        }
        rings.push_back(ring);
    }

    for (size_t k = 0; k + 1 < rings.size(); ++k) {
        const glm::vec3 axis = (m_samples[k].floor + m_samples[k + 1].floor) * 0.5f + UP * (H * 0.5f);
        auto tunnelColor = caveColor;
        for (int i = 0; i < LOOP_SIZE; ++i) {
            const int i1 = (i + 1) % LOOP_SIZE;
            const glm::vec3 a = rings[k][i], bb = rings[k][i1], c = rings[k + 1][i], d = rings[k + 1][i1];
            addFacing(b, a, bb, c, axis - (a + bb + c) / 3.0f, tunnelColor);
            addFacing(b, bb, d, c, axis - (bb + d + c) / 3.0f, tunnelColor);
        }
    }

    placeDecoration(b);
    m_mesh = b.build();
}

void CaveIsland::placeDecoration(MeshBuilder& b)
{
    const CaveSettings& s = m_settings;
    const PointLightSettings lightDefaults;
    const float tunnelLength = m_samples.back().distance;
    auto pointAt = [&](float d) -> const Sample& {
        for (const Sample& p : m_samples)
            if (p.distance >= d) return p;
        return m_samples.back();
    };
    const glm::vec3 glowColors[4] = {
        {0.30f, 0.95f, 1.00f},
        {0.72f, 0.45f, 1.00f},
        {0.35f, 1.00f, 0.85f},
        {0.85f, 0.45f, 1.00f},
    };
    auto addSpot = [&](const glm::vec3& floor, float yaw, float scale, GlowKind kind, const glm::vec3& lightOffset,
                       float radius, float intensity, bool pulses, float colliderRadius) {
        GlowSpot spot;
        spot.position = floor;
        spot.yaw = yaw;
        spot.scale = scale;
        spot.kind = kind;
        spot.light.position = floor + lightOffset;
        spot.light.color = glowColors[static_cast<int>(kind)];
        spot.light.radius = radius;
        spot.light.intensity = intensity;
        spot.light.pulses = pulses;
        spot.light.phase = yaw * 3.0f;
        m_glowSpots.push_back(spot);
        if (colliderRadius > 0.0f)
            m_colliders.push_back({glm::vec2(floor.x, floor.z), colliderRadius, floor.y - 1.0f, floor.y + 1000.0f, m_island});
    };

    int index = 0;
    for (float d = 3.0f; d < tunnelLength - 3.0f; d += s.glowSpacing, ++index) {
        const Sample& p = pointAt(d);
        const float sideSign = (index % 2 == 0) ? 1.0f : -1.0f;
        const glm::vec3 floor = p.floor + p.side * sideSign * (s.tunnelHalfWidth - 0.55f);
        const GlowKind kinds[4] = {GlowKind::CrystalTeal, GlowKind::MushroomsCyan, GlowKind::CrystalPurple,
                                   GlowKind::MushroomsPurple};
        const GlowKind kind = kinds[index % 4];
        const bool crystal = kind == GlowKind::CrystalTeal || kind == GlowKind::CrystalPurple;
        addSpot(floor, index * 1.7f, crystal ? 0.75f : 1.0f, kind,
                UP * 0.9f - p.side * sideSign * 0.5f, lightDefaults.defaultRadius, lightDefaults.defaultIntensity,
                index % 3 == 1, crystal ? 0.45f : 0.0f);
    }
    for (float d = 10.0f; d < tunnelLength - 5.0f; d += 12.0f)
        m_gemSpots.push_back(pointAt(d).floor + UP * 1.0f);

    const float exitAngle = std::atan2(m_exitDirection.y, m_exitDirection.x);
    const float heartAngle = exitAngle + glm::pi<float>();
    auto onFloor = [&](float angle, float radius) {
        glm::vec3 p = m_chamberFloor + glm::vec3(std::cos(angle), 0.0f, std::sin(angle)) * radius;
        p.y = chamberFloorAt(p.x, p.z);
        return p;
    };
    auto angleGap = [](float a, float b) { return std::abs(std::remainder(a - b, glm::two_pi<float>())); };

    const glm::vec3 pedestal = onFloor(heartAngle, s.chamberRadius - 2.6f);
    const glm::vec3 stone{0.42f, 0.42f, 0.50f};
    b.addPrism(glm::translate(glm::mat4(1.0f), pedestal - UP * 0.2f), 6, 1.5f, 1.4f, 0.55f, 0.0f, stone * 0.8f);
    b.addPrism(glm::translate(glm::mat4(1.0f), pedestal - UP * 0.1f), 6, 0.95f, 0.75f, 1.2f, 0.0f, stone);
    m_colliders.push_back({glm::vec2(pedestal.x, pedestal.z), 0.95f, pedestal.y - 1.0f, pedestal.y + 1.1f, m_island});
    m_heartPosition = pedestal + UP * 1.95f;
    m_heartLight.position = m_heartPosition;
    m_heartLight.color = {1.00f, 0.40f, 0.62f};
    m_heartLight.intensity = 1.6f;
    m_heartLight.radius = 9.0f;
    m_heartLight.pulses = true;

    for (float sideSign : {-1.0f, 1.0f}) {
        const glm::vec3 floor = onFloor(heartAngle + sideSign * 0.42f, s.chamberRadius - 1.4f);
        addSpot(floor, sideSign * 2.0f, 1.6f, sideSign < 0 ? GlowKind::CrystalTeal : GlowKind::CrystalPurple,
                UP * 1.6f - glm::normalize(floor - m_chamberFloor) * 0.6f, 7.5f, 1.1f, false, 0.8f);
    }
    for (int i = 0; i < 12; ++i) {
        const float angle = exitAngle + (i + 0.5f) * glm::two_pi<float>() / 12.0f;
        if (angleGap(angle, exitAngle) < 0.6f || angleGap(angle, heartAngle) < 0.8f)
            continue;
        const glm::vec3 floor = onFloor(angle, s.chamberRadius - 1.4f);
        addSpot(floor, i * 2.3f, 1.3f, i % 2 == 0 ? GlowKind::CrystalTeal : GlowKind::CrystalPurple,
                UP * 1.2f - glm::normalize(floor - m_chamberFloor) * 0.6f, 7.0f, 0.9f, i % 2 == 1, 0.8f);
    }
    const float mushroomAngles[5] = {0.9f, 2.6f, 3.75f, 4.5f, 5.5f};
    for (int i = 0; i < 5; ++i) {
        const glm::vec3 floor = onFloor(exitAngle + mushroomAngles[i], s.chamberRadius * 0.5f);
        addSpot(floor, i * 1.9f, 1.3f, i % 2 == 0 ? GlowKind::MushroomsCyan : GlowKind::MushroomsPurple,
                UP * 0.7f, 4.5f, 0.7f, i % 2 == 0, 0.0f);
    }
}

float CaveIsland::depthAt(const glm::vec3& p) const
{
    if (inChamber(p))
        return 1.0f;
    const TunnelQuery q = queryTunnel(p, 1.5f, 0.5f);
    if (q.hit && std::abs(q.lateral) < m_settings.tunnelHalfWidth + 0.5f)
        return q.along / m_samples.back().distance;
    return 0.0f;
}

CaveLightingInfo CaveIsland::lightingInfo() const
{
    CaveLightingInfo info;
    const size_t count = std::min<size_t>(m_samples.size(), 64);
    glm::vec3 sum(0.0f);
    for (size_t i = 0; i < count; ++i) {
        const size_t k = i * (m_samples.size() - 1) / (count - 1);
        info.path.push_back(m_samples[k].floor + UP * 1.5f);
        sum += info.path.back();
    }
    const glm::vec3 middle = sum / static_cast<float>(count);
    float radius = 0.0f;
    for (const glm::vec3& p : info.path)
        radius = std::max(radius, glm::length(p - middle));
    info.bounds = glm::vec4(middle, radius + 4.0f);
    info.chamber = m_chamberFloor;
    info.chamberSize = {m_settings.chamberRadius, m_settings.chamberHeight};
    info.fade = {m_settings.sunFadeStart, m_settings.sunFadeEnd};
    info.ambient = m_settings.caveAmbient;
    info.fogColor = m_settings.caveFogColor;
    info.fogDensity = m_settings.caveFogDensity;
    return info;
}

TunnelQuery CaveIsland::queryTunnel(const glm::vec3& p, float below, float above) const
{
    TunnelQuery best;
    float bestDistance = 1e9f;
    for (size_t i = 0; i + 1 < m_samples.size(); ++i) {
        const Sample& a = m_samples[i];
        const Sample& b = m_samples[i + 1];
        const glm::vec2 ab(b.floor.x - a.floor.x, b.floor.z - a.floor.z);
        const glm::vec2 ap(p.x - a.floor.x, p.z - a.floor.z);
        const float lengthSquared = glm::dot(ab, ab);
        if (lengthSquared < 1e-6f)
            continue;
        const float t = glm::dot(ap, ab) / lengthSquared;
        if ((i == 0 && t < 0.0f) || (i + 2 == m_samples.size() && t > 1.0f))
            continue;
        const float tc = glm::clamp(t, 0.0f, 1.0f);
        const glm::vec3 q = glm::mix(a.floor, b.floor, tc);
        if (p.y < q.y - below || p.y > q.y + m_settings.tunnelHeight + above)
            continue;
        const float d = glm::length(glm::vec2(p.x - q.x, p.z - q.z));
        if (d < bestDistance) {
            bestDistance = d;
            best.hit = true;
            best.floorY = q.y;
            best.side = glm::normalize(glm::mix(a.side, b.side, tc));
            best.lateral = glm::dot(horizontal(p - q), best.side);
            best.along = glm::mix(a.distance, b.distance, tc);
        }
    }
    if (bestDistance > m_settings.tunnelHalfWidth + 1.5f)
        best.hit = false;
    return best;
}

bool CaveIsland::inChamber(const glm::vec3& p) const
{
    const float d = glm::length(glm::vec2(p.x - m_chamberFloor.x, p.z - m_chamberFloor.z));
    return d < m_settings.chamberRadius + 0.5f && p.y > m_chamberFloor.y - 1.0f
        && p.y < m_chamberFloor.y + m_settings.chamberHeight;
}

bool CaveIsland::inChamberExit(const glm::vec3& p, float margin) const
{
    const glm::vec2 d(p.x - m_chamberFloor.x, p.z - m_chamberFloor.z);
    const float across = std::abs(d.x * m_exitDirection.y - d.y * m_exitDirection.x);
    return glm::dot(d, m_exitDirection) > 0.0f && across < m_settings.tunnelHalfWidth - margin;
}

bool CaveIsland::isInCave(const glm::vec3& p) const
{
    const TunnelQuery q = queryTunnel(p, 1.5f, 0.5f);
    return (q.hit && std::abs(q.lateral) < m_settings.tunnelHalfWidth + 0.5f) || inChamber(p);
}

bool CaveIsland::isNear(const glm::vec3& p) const
{
    const float d = glm::length(glm::vec2(p.x - m_mountainBase.x, p.z - m_mountainBase.z));
    return d < m_settings.mountainRadius + 15.0f || isInCave(p);
}

bool CaveIsland::floorBelow(const glm::vec3& p, float stepUp, float& height) const
{
    const TunnelQuery q = queryTunnel(p, 1.5f, 0.5f);
    if (q.hit && std::abs(q.lateral) < m_settings.tunnelHalfWidth + 0.5f)
        height = q.floorY;
    else if (inChamber(p))
        height = chamberFloorAt(p.x, p.z);
    else
        return false;
    return height <= p.y + stepUp;
}

bool CaveIsland::isSolid(const glm::vec3& p, const Island& island) const
{
    const CaveSettings& s = m_settings;

    const TunnelQuery q = queryTunnel(p, 0.3f, 0.0f);
    if (q.hit && std::abs(q.lateral) < s.tunnelHalfWidth - 0.35f && p.y > q.floorY + 0.1f
        && p.y < q.floorY + s.tunnelHeight - 0.1f)
        return false;
    const glm::vec2 d(p.x - m_chamberFloor.x, p.z - m_chamberFloor.z);
    const float hr = glm::length(d) / (s.chamberRadius - 0.6f);
    const float vr = (p.y - m_chamberFloor.y) / (s.chamberHeight - 0.5f);
    if (p.y > chamberFloorAt(p.x, p.z) + 0.1f && hr * hr + vr * vr < 1.0f)
        return false;
    if (inChamberExit(p, 0.35f) && glm::length(d) < s.chamberRadius + 0.6f && p.y > m_chamberFloor.y + 0.1f
        && p.y < m_chamberFloor.y + s.tunnelHeight - 0.1f)
        return false;

    const float h = p.y - m_mountainBase.y;
    const float dm = glm::length(glm::vec2(p.x - m_mountainBase.x, p.z - m_mountainBase.z));
    if (h >= 0.0f && h <= s.mountainHeight && dm < mountainRadiusAt(h) - 0.2f)
        return true;
    return island.isOnIsland(p.x, p.z) && p.y < island.heightAt(p.x, p.z) - 0.15f && dm < 30.0f;
}

void CaveIsland::constrain(glm::vec3& position, glm::vec3& velocity, float radius, float height) const
{
    const CaveSettings& s = m_settings;
    auto stopMovingToward = [&](const glm::vec3& direction) {
        const float into = glm::dot(velocity, direction);
        if (into > 0.0f)
            velocity -= direction * into;
    };

    const bool sealedInChamber = m_sealed && inChamber(position);

    const TunnelQuery q = queryTunnel(position, 1.5f, 0.5f);
    if (!sealedInChamber && q.hit && std::abs(q.lateral) < s.tunnelHalfWidth + 0.5f) {
        const float limit = s.tunnelHalfWidth - radius - s.wallRoughness;
        if (std::abs(q.lateral) > limit) {
            const float sign = q.lateral > 0.0f ? 1.0f : -1.0f;
            position -= q.side * (q.lateral - sign * limit);
            stopMovingToward(q.side * sign);
        }
        const float ceiling = q.floorY + s.tunnelHeight - 0.25f;
        if (position.y + height > ceiling) {
            position.y = ceiling - height;
            velocity.y = std::min(velocity.y, 0.0f);
        }
        return;
    }

    if (inChamber(position)) {
        glm::vec3 offset = horizontal(position - m_chamberFloor);
        const float distance = glm::length(offset);
        const float limit = s.chamberRadius - radius - 0.45f;
        const bool throughExit = !m_sealed && inChamberExit(position, radius + s.wallRoughness);
        if (distance > limit && distance > 1e-4f && !throughExit) {
            offset /= distance;
            position = glm::vec3(m_chamberFloor.x, position.y, m_chamberFloor.z) + offset * limit;
            stopMovingToward(offset);
        }
        return;
    }

    const glm::vec3 offset = horizontal(position - m_mountainBase);
    const float distance = glm::length(offset);
    if (distance < s.mountainRadius + radius && position.y < m_mountainBase.y + s.mountainHeight * 0.8f) {
        const float along = glm::dot(position - m_entrance, m_entranceForward);
        const float lateral = glm::dot(position - m_entrance, m_entranceSide);
        const bool inDoorway = along > -3.0f && along < 1.0f
                            && std::abs(lateral) < s.tunnelHalfWidth - radius - s.wallRoughness
                            && position.y < m_entrance.y + s.tunnelHeight;
        if (!inDoorway && distance > 1e-4f) {
            const glm::vec3 out = offset / distance;
            position = glm::vec3(m_mountainBase.x, position.y, m_mountainBase.z) + out * (s.mountainRadius + radius);
            stopMovingToward(-out);
        }
    }
}

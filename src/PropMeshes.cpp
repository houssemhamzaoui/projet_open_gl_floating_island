#include "PropMeshes.h"
#include "MeshBuilder.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

namespace {

const glm::vec3 TRUNK        {0.50f, 0.36f, 0.28f};
const glm::vec3 TRUNK_DARK   {0.40f, 0.28f, 0.26f};
const glm::vec3 LEAF_A       {0.40f, 0.70f, 0.34f};
const glm::vec3 LEAF_B       {0.52f, 0.80f, 0.40f};
const glm::vec3 PINK_A       {1.00f, 0.70f, 0.80f};
const glm::vec3 PINK_B       {0.98f, 0.79f, 0.87f};
const glm::vec3 PINK_C       {1.00f, 0.88f, 0.93f};
const glm::vec3 PINE         {0.24f, 0.50f, 0.48f};
const glm::vec3 SNOW         {0.93f, 0.96f, 1.00f};
const glm::vec3 STEM         {0.38f, 0.62f, 0.32f};
const glm::vec3 FLOWER_HEART {1.00f, 0.86f, 0.35f};
const glm::vec3 PURPLE       {0.60f, 0.44f, 0.95f};
const glm::vec3 DEEP_PURPLE  {0.44f, 0.34f, 0.82f};
const glm::vec3 CRYSTAL_BLUE {0.46f, 0.60f, 1.00f};
const glm::vec3 ICE_A        {0.72f, 0.92f, 1.00f};
const glm::vec3 ICE_B        {0.55f, 0.80f, 0.98f};
const glm::vec3 STONE        {0.66f, 0.63f, 0.62f};
const glm::vec3 STONE_LIGHT  {0.76f, 0.73f, 0.70f};

const glm::vec3 UP(0.0f, 1.0f, 0.0f);
const glm::mat4 I(1.0f);

glm::mat4 T(float x, float y, float z) { return glm::translate(I, glm::vec3(x, y, z)); }
glm::mat4 S(float x, float y, float z) { return glm::scale(I, glm::vec3(x, y, z)); }
glm::mat4 S(float s) { return S(s, s, s); }

glm::mat4 tiltToward(float dx, float dz, float degrees)
{
    const glm::vec3 axis = glm::cross(UP, glm::vec3(dx, 0.0f, dz));
    if (glm::length(axis) < 1e-5f)
        return I;
    return glm::rotate(I, glm::radians(degrees), glm::normalize(axis));
}

glm::vec2 onCircle(float degrees, float radius)
{
    const float a = glm::radians(degrees);
    return {std::cos(a) * radius, std::sin(a) * radius};
}

void addSnowyPine(MeshBuilder& b, const glm::mat4& m)
{
    b.addPrism(m, 6, 0.26f, 0.2f, 1.0f, 0.0f, TRUNK_DARK);
    const float tiers[3][3] = {{0.8f, 1.7f, 1.9f}, {1.9f, 1.3f, 1.7f}, {2.9f, 0.9f, 1.5f}};
    for (const auto& tier : tiers) {
        const float y = tier[0], r = tier[1], h = tier[2];
        b.addCone(m * T(0.0f, y, 0.0f), 8, r, h, PINE);
        b.addCone(m * T(0.0f, y + 0.38f * h, 0.0f), 8, 0.63f * r, 0.63f * h, SNOW);
    }
}

void addCrystalCluster(MeshBuilder& b, const glm::mat4& m, const glm::vec3& colorA, const glm::vec3& colorB)
{
    b.addPrism(m, 6, 0.30f, 0.26f, 1.5f, 0.5f, colorA);
    const float around[4][5] = {
        {20.0f, 0.22f, 1.0f, 0.35f, 30.0f}, {110.0f, 0.20f, 0.8f, 0.30f, 35.0f},
        {200.0f, 0.24f, 1.2f, 0.40f, 25.0f}, {290.0f, 0.18f, 0.7f, 0.28f, 38.0f}};
    for (int i = 0; i < 4; ++i) {
        const glm::vec2 dir = onCircle(around[i][0], 1.0f);
        const glm::mat4 place = m * T(dir.x * 0.35f, 0.0f, dir.y * 0.35f) * tiltToward(dir.x, dir.y, around[i][4]);
        b.addPrism(place, 6, around[i][1], around[i][1] * 0.85f, around[i][2], around[i][3],
                   i % 2 == 0 ? colorB : colorA);
    }
}

}

namespace PropMeshes {

Mesh roundTree()
{
    MeshBuilder b;
    b.addPrism(I, 6, 0.22f, 0.15f, 1.7f, 0.0f, TRUNK);
    b.addSphere(T(0.0f, 2.3f, 0.0f) * S(1.25f, 1.05f, 1.25f), LEAF_A);
    b.addSphere(T(0.65f, 1.95f, 0.35f) * S(0.85f), LEAF_B);
    b.addSphere(T(-0.55f, 2.0f, -0.4f) * S(0.9f), LEAF_B);
    b.addSphere(T(0.1f, 2.9f, -0.1f) * S(0.75f), LEAF_A);
    return b.build();
}

Mesh sakuraTree()
{
    MeshBuilder b;
    b.addPrism(glm::rotate(I, glm::radians(6.0f), glm::vec3(0, 0, 1)), 6, 0.24f, 0.14f, 2.1f, 0.0f, TRUNK_DARK);
    b.addSphere(T(0.0f, 2.7f, 0.0f) * S(1.35f, 1.0f, 1.35f), PINK_A);
    b.addSphere(T(1.0f, 2.35f, 0.4f) * S(0.95f), PINK_B);
    b.addSphere(T(-0.9f, 2.45f, -0.35f) * S(1.0f), PINK_A);
    b.addSphere(T(0.15f, 2.3f, -1.0f) * S(0.9f), PINK_B);
    b.addSphere(T(-0.2f, 2.4f, 1.0f) * S(0.85f), PINK_B);
    b.addSphere(T(0.1f, 3.35f, 0.2f) * S(0.85f), PINK_C);
    return b.build();
}

Mesh snowyPine()
{
    MeshBuilder b;
    addSnowyPine(b, I);
    return b.build();
}

Mesh rock(const glm::vec3& color, float seed)
{
    MeshBuilder b;
    b.addRock(I, color, seed);
    return b.build();
}

Mesh crystalCluster(const glm::vec3& colorA, const glm::vec3& colorB)
{
    MeshBuilder b;
    addCrystalCluster(b, I, colorA, colorB);
    return b.build();
}

Mesh grassTuft(const glm::vec3& color)
{
    MeshBuilder b;
    const float blades[5][4] = {
        {0.0f, 0.05f, 0.65f, 12.0f}, {75.0f, 0.12f, 0.50f, 22.0f}, {150.0f, 0.10f, 0.58f, 18.0f},
        {220.0f, 0.13f, 0.45f, 25.0f}, {295.0f, 0.09f, 0.55f, 20.0f}};
    const glm::vec3 light = glm::mix(color, glm::vec3(1.0f), 0.25f);
    for (int i = 0; i < 5; ++i) {
        const glm::vec2 dir = onCircle(blades[i][0], 1.0f);
        const glm::mat4 place = T(dir.x * blades[i][1], 0.0f, dir.y * blades[i][1]) * tiltToward(dir.x, dir.y, blades[i][3]);
        b.addCone(place, 3, 0.055f, blades[i][2], i % 2 == 0 ? color : light);
    }
    return b.build();
}

Mesh flowers(const glm::vec3& petalColor)
{
    MeshBuilder b;
    const float stems[3][3] = {{30.0f, 0.12f, 0.45f}, {160.0f, 0.15f, 0.38f}, {270.0f, 0.10f, 0.52f}};
    for (const auto& stem : stems) {
        const glm::vec2 dir = onCircle(stem[0], stem[1]);
        b.addCone(T(dir.x, 0.0f, dir.y), 3, 0.025f, stem[2], STEM);
        b.addSphere(T(dir.x, stem[2], dir.y) * S(0.10f, 0.07f, 0.10f), petalColor, 8, 5);
        b.addSphere(T(dir.x, stem[2] + 0.05f, dir.y) * S(0.045f), FLOWER_HEART, 6, 4);
    }
    return b.build();
}

Mesh petalPatch()
{
    MeshBuilder b;
    const float petals[7][5] = {
        {0.0f, 0.0f, 0.45f, 0.30f, 10.0f}, {0.6f, 0.3f, 0.35f, 0.22f, 60.0f}, {-0.5f, 0.4f, 0.30f, 0.20f, 120.0f},
        {0.2f, -0.6f, 0.38f, 0.25f, 30.0f}, {-0.7f, -0.3f, 0.28f, 0.18f, 80.0f},
        {0.8f, -0.4f, 0.25f, 0.17f, 150.0f}, {-0.2f, 0.8f, 0.30f, 0.20f, 45.0f}};
    for (int i = 0; i < 7; ++i) {
        const glm::mat4 place = T(petals[i][0], 0.03f, petals[i][1])
                              * glm::rotate(I, glm::radians(petals[i][4]), UP)
                              * S(petals[i][2], 0.025f, petals[i][3]);
        b.addSphere(place, i % 3 == 0 ? PINK_A : (i % 3 == 1 ? PINK_B : PINK_C), 8, 4);
    }
    return b.build();
}

Mesh deadTree()
{
    MeshBuilder b;
    const glm::vec3 bark{0.42f, 0.38f, 0.36f};
    b.addPrism(glm::rotate(I, glm::radians(4.0f), glm::vec3(0, 0, 1)), 6, 0.2f, 0.09f, 2.8f, 0.0f, bark);
    const float branches[4][4] = {{30.0f, 1.3f, 50.0f, 1.1f}, {150.0f, 1.8f, 45.0f, 0.9f},
                                  {260.0f, 2.2f, 55.0f, 0.8f}, {90.0f, 2.5f, 35.0f, 0.6f}};
    for (const auto& br : branches) {
        const glm::vec2 dir = onCircle(br[0], 1.0f);
        b.addPrism(T(0.0f, br[1], 0.0f) * tiltToward(dir.x, dir.y, br[2]), 5, 0.07f, 0.03f, br[3], 0.0f, bark);
    }
    return b.build();
}

Mesh landmarkCrystal(std::vector<LocalCollider>& colliders)
{
    MeshBuilder b;

    b.addPrism(I, 6, 1.9f, 1.9f, CRYSTAL_COLUMN_HEIGHT, 0.0f, DEEP_PURPLE);
    colliders.push_back({{0.0f, 0.0f}, 1.9f, CRYSTAL_COLUMN_HEIGHT, true});

    for (int i = 0; i < 6; ++i) {
        const float angle  = 50.0f - i * 32.0f;
        const float radius = 5.8f - i * 0.45f;
        const float height = (i + 1) * 1.0f;
        const glm::vec2 p = onCircle(angle, radius);
        const glm::vec3 color = glm::mix(glm::vec3(0.80f, 0.74f, 0.99f), PURPLE, i / 5.0f);
        b.addPrism(T(p.x, 0.0f, p.y), 6, 1.1f, 1.1f, height, 0.0f, color);
        colliders.push_back({p, 1.0f, height, true});
    }

    const float giants[4][4] = {{120.0f, 1.2f, 11.0f, 2.5f}, {150.0f, 1.5f, 13.5f, 3.0f},
                                {180.0f, 1.3f, 12.0f, 2.6f}, {210.0f, 1.1f, 10.0f, 2.2f}};
    for (int i = 0; i < 4; ++i) {
        const glm::vec2 p = onCircle(giants[i][0], 3.2f);
        const glm::vec2 out = glm::normalize(p);
        b.addPrism(T(p.x, 0.0f, p.y) * tiltToward(out.x, out.y, 12.0f), 6, giants[i][1], giants[i][1] * 0.85f,
                   giants[i][2], giants[i][3], i % 2 == 0 ? PURPLE : CRYSTAL_BLUE);
        colliders.push_back({p, giants[i][1] * 1.05f, 1000.0f, false});
    }

    const float smalls[3][2] = {{135.0f, 6.0f}, {175.0f, 6.0f}, {220.0f, 6.0f}};
    for (const auto& s : smalls) {
        const glm::vec2 p = onCircle(s[0], s[1]);
        addCrystalCluster(b, T(p.x, 0.0f, p.y) * S(1.4f), PURPLE, CRYSTAL_BLUE);
        colliders.push_back({p, 0.9f, 1000.0f, false});
    }
    return b.build();
}

Mesh landmarkSakura(std::vector<LocalCollider>& colliders)
{
    MeshBuilder b;
    b.addPrism(I, 8, 1.2f, 0.75f, 7.0f, 0.0f, TRUNK_DARK);
    for (int i = 0; i < 4; ++i) {
        const glm::vec2 dir = onCircle(45.0f + i * 90.0f, 1.0f);
        b.addPrism(T(dir.x * 0.4f, 5.0f, dir.y * 0.4f) * tiltToward(dir.x, dir.y, 45.0f),
                   6, 0.45f, 0.25f, 3.2f, 0.0f, TRUNK_DARK);
    }
    b.addSphere(T(0.0f, 10.0f, 0.0f) * S(4.0f, 2.8f, 4.0f), PINK_A, 14, 9);
    for (int i = 0; i < 7; ++i) {
        const glm::vec2 p = onCircle(i * 51.4f, 3.8f);
        const float size = 2.6f + 0.4f * std::sin(i * 2.1f);
        b.addSphere(T(p.x, 8.4f + 0.5f * std::sin(i * 1.3f), p.y) * S(size, size * 0.8f, size),
                    i % 2 == 0 ? PINK_B : PINK_C, 12, 8);
    }
    colliders.push_back({{0.0f, 0.0f}, 1.4f, 1000.0f, false});
    return b.build();
}

Mesh landmarkFrost(std::vector<LocalCollider>& colliders)
{
    MeshBuilder b;
    b.addPrism(I, 6, 1.4f, 1.2f, 9.0f, 2.5f, ICE_A);
    colliders.push_back({{0.0f, 0.0f}, 1.5f, 1000.0f, false});
    for (int i = 0; i < 5; ++i) {
        const glm::vec2 p = onCircle(i * 72.0f + 20.0f, 2.2f);
        const glm::vec2 out = glm::normalize(p);
        const float radius = 0.8f + 0.1f * (i % 3);
        const float height = 5.0f + 0.6f * i;
        b.addPrism(T(p.x, 0.0f, p.y) * tiltToward(out.x, out.y, 15.0f), 6, radius, radius * 0.85f,
                   height, 1.5f, i % 2 == 0 ? ICE_B : ICE_A);
        colliders.push_back({p, radius * 1.1f, 1000.0f, false});
    }
    for (int i = 0; i < 4; ++i) {
        const glm::vec2 p = onCircle(45.0f + i * 90.0f, 6.5f);
        addSnowyPine(b, T(p.x, 0.0f, p.y) * S(2.0f));
        colliders.push_back({p, 1.1f, 1000.0f, false});
    }
    return b.build();
}

Mesh landmarkArch(std::vector<LocalCollider>& colliders)
{
    MeshBuilder b;
    for (float side : {-1.0f, 1.0f}) {
        b.addPrism(T(side * 3.2f, 0.0f, 0.0f), 7, 1.0f, 0.8f, 7.2f, 0.0f, STONE);
        b.addRock(T(side * 3.4f, 0.2f, 0.6f) * S(0.8f, 0.6f, 0.7f), STONE_LIGHT, side * 3.0f);
        colliders.push_back({{side * 3.2f, 0.0f}, 1.1f, 1000.0f, false});
    }
    b.addBox(T(0.0f, 7.6f, 0.0f), {4.4f, 0.7f, 0.9f}, STONE_LIGHT);
    b.addRock(T(0.0f, 8.6f, 0.0f) * S(1.0f, 0.6f, 0.8f), STONE, 7.0f);
    return b.build();
}

}

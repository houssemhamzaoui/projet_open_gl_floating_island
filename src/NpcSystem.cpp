#include "NpcSystem.h"
#include "MathUtils.h"
#include "MeshBuilder.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

namespace {

const glm::mat4 I(1.0f);
glm::mat4 T(float x, float y, float z) { return glm::translate(I, glm::vec3(x, y, z)); }
glm::mat4 S(float x, float y, float z) { return glm::scale(I, glm::vec3(x, y, z)); }
glm::mat4 S(float s) { return S(s, s, s); }
glm::mat4 rotX(float degrees) { return glm::rotate(I, glm::radians(degrees), glm::vec3(1, 0, 0)); }
glm::mat4 rotZ(float degrees) { return glm::rotate(I, glm::radians(degrees), glm::vec3(0, 0, 1)); }

const glm::vec3 SKIN {1.00f, 0.86f, 0.76f};
const glm::vec3 EYE  {0.15f, 0.14f, 0.26f};
const glm::vec3 BLUSH{1.00f, 0.66f, 0.70f};

float smoothFactor(float speed, float deltaTime) { return 1.0f - std::exp(-speed * deltaTime); }

void addFace(MeshBuilder& b, float y, float r, bool blush)
{
    for (float side : {-1.0f, 1.0f}) {
        b.addSphere(T(0.09f * side * r / 0.27f, y + 0.02f, r * 0.9f) * S(0.045f, 0.06f, 0.03f), EYE, 8, 5);
        if (blush)
            b.addSphere(T(0.15f * side * r / 0.27f, y - 0.06f, r * 0.82f) * S(0.05f, 0.025f, 0.02f), BLUSH, 8, 4);
    }
}

void buildElder(MeshBuilder& b, MeshBuilder& floating)
{
    const glm::vec3 robe{0.20f, 0.46f, 0.50f}, robeDark{0.14f, 0.34f, 0.40f};
    const glm::vec3 gold{0.95f, 0.78f, 0.42f}, hat{0.45f, 0.33f, 0.28f}, beard{0.96f, 0.93f, 0.86f};
    const glm::vec3 wood{0.48f, 0.34f, 0.24f};
    b.addPrism(I, 10, 0.55f, 0.30f, 1.45f, 0.0f, robe);
    b.addPrism(T(0, 0.82f, 0), 10, 0.42f, 0.41f, 0.09f, 0.0f, gold);
    b.addSphere(T(0, 1.42f, 0) * S(0.40f, 0.18f, 0.36f), robeDark, 12, 7);
    b.addSphere(T(0, 1.72f, 0) * S(0.27f), SKIN);
    addFace(b, 1.74f, 0.27f, false);
    b.addCone(T(0, 1.66f, 0.15f) * rotX(180.0f) * S(1.0f, 1.0f, 0.7f), 8, 0.2f, 0.5f, beard);
    b.addSphere(T(0, 1.66f, 0.16f) * S(0.2f, 0.08f, 0.12f), beard, 10, 6);
    b.addPrism(T(0, 1.88f, 0), 16, 0.6f, 0.6f, 0.04f, 0.0f, hat);
    b.addPrism(T(0, 1.9f, 0), 12, 0.27f, 0.25f, 0.08f, 0.0f, gold);
    b.addCone(T(0, 1.9f, 0) * rotX(-14.0f), 12, 0.27f, 0.62f, hat);
    b.addPrism(T(0.55f, 0, 0.18f), 6, 0.045f, 0.035f, 2.0f, 0.0f, wood);
    b.addSphere(T(0.46f, 1.05f, 0.18f) * S(0.09f), SKIN, 8, 5);
    floating.addSphere(S(0.13f), {0.55f, 0.95f, 1.00f}, 12, 8);
    floating.addPrism(T(0, -0.05f, 0) * S(0.6f), 6, 0.12f, 0.1f, 0.05f, 0.0f, gold);
}

void buildGardener(MeshBuilder& b)
{
    const glm::vec3 dress{0.46f, 0.70f, 0.46f}, apron{1.00f, 0.74f, 0.82f};
    const glm::vec3 straw{0.98f, 0.86f, 0.55f}, ribbon{1.00f, 0.55f, 0.66f}, hair{0.66f, 0.42f, 0.36f};
    const glm::vec3 wicker{0.70f, 0.52f, 0.32f};
    b.addPrism(I, 10, 0.42f, 0.30f, 0.80f, 0.0f, dress);
    b.addBox(T(0, 0.42f, 0.33f) * rotX(-8.0f), {0.22f, 0.30f, 0.025f}, apron);
    b.addSphere(T(0, 0.82f, 0) * S(0.32f, 0.14f, 0.3f), dress, 10, 6);
    b.addSphere(T(0, 1.1f, 0) * S(0.30f), SKIN);
    addFace(b, 1.1f, 0.30f, true);
    for (float side : {-1.0f, 1.0f})
        b.addSphere(T(0.25f * side, 1.2f, -0.05f) * S(0.12f), hair, 10, 6);
    b.addSphere(T(0, 1.18f, -0.06f) * S(0.29f, 0.24f, 0.28f), hair, 12, 8);
    b.addPrism(T(0, 1.3f, 0), 16, 0.62f, 0.62f, 0.035f, 0.0f, straw);
    b.addSphere(T(0, 1.33f, 0) * S(0.30f, 0.17f, 0.30f), straw, 12, 7);
    b.addPrism(T(0, 1.33f, 0), 14, 0.305f, 0.30f, 0.06f, 0.0f, ribbon);
    b.addSphere(T(0.3f, 1.4f, 0.12f) * S(0.07f), ribbon, 8, 5);
    b.addPrism(T(0.32f, 0.42f, 0.22f), 8, 0.15f, 0.19f, 0.18f, 0.0f, wicker);
    for (int i = 0; i < 3; ++i)
        b.addSphere(T(0.26f + 0.06f * i, 0.64f, 0.2f + 0.03f * (i % 2)) * S(0.06f),
                    i == 1 ? glm::vec3(1.0f, 0.95f, 0.6f) : ribbon, 8, 5);
}

void buildExplorer(MeshBuilder& b)
{
    const glm::vec3 parka{0.96f, 0.56f, 0.28f}, parkaDark{0.82f, 0.44f, 0.22f};
    const glm::vec3 fur{0.97f, 0.95f, 0.90f}, pack{0.46f, 0.52f, 0.36f}, roll{0.55f, 0.32f, 0.30f};
    const glm::vec3 boots{0.32f, 0.24f, 0.22f}, lens{0.35f, 0.70f, 0.95f}, strap{0.25f, 0.22f, 0.24f};
    b.addPrism(I, 8, 0.46f, 0.40f, 1.12f, 0.0f, parka);
    b.addPrism(T(0, 0.55f, 0), 8, 0.43f, 0.43f, 0.1f, 0.0f, strap);
    for (float side : {-1.0f, 1.0f})
        b.addBox(T(0.17f * side, 0.08f, 0.14f), {0.11f, 0.09f, 0.17f}, boots);
    b.addSphere(T(0, 1.15f, 0) * S(0.42f, 0.13f, 0.42f), fur, 12, 6);
    b.addSphere(T(0, 1.42f, 0) * S(0.28f), SKIN);
    addFace(b, 1.42f, 0.28f, true);
    b.addSphere(T(0, 1.5f, -0.06f) * S(0.32f, 0.30f, 0.30f), parkaDark, 12, 8);
    b.addPrism(T(0, 1.52f, 0), 12, 0.3f, 0.3f, 0.05f, 0.0f, strap);
    for (float side : {-1.0f, 1.0f}) {
        b.addSphere(T(0.1f * side, 1.58f, 0.24f) * S(0.08f, 0.07f, 0.04f), strap, 8, 5);
        b.addSphere(T(0.1f * side, 1.58f, 0.27f) * S(0.06f, 0.05f, 0.02f), lens, 8, 5);
    }
    b.addBox(T(0, 0.85f, -0.48f), {0.28f, 0.32f, 0.15f}, pack);
    b.addPrism(T(-0.32f, 1.25f, -0.48f) * rotZ(-90.0f), 8, 0.1f, 0.1f, 0.64f, 0.0f, roll);
    b.addCone(T(0.2f, 1.17f, -0.5f), 4, 0.04f, 0.5f, {0.85f, 0.85f, 0.8f});
    b.addBox(T(0.27f, 1.55f, -0.5f), {0.07f, 0.05f, 0.005f}, {0.95f, 0.35f, 0.35f});
}

void buildScholar(MeshBuilder& b, MeshBuilder& floating)
{
    const glm::vec3 robe{0.70f, 0.62f, 0.92f}, robeDark{0.44f, 0.36f, 0.70f}, gold{0.95f, 0.80f, 0.45f};
    const glm::vec3 hair{0.80f, 0.84f, 0.95f}, rim{0.25f, 0.22f, 0.32f}, glass{0.80f, 0.92f, 1.00f};
    const glm::vec3 cover{0.52f, 0.32f, 0.70f}, pages{0.98f, 0.95f, 0.86f}, crystal{0.55f, 0.65f, 1.00f};
    b.addPrism(I, 10, 0.38f, 0.24f, 1.42f, 0.0f, robe);
    b.addPrism(T(0, 0, 0), 10, 0.40f, 0.385f, 0.1f, 0.0f, gold);
    b.addSphere(T(0, 1.4f, 0) * S(0.34f, 0.14f, 0.32f), robeDark, 12, 7);
    b.addSphere(T(0, 1.66f, 0) * S(0.25f), SKIN);
    addFace(b, 1.66f, 0.25f, false);
    b.addSphere(T(0, 1.72f, -0.05f) * S(0.27f, 0.25f, 0.26f), hair, 12, 8);
    b.addCone(T(0, 1.68f, -0.24f) * rotX(-160.0f), 6, 0.08f, 0.35f, hair);
    for (float side : {-1.0f, 1.0f}) {
        b.addSphere(T(0.09f * side, 1.68f, 0.235f) * S(0.075f, 0.075f, 0.015f), rim, 10, 5);
        b.addSphere(T(0.09f * side, 1.68f, 0.245f) * S(0.058f, 0.058f, 0.012f), glass, 10, 5);
    }
    b.addPrism(T(0, 1.82f, 0.15f) * rotX(15.0f), 6, 0.04f, 0.035f, 0.08f, 0.06f, crystal);
    for (float side : {-1.0f, 1.0f}) {
        floating.addBox(rotZ(side * 12.0f) * T(0.13f * side, 0, 0), {0.13f, 0.012f, 0.17f}, cover);
        floating.addBox(rotZ(side * 12.0f) * T(0.12f * side, 0.014f, 0), {0.115f, 0.006f, 0.155f}, pages);
    }
}

}

NpcSystem::NpcSystem(const World& world, const NpcSettings& settings)
    : settings(settings)
{
    for (int role = 0; role < 4; ++role) {
        MeshBuilder body, floating;
        Look look;
        switch (role) {
        case 0: buildElder(body, floating);   look.floatOffset = {0.55f, 2.17f, 0.18f}; look.height = 2.45f; break;
        case 1: buildGardener(body);          look.height = 1.55f; break;
        case 2: buildExplorer(body);          look.height = 1.85f; break;
        default: buildScholar(body, floating); look.floatOffset = {0.55f, 1.25f, 0.3f}; look.height = 2.0f; break;
        }
        look.body = std::make_unique<Mesh>(body.build());
        if (role == 0 || role == 3)
            look.floating = std::make_unique<Mesh>(floating.build());
        m_looks.push_back(std::move(look));
    }
    generate(world);
}

void NpcSystem::generate(const World& world)
{
    m_npcs.clear();
    for (const NpcSite& site : world.npcSites()) {
        Npc npc;
        npc.role = site.role;
        npc.look = static_cast<int>(site.role);
        switch (site.role) {
        case NpcRole::Elder:    npc.id = "elder";    npc.name = "Elder Mirou";     break;
        case NpcRole::Gardener: npc.id = "gardener"; npc.name = "Hana the Gardener"; break;
        case NpcRole::Explorer: npc.id = "explorer"; npc.name = "Rook the Explorer"; break;
        case NpcRole::Scholar:  npc.id = "scholar";  npc.name = "Professor Lumen";   break;
        }
        npc.island = site.island;
        npc.position = site.position;
        npc.restFacing = npc.facing = site.facing;
        npc.height = m_looks[npc.look].height;
        npc.seed = static_cast<float>(m_npcs.size()) * 1.7f;
        m_npcs.push_back(npc);
    }
}

void NpcSystem::update(Player& player, float time, float deltaTime, int talkingTo)
{
    m_time = time;
    const glm::vec3 p = player.getPosition();
    for (size_t i = 0; i < m_npcs.size(); ++i) {
        Npc& npc = m_npcs[i];
        const glm::vec2 toPlayer(p.x - npc.position.x, p.z - npc.position.z);
        const float distance = glm::length(toPlayer);
        const bool sameLevel = std::abs(p.y - npc.position.y) < 3.0f;

        float target = npc.restFacing;
        if ((sameLevel && distance < settings.lookDistance) || static_cast<int>(i) == talkingTo)
            target = std::atan2(toPlayer.x, toPlayer.y);
        npc.facing += shortestAngle(npc.facing, target) * smoothFactor(settings.turnSpeed, deltaTime);

        if (sameLevel)
            player.pushOutOfCircle({npc.position.x, npc.position.z}, settings.bodyRadius);
    }
}

int NpcSystem::nearestTalkable(const glm::vec3& playerPosition) const
{
    int best = -1;
    float bestDistance = settings.talkDistance;
    for (size_t i = 0; i < m_npcs.size(); ++i) {
        const Npc& npc = m_npcs[i];
        if (std::abs(playerPosition.y - npc.position.y) > 2.5f)
            continue;
        const float d = glm::length(glm::vec2(playerPosition.x - npc.position.x, playerPosition.z - npc.position.z));
        if (d < bestDistance) {
            bestDistance = d;
            best = static_cast<int>(i);
        }
    }
    return best;
}

glm::vec3 NpcSystem::headTop(int index) const
{
    const Npc& npc = m_npcs[index];
    return npc.position + glm::vec3(0.0f, npc.height + 0.35f, 0.0f);
}

void NpcSystem::draw(const Shader& objectShader) const
{
    for (const Npc& npc : m_npcs) {
        const Look& look = m_looks[npc.look];
        const float breathe = 0.018f * std::sin(m_time * 2.0f + npc.seed);
        const float sway = 0.025f * std::sin(m_time * 0.8f + npc.seed);
        glm::mat4 model = glm::translate(glm::mat4(1.0f), npc.position);
        model = glm::rotate(model, npc.facing, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, sway, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(1.0f - breathe * 0.5f, 1.0f + breathe, 1.0f - breathe * 0.5f));
        objectShader.setMat4("uModel", model);
        look.body->draw();

        if (look.floating) {
            glm::mat4 f = glm::translate(model, look.floatOffset + glm::vec3(0.0f, 0.06f * std::sin(m_time * 1.8f + npc.seed), 0.0f));
            f = glm::rotate(f, 0.3f * std::sin(m_time * 0.7f), glm::vec3(0.0f, 1.0f, 0.0f));
            objectShader.setMat4("uModel", f);
            look.floating->draw();
        }
    }
}

#include "BossSystem.h"
#include "Camera.h"
#include "CaveDecor.h"
#include "Collectibles.h"
#include "CombatSystem.h"
#include "Creatures.h"
#include "DialogueSystem.h"
#include "EnemySystem.h"
#include "EventBus.h"
#include "GameConfig.h"
#include "Hud.h"
#include "Lighting.h"
#include "Menu.h"
#include "Mesh.h"
#include "NightLife.h"
#include "NpcSystem.h"
#include "Paths.h"
#include "Player.h"
#include "PortalSystem.h"
#include "PropSystem.h"
#include "QuestSystem.h"
#include "SaveGame.h"
#include "ScreenFade.h"
#include "Shader.h"
#include "Sky.h"
#include "ThirdPersonCamera.h"
#include "TimeOfDay.h"
#include "UIRenderer.h"
#include "Window.h"
#include "Weather.h"
#include "World.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <exception>
#include <iostream>
#include <random>
#include <string>

enum class CameraMode { ThirdPerson, FreeFly };

enum class Screen { Title, Playing, Paused };

enum class Launch { Title, NewGame, Continue };

static void processFreeFlyInput(Window& window, Camera& camera, const glm::vec2& mouseDelta,
                                float scrollDelta, float deltaTime)
{
    camera.movementSpeed = window.isKeyPressed(GLFW_KEY_LEFT_SHIFT) ? 25.0f : 8.0f;

    if (window.isKeyPressed(GLFW_KEY_W))            camera.move(CameraMove::Forward,  deltaTime);
    if (window.isKeyPressed(GLFW_KEY_S))            camera.move(CameraMove::Backward, deltaTime);
    if (window.isKeyPressed(GLFW_KEY_A))            camera.move(CameraMove::Left,     deltaTime);
    if (window.isKeyPressed(GLFW_KEY_D))            camera.move(CameraMove::Right,    deltaTime);
    if (window.isKeyPressed(GLFW_KEY_SPACE))        camera.move(CameraMove::Up,       deltaTime);
    if (window.isKeyPressed(GLFW_KEY_LEFT_CONTROL)) camera.move(CameraMove::Down,     deltaTime);

    camera.rotate(mouseDelta.x, mouseDelta.y);
    camera.zoom(scrollDelta);
}

static PlayerInput readPlayerInput(const Window& window)
{
    PlayerInput input;
    if (window.isKeyPressed(GLFW_KEY_W)) input.move.y += 1.0f;
    if (window.isKeyPressed(GLFW_KEY_S)) input.move.y -= 1.0f;
    if (window.isKeyPressed(GLFW_KEY_D)) input.move.x += 1.0f;
    if (window.isKeyPressed(GLFW_KEY_A)) input.move.x -= 1.0f;
    input.sprint = window.isKeyPressed(GLFW_KEY_LEFT_SHIFT);
    input.jump   = window.isKeyJustPressed(GLFW_KEY_SPACE);
    input.glide  = window.isKeyPressed(GLFW_KEY_SPACE);
    return input;
}

static bool processLightingKeys(Window& window, Lighting& lighting, float deltaTime)
{
    if (window.isKeyJustPressed(GLFW_KEY_T)) {
        lighting.toggleToonShading();
        std::cout << "Shading: " << lighting.describeMode() << std::endl;
    }
    if (window.isKeyJustPressed(GLFW_KEY_B)) {
        lighting.toggleToonBands();
        std::cout << "Shading: " << lighting.describeMode() << std::endl;
    }

    const float speed = 60.0f * deltaTime;
    if (window.isKeyPressed(GLFW_KEY_LEFT))  lighting.rotateSun(-speed, 0.0f);
    if (window.isKeyPressed(GLFW_KEY_RIGHT)) lighting.rotateSun( speed, 0.0f);
    if (window.isKeyPressed(GLFW_KEY_UP))    lighting.rotateSun(0.0f,  speed);
    if (window.isKeyPressed(GLFW_KEY_DOWN))  lighting.rotateSun(0.0f, -speed);
    return window.isKeyPressed(GLFW_KEY_LEFT) || window.isKeyPressed(GLFW_KEY_RIGHT)
        || window.isKeyPressed(GLFW_KEY_UP) || window.isKeyPressed(GLFW_KEY_DOWN);
}

static void processTimeKeys(Window& window, TimeOfDay& clock, const Lighting& lighting)
{
    if (window.isKeyJustPressed(GLFW_KEY_N)) {
        if (clock.isPaused())
            clock.syncToSun(lighting.sunAzimuth);
        clock.setPaused(!clock.isPaused());
        std::cout << "Time: " << (clock.isPaused() ? "paused" : "running") << std::endl;
    }
    if (window.isKeyJustPressed(GLFW_KEY_PAGE_UP)) {
        clock.faster();
        std::cout << "Time speed: x" << clock.speed() << std::endl;
    }
    if (window.isKeyJustPressed(GLFW_KEY_PAGE_DOWN)) {
        clock.slower();
        std::cout << "Time speed: x" << clock.speed() << std::endl;
    }
}

static void processSkyKeys(Window& window, Sky& sky)
{
    if (window.isKeyJustPressed(GLFW_KEY_C)) {
        sky.cloudsEnabled = !sky.cloudsEnabled;
        std::cout << "Clouds: " << (sky.cloudsEnabled ? "on" : "off") << std::endl;
    }
    if (window.isKeyJustPressed(GLFW_KEY_F)) {
        sky.fogEnabled = !sky.fogEnabled;
        std::cout << "Fog: " << (sky.fogEnabled ? "on" : "off") << std::endl;
    }
}

static World::Arrival chamberSpot(const World& world)
{
    const CaveIsland* cave = world.cave();
    const glm::vec3 center = cave->chamberFloorCenter();
    const glm::vec2 exit = cave->exitDirection();
    const float distance = cave->chamberRadius() * 0.55f;
    const glm::vec2 pool(cave->poolCenter().x, cave->poolCenter().z);
    glm::vec2 spot(center.x + exit.x * distance, center.z + exit.y * distance);
    for (float angle : {0.0f, 0.5f, -0.5f, 1.0f, -1.0f}) {
        const glm::vec2 d(exit.x * std::cos(angle) - exit.y * std::sin(angle),
                          exit.x * std::sin(angle) + exit.y * std::cos(angle));
        spot = glm::vec2(center.x, center.z) + d * distance;
        if (glm::length(spot - pool) > cave->poolRadius() + 1.2f)
            break;
    }
    const float facing = std::atan2(center.x - spot.x, center.z - spot.y);
    return {{spot.x, cave->chamberFloorAt(spot.x, spot.y), spot.y}, facing, cave->islandIndex()};
}

static bool runSession(Window& window, Launch launch, Launch& next)
{
    Shader terrainShader("shaders/terrain.vert", "shaders/terrain.frag");
    Shader objectShader ("shaders/object.vert",  "shaders/object.frag");
    Shader blobShader   ("shaders/blob.vert",    "shaders/blob.frag");
    Shader propShader   ("shaders/instanced.vert", "shaders/object.frag");

    World world;
    std::cout << "World seed: " << world.getSeed() << std::endl;

    PropSystem props(world);
    std::cout << "Props: " << props.totalInstances() << " instances" << std::endl;

    PortalSystem portals(world);
    Collectibles gems(world, props.rareDiamondSpots());
    Creatures creatures(world);

    CaveDecor caveDecor;
    caveDecor.generate(world.cave());
    std::cout << "Creatures: " << creatures.count() << " slimes + the Sakura Spirit" << std::endl;

    NpcSystem npcs(world);
    DialogueSystem dialogue("assets/dialogue/npcs.txt");
    dialogue.load("assets/quests/quest_dialogue.txt");
    int talkingTo = -1;

    EventBus events;
    QuestSystem quests("assets/quests/quests.txt", events);
    QuestSystem::Talk questTalk;
    bool questTalkPending = false;
    int visitedIsland = -1;

    EnemySystem enemies(world, events);
    CombatSystem combat;
    BossSystem boss(world, events);
    NightLife nightLife(world);
    Weather weather(world);
    ScreenFade deathFade;
    bool dying = false;
    int lastHearts = 0;
    float hurtFlash = 0.0f;
    UIRenderer ui("assets/fonts/Fredoka.ttf");

    Lighting lighting;
    if (world.cave()) {
        const CaveLightingInfo info = world.cave()->lightingInfo();
        lighting.setCave(&info);
    }
    std::cout << "Shading: " << lighting.describeMode() << std::endl;

    Sky sky;
    TimeOfDay timeOfDay;

    Player player;
    player.respawn(world, World::MAIN_ISLAND);

    ThirdPersonCamera followCamera;
    followCamera.snapTo(player.getPosition(), world);
    int lastRespawnCount = player.getRespawnCount();

    Camera freeCamera({70.0f, 20.0f, 70.0f});
    freeCamera.lookAt({0.0f, -10.0f, 0.0f});
    Camera titleCamera({0.0f, 20.0f, 34.0f});

    CameraMode mode = CameraMode::ThirdPerson;
    bool wireframe = false;
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    Menu menu;
    Screen screen = Screen::Title;
    bool played = false;
    int savedRevision = -1;
    float saveNotice = 0.0f;
    std::string demoMessage;
    float demoMessageTime = 0.0f;
    const SaveTargets saveTargets{world, quests, gems, creatures, player, boss, enemies, timeOfDay, weather};

    auto saveNow = [&]() {
        if (SaveGame::write(SaveGame::capture(saveTargets)))
            saveNotice = 1.0f;
        savedRevision = quests.revision();
    };

    auto regenerate = [&](unsigned seed) {
        world.generate(seed);
        props.generate(world);
        portals.rebuild(world);
        gems.generate(world, props.rareDiamondSpots());
        creatures.generate(world);
        caveDecor.generate(world.cave());
        npcs.generate(world);
        enemies.generate(world);
        boss.generate(world);
        nightLife.generate(world);
        weather.generate(world);
        if (world.cave()) {
            const CaveLightingInfo info = world.cave()->lightingInfo();
            lighting.setCave(&info);
        }
        player.respawn(world, World::MAIN_ISLAND);
        std::cout << "World seed: " << world.getSeed() << std::endl;
    };

    auto loadSave = [&]() {
        SaveData data;
        if (!SaveGame::read(data))
            return false;
        if (data.seed != world.getSeed())
            regenerate(data.seed);
        SaveGame::apply(data, saveTargets);
        followCamera.snapTo(player.getPosition(), world);
        lastRespawnCount = player.getRespawnCount();
        lastHearts = player.hearts();
        savedRevision = quests.revision();
        std::cout << "Loaded " << SaveGame::filePath().string() << std::endl;
        return true;
    };

    auto startPlaying = [&]() {
        screen = Screen::Playing;
        played = true;
        menu.close();
        mode = CameraMode::ThirdPerson;
        followCamera.snapTo(player.getPosition(), world);
        window.setCursorCaptured(true);
    };

    auto openTitle = [&]() {
        screen = Screen::Title;
        menu.openTitle(SaveGame::exists());
        window.setCursorCaptured(false);
    };

    auto demoTeleport = [&](const World::Arrival& to) {
        if (dialogue.isActive())
            dialogue.close();
        player.teleport(to.position, to.facing, to.island);
        mode = CameraMode::ThirdPerson;
        followCamera.snapBehind(player.getPosition(), player.getFacing(), world);
    };
    auto demoSay = [&](const std::string& message) {
        demoMessage = message;
        demoMessageTime = 3.0f;
        std::cout << "[demo] " << message << std::endl;
    };

    if (launch == Launch::Continue) {
        if (!loadSave())
            std::cerr << "No valid save: starting a new game" << std::endl;
        startPlaying();
    } else if (launch == Launch::NewGame) {
        startPlaying();
    } else {
        openTitle();
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    float lastTime = static_cast<float>(glfwGetTime());
    float gameTime = lastTime;

    int framesCounted = 0;
    float fpsTimer = 0.0f;

    while (!window.shouldClose()) {
        const float realNow = static_cast<float>(glfwGetTime());
        const float frameTime = realNow - lastTime;
        const float deltaTime = std::min(frameTime, 0.05f);
        lastTime = realNow;

        ++framesCounted;
        fpsTimer += frameTime;
        if (fpsTimer >= 1.0f) {
            window.setTitle("Floating Island | " + std::to_string(framesCounted) + " FPS");
            framesCounted = 0;
            fpsTimer = 0.0f;
        }

        window.pollEvents();

        if (menu.isOpen()) {
            const MenuAction action = menu.update(window);
            switch (action) {
            case MenuAction::Continue:
                if (played) { next = Launch::Continue; return true; }
                if (!loadSave())
                    std::cerr << "The save could not be read: starting a new game" << std::endl;
                startPlaying();
                break;
            case MenuAction::NewGame:
                SaveGame::erase();
                if (played) { next = Launch::NewGame; return true; }
                timeOfDay.setTime(timeOfDay.settings.startTime);
                startPlaying();
                break;
            case MenuAction::Resume:
                screen = Screen::Playing;
                menu.close();
                window.setCursorCaptured(true);
                break;
            case MenuAction::MainMenu:
                saveNow();
                openTitle();
                break;
            case MenuAction::Quit:
                if (screen == Screen::Paused)
                    saveNow();
                window.requestClose();
                break;
            case MenuAction::None:
                break;
            }
            if (action != MenuAction::None)
                window.discardPresses();
        } else if (window.isKeyJustPressed(GLFW_KEY_ESCAPE)) {
            if (dialogue.isActive()) {
                dialogue.close();
            } else {
                screen = Screen::Paused;
                menu.openPause();
                window.setCursorCaptured(false);
            }
        }

        const bool playing = screen == Screen::Playing;
        const bool paused  = screen == Screen::Paused;
        if (!paused)
            gameTime += deltaTime;
        const float now = gameTime;
        int talkable = -1;

        if (playing) {
            if (window.isKeyJustPressed(GLFW_KEY_TAB))
                window.setCursorCaptured(!window.isCursorCaptured());
            if (window.isKeyJustPressed(GLFW_KEY_F1)) {
                wireframe = !wireframe;
                glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
            }
            if (window.isKeyJustPressed(GLFW_KEY_K)) {
                weather.cycle(player.getLastIsland());
                std::cout << "Weather on this island: " << Weather::name(weather.stateOf(player.getLastIsland())) << std::endl;
            }
            if (processLightingKeys(window, lighting, deltaTime))
                timeOfDay.setPaused(true);
            processTimeKeys(window, timeOfDay, lighting);
            processSkyKeys(window, sky);

            if (DEMO_MODE && !dying && !portals.isTeleporting()) {
                if (window.isKeyJustPressed(GLFW_KEY_R)) {
                    static std::random_device randomDevice;
                    regenerate(randomDevice() % 1000000);
                    demoSay("New world, seed " + std::to_string(world.getSeed()));
                    saveNow();
                }
                if (window.isKeyJustPressed(GLFW_KEY_F2) && world.cave()) {
                    demoTeleport(chamberSpot(world));
                    demoSay("Cave chamber");
                }
                if (window.isKeyJustPressed(GLFW_KEY_F3)) {
                    const std::string title = quests.debugCompleteActive();
                    demoSay(title.empty() ? "No active quest" : "Objectives done: " + title);
                }
                if (window.isKeyJustPressed(GLFW_KEY_F4)) {
                    player.heal(player.maxHearts());
                    demoSay("Full health");
                }
                if (window.isKeyJustPressed(GLFW_KEY_F5) && world.cave()) {
                    std::vector<std::string> rewards;
                    if (boss.isDefeated() || !quests.debugStartQuest("cave_guardian", rewards)) {
                        demoSay("The Cave Guardian was already defeated");
                    } else {
                        for (const std::string& reward : rewards)
                            player.applyReward(reward);
                        enemies.activate();
                        player.heal(player.maxHearts());
                        demoSay("Boss fight!");
                    }
                    demoTeleport(chamberSpot(world));
                }
                if (window.isKeyJustPressed(GLFW_KEY_F6)) {
                    if (boss.isFighting())
                        boss.onPlayerDefeated(world);
                    demoTeleport({world.spawnPoint(World::MAIN_ISLAND), 0.0f, World::MAIN_ISLAND});
                    demoSay("Plaza");
                }
            }

            timeOfDay.update(deltaTime);
            if (!timeOfDay.isPaused()) {
                lighting.sunAzimuth = timeOfDay.sunAzimuth();
                lighting.sunElevation = timeOfDay.sunElevation();
            }

            if (window.isKeyJustPressed(GLFW_KEY_V)) {
                if (mode == CameraMode::ThirdPerson) {
                    mode = CameraMode::FreeFly;
                    freeCamera.setPosition(followCamera.getPosition());
                    freeCamera.lookAt(followCamera.getTarget());
                    std::cout << "Camera: free-fly" << std::endl;
                } else {
                    mode = CameraMode::ThirdPerson;
                    followCamera.snapTo(player.getPosition(), world);
                    std::cout << "Camera: third-person" << std::endl;
                }
            }

            glm::vec2 mouseDelta = window.consumeMouseDelta();
            if (!window.isCursorCaptured())
                mouseDelta = glm::vec2(0.0f);
            const float scrollDelta = window.consumeScrollDelta();

            talkable = (mode == CameraMode::ThirdPerson && !dialogue.isActive())
                     ? npcs.nearestTalkable(player.getPosition()) : -1;
            if (window.isKeyJustPressed(GLFW_KEY_E)) {
                if (dialogue.isActive()) {
                    dialogue.advance();
                } else if (talkable >= 0) {
                    talkingTo = talkable;
                    const NpcSystem::Npc& npc = npcs.npcs()[talkable];
                    if (quests.isGiver(npc.id)) {
                        questTalk = quests.talkTo(npc.id);
                        questTalkPending = true;
                        dialogue.start(questTalk.dialogueKey, npc.name);
                    } else {
                        const bool night = sky.getState().darkness > 0.5f;
                        const std::string key = night && dialogue.has(npc.id + ".night") ? npc.id + ".night" : npc.id;
                        dialogue.start(key, npc.name);
                    }
                    events.emit({EventType::NpcTalkedTo, npc.id});
                }
            }
            if (!dialogue.isActive()) {
                talkingTo = -1;
                if (questTalkPending) {
                    questTalkPending = false;
                    const std::string reward = quests.finishTalk(questTalk);
                    if (!reward.empty())
                        player.applyReward(reward);
                }
            }
            dialogue.update(deltaTime);

            if (mode == CameraMode::ThirdPerson) {
                const bool frozen = portals.isTeleporting() || dialogue.isActive() || dying || boss.isIntro();
                const PlayerInput input = frozen ? PlayerInput{} : readPlayerInput(window);
                player.update(input, followCamera.getYaw(), world, deltaTime);

                if (player.getRespawnCount() != lastRespawnCount) {
                    lastRespawnCount = player.getRespawnCount();
                    followCamera.snapTo(player.getPosition(), world);
                }
                if (talkingTo >= 0)
                    followCamera.frame(player.getPosition(), npcs.npcs()[talkingTo].position, world, deltaTime);
                else if (boss.isIntro())
                    followCamera.frame(player.getPosition(), boss.center() - glm::vec3(0.0f, 1.0f, 0.0f), world, deltaTime);
                else
                    followCamera.update(player.getPosition(), mouseDelta, scrollDelta, world, deltaTime);

                if (portals.update(player, world, deltaTime))
                    followCamera.snapBehind(player.getPosition(), player.getFacing(), world);
            } else {
                processFreeFlyInput(window, freeCamera, mouseDelta, scrollDelta, deltaTime);
            }

            npcs.update(player, now, deltaTime, talkingTo);

            const bool canFight = mode == CameraMode::ThirdPerson && !dialogue.isActive()
                               && !portals.isTeleporting() && !dying && !boss.isIntro() && window.isCursorCaptured();
            const bool targetable = canFight && !player.isSafe() && !player.isDead();
            enemies.update(player, world, targetable, now, deltaTime);
            boss.update(player, world, quests.stateOf("cave_guardian") == QuestState::Active, targetable, deltaTime);
            gems.setHeartLocked(!boss.isDefeated());
            creatures.setCaveSlimesHidden(boss.isFighting());
            combat.update(player, enemies, &boss, world,
                          canFight && window.isMouseJustPressed(GLFW_MOUSE_BUTTON_LEFT),
                          canFight && window.isMouseJustPressed(GLFW_MOUSE_BUTTON_RIGHT),
                          followCamera.getYaw(), deltaTime);
            followCamera.shake(std::max(combat.takeShake(), boss.takeShake()));
            if (player.hearts() < lastHearts)
                hurtFlash = 1.0f;
            lastHearts = player.hearts();
            hurtFlash = std::max(0.0f, hurtFlash - deltaTime * 3.0f);

            if (player.isDead() && !dying) {
                dying = true;
                if (boss.isFighting())
                    boss.onPlayerDefeated(world);
                deathFade.start({0.25f, 0.05f, 0.12f}, 2.0f);
            }
            if (deathFade.update(deltaTime) && dying) {
                const World::Arrival arrival = world.nearestArrival(player.getPosition());
                player.revive(arrival.position, arrival.facing, arrival.island);
                followCamera.snapBehind(player.getPosition(), player.getFacing(), world);
                lastRespawnCount = player.getRespawnCount();
                dying = false;
            }

            creatures.update(player, world, gems, now, deltaTime, enemies.dangerPositions());

            const bool gemCollected = gems.update(player.getPosition(), now, deltaTime, events);

            if (player.getLastIsland() != visitedIsland) {
                visitedIsland = player.getLastIsland();
                events.emit({EventType::IslandVisited, biomeId(world.islands()[visitedIsland].settings().biome.kind)});
            }
            quests.update(deltaTime);

            if (gemCollected || quests.revision() != savedRevision)
                saveNow();
        } else if (screen == Screen::Title) {
            timeOfDay.update(deltaTime * 4.0f);
            lighting.sunAzimuth = timeOfDay.sunAzimuth();
            lighting.sunElevation = timeOfDay.sunElevation();
            npcs.update(player, now, deltaTime, -1);
            creatures.update(player, world, gems, now, deltaTime);
            gems.update(glm::vec3(1.0e6f), now, deltaTime, events);
            const glm::vec3 center = world.islands()[World::MAIN_ISLAND].center() + glm::vec3(0.0f, 2.0f, 6.0f);
            const float angle = now * 0.06f;
            titleCamera.setPosition(center + glm::vec3(std::cos(angle) * 34.0f, 20.0f, std::sin(angle) * 34.0f));
            titleCamera.lookAt(center);
        }

        const bool thirdPerson = screen != Screen::Title && mode == CameraMode::ThirdPerson;
        const Camera& otherCamera = screen == Screen::Title ? titleCamera : freeCamera;
        const glm::mat4 view = thirdPerson ? followCamera.getViewMatrix() : otherCamera.getViewMatrix();
        const glm::mat4 projection = thirdPerson
            ? followCamera.getProjectionMatrix(window.getAspectRatio())
            : otherCamera.getProjectionMatrix(window.getAspectRatio());
        const glm::vec3 cameraPosition = thirdPerson ? followCamera.getPosition() : otherCamera.getPosition();

        if (!paused) {
            const bool outdoors = !(world.cave() && world.cave()->depthAt(cameraPosition) > 0.02f) && !boss.isFighting();
            weather.update(world, cameraPosition, outdoors, deltaTime);
            sky.setWeather(weather.clouds(), weather.rain(), weather.settings.rainCloudBoost,
                           weather.settings.rainLightDim, weather.settings.rainFogBoost);
            sky.update(lighting.getDirectionToSun(), lighting.settings.sunColor, timeOfDay.time() * 6.2831853f);
            nightLife.update(sky.getState().darkness, now, 1.0f - glm::smoothstep(0.1f, 0.4f, weather.rain()));
        }
        saveNotice = std::max(0.0f, saveNotice - frameTime * 0.6f);
        demoMessageTime = std::max(0.0f, demoMessageTime - frameTime);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        std::vector<PointLight> pointLights;
        caveDecor.appendLights(pointLights);
        creatures.appendLights(pointLights);
        if (world.cave() && !gems.caveHeartCollected())
            pointLights.push_back(world.cave()->heartLight());
        for (PointLight& light : pointLights) {
            light.intensity *= boss.lightBrightness();
            light.color = glm::mix(light.color, glm::vec3(1.0f, 0.25f, 0.18f), boss.lightRed());
        }
        boss.appendLights(pointLights);
        nightLife.appendLights(pointLights, player.getPosition());
        lighting.setPointLights(pointLights, cameraPosition, now);
        lighting.setViewerCaveDepth(world.cave() ? world.cave()->depthAt(cameraPosition) : 0.0f);

        terrainShader.use();
        terrainShader.setMat4("uView", view);
        terrainShader.setMat4("uProjection", projection);
        lighting.apply(terrainShader, cameraPosition, sky.getState());
        Material terrain = lighting.settings.terrainMaterial;
        terrain.specularStrength = glm::mix(terrain.specularStrength, weather.settings.wetSpecular, weather.rain());
        terrain.shininess = glm::mix(terrain.shininess, 40.0f, weather.rain());
        Lighting::applyMaterial(terrainShader, terrain);
        world.draw(terrainShader);

        objectShader.use();
        objectShader.setMat4("uView", view);
        objectShader.setMat4("uProjection", projection);
        lighting.apply(objectShader, cameraPosition, sky.getState());

        Lighting::applyMaterial(objectShader, lighting.settings.characterMaterial);
        player.draw(objectShader);
        npcs.draw(objectShader);
        enemies.draw(objectShader, now);
        boss.draw(objectShader, now);

        if (world.cave()) {
            objectShader.setMat4("uModel", glm::mat4(1.0f));
            Lighting::applyMaterial(objectShader, lighting.settings.stoneMaterial);
            world.cave()->draw();
            caveDecor.drawPool(objectShader);
        }

        Lighting::applyMaterial(objectShader, lighting.settings.stoneMaterial);
        portals.drawFrames(objectShader, now);

        propShader.use();
        propShader.setMat4("uView", view);
        propShader.setMat4("uProjection", projection);
        lighting.apply(propShader, cameraPosition, sky.getState());
        props.draw(propShader);
        nightLife.drawLanterns();
        creatures.draw(propShader, now);

        gems.shader().use();
        gems.shader().setMat4("uView", view);
        gems.shader().setMat4("uProjection", projection);
        lighting.apply(gems.shader(), cameraPosition, sky.getState());
        gems.draw(now);
        creatures.drawGlow(gems.shader(), now);
        player.drawEffects(gems.shader());
        enemies.drawGlow(gems.shader(), now);
        combat.draw(gems.shader(), now);
        boss.drawGlow(gems.shader(), now);
        nightLife.drawGlow(gems.shader());
        caveDecor.drawGlow(gems.shader(), now);

        sky.draw(view, projection, now);

        blobShader.use();
        blobShader.setMat4("uView", view);
        blobShader.setMat4("uProjection", projection);
        player.drawShadow(blobShader, world);
        creatures.drawShadows(blobShader);
        enemies.drawShadows(blobShader);
        boss.drawFloorMarkers(blobShader, now);

        portals.drawEffects(view, projection, blobShader, now);

        gems.drawEffects(now);

        weather.draw(view, projection, cameraPosition,
                     sky.getState().ambientSky * 0.8f + sky.getState().lightColor * 0.6f,
                     sky.getState().fogColor, sky.getState().fogDensity);

        gems.shader().use();
        combat.drawSwing(gems.shader(), player);
        boss.drawTransparent(gems.shader(), now);

        ui.begin(window.getWidth(), window.getHeight());
        if (screen != Screen::Title) {
            Hud::drawHearts(ui, player.hearts(), player.maxHearts(), hurtFlash, now);
            Hud::drawGemCounters(ui, gems.counts());
            {
                int icon = 0;
                switch (weather.stateNear()) {
                case WeatherState::Cloudy: case WeatherState::Clearing: icon = 1; break;
                case WeatherState::Rain:   icon = weather.rain() > 0.3f ? 2 : 1; break;
                case WeatherState::Snow: case WeatherState::Blizzard: icon = 3; break;
                case WeatherState::Petals: icon = 4; break;
                default: break;
                }
                Hud::drawClock(ui, timeOfDay.clockText(), sky.getState().darkness > 0.5f, timeOfDay.isPaused(),
                               timeOfDay.speed(), now, icon);
            }
            enemies.drawHealthBars(ui, projection * view);
            boss.drawHud(ui, now);
            if (!dialogue.isActive())
                Hud::drawPortalHint(ui, portals.nearbyDestination(player.getPosition()));
            quests.drawTracker(ui);
            glm::vec2 promptPosition;
            if (talkable >= 0 && UIRenderer::worldToScreen(npcs.headTop(talkable), projection * view,
                                                           ui.width(), ui.height(), promptPosition))
                Hud::drawTalkPrompt(ui, promptPosition, now);
            for (size_t i = 0; i < npcs.npcs().size() && !dialogue.isActive(); ++i) {
                const QuestMarker marker = quests.marker(npcs.npcs()[i].id);
                if (marker == QuestMarker::None)
                    continue;
                const glm::vec3 above = npcs.headTop(static_cast<int>(i)) + glm::vec3(0.0f, 1.1f, 0.0f);
                const float distance = glm::length(above - cameraPosition);
                glm::vec2 screenPoint;
                if (distance < 110.0f && UIRenderer::worldToScreen(above, projection * view, ui.width(), ui.height(), screenPoint))
                    Hud::drawQuestMarker(ui, screenPoint, marker == QuestMarker::Ready, distance, now);
            }
            dialogue.draw(ui, now);
            quests.drawBanners(ui, boss.hudVisible() ? 80.0f * ui.scale() : 0.0f);
            Hud::drawSaveNotice(ui, std::min(1.0f, saveNotice * 2.0f));
        }
        if (DEMO_MODE)
            Hud::drawDemoTag(ui, demoMessage, std::min(1.0f, demoMessageTime));
        menu.draw(ui, realNow);
        ui.end();

        portals.drawFade();
        deathFade.draw();

        window.swapBuffers();
    }

    if (played && screen != Screen::Title)
        saveNow();
    return false;
}

int main()
{
    try {
        Window window(1280, 720, "Floating Island");
        std::cout << "Save file: " << SaveGame::filePath().string() << std::endl;
        try {
            Launch launch = Launch::Title;
            while (runSession(window, launch, launch)) {
            }
        } catch (const std::exception& e) {
            showFatalError(e.what());
            return 1;
        }
    } catch (const std::exception& e) {
        showFatalError(e.what());
        return 1;
    }
    return 0;
}

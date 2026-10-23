#include "SaveGame.h"

#include "BossSystem.h"
#include "Collectibles.h"
#include "Creatures.h"
#include "EnemySystem.h"
#include "Paths.h"
#include "Player.h"
#include "QuestSystem.h"
#include "TimeOfDay.h"
#include "Weather.h"
#include "World.h"

#include <fstream>
#include <iostream>
#include <sstream>

namespace {
constexpr const char* HEADER = "FloatingIslandSave";
constexpr int VERSION = 1;
}

namespace SaveGame {

std::filesystem::path filePath()
{
    return userDataPath("save.txt");
}

bool exists()
{
    SaveData unused;
    return read(unused);
}

bool read(SaveData& data)
{
    std::ifstream file(filePath());
    if (!file)
        return false;

    std::string line, header;
    int version = 0;
    std::getline(file, line);
    std::istringstream first(line);
    if (!(first >> header >> version) || header != HEADER || version != VERSION) {
        std::cerr << "Save file " << filePath().string() << " is not a valid save: ignored\n";
        return false;
    }

    data = SaveData{};
    int lineNumber = 1;
    while (std::getline(file, line)) {
        ++lineNumber;
        std::istringstream in(line);
        std::string key;
        if (!(in >> key) || key[0] == '#')
            continue;

        bool ok = true;
        if (key == "seed") {
            ok = static_cast<bool>(in >> data.seed);
        } else if (key == "quest") {
            SaveData::QuestEntry entry;
            ok = static_cast<bool>(in >> entry.id >> entry.state);
            for (int value; in >> value;)
                entry.progress.push_back(value);
            if (ok)
                data.quests.push_back(entry);
        } else if (key == "total") {
            std::string name;
            int value = 0;
            ok = static_cast<bool>(in >> name >> value);
            if (ok)
                data.totals[name] = value;
        } else if (key == "unique") {
            std::string name;
            ok = static_cast<bool>(in >> name);
            if (ok)
                data.unique.push_back(name);
        } else if (key == "last_turned_in") {
            in >> data.lastTurnedIn;
        } else if (key == "gems") {
            in >> data.gems;
            ok = data.gems.find_first_not_of("01") == std::string::npos;
        } else if (key == "spirit_gift") {
            ok = static_cast<bool>(in >> data.spiritGift);
        } else if (key == "cave_heart") {
            ok = static_cast<bool>(in >> data.caveHeart);
        } else if (key == "boss_defeated") {
            ok = static_cast<bool>(in >> data.bossDefeated);
        } else if (key == "rewards") {
            for (std::string reward; in >> reward;)
                data.rewards.push_back(reward);
        } else if (key == "max_hearts") {
            ok = static_cast<bool>(in >> data.maxHearts) && data.maxHearts > 0 && data.maxHearts < 20;
        } else if (key == "island") {
            ok = static_cast<bool>(in >> data.island);
        } else if (key == "time") {
            ok = static_cast<bool>(in >> data.timeOfDay);
        } else if (key == "weather") {
            std::pair<int, std::string> zone;
            ok = static_cast<bool>(in >> zone.first >> zone.second);
            if (ok)
                data.weather.push_back(zone);
        }

        if (!ok) {
            std::cerr << "Save file broken at line " << lineNumber << ": \"" << line << "\": ignored\n";
            return false;
        }
    }
    return true;
}

bool write(const SaveData& data)
{
    const std::filesystem::path path = filePath();
    std::filesystem::path temporary = path;
    temporary += ".tmp";
    {
        std::ofstream file(temporary, std::ios::trunc);
        if (!file) {
            std::cerr << "Cannot write the save file " << path.string() << '\n';
            return false;
        }
        file << HEADER << ' ' << VERSION << '\n'
             << "# Floating Island save. Delete this file to start over.\n"
             << "seed " << data.seed << '\n';
        for (const SaveData::QuestEntry& quest : data.quests) {
            file << "quest " << quest.id << ' ' << quest.state;
            for (int value : quest.progress)
                file << ' ' << value;
            file << '\n';
        }
        for (const auto& [name, value] : data.totals)
            file << "total " << name << ' ' << value << '\n';
        for (const std::string& name : data.unique)
            file << "unique " << name << '\n';
        file << "last_turned_in " << data.lastTurnedIn << '\n'
             << "gems " << data.gems << '\n'
             << "spirit_gift " << data.spiritGift << '\n'
             << "cave_heart " << data.caveHeart << '\n'
             << "boss_defeated " << data.bossDefeated << '\n'
             << "rewards";
        for (const std::string& reward : data.rewards)
            file << ' ' << reward;
        file << '\n'
             << "max_hearts " << data.maxHearts << '\n'
             << "island " << data.island << '\n'
             << "time " << data.timeOfDay << '\n';
        for (const auto& [island, state] : data.weather)
            file << "weather " << island << ' ' << state << '\n';
        if (!file)
            return false;
    }

    std::error_code error;
    std::filesystem::rename(temporary, path, error);
    if (error) {
        std::cerr << "Cannot replace the save file: " << error.message() << '\n';
        return false;
    }
    return true;
}

void erase()
{
    std::error_code error;
    std::filesystem::remove(filePath(), error);
}

SaveData capture(const SaveTargets& game)
{
    SaveData data;
    data.seed = game.world.getSeed();
    game.quests.exportState(data);

    data.gems = game.gems.collectedMask();
    data.spiritGift = game.gems.spiritGiftCollected();
    data.caveHeart = game.gems.caveHeartCollected();
    data.bossDefeated = game.boss.isDefeated();

    const PlayerRewards& rewards = game.player.rewards();
    if (rewards.glideTrail)       data.rewards.push_back("glide_trail");
    if (rewards.sakuraGlider)     data.rewards.push_back("sakura_glider");
    if (rewards.swiftBoots)       data.rewards.push_back("swift_boots");
    if (rewards.extraHearts > 0)  data.rewards.push_back("extra_heart");
    if (rewards.crystalCrown)     data.rewards.push_back("crystal_crown");
    data.maxHearts = game.player.maxHearts();

    data.island = game.player.getLastIsland();
    data.timeOfDay = game.clock.time();
    for (int i = 0; i < game.weather.zoneCount(); ++i)
        data.weather.push_back({i, Weather::id(game.weather.stateOf(i))});
    return data;
}

void apply(const SaveData& data, const SaveTargets& game)
{
    game.quests.importState(data);

    for (const std::string& reward : data.rewards)
        game.player.applyReward(reward);
    game.player.setExtraHearts(data.maxHearts - game.player.health.baseHearts);

    if (!game.gems.restoreCollected(data.gems))
        std::cerr << "Save: the gems do not match this world, they are kept uncollected\n";
    if (data.caveHeart)
        game.gems.restoreCaveHeart();
    if (data.spiritGift) {
        game.gems.restoreSpiritGift();
        game.creatures.setSpiritGiftGiven();
    }

    if (data.bossDefeated)
        game.boss.restoreDefeated();
    const QuestState shadows = game.quests.stateOf("shadows");
    if (shadows == QuestState::Active || shadows == QuestState::Completed || shadows == QuestState::TurnedIn)
        game.enemies.activate();

    int island = data.island;
    const int islandCount = static_cast<int>(game.world.islands().size());
    if (island < 0 || island >= islandCount || !game.world.islands()[island].settings().isRespawnPoint)
        island = World::MAIN_ISLAND;
    game.player.respawn(game.world, island);

    game.clock.setTime(data.timeOfDay);
    for (const auto& [zone, stateId] : data.weather) {
        WeatherState state;
        if (Weather::fromId(stateId, state))
            game.weather.setState(zone, state);
    }
}

}

#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <utility>
#include <vector>

class BossSystem;
class Collectibles;
class Creatures;
class EnemySystem;
class Player;
class QuestSystem;
class TimeOfDay;
class Weather;
class World;

struct SaveData {
    unsigned seed = 0;

    struct QuestEntry {
        std::string      id;
        std::string      state;
        std::vector<int> progress;
    };
    std::vector<QuestEntry>    quests;
    std::map<std::string, int> totals;
    std::vector<std::string>   unique;
    std::string                lastTurnedIn;

    std::string gems;
    bool spiritGift   = false;
    bool caveHeart    = false;
    bool bossDefeated = false;
    std::vector<std::string> rewards;
    int   maxHearts = 5;
    int   island = 0;
    float timeOfDay = 0.65f;
    std::vector<std::pair<int, std::string>> weather;
};

struct SaveTargets {
    World&        world;
    QuestSystem&  quests;
    Collectibles& gems;
    Creatures&    creatures;
    Player&       player;
    BossSystem&   boss;
    EnemySystem&  enemies;
    TimeOfDay&    clock;
    Weather&      weather;
};

namespace SaveGame {

std::filesystem::path filePath();

bool exists();

bool read(SaveData& data);

bool write(const SaveData& data);

void erase();

SaveData capture(const SaveTargets& game);
void     apply(const SaveData& data, const SaveTargets& game);

}

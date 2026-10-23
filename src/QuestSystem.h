#pragma once

#include "EventBus.h"
#include "SaveGame.h"
#include "UIRenderer.h"

#include <map>
#include <set>
#include <string>
#include <vector>

enum class QuestState { Locked, Available, Active, Completed, TurnedIn };

struct Objective {
    std::string type;
    std::string target;
    int         count = 1;
    std::string label;
    int         progress = 0;
    bool done() const { return progress >= count; }
};

struct Quest {
    std::string id, title, description;
    std::string giver = "elder";
    std::string requiredQuest;
    bool        locked = false;
    std::vector<Objective> objectives;
    std::string rewardId, rewardText;
    QuestState  state = QuestState::Locked;
};

enum class QuestMarker { None, Available, Ready };

class QuestSystem {
public:
    QuestSystem(const std::string& relativePath, EventBus& events);

    struct Talk {
        enum class Action { None, Accept, TurnIn };
        std::string dialogueKey;
        Action      action = Action::None;
        int         quest = -1;
    };
    Talk talkTo(const std::string& npcId) const;
    bool isGiver(const std::string& npcId) const;
    std::string finishTalk(const Talk& talk);

    QuestMarker marker(const std::string& npcId) const;
    QuestState stateOf(const std::string& questId) const;

    void unlock(const std::string& questId);

    int revision() const { return m_revision; }

    void exportState(SaveData& data) const;
    void importState(const SaveData& data);

    std::string debugCompleteActive();
    bool debugStartQuest(const std::string& questId, std::vector<std::string>& rewards);

    void update(float deltaTime);
    void drawTracker(UIRenderer& ui) const;
    void drawBanners(UIRenderer& ui, float topOffset = 0.0f) const;

    const std::vector<Quest>& quests() const { return m_quests; }

private:
    struct Banner {
        std::string caption, title, subtitle;
        float age = 0.0f;
    };

    void onEvent(const GameEvent& event);
    void refreshAvailability();
    void updateObjectives(Quest& quest);
    void showBanner(const std::string& caption, const std::string& title, const std::string& subtitle);
    int  findQuest(const std::string& id) const;

    std::vector<Quest> m_quests;
    std::map<std::string, int> m_totals;
    std::set<std::string>      m_unique;
    std::string                m_lastTurnedIn;
    std::vector<Banner>        m_banners;
    EventBus&                  m_events;
    int                        m_revision = 0;
};

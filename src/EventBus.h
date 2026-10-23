#pragma once

#include <functional>
#include <string>
#include <vector>

enum class EventType {
    GemCollected,
    IslandVisited,
    NpcTalkedTo,
    EnemyDefeated,
    QuestAccepted,
    QuestTurnedIn,
    BossDefeated,
    ItemCollected,
};

struct GameEvent {
    EventType   type;
    std::string id;
    int         amount = 1;
};

class EventBus {
public:
    using Listener = std::function<void(const GameEvent&)>;

    void subscribe(Listener listener) { m_listeners.push_back(std::move(listener)); }

    void emit(const GameEvent& event) const
    {
        for (const Listener& listener : m_listeners)
            listener(event);
    }

private:
    std::vector<Listener> m_listeners;
};

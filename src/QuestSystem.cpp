#include "QuestSystem.h"
#include "Paths.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {

constexpr float BANNER_TIME = 3.8f;
constexpr float BANNER_FADE = 0.4f;

std::string trim(const std::string& s)
{
    const size_t first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return "";
    const size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

std::string typeName(EventType type)
{
    switch (type) {
    case EventType::GemCollected:  return "collect";
    case EventType::IslandVisited: return "visit";
    case EventType::NpcTalkedTo:   return "talk";
    case EventType::EnemyDefeated: return "defeat";
    case EventType::BossDefeated:  return "defeat";
    case EventType::ItemCollected: return "collect";
    default:                       return "";
    }
    return "";
}

}

QuestSystem::QuestSystem(const std::string& relativePath, EventBus& events)
    : m_events(events)
{
    std::ifstream file(resourcePath(relativePath));
    if (!file)
        throw std::runtime_error("Cannot open quest file: " + relativePath);

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue;
        if (line.front() == '[' && line.back() == ']') {
            Quest quest;
            quest.id = line.substr(1, line.size() - 2);
            m_quests.push_back(quest);
            continue;
        }
        if (m_quests.empty())
            continue;
        Quest& quest = m_quests.back();
        const size_t colon = line.find(':');
        if (colon == std::string::npos)
            continue;
        const std::string key = trim(line.substr(0, colon));
        const std::string value = trim(line.substr(colon + 1));
        const size_t bar = value.find('|');
        const std::string words = trim(value.substr(0, bar));
        const std::string text = bar == std::string::npos ? "" : trim(value.substr(bar + 1));

        if (key == "title")            quest.title = value;
        else if (key == "description") quest.description = value;
        else if (key == "giver")       quest.giver = value;
        else if (key == "requires")    quest.requiredQuest = value;
        else if (key == "locked")      quest.locked = (value == "yes" || value == "true");
        else if (key == "reward")      { quest.rewardId = words; quest.rewardText = text; }
        else if (key == "objective") {
            Objective objective;
            std::istringstream in(words);
            in >> objective.type >> objective.target;
            if (!(in >> objective.count))
                objective.count = 1;
            objective.label = text;
            quest.objectives.push_back(objective);
        }
    }
    refreshAvailability();

    events.subscribe([this](const GameEvent& event) { onEvent(event); });
}

int QuestSystem::findQuest(const std::string& id) const
{
    for (size_t i = 0; i < m_quests.size(); ++i)
        if (m_quests[i].id == id)
            return static_cast<int>(i);
    return -1;
}

void QuestSystem::refreshAvailability()
{
    for (Quest& quest : m_quests) {
        if (quest.state != QuestState::Locked || quest.locked)
            continue;
        const int required = findQuest(quest.requiredQuest);
        if (quest.requiredQuest.empty() || (required >= 0 && m_quests[required].state == QuestState::TurnedIn))
            quest.state = QuestState::Available;
    }
}

void QuestSystem::unlock(const std::string& questId)
{
    const int i = findQuest(questId);
    if (i >= 0)
        m_quests[i].locked = false;
    refreshAvailability();
}

void QuestSystem::onEvent(const GameEvent& event)
{
    if (typeName(event.type).empty())
        return;
    const std::string key = typeName(event.type) + ":" + event.id;
    if (event.type == EventType::IslandVisited || event.type == EventType::NpcTalkedTo) {
        if (!m_unique.insert(key).second)
            return;
        m_totals[key] = 1;
    } else {
        m_totals[key] += event.amount;
        m_totals[typeName(event.type) + ":any"] += event.amount;
    }
    ++m_revision;

    for (Quest& quest : m_quests) {
        if (quest.state != QuestState::Active)
            continue;
        if (event.type == EventType::EnemyDefeated || event.type == EventType::BossDefeated) {
            for (Objective& o : quest.objectives)
                if (o.type == "defeat" && (o.target == event.id || o.target == "any"))
                    o.progress = std::min(o.count, o.progress + event.amount);
        }
        updateObjectives(quest);
    }
}

void QuestSystem::updateObjectives(Quest& quest)
{
    for (Objective& o : quest.objectives) {
        if (o.type == "defeat")
            continue;
        const auto it = m_totals.find(o.type + ":" + o.target);
        o.progress = std::min(o.count, it == m_totals.end() ? 0 : it->second);
    }
    const bool allDone = std::all_of(quest.objectives.begin(), quest.objectives.end(),
                                     [](const Objective& o) { return o.done(); });
    if (quest.state == QuestState::Active && allDone) {
        quest.state = QuestState::Completed;
        showBanner("Objectives complete", quest.title, "Return to Elder Mirou");
    }
}

bool QuestSystem::isGiver(const std::string& npcId) const
{
    for (const Quest& quest : m_quests)
        if (quest.giver == npcId)
            return true;
    return false;
}

QuestSystem::Talk QuestSystem::talkTo(const std::string& npcId) const
{
    Talk talk;
    for (size_t i = 0; i < m_quests.size(); ++i) {
        const Quest& q = m_quests[i];
        if (q.giver != npcId)
            continue;
        if (q.state == QuestState::Completed)
            return {q.id + ".ready", Talk::Action::TurnIn, static_cast<int>(i)};
        if (q.state == QuestState::Active)
            return {q.id + ".progress", Talk::Action::None, static_cast<int>(i)};
    }
    for (size_t i = 0; i < m_quests.size(); ++i) {
        const Quest& q = m_quests[i];
        if (q.giver == npcId && q.state == QuestState::Available)
            return {q.id + ".offer", Talk::Action::Accept, static_cast<int>(i)};
    }
    talk.dialogueKey = m_lastTurnedIn.empty() ? npcId + ".later" : m_lastTurnedIn + ".done";
    return talk;
}

std::string QuestSystem::finishTalk(const Talk& talk)
{
    if (talk.quest < 0)
        return "";
    Quest& quest = m_quests[talk.quest];
    if (talk.action == Talk::Action::Accept && quest.state == QuestState::Available) {
        quest.state = QuestState::Active;
        ++m_revision;
        showBanner("Quest Accepted", quest.title, quest.description);
        updateObjectives(quest);
        m_events.emit({EventType::QuestAccepted, quest.id});
        return "";
    }
    if (talk.action == Talk::Action::TurnIn && quest.state == QuestState::Completed) {
        quest.state = QuestState::TurnedIn;
        ++m_revision;
        m_lastTurnedIn = quest.id;
        showBanner("Quest Complete", quest.title, "Reward: " + quest.rewardText);
        refreshAvailability();
        m_events.emit({EventType::QuestTurnedIn, quest.id});
        return quest.rewardId;
    }
    return "";
}

QuestMarker QuestSystem::marker(const std::string& npcId) const
{
    QuestMarker result = QuestMarker::None;
    for (const Quest& q : m_quests) {
        if (q.giver != npcId)
            continue;
        if (q.state == QuestState::Completed)
            return QuestMarker::Ready;
        if (q.state == QuestState::Available)
            result = QuestMarker::Available;
    }
    for (const Quest& q : m_quests)
        if (q.giver == npcId && q.state == QuestState::Active)
            return QuestMarker::None;
    return result;
}

void QuestSystem::showBanner(const std::string& caption, const std::string& title, const std::string& subtitle)
{
    m_banners.push_back({caption, title, subtitle, 0.0f});
}

void QuestSystem::update(float deltaTime)
{
    if (!m_banners.empty()) {
        m_banners.front().age += deltaTime;
        if (m_banners.front().age > BANNER_TIME)
            m_banners.erase(m_banners.begin());
    }
}

void QuestSystem::drawTracker(UIRenderer& ui) const
{
    const Quest* active = nullptr;
    for (const Quest& q : m_quests)
        if (q.state == QuestState::Active || q.state == QuestState::Completed)
            active = &q;
    if (!active)
        return;

    const float s = ui.scale();
    const float x = 18.0f * s, y = 90.0f * s;
    const float w = 290.0f * s, pad = 14.0f * s;
    const float titleSize = 21.0f * s, size = 17.0f * s;
    const float rowHeight = 26.0f * s;
    const bool ready = active->state == QuestState::Completed;
    const int rows = static_cast<int>(active->objectives.size()) + (ready ? 1 : 0);
    const float h = pad * 2 + 30.0f * s + rows * rowHeight;

    ui.panel(x, y, w, h, 12.0f * s);
    ui.gem(x + pad + 7.0f * s, y + pad + 12.0f * s, 16.0f * s, {1.0f, 0.82f, 0.45f});
    ui.text(active->title, x + pad + 22.0f * s, y + pad, titleSize, UIColors::GOLD);

    float rowY = y + pad + 32.0f * s;
    for (const Objective& o : active->objectives) {
        const float box = 16.0f * s;
        const float boxY = rowY + 3.0f * s;
        if (o.done()) {
            ui.checkMark(x + pad - 2.0f * s, boxY - 3.0f * s, box + 4.0f * s, UIColors::GOLD);
        } else {
            ui.rect(x + pad, boxY, box, box, UIColors::TEXT_DIM, 4.0f * s);
            ui.rect(x + pad + 2.0f * s, boxY + 2.0f * s, box - 4.0f * s, box - 4.0f * s, UIColors::PANEL, 3.0f * s);
        }
        const glm::vec4 color = o.done() ? UIColors::TEXT_DIM : UIColors::TEXT;
        ui.text(o.label, x + pad + 24.0f * s, rowY, size, color);
        if (o.count > 1)
            ui.text(std::to_string(o.progress) + "/" + std::to_string(o.count), x + w - pad, rowY, size,
                    o.done() ? UIColors::GOLD : UIColors::TEXT, TextAlign::Right);
        rowY += rowHeight;
    }
    if (ready)
        ui.text("Return to Elder Mirou", x + pad, rowY, size, UIColors::GOLD);
}

QuestState QuestSystem::stateOf(const std::string& questId) const
{
    const int i = findQuest(questId);
    return i < 0 ? QuestState::Locked : m_quests[i].state;
}

namespace {
const char* stateName(QuestState state)
{
    switch (state) {
    case QuestState::Locked:    return "locked";
    case QuestState::Available: return "available";
    case QuestState::Active:    return "active";
    case QuestState::Completed: return "completed";
    case QuestState::TurnedIn:  return "turnedin";
    }
    return "locked";
}
}

void QuestSystem::exportState(SaveData& data) const
{
    data.quests.clear();
    for (const Quest& quest : m_quests) {
        SaveData::QuestEntry entry{quest.id, stateName(quest.state), {}};
        for (const Objective& o : quest.objectives)
            entry.progress.push_back(o.progress);
        data.quests.push_back(entry);
    }
    data.totals = m_totals;
    data.unique.assign(m_unique.begin(), m_unique.end());
    data.lastTurnedIn = m_lastTurnedIn;
}

void QuestSystem::importState(const SaveData& data)
{
    m_totals = data.totals;
    m_unique = std::set<std::string>(data.unique.begin(), data.unique.end());
    m_lastTurnedIn = data.lastTurnedIn;
    m_banners.clear();

    for (const SaveData::QuestEntry& entry : data.quests) {
        const int i = findQuest(entry.id);
        if (i < 0)
            continue;
        Quest& quest = m_quests[i];
        for (QuestState state : {QuestState::Locked, QuestState::Available, QuestState::Active,
                                 QuestState::Completed, QuestState::TurnedIn})
            if (entry.state == stateName(state))
                quest.state = state;
        for (size_t k = 0; k < quest.objectives.size() && k < entry.progress.size(); ++k)
            quest.objectives[k].progress = std::clamp(entry.progress[k], 0, quest.objectives[k].count);
    }
    refreshAvailability();
    ++m_revision;
}

std::string QuestSystem::debugCompleteActive()
{
    for (Quest& quest : m_quests) {
        if (quest.state != QuestState::Active)
            continue;
        for (Objective& o : quest.objectives) {
            if (o.type == "defeat") {
                o.progress = o.count;
                continue;
            }
            const std::string key = o.type + ":" + o.target;
            if (o.type == "visit" || o.type == "talk")
                m_unique.insert(key);
            m_totals[key] = std::max(m_totals[key], o.count);
        }
        updateObjectives(quest);
        ++m_revision;
        return quest.title;
    }
    return "";
}

bool QuestSystem::debugStartQuest(const std::string& questId, std::vector<std::string>& rewards)
{
    const int target = findQuest(questId);
    if (target < 0 || m_quests[target].state == QuestState::Completed || m_quests[target].state == QuestState::TurnedIn)
        return false;
    for (int i = 0; i < target; ++i) {
        Quest& quest = m_quests[i];
        if (quest.state == QuestState::TurnedIn)
            continue;
        for (Objective& o : quest.objectives)
            o.progress = o.count;
        quest.state = QuestState::TurnedIn;
        m_lastTurnedIn = quest.id;
        if (!quest.rewardId.empty())
            rewards.push_back(quest.rewardId);
        m_events.emit({EventType::QuestTurnedIn, quest.id});
    }
    Quest& quest = m_quests[target];
    if (quest.state != QuestState::Active) {
        quest.locked = false;
        quest.state = QuestState::Active;
        showBanner("Quest Accepted", quest.title, quest.description);
        updateObjectives(quest);
        m_events.emit({EventType::QuestAccepted, quest.id});
    }
    refreshAvailability();
    ++m_revision;
    return true;
}

void QuestSystem::drawBanners(UIRenderer& ui, float topOffset) const
{
    if (m_banners.empty())
        return;
    const Banner& b = m_banners.front();
    const float s = ui.scale();

    const float alpha = std::min({1.0f, b.age / BANNER_FADE, (BANNER_TIME - b.age) / BANNER_FADE});
    const float slide = (1.0f - std::min(1.0f, b.age / BANNER_FADE)) * -12.0f * s;
    const float captionSize = 18.0f * s, titleSize = 34.0f * s, subtitleSize = 18.0f * s;
    const float w = std::max({ui.textWidth(b.title, titleSize), ui.textWidth(b.subtitle, subtitleSize),
                              260.0f * s}) + 80.0f * s;
    const float h = 118.0f * s;
    const float x = (ui.width() - w) * 0.5f, y = ui.height() * 0.06f + slide + topOffset;

    auto faded = [alpha](glm::vec4 c) { c.a *= alpha; return c; };
    const glm::vec4 panel(UIColors::PANEL.r, UIColors::PANEL.g, UIColors::PANEL.b, UIColors::PANEL.a * alpha);
    ui.rect(x, y, w, h, panel, 16.0f * s);
    ui.outline(x, y, w, h, faded(UIColors::PANEL_EDGE), 16.0f * s, std::max(1.5f, 1.5f * s));
    ui.rect(x + 30.0f * s, y + 36.0f * s, w - 60.0f * s, 1.5f * s, {1.0f, 0.84f, 0.46f, 0.6f * alpha});
    ui.gem(x + 26.0f * s, y + 20.0f * s, 16.0f * s, {1.0f, 0.82f, 0.45f}, alpha);
    ui.gem(x + w - 26.0f * s, y + 20.0f * s, 16.0f * s, {1.0f, 0.82f, 0.45f}, alpha);
    ui.text(b.caption, x + w * 0.5f, y + 10.0f * s, captionSize, faded(UIColors::GOLD), TextAlign::Center);
    ui.text(b.title, x + w * 0.5f, y + 40.0f * s, titleSize, faded(UIColors::TEXT), TextAlign::Center);
    ui.text(b.subtitle, x + w * 0.5f, y + 84.0f * s, subtitleSize, faded(UIColors::TEXT_DIM), TextAlign::Center);
}

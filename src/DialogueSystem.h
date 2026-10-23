#pragma once

#include "UIRenderer.h"

#include <map>
#include <string>
#include <vector>

struct DialogueSettings {
    float charactersPerSecond = 45.0f;
};

class DialogueSystem {
public:
    explicit DialogueSystem(const std::string& relativePath, const DialogueSettings& settings = {});

    void load(const std::string& relativePath);

    DialogueSettings settings;

    bool has(const std::string& key) const { return m_conversations.count(key) > 0; }
    void start(const std::string& key, const std::string& speaker);
    void advance();
    void close() { m_active = false; }
    bool isActive() const { return m_active; }

    void update(float deltaTime);
    void draw(UIRenderer& ui, float time) const;

private:
    std::map<std::string, std::vector<std::string>> m_conversations;
    const std::vector<std::string>* m_lines = nullptr;
    std::string m_speaker;
    size_t m_lineIndex = 0;
    float  m_visibleCharacters = 0.0f;
    bool   m_active = false;

    size_t currentLength() const;
};

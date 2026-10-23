#include "DialogueSystem.h"
#include "Paths.h"

#include <cmath>
#include <fstream>
#include <stdexcept>

namespace {
std::string trim(const std::string& s)
{
    const size_t first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return "";
    const size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}
}

DialogueSystem::DialogueSystem(const std::string& relativePath, const DialogueSettings& settings)
    : settings(settings)
{
    load(relativePath);
}

void DialogueSystem::load(const std::string& relativePath)
{
    std::ifstream file(resourcePath(relativePath));
    if (!file)
        throw std::runtime_error("Cannot open dialogue file: " + relativePath);

    std::string line, section;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue;
        if (line.front() == '[' && line.back() == ']') {
            section = line.substr(1, line.size() - 2);
            m_conversations[section];
        } else if (!section.empty()) {
            m_conversations[section].push_back(line);
        }
    }
}

void DialogueSystem::start(const std::string& key, const std::string& speaker)
{
    const auto it = m_conversations.find(key);
    if (it == m_conversations.end() || it->second.empty())
        return;
    m_lines = &it->second;
    m_speaker = speaker;
    m_lineIndex = 0;
    m_visibleCharacters = 0.0f;
    m_active = true;
}

size_t DialogueSystem::currentLength() const
{
    return Font::decode((*m_lines)[m_lineIndex]).size();
}

void DialogueSystem::advance()
{
    if (!m_active)
        return;
    if (m_visibleCharacters < currentLength()) {
        m_visibleCharacters = static_cast<float>(currentLength());
    } else if (m_lineIndex + 1 < m_lines->size()) {
        ++m_lineIndex;
        m_visibleCharacters = 0.0f;
    } else {
        m_active = false;
    }
}

void DialogueSystem::update(float deltaTime)
{
    if (m_active)
        m_visibleCharacters += settings.charactersPerSecond * deltaTime;
}

void DialogueSystem::draw(UIRenderer& ui, float time) const
{
    if (!m_active)
        return;
    const float s = ui.scale();
    const float textSize = 22.0f * s;
    const float boxWidth = std::min(ui.width() - 60.0f * s, 960.0f * s);
    const float boxHeight = 150.0f * s;
    const float x = (ui.width() - boxWidth) * 0.5f;
    const float y = ui.height() - boxHeight - 28.0f * s;
    const float pad = 26.0f * s;

    ui.panel(x, y, boxWidth, boxHeight, 16.0f * s);

    const float nameSize = 20.0f * s;
    const float nameWidth = ui.textWidth(m_speaker, nameSize) + 28.0f * s;
    const float nameHeight = 34.0f * s;
    ui.rect(x + 20.0f * s, y - nameHeight * 0.6f, nameWidth, nameHeight, UIColors::GOLD, 10.0f * s);
    ui.text(m_speaker, x + 34.0f * s, y - nameHeight * 0.6f + 5.0f * s, nameSize, {0.22f, 0.15f, 0.20f, 1.0f},
            TextAlign::Left, false);

    const std::vector<std::string> wrapped = ui.wrap((*m_lines)[m_lineIndex], textSize, boxWidth - 2 * pad);
    size_t remaining = static_cast<size_t>(m_visibleCharacters);
    float lineY = y + pad;
    for (const std::string& line : wrapped) {
        const std::u32string letters = Font::decode(line);
        if (remaining == 0)
            break;
        const size_t shown = std::min(remaining, letters.size());
        size_t bytes = 0;
        for (size_t count = 0; bytes < line.size() && count < shown; ++count) {
            const unsigned char c = static_cast<unsigned char>(line[bytes]);
            bytes += c >= 0xF0 ? 4 : c >= 0xE0 ? 3 : c >= 0xC0 ? 2 : 1;
        }
        ui.text(line.substr(0, bytes), x + pad, lineY, textSize, UIColors::TEXT);
        remaining -= shown;
        lineY += ui.lineHeight(textSize) * 1.05f;
    }

    if (m_visibleCharacters >= currentLength()) {
        const float bounce = 3.0f * s * std::sin(time * 5.0f);
        ui.gem(x + boxWidth - 30.0f * s, y + boxHeight - 26.0f * s + bounce, 18.0f * s, {1.0f, 0.82f, 0.45f});
        const std::string hint = m_lineIndex + 1 < m_lines->size() ? "E" : "E  close";
        ui.text(hint, x + boxWidth - 48.0f * s, y + boxHeight - 37.0f * s, 16.0f * s, UIColors::TEXT_DIM, TextAlign::Right);
    }
}

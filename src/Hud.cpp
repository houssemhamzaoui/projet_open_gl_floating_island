#include "Hud.h"

#include <cmath>

namespace Hud {

void drawHearts(UIRenderer& ui, int hearts, int maxHearts, float hurtFlash, float time)
{
    const float s = ui.scale();
    const float size = 28.0f * s, gap = 6.0f * s, pad = 12.0f * s;
    const float w = pad * 2 + maxHearts * size + (maxHearts - 1) * gap, h = pad * 2 + size;
    const float x = 18.0f * s, y = 18.0f * s;
    ui.panel(x, y, w, h, 14.0f * s);
    for (int i = 0; i < maxHearts; ++i) {
        const float cx = x + pad + size * 0.5f + i * (size + gap);
        const float cy = y + pad + size * 0.5f;
        if (i < hearts) {
            const float beat = (hearts <= 2 && i == hearts - 1) ? 1.0f + 0.12f * std::sin(time * 8.0f) : 1.0f;
            ui.heart(cx, cy + 1.5f * s, size * 1.08f * beat, {0.18f, 0.06f, 0.10f, 0.9f});
            ui.heart(cx, cy, size * beat, glm::mix(glm::vec4(1.0f, 0.38f, 0.48f, 1.0f), glm::vec4(1.0f), hurtFlash));
        } else {
            ui.heart(cx, cy, size, {0.0f, 0.0f, 0.0f, 0.45f});
        }
    }
}

static void cloudIcon(UIRenderer& ui, glm::vec2 c, float s, const glm::vec4& color)
{
    ui.rect(c.x - 9 * s, c.y - 2 * s, 18 * s, 8 * s, color, 4 * s);
    ui.rect(c.x - 7 * s, c.y - 7 * s, 9 * s, 9 * s, color, 4.5f * s);
    ui.rect(c.x - 2 * s, c.y - 9 * s, 10 * s, 10 * s, color, 5 * s);
}

static void weatherIcon(UIRenderer& ui, glm::vec2 c, float s, int weather, bool night, float time)
{
    const glm::vec4 white{0.92f, 0.94f, 1.0f, 1.0f};
    switch (weather) {
    case 1: cloudIcon(ui, c, s, white); break;
    case 2:
        cloudIcon(ui, c - glm::vec2(0, 3 * s), s, {0.75f, 0.78f, 0.86f, 1.0f});
        for (int i = 0; i < 3; ++i) {
            const float x = c.x - 6 * s + i * 6 * s;
            const float y = c.y + 6 * s + std::fmod(time * 12.0f + i * 3.0f, 4.0f) * s;
            ui.line({x, y}, {x - 1.5f * s, y + 4 * s}, 1.8f * s, {0.55f, 0.75f, 1.0f, 1.0f});
        }
        break;
    case 3:
        for (int i = 0; i < 3; ++i) {
            const float a = i * 1.0472f + 0.5236f;
            const glm::vec2 d(std::cos(a), std::sin(a));
            ui.line(c - d * (8 * s), c + d * (8 * s), 2.0f * s, white);
        }
        break;
    case 4:
        ui.rect(c.x - 9 * s, c.y - 5 * s, 9 * s, 6 * s, {1.0f, 0.65f, 0.78f, 1.0f}, 3 * s);
        ui.rect(c.x + 1 * s, c.y + 0 * s, 8 * s, 5 * s, {1.0f, 0.8f, 0.88f, 1.0f}, 2.5f * s);
        break;
    default:
        if (night) {
            ui.line(c - glm::vec2(6 * s, 0), c + glm::vec2(6 * s, 0), 2.0f * s, UIColors::GOLD);
            ui.line(c - glm::vec2(0, 6 * s), c + glm::vec2(0, 6 * s), 2.0f * s, UIColors::GOLD);
        } else {
            ui.rect(c.x - 6 * s, c.y - 6 * s, 12 * s, 12 * s, UIColors::GOLD, 6 * s);
        }
        break;
    }
}

void drawClock(UIRenderer& ui, const std::string& clock, bool night, bool paused, float speed, float time, int weather)
{
    const float s = ui.scale();
    const float w = 198.0f * s, h = 44.0f * s;
    const float x = ui.width() - w - 18.0f * s, y = 206.0f * s;
    ui.panel(x, y, w, h, 12.0f * s);
    const glm::vec2 c(x + 24.0f * s, y + h * 0.5f);
    const float r = 10.0f * s;
    if (!night) {
        for (int i = 0; i < 8; ++i) {
            const float a = i * 0.785398f + time * 0.3f;
            const glm::vec2 d(std::cos(a), std::sin(a));
            ui.line(c + d * (r * 1.25f), c + d * (r * 1.7f), 2.2f * s, UIColors::GOLD);
        }
        ui.rect(c.x - r, c.y - r, 2 * r, 2 * r, UIColors::GOLD, r);
    } else {
        ui.rect(c.x - r, c.y - r, 2 * r, 2 * r, {0.88f, 0.92f, 1.0f, 1.0f}, r);
        ui.rect(c.x - r + 5.0f * s, c.y - r - 3.0f * s, 2 * r, 2 * r, {0.10f, 0.09f, 0.16f, 1.0f}, r);
    }
    ui.text(clock, x + 46.0f * s, y + 9.0f * s, 22.0f * s, UIColors::TEXT);
    weatherIcon(ui, {x + 128.0f * s, y + h * 0.5f}, s, weather, night, time);
    std::string status = paused ? "II" : (speed > 1.0f ? "x" + std::to_string(static_cast<int>(speed)) : "");
    if (!status.empty())
        ui.text(status, x + w - 12.0f * s, y + 12.0f * s, 17.0f * s, UIColors::GOLD, TextAlign::Right);
}

void drawGemCounters(UIRenderer& ui, const GemCounts& c)
{
    const float s = ui.scale();
    struct Row { const char* label; glm::vec3 color; int got; int total; };
    const Row rows[] = {
        {"Gems",        {0.35f, 0.62f, 1.00f}, c.common, c.commonTotal},
        {"Biome gems",  {0.85f, 0.50f, 1.00f}, c.biome,  c.biomeTotal},
        {"Diamonds",    {0.92f, 0.95f, 1.00f}, c.rare,   c.rareTotal},
        {"Spirit gift", {1.00f, 0.82f, 0.40f}, c.spirit, 1},
        {"Cave Heart",  {1.00f, 0.35f, 0.60f}, c.heart ? 1 : 0, 1},
    };
    const float rowHeight = 30.0f * s, pad = 14.0f * s;
    const float w = 230.0f * s, h = pad * 2 + rowHeight * 5;
    const float x = ui.width() - w - 18.0f * s, y = 18.0f * s;
    ui.panel(x, y, w, h, 14.0f * s);

    const float size = 19.0f * s;
    for (int i = 0; i < 5; ++i) {
        const Row& r = rows[i];
        const float rowY = y + pad + i * rowHeight;
        const bool complete = r.got >= r.total && r.total > 0;
        ui.gem(x + pad + 9.0f * s, rowY + rowHeight * 0.5f, 20.0f * s, r.color);
        ui.text(r.label, x + pad + 26.0f * s, rowY + 3.0f * s, size, UIColors::TEXT);
        const glm::vec4 countColor = complete ? UIColors::GOLD : UIColors::TEXT_DIM;
        if (i == 4 && complete)
            ui.checkMark(x + w - pad - 22.0f * s, rowY + 4.0f * s, 22.0f * s, UIColors::GOLD);
        else
            ui.text(std::to_string(r.got) + "/" + std::to_string(r.total), x + w - pad, rowY + 3.0f * s, size,
                    countColor, TextAlign::Right);
    }
}

void drawPortalHint(UIRenderer& ui, const std::string& destination)
{
    if (destination.empty())
        return;
    const float s = ui.scale();
    const std::string text = "Portal to " + destination;
    const float size = 20.0f * s;
    const float w = ui.textWidth(text, size) + 70.0f * s, h = 40.0f * s;
    const float x = (ui.width() - w) * 0.5f, y = ui.height() - h - 40.0f * s;
    ui.panel(x, y, w, h, h * 0.5f);
    const glm::vec2 tip(x + 34.0f * s, y + h * 0.5f);
    ui.line(tip - glm::vec2(14.0f * s, 0.0f), tip, 3.0f * s, UIColors::GOLD);
    ui.line(tip - glm::vec2(6.0f * s, 6.0f * s), tip, 3.0f * s, UIColors::GOLD);
    ui.line(tip - glm::vec2(6.0f * s, -6.0f * s), tip, 3.0f * s, UIColors::GOLD);
    ui.text(text, x + 48.0f * s, y + 8.0f * s, size, UIColors::TEXT);
}

void drawTalkPrompt(UIRenderer& ui, const glm::vec2& screen, float time)
{
    const float s = ui.scale();
    const float bob = 3.0f * s * std::sin(time * 3.0f);
    const float size = 19.0f * s;
    const float keySize = 26.0f * s;
    const float w = keySize + ui.textWidth("Talk", size) + 30.0f * s, h = 36.0f * s;
    const float x = screen.x - w * 0.5f, y = screen.y - h + bob;
    ui.panel(x, y, w, h, h * 0.5f);
    ui.rect(x + 6.0f * s, y + (h - keySize) * 0.5f, keySize, keySize, UIColors::GOLD, 6.0f * s);
    ui.text("E", x + 6.0f * s + keySize * 0.5f, y + (h - keySize) * 0.5f + 2.0f * s, size,
            {0.22f, 0.15f, 0.20f, 1.0f}, TextAlign::Center, false);
    ui.text("Talk", x + keySize + 14.0f * s, y + 6.0f * s, size, UIColors::TEXT);
    ui.gem(screen.x, y + h + 5.0f * s, 10.0f * s, {1.0f, 0.84f, 0.46f});
}

void drawQuestMarker(UIRenderer& ui, const glm::vec2& screen, bool ready, float distance, float time)
{
    const float s = ui.scale();
    const float size = glm::clamp(46.0f - distance * 0.35f, 26.0f, 46.0f) * s;
    const float bob = 4.0f * s * std::sin(time * 2.5f);
    const float pulse = 1.0f + 0.06f * std::sin(time * 5.0f);
    const float r = size * 0.5f * pulse;
    const glm::vec2 c(screen.x, screen.y - r + bob);
    ui.rect(c.x - r - 2.0f * s, c.y - r - 2.0f * s, 2 * r + 4.0f * s, 2 * r + 4.0f * s, {0.15f, 0.10f, 0.12f, 0.85f}, r + 2.0f * s);
    ui.rect(c.x - r, c.y - r, 2 * r, 2 * r, UIColors::GOLD, r);
    ui.rect(c.x - r * 0.78f, c.y - r * 0.78f, 2 * r * 0.78f, 2 * r * 0.78f, {1.0f, 0.93f, 0.66f, 1.0f}, r * 0.78f);
    const float textSize = size * 0.9f;
    ui.text(ready ? "?" : "!", c.x, c.y - ui.lineHeight(textSize) * 0.5f, textSize,
            {0.42f, 0.22f, 0.12f, 1.0f}, TextAlign::Center, false);
}

void drawDemoTag(UIRenderer& ui, const std::string& message, float messageAlpha)
{
    const float s = ui.scale();
    const float size = 15.0f * s, h = 26.0f * s;
    const float x = 14.0f * s, y = ui.height() - h - 14.0f * s;
    const float w = ui.textWidth("DEMO", size) + 22.0f * s;
    ui.rect(x, y, w, h, {0.85f, 0.32f, 0.45f, 0.85f}, 7.0f * s);
    ui.text("DEMO", x + w * 0.5f, y + (h - ui.lineHeight(size)) * 0.5f, size, UIColors::TEXT, TextAlign::Center, false);
    if (message.empty() || messageAlpha <= 0.0f)
        return;
    const float mw = ui.textWidth(message, size) + 22.0f * s;
    ui.rect(x + w + 8.0f * s, y, mw, h, {UIColors::PANEL.r, UIColors::PANEL.g, UIColors::PANEL.b, UIColors::PANEL.a * messageAlpha}, 7.0f * s);
    glm::vec4 color = UIColors::TEXT;
    color.a *= messageAlpha;
    ui.text(message, x + w + 19.0f * s, y + (h - ui.lineHeight(size)) * 0.5f, size, color, TextAlign::Left, false);
}

void drawSaveNotice(UIRenderer& ui, float alpha)
{
    if (alpha <= 0.0f)
        return;
    const float s = ui.scale();
    const float size = 16.0f * s;
    const float x = ui.width() - 16.0f * s, y = ui.height() - 34.0f * s;
    glm::vec4 color = UIColors::TEXT_DIM;
    color.a *= alpha;
    const float w = ui.text("Game saved", x, y, size, color, TextAlign::Right);
    ui.gem(x - w - 14.0f * s, y + ui.lineHeight(size) * 0.5f, 13.0f * s, {1.0f, 0.82f, 0.45f}, alpha);
}

}

#include "Menu.h"

#include "GameConfig.h"

#include <algorithm>
#include <cmath>
#include <iterator>

namespace {

const glm::vec4 BUTTON      {0.10f, 0.09f, 0.16f, 0.72f};
const glm::vec4 BUTTON_HOVER{0.24f, 0.19f, 0.30f, 0.90f};
const glm::vec4 DIM_SCREEN  {0.03f, 0.02f, 0.06f, 0.50f};

struct KeyRow { const char* key; const char* action; };

const KeyRow EXPLORE[] = {
    {"", "Explore"},
    {"Mouse", "Turn the camera"},
    {"W A S D", "Move"},
    {"Shift", "Sprint"},
    {"Space", "Jump"},
    {"Hold Space in the air", "Glide"},
    {"E", "Talk / next line"},
    {"Mouse wheel", "Camera distance"},
    {"", "Fight"},
    {"Left click", "Staff swing"},
    {"Right click", "Magic orb"},
};
const KeyRow WORLD[] = {
    {"", "World and view"},
    {"T", "Toon / realistic shading"},
    {"B", "2 / 3 toon bands"},
    {"F1", "Wireframe"},
    {"V", "Free camera"},
    {"K", "Next weather here"},
    {"N", "Pause / resume time"},
    {"Page Up / Down", "Time faster / slower"},
    {"Arrows", "Move the sun by hand"},
    {"C / F", "Clouds / fog"},
    {"Tab", "Free the mouse"},
    {"Esc", "Pause menu"},
};
const KeyRow DEMO[] = {
    {"", "Demo keys"},
    {"F2", "Go to the cave chamber"},
    {"F3", "Complete the quest objectives"},
    {"F4", "Full health"},
    {"F5", "Boss fight (quest 5)"},
    {"F6", "Go to the plaza"},
    {"R", "New random world"},
};

bool inside(const glm::vec2& p, const glm::vec4& r)
{
    return p.x >= r.x && p.x <= r.x + r.z && p.y >= r.y && p.y <= r.y + r.w;
}

}

void Menu::openTitle(bool hasSave)
{
    m_hasSave = hasSave;
    go(Page::Title);
}

void Menu::openPause()
{
    go(Page::Pause);
}

void Menu::go(Page page, int selected)
{
    m_page = page;
    m_selected = selected;
    m_buttonRects.clear();
}

std::vector<Menu::Button> Menu::buttons() const
{
    switch (m_page) {
    case Page::Title: {
        std::vector<Button> list;
        if (m_hasSave)
            list.push_back({"Continue", MenuAction::Continue, Page::Hidden});
        list.push_back(m_hasSave ? Button{"New Game", MenuAction::None, Page::ConfirmNewGame}
                                 : Button{"New Game", MenuAction::NewGame, Page::Hidden});
        list.push_back({"Quit", MenuAction::Quit, Page::Hidden});
        return list;
    }
    case Page::ConfirmNewGame:
        return {{"Erase and start", MenuAction::NewGame, Page::Hidden},
                {"Cancel", MenuAction::None, Page::Title}};
    case Page::Pause:
        return {{"Resume", MenuAction::Resume, Page::Hidden},
                {"Controls", MenuAction::None, Page::Controls},
                {"Main Menu", MenuAction::MainMenu, Page::Hidden},
                {"Quit", MenuAction::Quit, Page::Hidden}};
    case Page::Controls:
        return {{"Back", MenuAction::None, Page::Pause}};
    case Page::Hidden:
        break;
    }
    return {};
}

MenuAction Menu::choose(int index)
{
    const std::vector<Button> list = buttons();
    if (index < 0 || index >= static_cast<int>(list.size()))
        return MenuAction::None;
    const Button& button = list[index];
    if (button.action != MenuAction::None)
        return button.action;
    int selected = 0;
    if (button.next == Page::Pause || button.next == Page::ConfirmNewGame)
        selected = 1;
    else if (button.next == Page::Title)
        selected = m_hasSave ? 1 : 0;
    go(button.next, selected);
    return MenuAction::None;
}

MenuAction Menu::back()
{
    switch (m_page) {
    case Page::Pause:          return MenuAction::Resume;
    case Page::Controls:       go(Page::Pause, 1); break;
    case Page::ConfirmNewGame: go(Page::Title, 1); break;
    default:                   break;
    }
    return MenuAction::None;
}

MenuAction Menu::update(const Window& window)
{
    if (m_page == Page::Hidden)
        return MenuAction::None;
    const int count = static_cast<int>(buttons().size());

    if (window.isKeyJustPressed(GLFW_KEY_ESCAPE))
        return back();
    if (window.isKeyJustPressed(GLFW_KEY_UP) || window.isKeyJustPressed(GLFW_KEY_W))
        m_selected = (m_selected + count - 1) % count;
    if (window.isKeyJustPressed(GLFW_KEY_DOWN) || window.isKeyJustPressed(GLFW_KEY_S))
        m_selected = (m_selected + 1) % count;
    if (window.isKeyJustPressed(GLFW_KEY_ENTER) || window.isKeyJustPressed(GLFW_KEY_KP_ENTER)
        || window.isKeyJustPressed(GLFW_KEY_SPACE))
        return choose(m_selected);

    const glm::vec2 mouse = window.cursorPosition();
    const bool moved = glm::length(mouse - m_lastMouse) > 0.5f;
    m_lastMouse = mouse;
    for (int i = 0; i < static_cast<int>(m_buttonRects.size()) && i < count; ++i) {
        if (!inside(mouse, m_buttonRects[i]))
            continue;
        if (moved)
            m_selected = i;
        if (window.isMouseJustPressed(GLFW_MOUSE_BUTTON_LEFT))
            return choose(i);
    }
    return MenuAction::None;
}

void Menu::draw(UIRenderer& ui, float time) const
{
    switch (m_page) {
    case Page::Title:          drawTitle(ui, time); break;
    case Page::ConfirmNewGame: drawTitle(ui, time); drawConfirm(ui, time); break;
    case Page::Pause:          drawPause(ui, time); break;
    case Page::Controls:       drawControls(ui, time); break;
    case Page::Hidden:         break;
    }
}

void Menu::drawButtons(UIRenderer& ui, float centerX, float top, float width, float time) const
{
    const float s = ui.scale();
    const float h = 50.0f * s, gap = 14.0f * s, size = 23.0f * s;
    const std::vector<Button> list = buttons();
    m_buttonRects.clear();
    for (size_t i = 0; i < list.size(); ++i) {
        const float x = centerX - width * 0.5f, y = top + i * (h + gap);
        const bool selected = static_cast<int>(i) == m_selected;
        m_buttonRects.push_back({x, y, width, h});

        ui.rect(x, y, width, h, selected ? BUTTON_HOVER : BUTTON, 14.0f * s);
        if (selected) {
            const float pulse = 0.75f + 0.25f * std::sin(time * 4.0f);
            ui.outline(x, y, width, h, {1.0f, 0.84f, 0.46f, pulse}, 14.0f * s, 2.0f * s);
            ui.gem(x + 24.0f * s, y + h * 0.5f, 15.0f * s, {1.0f, 0.82f, 0.45f});
            ui.gem(x + width - 24.0f * s, y + h * 0.5f, 15.0f * s, {1.0f, 0.82f, 0.45f});
        } else {
            ui.outline(x, y, width, h, {0.93f, 0.78f, 0.45f, 0.35f}, 14.0f * s, 1.5f * s);
        }
        ui.text(list[i].label, centerX, y + (h - ui.lineHeight(size)) * 0.5f, size,
                selected ? UIColors::GOLD : UIColors::TEXT, TextAlign::Center);
    }
}

void Menu::drawTitle(UIRenderer& ui, float time) const
{
    const float s = ui.scale();
    const float cx = ui.width() * 0.5f;

    const int layers = 10;
    const float bandCenter = ui.height() * 0.23f, bandHeight = ui.height() * 0.36f;
    for (int i = 1; i <= layers; ++i) {
        const float h = bandHeight * i / layers;
        ui.rect(0.0f, bandCenter - h * 0.5f, ui.width(), h, {0.05f, 0.04f, 0.10f, 0.045f});
    }

    const float bob = std::sin(time * 1.2f) * 4.0f * s;
    const float titleSize = 84.0f * s;
    const float titleY = ui.height() * 0.13f + bob;
    ui.text("Floating Island", cx, titleY, titleSize, UIColors::GOLD, TextAlign::Center);
    const float halfWidth = ui.textWidth("Floating Island", titleSize) * 0.5f;
    ui.gem(cx - halfWidth - 34.0f * s, titleY + titleSize * 0.62f, 26.0f * s, {0.70f, 0.40f, 1.00f});
    ui.gem(cx + halfWidth + 34.0f * s, titleY + titleSize * 0.62f, 26.0f * s, {1.00f, 0.52f, 0.74f});
    ui.text("A small stylized world in C++ and OpenGL", cx, titleY + titleSize * 1.12f, 22.0f * s,
            UIColors::TEXT, TextAlign::Center);

    if (m_page == Page::Title)
        drawButtons(ui, cx, ui.height() * 0.50f, 300.0f * s, time);

    const char* hint = "Arrows + Enter, or the mouse";
    const float hintSize = 16.0f * s;
    const float hintWidth = ui.textWidth(hint, hintSize) + 28.0f * s;
    ui.rect(cx - hintWidth * 0.5f, ui.height() - 46.0f * s, hintWidth, 30.0f * s, UIColors::PANEL, 15.0f * s);
    ui.text(hint, cx, ui.height() - 46.0f * s + (30.0f * s - ui.lineHeight(hintSize)) * 0.5f, hintSize,
            UIColors::TEXT_DIM, TextAlign::Center);
}

void Menu::drawConfirm(UIRenderer& ui, float time) const
{
    const float s = ui.scale();
    ui.rect(0.0f, 0.0f, ui.width(), ui.height(), DIM_SCREEN);
    const float w = 480.0f * s, h = 258.0f * s;
    const float x = (ui.width() - w) * 0.5f, y = ui.height() * 0.42f - 40.0f * s;
    ui.panel(x, y, w, h, 18.0f * s);
    ui.text("Start a new game?", x + w * 0.5f, y + 22.0f * s, 30.0f * s, UIColors::GOLD, TextAlign::Center);
    ui.text("This will erase your progress.", x + w * 0.5f, y + 66.0f * s, 20.0f * s, UIColors::TEXT,
            TextAlign::Center);
    drawButtons(ui, x + w * 0.5f, y + 120.0f * s, 300.0f * s, time);
}

void Menu::drawPause(UIRenderer& ui, float time) const
{
    const float s = ui.scale();
    ui.rect(0.0f, 0.0f, ui.width(), ui.height(), DIM_SCREEN);
    const float w = 380.0f * s, h = 352.0f * s;
    const float x = (ui.width() - w) * 0.5f, y = (ui.height() - h) * 0.5f;
    ui.panel(x, y, w, h, 18.0f * s);
    ui.text("Paused", x + w * 0.5f, y + 20.0f * s, 36.0f * s, UIColors::GOLD, TextAlign::Center);
    drawButtons(ui, x + w * 0.5f, y + 82.0f * s, 300.0f * s, time);
}

void Menu::drawControls(UIRenderer& ui, float time) const
{
    const float s = ui.scale();
    ui.rect(0.0f, 0.0f, ui.width(), ui.height(), DIM_SCREEN);
    const float w = std::min(ui.width() - 32.0f * s, 960.0f * s);
    const float h = std::min(ui.height() - 24.0f * s, 650.0f * s);
    const float x = (ui.width() - w) * 0.5f, y = (ui.height() - h) * 0.5f;
    ui.panel(x, y, w, h, 18.0f * s);
    ui.text("Controls", x + w * 0.5f, y + 14.0f * s, 32.0f * s, UIColors::GOLD, TextAlign::Center);

    const float row = 23.0f * s, size = 17.0f * s, headingSize = 19.0f * s;
    auto column = [&](float left, float keyWidth, float top, const KeyRow* rows, size_t count) {
        float rowY = top;
        for (size_t i = 0; i < count; ++i) {
            if (rows[i].key[0] == '\0') {
                rowY += (i == 0 ? 0.0f : 8.0f * s);
                ui.text(rows[i].action, left, rowY, headingSize, UIColors::GOLD);
                ui.rect(left, rowY + headingSize * 1.25f, w * 0.40f, 1.0f * s, {1.0f, 0.84f, 0.46f, 0.45f});
                rowY += row + 6.0f * s;
                continue;
            }
            ui.text(rows[i].key, left + keyWidth, rowY, size, UIColors::GOLD, TextAlign::Right);
            ui.text(rows[i].action, left + keyWidth + 14.0f * s, rowY, size, UIColors::TEXT);
            rowY += row;
        }
        return rowY;
    };
    const float top = y + 64.0f * s;
    const float left = x + 36.0f * s, right = x + w * 0.52f;
    const float leftEnd = column(left, 190.0f * s, top, EXPLORE, std::size(EXPLORE));
    if (DEMO_MODE)
        column(left, 190.0f * s, leftEnd + 4.0f * s, DEMO, std::size(DEMO));
    column(right, 150.0f * s, top, WORLD, std::size(WORLD));

    drawButtons(ui, x + w * 0.5f, y + h - 66.0f * s, 220.0f * s, time);
}

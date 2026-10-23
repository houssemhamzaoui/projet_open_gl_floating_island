#pragma once

#include "UIRenderer.h"
#include "Window.h"

#include <glm/glm.hpp>

#include <string>
#include <vector>

enum class MenuAction { None, Continue, NewGame, Resume, MainMenu, Quit };

class Menu {
public:
    enum class Page { Hidden, Title, ConfirmNewGame, Pause, Controls };

    void openTitle(bool hasSave);
    void openPause();
    void close() { m_page = Page::Hidden; }
    Page page() const { return m_page; }
    bool isOpen() const { return m_page != Page::Hidden; }

    MenuAction update(const Window& window);

    void draw(UIRenderer& ui, float time) const;

private:
    struct Button {
        std::string label;
        MenuAction  action;
        Page        next;
    };
    std::vector<Button> buttons() const;
    void go(Page page, int selected = 0);
    MenuAction choose(int index);
    MenuAction back();

    void drawButtons(UIRenderer& ui, float centerX, float top, float width, float time) const;
    void drawTitle(UIRenderer& ui, float time) const;
    void drawConfirm(UIRenderer& ui, float time) const;
    void drawPause(UIRenderer& ui, float time) const;
    void drawControls(UIRenderer& ui, float time) const;

    Page m_page = Page::Hidden;
    bool m_hasSave = false;
    int  m_selected = 0;
    glm::vec2 m_lastMouse{-1.0f};
    mutable std::vector<glm::vec4> m_buttonRects;
};

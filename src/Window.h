#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <array>
#include <string>

class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool shouldClose() const;
    void setTitle(const std::string& title);
    void requestClose();
    void swapBuffers();
    void pollEvents();

    bool isKeyPressed(int key) const;
    bool isKeyJustPressed(int key) const;
    bool isMouseJustPressed(int button) const;

    void discardPresses();

    glm::vec2 cursorPosition() const;

    glm::vec2 consumeMouseDelta();
    float     consumeScrollDelta();

    void setCursorCaptured(bool captured);
    bool isCursorCaptured() const { return m_cursorCaptured; }

    int   getWidth()  const { return m_width; }
    int   getHeight() const { return m_height; }
    float getAspectRatio() const;
    GLFWwindow* getHandle() const { return m_handle; }

private:
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void cursorPosCallback(GLFWwindow* window, double x, double y);
    static void scrollCallback(GLFWwindow* window, double xOffset, double yOffset);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

    GLFWwindow* m_handle = nullptr;
    int m_width  = 0;
    int m_height = 0;

    std::array<bool, GLFW_KEY_LAST + 1> m_keysJustPressed{};
    std::array<bool, GLFW_MOUSE_BUTTON_LAST + 1> m_mouseJustPressed{};

    glm::dvec2 m_lastMousePos{0.0};
    glm::vec2  m_mouseDelta{0.0f};
    float      m_scrollDelta    = 0.0f;
    bool       m_firstMouse     = true;
    bool       m_cursorCaptured = false;
};

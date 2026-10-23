#include "Window.h"

#include <iostream>
#include <stdexcept>

Window::Window(int width, int height, const std::string& title)
    : m_width(width), m_height(height)
{
    glfwSetErrorCallback([](int code, const char* description) {
        std::cerr << "[GLFW error " << code << "] " << description << '\n';
    });

    if (!glfwInit())
        throw std::runtime_error("Failed to initialize GLFW");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    m_handle = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_handle) {
        glfwTerminate();
        throw std::runtime_error("Failed to create window (is OpenGL 3.3 supported?)");
    }

    glfwMakeContextCurrent(m_handle);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        glfwDestroyWindow(m_handle);
        glfwTerminate();
        throw std::runtime_error("Failed to load OpenGL functions with GLAD");
    }

    glfwSetWindowUserPointer(m_handle, this);
    glfwSetFramebufferSizeCallback(m_handle, framebufferSizeCallback);
    glfwSetCursorPosCallback(m_handle, cursorPosCallback);
    glfwSetScrollCallback(m_handle, scrollCallback);
    glfwSetKeyCallback(m_handle, keyCallback);
    glfwSetMouseButtonCallback(m_handle, mouseButtonCallback);

    glfwGetFramebufferSize(m_handle, &m_width, &m_height);
    glViewport(0, 0, m_width, m_height);

    glfwSwapInterval(1);
    glEnable(GL_MULTISAMPLE);

    std::cout << "OpenGL " << glGetString(GL_VERSION) << '\n'
              << "GPU    " << glGetString(GL_RENDERER) << '\n';
}

Window::~Window()
{
    glfwDestroyWindow(m_handle);
    glfwTerminate();
}

bool Window::shouldClose() const { return glfwWindowShouldClose(m_handle); }
void Window::setTitle(const std::string& title) { glfwSetWindowTitle(m_handle, title.c_str()); }
void Window::requestClose()      { glfwSetWindowShouldClose(m_handle, GLFW_TRUE); }
void Window::swapBuffers()       { glfwSwapBuffers(m_handle); }

void Window::pollEvents()
{
    m_keysJustPressed.fill(false);
    m_mouseJustPressed.fill(false);
    glfwPollEvents();
}

void Window::discardPresses()
{
    m_keysJustPressed.fill(false);
    m_mouseJustPressed.fill(false);
}

glm::vec2 Window::cursorPosition() const
{
    double x = 0.0, y = 0.0;
    int windowWidth = 1, windowHeight = 1;
    glfwGetCursorPos(m_handle, &x, &y);
    glfwGetWindowSize(m_handle, &windowWidth, &windowHeight);
    if (windowWidth <= 0 || windowHeight <= 0)
        return glm::vec2(-1.0f);
    return {static_cast<float>(x) * m_width / windowWidth, static_cast<float>(y) * m_height / windowHeight};
}

bool Window::isKeyPressed(int key) const
{
    return glfwGetKey(m_handle, key) == GLFW_PRESS;
}

bool Window::isMouseJustPressed(int button) const
{
    return button >= 0 && button <= GLFW_MOUSE_BUTTON_LAST && m_mouseJustPressed[button];
}

bool Window::isKeyJustPressed(int key) const
{
    return key >= 0 && key <= GLFW_KEY_LAST && m_keysJustPressed[key];
}

glm::vec2 Window::consumeMouseDelta()
{
    glm::vec2 delta = m_mouseDelta;
    m_mouseDelta = glm::vec2(0.0f);
    return delta;
}

float Window::consumeScrollDelta()
{
    float delta = m_scrollDelta;
    m_scrollDelta = 0.0f;
    return delta;
}

void Window::setCursorCaptured(bool captured)
{
    m_cursorCaptured = captured;
    glfwSetInputMode(m_handle, GLFW_CURSOR,
                     captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(m_handle, GLFW_RAW_MOUSE_MOTION, captured ? GLFW_TRUE : GLFW_FALSE);

    m_firstMouse = true;
    m_mouseDelta = glm::vec2(0.0f);
}

float Window::getAspectRatio() const
{
    return m_height > 0 ? static_cast<float>(m_width) / m_height : 1.0f;
}

void Window::framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    self->m_width  = width;
    self->m_height = height;
    glViewport(0, 0, width, height);
}

void Window::cursorPosCallback(GLFWwindow* window, double x, double y)
{
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    const glm::dvec2 position(x, y);

    if (self->m_firstMouse) {
        self->m_lastMousePos = position;
        self->m_firstMouse = false;
        return;
    }
    self->m_mouseDelta  += glm::vec2(position - self->m_lastMousePos);
    self->m_lastMousePos = position;
}

void Window::scrollCallback(GLFWwindow* window, double , double yOffset)
{
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    self->m_scrollDelta += static_cast<float>(yOffset);
}

void Window::keyCallback(GLFWwindow* window, int key, int , int action, int )
{
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (action == GLFW_PRESS && key >= 0 && key <= GLFW_KEY_LAST)
        self->m_keysJustPressed[key] = true;
}

void Window::mouseButtonCallback(GLFWwindow* window, int button, int action, int )
{
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (action == GLFW_PRESS && button >= 0 && button <= GLFW_MOUSE_BUTTON_LAST)
        self->m_mouseJustPressed[button] = true;
}

#pragma once

#include "Font.h"
#include "Shader.h"

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>

namespace UIColors {
const glm::vec4 PANEL       {0.10f, 0.09f, 0.16f, 0.78f};
const glm::vec4 PANEL_EDGE  {0.93f, 0.78f, 0.45f, 0.85f};
const glm::vec4 TEXT        {1.00f, 0.96f, 0.86f, 1.00f};
const glm::vec4 TEXT_DIM    {0.86f, 0.82f, 0.74f, 0.90f};
const glm::vec4 GOLD        {1.00f, 0.84f, 0.46f, 1.00f};
const glm::vec4 SHADOW      {0.00f, 0.00f, 0.00f, 0.55f};
}

enum class TextAlign { Left, Center, Right };

class UIRenderer {
public:
    explicit UIRenderer(const std::string& fontPath);
    ~UIRenderer();

    UIRenderer(const UIRenderer&) = delete;
    UIRenderer& operator=(const UIRenderer&) = delete;

    void begin(int width, int height);
    void end();

    float width() const  { return m_width; }
    float height() const { return m_height; }
    float scale() const  { return m_height / 720.0f; }

    void rect(float x, float y, float w, float h, const glm::vec4& color, float radius = 0.0f);
    void outline(float x, float y, float w, float h, const glm::vec4& color, float radius, float border);
    void panel(float x, float y, float w, float h, float radius = 10.0f);
    void line(const glm::vec2& a, const glm::vec2& b, float thickness, const glm::vec4& color);
    void gem(float cx, float cy, float size, const glm::vec3& color, float alpha = 1.0f);
    void heart(float cx, float cy, float size, const glm::vec4& color);
    void bar(float x, float y, float w, float h, float fraction, const glm::vec4& fill);
    void checkMark(float x, float y, float size, const glm::vec4& color);

    float text(const std::string& utf8, float x, float y, float size, const glm::vec4& color,
               TextAlign align = TextAlign::Left, bool shadow = true);
    float textWidth(const std::string& utf8, float size) const;
    float lineHeight(float size) const { return m_font.lineHeight() * size / Font::BAKE_SIZE; }
    std::vector<std::string> wrap(const std::string& utf8, float size, float maxWidth) const;

    static bool worldToScreen(const glm::vec3& world, const glm::mat4& viewProjection,
                              float screenWidth, float screenHeight, glm::vec2& screen);

private:
    struct Vertex {
        glm::vec2 position;
        glm::vec2 uv;
        glm::vec4 color;
        glm::vec4 params;
    };
    void quad(const glm::vec2 corners[4], const glm::vec2 uvs[4], const glm::vec4& color, const glm::vec4& params);
    void glyphs(const std::u32string& text, float x, float y, float size, const glm::vec4& color);

    Font   m_font;
    Shader m_shader;
    GLuint m_vao = 0, m_vbo = 0;
    std::vector<Vertex> m_vertices;
    float m_width = 1.0f, m_height = 1.0f;
};

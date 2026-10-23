#include "UIRenderer.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cstddef>

namespace {
constexpr float MODE_SHAPE = 0.0f;
constexpr float MODE_TEXT  = 1.0f;
constexpr float MODE_GEM   = 2.0f;
}

UIRenderer::UIRenderer(const std::string& fontPath)
    : m_font(fontPath), m_shader("shaders/ui.vert", "shaders/ui.frag")
{
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    const GLsizei stride = sizeof(Vertex);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, position)));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, uv)));
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, color)));
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, params)));
    for (GLuint i = 0; i < 4; ++i)
        glEnableVertexAttribArray(i);
    glBindVertexArray(0);
}

UIRenderer::~UIRenderer()
{
    glDeleteBuffers(1, &m_vbo);
    glDeleteVertexArrays(1, &m_vao);
}

void UIRenderer::begin(int width, int height)
{
    m_width = static_cast<float>(width);
    m_height = static_cast<float>(height);
    m_vertices.clear();
}

void UIRenderer::end()
{
    if (m_vertices.empty() || m_width < 1.0f || m_height < 1.0f)
        return;

    const glm::mat4 projection = glm::ortho(0.0f, m_width, m_height, 0.0f, -1.0f, 1.0f);

    m_shader.use();
    m_shader.setMat4("uProjection", projection);
    m_shader.setInt("uAtlas", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_font.texture());

    GLint polygonMode[2];
    glGetIntegerv(GL_POLYGON_MODE, polygonMode);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex), m_vertices.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_vertices.size()));

    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, polygonMode[0]);
}

void UIRenderer::quad(const glm::vec2 c[4], const glm::vec2 uv[4], const glm::vec4& color, const glm::vec4& params)
{
    const int order[6] = {0, 1, 2, 0, 2, 3};
    for (int i : order)
        m_vertices.push_back({c[i], uv[i], color, params});
}

void UIRenderer::rect(float x, float y, float w, float h, const glm::vec4& color, float radius)
{
    const float hw = w * 0.5f, hh = h * 0.5f;
    const glm::vec2 corners[4] = {{x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}};
    const glm::vec2 local[4] = {{-hw, -hh}, {hw, -hh}, {hw, hh}, {-hw, hh}};
    quad(corners, local, color, {hw, hh, glm::min(radius, glm::min(hw, hh)), MODE_SHAPE});
}

void UIRenderer::outline(float x, float y, float w, float h, const glm::vec4& color, float radius, float border)
{
    const float hw = w * 0.5f, hh = h * 0.5f;
    const glm::vec2 corners[4] = {{x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}};
    const glm::vec2 local[4] = {{-hw, -hh}, {hw, -hh}, {hw, hh}, {-hw, hh}};
    quad(corners, local, color, {hw, hh, glm::min(radius, glm::min(hw, hh)), 3.0f + border});
}

void UIRenderer::panel(float x, float y, float w, float h, float radius)
{
    rect(x, y, w, h, UIColors::PANEL, radius);
    outline(x, y, w, h, UIColors::PANEL_EDGE, radius, glm::max(1.5f, 1.5f * scale()));
}

void UIRenderer::line(const glm::vec2& a, const glm::vec2& b, float thickness, const glm::vec4& color)
{
    glm::vec2 along = b - a;
    const float length = glm::length(along);
    if (length < 1e-3f)
        return;
    along /= length;
    const glm::vec2 side(-along.y, along.x);
    const float r = thickness * 0.5f;
    const glm::vec2 p0 = a - along * r, p1 = b + along * r;
    const glm::vec2 corners[4] = {p0 - side * r, p1 - side * r, p1 + side * r, p0 + side * r};
    const float hl = length * 0.5f + r;
    const glm::vec2 local[4] = {{-hl, -r}, {hl, -r}, {hl, r}, {-hl, r}};
    quad(corners, local, color, {hl, r, r, MODE_SHAPE});
}

void UIRenderer::gem(float cx, float cy, float size, const glm::vec3& color, float alpha)
{
    const float hw = size * 0.36f, hh = size * 0.5f;
    const glm::vec2 corners[4] = {{cx - hw, cy - hh}, {cx + hw, cy - hh}, {cx + hw, cy + hh}, {cx - hw, cy + hh}};
    const glm::vec2 local[4] = {{-hw, -hh}, {hw, -hh}, {hw, hh}, {-hw, hh}};
    quad(corners, local, glm::vec4(color, alpha), {hw, hh, 0.0f, MODE_GEM});
}

void UIRenderer::heart(float cx, float cy, float size, const glm::vec4& color)
{
    const float h = size * 0.5f;
    const glm::vec2 corners[4] = {{cx - h, cy - h}, {cx + h, cy - h}, {cx + h, cy + h}, {cx - h, cy + h}};
    const glm::vec2 local[4] = {{-h, -h}, {h, -h}, {h, h}, {-h, h}};
    quad(corners, local, color, {h, h, 0.0f, -1.0f});
}

void UIRenderer::bar(float x, float y, float w, float h, float fraction, const glm::vec4& fill)
{
    rect(x, y, w, h, {0.0f, 0.0f, 0.0f, 0.55f}, h * 0.5f);
    const float inset = glm::max(1.0f, h * 0.15f);
    const float inner = (w - 2 * inset) * glm::clamp(fraction, 0.0f, 1.0f);
    if (inner > 0.5f)
        rect(x + inset, y + inset, inner, h - 2 * inset, fill, (h - 2 * inset) * 0.5f);
}

void UIRenderer::checkMark(float x, float y, float size, const glm::vec4& color)
{
    const float t = size * 0.16f;
    line({x + size * 0.15f, y + size * 0.55f}, {x + size * 0.40f, y + size * 0.80f}, t, color);
    line({x + size * 0.40f, y + size * 0.80f}, {x + size * 0.85f, y + size * 0.22f}, t, color);
}

void UIRenderer::glyphs(const std::u32string& text, float x, float y, float size, const glm::vec4& color)
{
    const float s = size / Font::BAKE_SIZE;
    const glm::vec2 origin(x, y + m_font.ascent() * s);
    glm::vec2 pen(0.0f);
    Font::Quad g;
    for (char32_t c : text) {
        if (!m_font.glyph(c, pen, g))
            continue;
        const glm::vec2 a = origin + g.min * s, b = origin + g.max * s;
        const glm::vec2 corners[4] = {{a.x, a.y}, {b.x, a.y}, {b.x, b.y}, {a.x, b.y}};
        const glm::vec2 uvs[4] = {{g.uvMin.x, g.uvMin.y}, {g.uvMax.x, g.uvMin.y},
                                  {g.uvMax.x, g.uvMax.y}, {g.uvMin.x, g.uvMax.y}};
        quad(corners, uvs, color, {0.0f, 0.0f, 0.0f, MODE_TEXT});
    }
}

float UIRenderer::text(const std::string& utf8, float x, float y, float size, const glm::vec4& color,
                       TextAlign align, bool shadow)
{
    const float width = textWidth(utf8, size);
    if (align == TextAlign::Center) x -= width * 0.5f;
    if (align == TextAlign::Right)  x -= width;
    const std::u32string decoded = Font::decode(utf8);
    if (shadow) {
        const float offset = glm::max(1.0f, size * 0.06f);
        glyphs(decoded, x + offset * 0.5f, y + offset, size, {0.0f, 0.0f, 0.0f, 0.5f * color.a});
    }
    glyphs(decoded, x, y, size, color);
    return width;
}

float UIRenderer::textWidth(const std::string& utf8, float size) const
{
    return m_font.measure(utf8) * size / Font::BAKE_SIZE;
}

std::vector<std::string> UIRenderer::wrap(const std::string& utf8, float size, float maxWidth) const
{
    std::vector<std::string> lines;
    std::string current, word;
    auto flushWord = [&]() {
        if (word.empty())
            return;
        const std::string candidate = current.empty() ? word : current + " " + word;
        if (!current.empty() && textWidth(candidate, size) > maxWidth) {
            lines.push_back(current);
            current = word;
        } else {
            current = candidate;
        }
        word.clear();
    };
    for (char c : utf8) {
        if (c == ' ') {
            flushWord();
        } else if (c == '\n') {
            flushWord();
            lines.push_back(current);
            current.clear();
        } else {
            word += c;
        }
    }
    flushWord();
    if (!current.empty())
        lines.push_back(current);
    return lines;
}

bool UIRenderer::worldToScreen(const glm::vec3& world, const glm::mat4& viewProjection,
                               float screenWidth, float screenHeight, glm::vec2& screen)
{
    const glm::vec4 clip = viewProjection * glm::vec4(world, 1.0f);
    if (clip.w <= 0.01f)
        return false;
    const glm::vec2 ndc = glm::vec2(clip) / clip.w;
    screen.x = (ndc.x * 0.5f + 0.5f) * screenWidth;
    screen.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * screenHeight;
    return true;
}

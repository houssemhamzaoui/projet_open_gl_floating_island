#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>

class Font {
public:
    struct Quad {
        glm::vec2 min, max;
        glm::vec2 uvMin, uvMax;
    };

    explicit Font(const std::string& relativePath);
    ~Font();

    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

    static constexpr float BAKE_SIZE = 48.0f;

    GLuint texture() const { return m_texture; }
    float  ascent() const { return m_ascent; }
    float  lineHeight() const { return m_lineHeight; }

    bool glyph(char32_t codepoint, glm::vec2& pen, Quad& quad) const;
    float measure(const std::string& utf8) const;

    static std::u32string decode(const std::string& utf8);

private:
    GLuint m_texture = 0;
    int    m_atlasWidth = 0, m_atlasHeight = 0;
    float  m_ascent = 0.0f, m_lineHeight = 0.0f;
    std::vector<unsigned char> m_packedChars;
};

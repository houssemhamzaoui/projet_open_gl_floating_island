#include "Font.h"
#include "Paths.h"

#include <stb_truetype.h>

#include <fstream>
#include <iterator>
#include <stdexcept>

namespace {

constexpr int FIRST_CHAR = 32;
constexpr int CHAR_COUNT = 224;

const stbtt_packedchar* chars(const std::vector<unsigned char>& storage)
{
    return reinterpret_cast<const stbtt_packedchar*>(storage.data());
}

}

Font::Font(const std::string& relativePath)
{
    std::ifstream file(resourcePath(relativePath), std::ios::binary);
    if (!file)
        throw std::runtime_error("Cannot open font: " + relativePath);
    const std::vector<unsigned char> ttf((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    stbtt_fontinfo info;
    if (!stbtt_InitFont(&info, ttf.data(), stbtt_GetFontOffsetForIndex(ttf.data(), 0)))
        throw std::runtime_error("Invalid font file: " + relativePath);

    int ascent = 0, descent = 0, gap = 0;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &gap);
    const float scale = stbtt_ScaleForPixelHeight(&info, BAKE_SIZE);
    m_ascent = ascent * scale;
    m_lineHeight = (ascent - descent + gap) * scale;

    m_packedChars.resize(sizeof(stbtt_packedchar) * CHAR_COUNT);
    m_atlasWidth = 2048;
    m_atlasHeight = 1024;
    std::vector<unsigned char> pixels;
    bool packed = false;
    while (!packed && m_atlasHeight <= 4096) {
        pixels.assign(static_cast<size_t>(m_atlasWidth) * m_atlasHeight, 0);
        stbtt_pack_context context;
        stbtt_PackBegin(&context, pixels.data(), m_atlasWidth, m_atlasHeight, 0, 2, nullptr);
        stbtt_PackSetOversampling(&context, 2, 2);
        packed = stbtt_PackFontRange(&context, ttf.data(), 0, BAKE_SIZE, FIRST_CHAR, CHAR_COUNT,
                                     reinterpret_cast<stbtt_packedchar*>(m_packedChars.data())) != 0;
        stbtt_PackEnd(&context);
        if (!packed)
            m_atlasHeight *= 2;
    }
    if (!packed)
        throw std::runtime_error("Font atlas too small for: " + relativePath);

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, m_atlasWidth, m_atlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, pixels.data());
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

Font::~Font()
{
    glDeleteTextures(1, &m_texture);
}

bool Font::glyph(char32_t codepoint, glm::vec2& pen, Quad& quad) const
{
    if (codepoint < FIRST_CHAR || codepoint >= static_cast<char32_t>(FIRST_CHAR + CHAR_COUNT))
        codepoint = '?';
    stbtt_aligned_quad q;
    stbtt_GetPackedQuad(chars(m_packedChars), m_atlasWidth, m_atlasHeight,
                        static_cast<int>(codepoint) - FIRST_CHAR, &pen.x, &pen.y, &q, 0);
    quad.min = {q.x0, q.y0};
    quad.max = {q.x1, q.y1};
    quad.uvMin = {q.s0, q.t0};
    quad.uvMax = {q.s1, q.t1};
    return codepoint != ' ';
}

float Font::measure(const std::string& utf8) const
{
    glm::vec2 pen(0.0f);
    Quad quad;
    for (char32_t c : decode(utf8))
        glyph(c, pen, quad);
    return pen.x;
}

std::u32string Font::decode(const std::string& utf8)
{
    std::u32string result;
    for (size_t i = 0; i < utf8.size();) {
        const unsigned char c = static_cast<unsigned char>(utf8[i]);
        int extra = 0;
        char32_t code = c;
        if (c >= 0xF0)      { extra = 3; code = c & 0x07; }
        else if (c >= 0xE0) { extra = 2; code = c & 0x0F; }
        else if (c >= 0xC0) { extra = 1; code = c & 0x1F; }
        ++i;
        for (int k = 0; k < extra && i < utf8.size(); ++k, ++i)
            code = (code << 6) | (static_cast<unsigned char>(utf8[i]) & 0x3F);
        result.push_back(code);
    }
    return result;
}

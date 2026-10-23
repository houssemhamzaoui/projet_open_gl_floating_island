#include "Shader.h"
#include "Paths.h"

#include <glm/gtc/type_ptr.hpp>

#include <filesystem>
#include <fstream>
#include <stdexcept>

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath)
{
    GLuint vertex   = compileStage(GL_VERTEX_SHADER,   loadSource(vertexPath),   vertexPath);
    GLuint fragment = compileStage(GL_FRAGMENT_SHADER, loadSource(fragmentPath), fragmentPath);

    m_program = glCreateProgram();
    glAttachShader(m_program, vertex);
    glAttachShader(m_program, fragment);
    glLinkProgram(m_program);

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint success = 0;
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetProgramInfoLog(m_program, sizeof(log), nullptr, log);
        glDeleteProgram(m_program);
        throw std::runtime_error("Shader link error (" + vertexPath + " + "
                                 + fragmentPath + "):\n" + log);
    }
}

Shader::~Shader()
{
    glDeleteProgram(m_program);
}

void Shader::use() const
{
    glUseProgram(m_program);
}

void Shader::setInt(const std::string& name, int value) const
{
    glUniform1i(uniformLocation(name), value);
}

void Shader::setFloat(const std::string& name, float value) const
{
    glUniform1f(uniformLocation(name), value);
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const
{
    glUniform3fv(uniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setVec4(const std::string& name, const glm::vec4& value) const
{
    glUniform4fv(uniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setMat4(const std::string& name, const glm::mat4& value) const
{
    glUniformMatrix4fv(uniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

std::string Shader::loadSource(const std::string& relativePath, int depth)
{
    if (depth > 8)
        throw std::runtime_error("Shader #include nested too deep: " + relativePath);

    std::ifstream file(resourcePath(relativePath));
    if (!file)
        throw std::runtime_error("Cannot open shader file: " + relativePath);

    const std::filesystem::path folder = std::filesystem::path(relativePath).parent_path();
    std::string source;
    std::string line;
    while (std::getline(file, line)) {
        const size_t start = line.find_first_not_of(" \t");
        if (start != std::string::npos && line.compare(start, 8, "#include") == 0) {
            const size_t open  = line.find('"', start);
            const size_t close = line.find('"', open + 1);
            if (open == std::string::npos || close == std::string::npos)
                throw std::runtime_error("Bad #include line in " + relativePath + ": " + line);

            const std::string name = line.substr(open + 1, close - open - 1);
            source += loadSource((folder / name).generic_string(), depth + 1);
        } else {
            source += line;
        }
        source += '\n';
    }
    return source;
}

GLuint Shader::compileStage(GLenum type, const std::string& source,
                            const std::string& fileName)
{
    GLuint shader = glCreateShader(type);
    const char* code = source.c_str();
    glShaderSource(shader, 1, &code, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        glDeleteShader(shader);
        throw std::runtime_error("Shader compile error in " + fileName + ":\n" + log);
    }
    return shader;
}

GLint Shader::uniformLocation(const std::string& name) const
{
    return glGetUniformLocation(m_program, name.c_str());
}

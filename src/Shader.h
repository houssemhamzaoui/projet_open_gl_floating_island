#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>

class Shader {
public:
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    void use() const;

    void setInt  (const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec3 (const std::string& name, const glm::vec3& value) const;
    void setVec4 (const std::string& name, const glm::vec4& value) const;
    void setMat4 (const std::string& name, const glm::mat4& value) const;

    GLuint getId() const { return m_program; }

private:
    static std::string loadSource(const std::string& relativePath, int depth = 0);
    static GLuint compileStage(GLenum type, const std::string& source,
                               const std::string& fileName);
    GLint uniformLocation(const std::string& name) const;

    GLuint m_program = 0;
};

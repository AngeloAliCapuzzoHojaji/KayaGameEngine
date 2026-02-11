#pragma once

#include <string>
#include <glm/glm.hpp>
#include <unordered_map>

namespace Kaya {

class Shader {
public:
    Shader(const std::string& vertexSrc, const std::string& fragmentSrc);
    ~Shader();

    void Bind() const;
    void Unbind() const;

    void SetInt(const std::string& name, int value);
    void SetFloat(const std::string& name, float value);
    void SetFloat3(const std::string& name, const glm::vec3& value);
    void SetFloat4(const std::string& name, const glm::vec4& value);
    void SetMat3(const std::string& name, const glm::mat3& value);
    void SetMat4(const std::string& name, const glm::mat4& value);

private:
    unsigned int m_RendererID;
    mutable std::unordered_map<std::string, int> m_UniformLocationCache;

    int GetUniformLocation(const std::string& name) const;
};

} // namespace Kaya

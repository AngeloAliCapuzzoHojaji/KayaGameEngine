#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace Kaya {

class Texture;

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct Material {
    glm::vec4 Color = glm::vec4(1.0f);
    std::shared_ptr<Texture> DiffuseMap = nullptr;
    std::shared_ptr<Texture> SpecularMap = nullptr;
    std::shared_ptr<Texture> NormalMap = nullptr;
    float Shininess = 32.0f;
};

class Mesh {
public:
    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, const Material& material);
    ~Mesh();

    void Draw();

    const Material& GetMaterial() const { return m_Material; }
    Material& GetMaterial() { return m_Material; }

private:
    void SetupMesh();

private:
    std::vector<Vertex> m_Vertices;
    std::vector<unsigned int> m_Indices;
    Material m_Material;

    unsigned int m_VAO = 0;
    unsigned int m_VBO = 0;
    unsigned int m_EBO = 0;
};

} // namespace Kaya

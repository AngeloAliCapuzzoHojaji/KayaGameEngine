#pragma once

#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>

namespace Kaya {

class Mesh;
class Texture;

class Model {
public:
    Model(const std::string& path);
    ~Model() = default;

    void Draw();

    const std::vector<std::shared_ptr<Mesh>>& GetMeshes() const { return m_Meshes; }
    const std::string& GetPath() const { return m_Path; }
    const std::string& GetDirectory() const { return m_Directory; }

private:
    void LoadModel(const std::string& path);
    void ProcessNode(void* node, const void* scene);
    std::shared_ptr<Mesh> ProcessMesh(void* mesh, const void* scene);
    std::vector<std::shared_ptr<Texture>> LoadMaterialTextures(void* mat, int type, const std::string& typeName);

private:
    std::vector<std::shared_ptr<Mesh>> m_Meshes;
    std::string m_Path;
    std::string m_Directory;
    std::vector<std::shared_ptr<Texture>> m_LoadedTextures;
};

} // namespace Kaya

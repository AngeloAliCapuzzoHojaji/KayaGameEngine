#pragma once

#include <string>
#include <vector>
#include <memory>

namespace Kaya {

class Cubemap {
public:
    // Load cubemap from 6 individual images
    Cubemap(const std::vector<std::string>& faces);
    
    // Load cubemap from HDR equirectangular map
    Cubemap(const std::string& hdrPath);
    
    ~Cubemap();

    void Bind(unsigned int slot = 0) const;
    void Unbind() const;

    unsigned int GetRendererID() const { return m_RendererID; }

private:
    void LoadFromFaces(const std::vector<std::string>& faces);
    void LoadFromHDR(const std::string& hdrPath);

private:
    unsigned int m_RendererID = 0;
};

class Skybox {
public:
    Skybox(const std::shared_ptr<Cubemap>& cubemap);
    ~Skybox();

    void Draw();

    void SetCubemap(const std::shared_ptr<Cubemap>& cubemap) { m_Cubemap = cubemap; }
    const std::shared_ptr<Cubemap>& GetCubemap() const { return m_Cubemap; }

private:
    void InitCube();

private:
    std::shared_ptr<Cubemap> m_Cubemap;
    unsigned int m_VAO = 0;
    unsigned int m_VBO = 0;
};

} // namespace Kaya

#pragma once

#include <glm/glm.hpp>
#include <memory>

namespace Kaya {

class Texture;

struct PBRMaterial {
    // Base color
    glm::vec3 Albedo = glm::vec3(1.0f);
    std::shared_ptr<Texture> AlbedoMap = nullptr;

    // Material properties
    float Metallic = 0.0f;
    float Roughness = 0.5f;
    float AO = 1.0f; // Ambient Occlusion

    std::shared_ptr<Texture> MetallicMap = nullptr;
    std::shared_ptr<Texture> RoughnessMap = nullptr;
    std::shared_ptr<Texture> AOMap = nullptr;
    std::shared_ptr<Texture> NormalMap = nullptr;

    // Emissive
    glm::vec3 Emissive = glm::vec3(0.0f);
    std::shared_ptr<Texture> EmissiveMap = nullptr;
};

} // namespace Kaya

#pragma once

#include <glm/glm.hpp>
#include <memory>

namespace Kaya {

class Shader;
class Camera;

class Renderer {
public:
    static void Init();
    static void Shutdown();

    static void BeginScene(Camera& camera);
    static void EndScene();

    static void Clear(const glm::vec4& color = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));
    static void SetViewport(unsigned int x, unsigned int y, unsigned int width, unsigned int height);

    // Simple rendering functions
    static void DrawCube(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color);
    static void DrawSphere(const glm::vec3& position, float radius, const glm::vec4& color);

private:
    struct RendererData {
        std::shared_ptr<Shader> BasicShader;
        glm::mat4 ViewProjectionMatrix;
        
        unsigned int CubeVAO = 0;
        unsigned int CubeVBO = 0;
        
        unsigned int SphereVAO = 0;
        unsigned int SphereVBO = 0;
        unsigned int SphereEBO = 0;
        unsigned int SphereIndexCount = 0;
    };

    static RendererData* s_Data;
    
    static void InitCube();
    static void InitSphere();
};

} // namespace Kaya

#pragma once

#include <glm/glm.hpp>
#include <memory>

namespace Kaya {

class Shader;
class Camera;
class Texture;
class Model;
class Mesh;
class DirectionalLight;
class ShadowMap;

class Renderer {
public:
    static void Init();
    static void Shutdown();

    static void BeginScene(Camera& camera);
    static void EndScene();
    
    // Shadow mapping
    static void SetDirectionalLight(DirectionalLight* light);
    static void SetShadowMap(ShadowMap* shadowMap);
    static void BeginShadowPass();
    static void EndShadowPass();

    static void Clear(const glm::vec4& color = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));
    static void SetViewport(unsigned int x, unsigned int y, unsigned int width, unsigned int height);

    // Simple rendering functions
    static void DrawCube(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color);
    static void DrawSphere(const glm::vec3& position, float radius, const glm::vec4& color);
    
    // Textured rendering functions
    static void DrawCube(const glm::vec3& position, const glm::vec3& size, const std::shared_ptr<Texture>& texture);
    static void DrawSphere(const glm::vec3& position, float radius, const std::shared_ptr<Texture>& texture);
    
    // Model rendering
    static void DrawModel(const std::shared_ptr<Model>& model, const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale);
    static void DrawMesh(Mesh* mesh, const glm::mat4& transform);

private:
    struct RendererData {
        std::shared_ptr<Shader> BasicShader;
        std::shared_ptr<Shader> ShadowShader;
        glm::mat4 ViewProjectionMatrix;
        
        DirectionalLight* DirectionalLight = nullptr;
        ShadowMap* ShadowMap = nullptr;
        
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

#pragma once

#include <glm/glm.hpp>
#include "Rendering/Frustum.h"
#include <memory>
#include <vector>

namespace Kaya {

class Shader;
class Camera;
class Texture;
class Model;
class Mesh;
class DirectionalLight;
class ShadowMap;
class Skybox;
class ParticleSystem;

class Renderer {
public:
    static void Init();
    static void Shutdown();

    static void BeginScene(Camera& camera);
    static void EndScene();
    
    // Camera
    static void SetCameraPosition(const glm::vec3& position);
    
    // Shadow mapping
    static void SetDirectionalLight(DirectionalLight* light);
    static void SetShadowMap(ShadowMap* shadowMap);
    static void BeginShadowPass();
    static void EndShadowPass();

    static void Clear(const glm::vec4& color = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));
    static void SetViewport(unsigned int x, unsigned int y, unsigned int width, unsigned int height);

    // Culling control
    static void SetFaceCullingEnabled(bool enabled);
    static void SetFrustumCullingEnabled(bool enabled);
    static bool IsFaceCullingEnabled();
    static bool IsFrustumCullingEnabled();

    // Debug culling visualization (F4)
    static void SetDebugCullingMode(bool enabled);
    static bool IsDebugCullingMode();

    // Simple rendering functions
    static void DrawCube(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color);
    static void DrawSphere(const glm::vec3& position, float radius, const glm::vec4& color);
    
    // Textured rendering functions
    static void DrawCube(const glm::vec3& position, const glm::vec3& size, const std::shared_ptr<Texture>& texture);
    static void DrawSphere(const glm::vec3& position, float radius, const std::shared_ptr<Texture>& texture);
    
    // Model rendering
    static void DrawModel(const std::shared_ptr<Model>& model, const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale);
    static void DrawMesh(Mesh* mesh, const glm::mat4& transform);
    
    // Skybox rendering
    static void DrawSkybox(Skybox* skybox);
    
    // Particle system rendering
    static void DrawParticles(ParticleSystem* particles);

private:
    struct RendererData {
        std::shared_ptr<Shader> BasicShader;
        std::shared_ptr<Shader> ShadowShader;
        std::shared_ptr<Shader> PBRShader;
        std::shared_ptr<Shader> SkyboxShader;
        glm::mat4 ViewProjectionMatrix;
        glm::mat4 ViewMatrix;
        glm::mat4 ProjectionMatrix;
        glm::vec3 CameraPosition;
        
        DirectionalLight* DirectionalLight = nullptr;
        ShadowMap* ShadowMap = nullptr;
        
        // Frustum culling
        Frustum SceneFrustum;
        bool FrustumCullingEnabled = true;
        bool FaceCullingEnabled = true;

        // Depth pre-pass shader
        std::shared_ptr<Shader> DepthOnlyShader;
        bool DepthPrePassEnabled = true;
        
        // Debug culling visualization
        bool DebugCullingMode = false;
        std::shared_ptr<Shader> DebugLineShader;
        std::shared_ptr<Shader> DebugOverlayShader;
        unsigned int DebugLineVAO = 0;
        unsigned int DebugLineVBO = 0;
        std::vector<float> DebugLineVertices;

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
    static void AddDebugAABB(const AABB& box, const glm::vec4& color);
    static void FlushDebugDraw();
};

} // namespace Kaya

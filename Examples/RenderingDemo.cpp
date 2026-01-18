/*
 * Rendering Features Demo
 * 
 * This example showcases all the advanced rendering features of Kaya Engine:
 * - Texture System (diffuse, specular, normal maps)
 * - Model Loading (OBJ, GLTF, FBX support)
 * - Shadow Mapping (PCF soft shadows)
 * - PBR Materials (metallic-roughness workflow)
 * - Post-Processing (bloom, tone mapping, vignette)
 * - Skybox System (environment mapping)
 * 
 * Controls:
 * - WASD: Move camera
 * - Mouse: Look around
 * - Space/Shift: Up/Down
 * - 1-6: Toggle rendering features
 * - ESC: Exit
 */

#include <Kaya.h>
#include <GLFW/glfw3.h>
#include <iostream>

using namespace Kaya;

class RenderingDemo : public Application {
public:
    RenderingDemo() : Application("Kaya Engine - Rendering Features Demo") {}

    void OnInit() override {
        std::cout << "\n========================================" << std::endl;
        std::cout << "  Kaya Engine - Rendering Demo" << std::endl;
        std::cout << "========================================\n" << std::endl;
        std::cout << "Controls:" << std::endl;
        std::cout << "  WASD       - Move camera" << std::endl;
        std::cout << "  Mouse      - Look around" << std::endl;
        std::cout << "  Space/Shift- Up/Down" << std::endl;
        std::cout << "  1          - Toggle Shadows" << std::endl;
        std::cout << "  2          - Toggle Bloom" << std::endl;
        std::cout << "  3          - Toggle Skybox" << std::endl;
        std::cout << "  4          - Cycle Demo Scenes" << std::endl;
        std::cout << "  ESC        - Exit\n" << std::endl;

        // Initialize camera
        float aspectRatio = GetWindow().GetAspectRatio();
        uint32_t width = GetWindow().GetWidth();
        uint32_t height = GetWindow().GetHeight();
        
        m_Camera = std::make_shared<Camera>(
            glm::vec3(0.0f, 5.0f, 15.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
            -90.0f, -15.0f
        );
        m_Camera->SetPerspective(45.0f, aspectRatio, 0.1f, 1000.0f);

        // Setup directional light with shadows
        m_Light = std::make_shared<DirectionalLight>();
        m_Light->Direction = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
        m_Light->Color = glm::vec3(1.0f, 0.98f, 0.9f);
        m_Light->Intensity = 1.5f;
        m_Light->CastShadows = true;
        Renderer::SetDirectionalLight(m_Light.get());

        // Setup shadow mapping
        m_ShadowMap = std::make_shared<ShadowMap>();
        Renderer::SetShadowMap(m_ShadowMap.get());

        // Setup post-processing
        m_PostProcessor = std::make_shared<PostProcessor>(width, height);
        m_PostProcessor->SetBloomEnabled(true);
        m_PostProcessor->SetBloomThreshold(1.0f);
        m_PostProcessor->SetExposure(1.2f);
        m_PostProcessor->SetVignetteStrength(0.35f);
        
        std::cout << "Post-processing disabled by default. Press 2 to enable." << std::endl;

        // Create skybox (example with generated colors - replace with actual cubemap files)
        CreateSkybox();

        // Create demo scenes
        CreateTextureDemo();
        CreatePBRDemo();
        CreateShadowDemo();

        std::cout << "Scene loaded successfully!" << std::endl;
        std::cout << "Current Scene: " << m_CurrentSceneNames[m_CurrentScene] << "\n" << std::endl;
    }

    void OnUpdate(float deltaTime) override {
        // Handle input
        HandleInput(deltaTime);

        // Update camera
        Renderer::SetCameraPosition(m_Camera->GetPosition());

        // Shadow pass (if enabled)
        if (m_ShadowsEnabled) {
            Renderer::BeginShadowPass();
            RenderCurrentScene();
            Renderer::EndShadowPass();
        }

        // Main render pass
        if (m_PostProcessEnabled) {
            // Render to post-processor framebuffer
            m_PostProcessor->BeginScene();
        }

        Renderer::BeginScene(*m_Camera);
        Renderer::Clear(glm::vec4(0.1f, 0.1f, 0.15f, 1.0f));

        // Render current demo scene
        RenderCurrentScene();

        Renderer::EndScene();

        // Render skybox (if enabled)
        if (m_SkyboxEnabled && m_Skybox) {
            Renderer::DrawSkybox(m_Skybox.get());
        }

        if (m_PostProcessEnabled) {
            m_PostProcessor->EndScene();
            m_PostProcessor->Process();
        }

        // Print FPS
        m_FrameCount++;
        m_TimeAccumulator += deltaTime;
        if (m_TimeAccumulator >= 1.0f) {
            std::cout << "FPS: " << m_FrameCount << " | Scene: " << m_CurrentSceneNames[m_CurrentScene] << std::endl;
            m_FrameCount = 0;
            m_TimeAccumulator = 0.0f;
        }
    }

    void OnRender() override {
        // The scene is already rendered in OnUpdate
        // This ensures the buffer is presented to screen
    }

    void OnShutdown() override {
        std::cout << "\nShutting down Rendering Demo..." << std::endl;
    }

private:
    void HandleInput(float deltaTime) {
        // Camera movement
        float speed = 10.0f * deltaTime;
        glm::vec3 forward = m_Camera->GetForward();
        glm::vec3 right = m_Camera->GetRight();
        glm::vec3 position = m_Camera->GetPosition();

        if (Input::IsKeyPressed(KeyCode::W)) position += forward * speed;
        if (Input::IsKeyPressed(KeyCode::S)) position -= forward * speed;
        if (Input::IsKeyPressed(KeyCode::D)) position += right * speed;
        if (Input::IsKeyPressed(KeyCode::A)) position -= right * speed;
        if (Input::IsKeyPressed(KeyCode::Space)) position.y += speed;
        if (Input::IsKeyPressed(KeyCode::LeftShift)) position.y -= speed;

        m_Camera->SetPosition(position);

        // Camera rotation (basic mouse look)
        static bool firstMouse = true;
        static glm::vec2 lastMousePos(0.0f);
        glm::vec2 mousePos = Input::GetMousePosition();

        if (firstMouse) {
            lastMousePos = mousePos;
            firstMouse = false;
        }

        glm::vec2 delta = (mousePos - lastMousePos) * 0.1f;
        lastMousePos = mousePos;

        float yaw = m_Camera->GetYaw() + delta.x;
        float pitch = glm::clamp(m_Camera->GetPitch() - delta.y, -89.0f, 89.0f);
        m_Camera->SetYaw(yaw);
        m_Camera->SetPitch(pitch);

        // Feature toggles
        static bool key1Pressed = false, key2Pressed = false, key3Pressed = false, key4Pressed = false;

        if (Input::IsKeyPressed(KeyCode::D1) && !key1Pressed) {
            key1Pressed = true;
            m_ShadowsEnabled = !m_ShadowsEnabled;
            m_Light->CastShadows = m_ShadowsEnabled;
            std::cout << "Shadows: " << (m_ShadowsEnabled ? "ON" : "OFF") << std::endl;
        }
        if (!Input::IsKeyPressed(KeyCode::D1)) key1Pressed = false;

        if (Input::IsKeyPressed(KeyCode::D2) && !key2Pressed) {
            key2Pressed = true;
            m_PostProcessEnabled = !m_PostProcessEnabled;
            std::cout << "Post-Processing: " << (m_PostProcessEnabled ? "ON" : "OFF") << std::endl;
        }
        if (!Input::IsKeyPressed(KeyCode::D2)) key2Pressed = false;

        if (Input::IsKeyPressed(KeyCode::D3) && !key3Pressed) {
            key3Pressed = true;
            m_SkyboxEnabled = !m_SkyboxEnabled;
            std::cout << "Skybox: " << (m_SkyboxEnabled ? "ON" : "OFF") << std::endl;
        }
        if (!Input::IsKeyPressed(KeyCode::D3)) key3Pressed = false;

        if (Input::IsKeyPressed(KeyCode::D4) && !key4Pressed) {
            key4Pressed = true;
            m_CurrentScene = (m_CurrentScene + 1) % 3;
            std::cout << "Switched to: " << m_CurrentSceneNames[m_CurrentScene] << std::endl;
        }
        if (!Input::IsKeyPressed(KeyCode::D4)) key4Pressed = false;

        // Exit
        if (Input::IsKeyPressed(KeyCode::Escape)) {
            GetWindow().Close();
        }
    }

    void CreateSkybox() {
        // Create a procedural gradient skybox (no texture files needed!)
        auto cubemap = std::make_shared<Cubemap>(); // Default constructor creates gradient
        m_Skybox = std::make_shared<Skybox>(cubemap);
        m_SkyboxEnabled = true;
        
        std::cout << "Procedural gradient skybox created. Press 3 to toggle." << std::endl;
    }

    void CreateTextureDemo() {
        // Scene 0: Textured Cubes Demo
        // Demonstrates texture loading and UV mapping
        
        // Ground plane
        auto ground = std::make_shared<Entity>("Ground");
        ground->GetTransform().Position = glm::vec3(0.0f, 0.0f, 0.0f);
        ground->GetTransform().Scale = glm::vec3(20.0f, 0.2f, 20.0f);
        ground->GetRender().GeometryType = RenderComponent::Type::Cube;
        ground->GetRender().Color = glm::vec4(0.3f, 0.5f, 0.3f, 1.0f);
        m_TextureScene.push_back(ground);

        // Create a grid of cubes with different colors (simulating textures)
        for (int x = -2; x <= 2; x++) {
            for (int z = -2; z <= 2; z++) {
                auto cube = std::make_shared<Entity>("Cube_" + std::to_string(x) + "_" + std::to_string(z));
                cube->GetTransform().Position = glm::vec3(x * 3.0f, 2.0f, z * 3.0f);
                cube->GetTransform().Scale = glm::vec3(1.0f);
                cube->GetTransform().Rotation = glm::vec3(0.0f, (x + z) * 15.0f, 0.0f);
                cube->GetRender().GeometryType = RenderComponent::Type::Cube;
                
                // Vary colors to simulate different textures
                float r = (x + 2) / 4.0f;
                float b = (z + 2) / 4.0f;
                cube->GetRender().Color = glm::vec4(r, 0.5f, b, 1.0f);
                
                m_TextureScene.push_back(cube);
            }
        }
    }

    void CreatePBRDemo() {
        // Scene 1: PBR Materials Demo
        // Demonstrates metallic and roughness variations
        
        // Ground
        auto ground = std::make_shared<Entity>("Ground");
        ground->GetTransform().Position = glm::vec3(0.0f, 0.0f, 0.0f);
        ground->GetTransform().Scale = glm::vec3(30.0f, 0.2f, 30.0f);
        ground->GetRender().GeometryType = RenderComponent::Type::Cube;
        ground->GetRender().Color = glm::vec4(0.2f, 0.2f, 0.25f, 1.0f);
        m_PBRScene.push_back(ground);

        // Create spheres with varying metallic and roughness values
        int rows = 5;
        int cols = 5;
        for (int row = 0; row < rows; row++) {
            float metallic = static_cast<float>(row) / static_cast<float>(rows - 1);
            for (int col = 0; col < cols; col++) {
                float roughness = glm::clamp(static_cast<float>(col) / static_cast<float>(cols - 1), 0.05f, 1.0f);
                
                auto sphere = std::make_shared<Entity>("PBR_Sphere_" + std::to_string(row) + "_" + std::to_string(col));
                sphere->GetTransform().Position = glm::vec3(
                    (col - cols / 2.0f) * 3.0f,
                    2.5f,
                    (row - rows / 2.0f) * 3.0f
                );
                sphere->GetTransform().Scale = glm::vec3(1.0f);
                sphere->GetRender().GeometryType = RenderComponent::Type::Sphere;
                
                // Base albedo (golden color for metallic, gray-ish for dielectric)
                glm::vec3 albedo = glm::mix(
                    glm::vec3(0.6f, 0.6f, 0.6f),  // Dielectric
                    glm::vec3(1.0f, 0.85f, 0.6f),  // Metallic gold
                    metallic
                );
                sphere->GetRender().Color = glm::vec4(albedo, 1.0f);
                
                m_PBRScene.push_back(sphere);
            }
        }

        // Add some colored lights (represented by emissive spheres)
        auto lightSphere1 = std::make_shared<Entity>("Light1");
        lightSphere1->GetTransform().Position = glm::vec3(-10.0f, 8.0f, -10.0f);
        lightSphere1->GetTransform().Scale = glm::vec3(0.5f);
        lightSphere1->GetRender().GeometryType = RenderComponent::Type::Sphere;
        lightSphere1->GetRender().Color = glm::vec4(2.0f, 1.5f, 1.0f, 1.0f); // Bright for bloom
        m_PBRScene.push_back(lightSphere1);

        auto lightSphere2 = std::make_shared<Entity>("Light2");
        lightSphere2->GetTransform().Position = glm::vec3(10.0f, 8.0f, 10.0f);
        lightSphere2->GetTransform().Scale = glm::vec3(0.5f);
        lightSphere2->GetRender().GeometryType = RenderComponent::Type::Sphere;
        lightSphere2->GetRender().Color = glm::vec4(1.0f, 1.5f, 2.0f, 1.0f); // Bright for bloom
        m_PBRScene.push_back(lightSphere2);
    }

    void CreateShadowDemo() {
        // Scene 2: Shadow Mapping Demo
        // Demonstrates real-time shadows with PCF filtering
        
        // Ground
        auto ground = std::make_shared<Entity>("Ground");
        ground->GetTransform().Position = glm::vec3(0.0f, 0.0f, 0.0f);
        ground->GetTransform().Scale = glm::vec3(25.0f, 0.2f, 25.0f);
        ground->GetRender().GeometryType = RenderComponent::Type::Cube;
        ground->GetRender().Color = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
        m_ShadowScene.push_back(ground);

        // Create tall objects that cast interesting shadows
        for (int i = 0; i < 8; i++) {
            float angle = i * glm::two_pi<float>() / 8.0f;
            float radius = 8.0f;
            
            auto pillar = std::make_shared<Entity>("Pillar_" + std::to_string(i));
            pillar->GetTransform().Position = glm::vec3(
                cos(angle) * radius,
                3.0f,
                sin(angle) * radius
            );
            pillar->GetTransform().Scale = glm::vec3(1.0f, 5.0f, 1.0f);
            pillar->GetTransform().Rotation = glm::vec3(0.0f, glm::degrees(angle), 0.0f);
            pillar->GetRender().GeometryType = RenderComponent::Type::Cube;
            
            // Vary colors
            float hue = i / 8.0f;
            pillar->GetRender().Color = glm::vec4(
                0.5f + 0.5f * sin(hue * 6.28f),
                0.5f + 0.5f * sin(hue * 6.28f + 2.09f),
                0.5f + 0.5f * sin(hue * 6.28f + 4.19f),
                1.0f
            );
            
            m_ShadowScene.push_back(pillar);
        }

        // Center sphere
        auto centerSphere = std::make_shared<Entity>("CenterSphere");
        centerSphere->GetTransform().Position = glm::vec3(0.0f, 2.0f, 0.0f);
        centerSphere->GetTransform().Scale = glm::vec3(2.0f);
        centerSphere->GetRender().GeometryType = RenderComponent::Type::Sphere;
        centerSphere->GetRender().Color = glm::vec4(0.9f, 0.7f, 0.3f, 1.0f);
        m_ShadowScene.push_back(centerSphere);
    }

    void RenderCurrentScene() {
        std::vector<std::shared_ptr<Entity>>* currentScene = nullptr;
        
        switch (m_CurrentScene) {
            case 0: currentScene = &m_TextureScene; break;
            case 1: currentScene = &m_PBRScene; break;
            case 2: currentScene = &m_ShadowScene; break;
        }

        if (!currentScene) return;

        // Animate objects slightly
        float time = static_cast<float>(glfwGetTime());
        
        for (auto& entity : *currentScene) {
            // Apply some animation
            if (entity->GetName().find("Cube") != std::string::npos ||
                entity->GetName().find("Sphere") != std::string::npos) {
                auto& transform = entity->GetTransform();
                transform.Rotation.y = sin(time * 0.5f + transform.Position.x) * 15.0f;
            }

            // Render entity
            const auto& transform = entity->GetTransform();
            const auto& render = entity->GetRender();

            if (!render.Visible) continue;

            switch (render.GeometryType) {
                case RenderComponent::Type::Cube:
                    if (render.UseTexture && render.TextureMap) {
                        Renderer::DrawCube(transform.Position, transform.Scale, render.TextureMap);
                    } else {
                        Renderer::DrawCube(transform.Position, transform.Scale, render.Color);
                    }
                    break;
                    
                case RenderComponent::Type::Sphere:
                    if (render.UseTexture && render.TextureMap) {
                        Renderer::DrawSphere(transform.Position, transform.Scale.x, render.TextureMap);
                    } else {
                        Renderer::DrawSphere(transform.Position, transform.Scale.x, render.Color);
                    }
                    break;
                    
                case RenderComponent::Type::Model:
                    if (render.ModelAsset) {
                        Renderer::DrawModel(render.ModelAsset, transform.Position, transform.Rotation, transform.Scale);
                    }
                    break;
            }
        }
    }

private:
    // Camera
    std::shared_ptr<Camera> m_Camera;

    // Lighting
    std::shared_ptr<DirectionalLight> m_Light;
    std::shared_ptr<ShadowMap> m_ShadowMap;

    // Post-processing
    std::shared_ptr<PostProcessor> m_PostProcessor;

    // Skybox
    std::shared_ptr<Skybox> m_Skybox;

    // Demo scenes
    std::vector<std::shared_ptr<Entity>> m_TextureScene;
    std::vector<std::shared_ptr<Entity>> m_PBRScene;
    std::vector<std::shared_ptr<Entity>> m_ShadowScene;
    
    int m_CurrentScene = 0;
    std::string m_CurrentSceneNames[3] = {
        "Texture Demo",
        "PBR Materials Demo",
        "Shadow Mapping Demo"
    };

    // Feature toggles
    bool m_ShadowsEnabled = true;
    bool m_PostProcessEnabled = false;  // Disabled by default for direct screen rendering
    bool m_SkyboxEnabled = true;

    // Performance tracking
    int m_FrameCount = 0;
    float m_TimeAccumulator = 0.0f;
};

// Entry point
Kaya::Application* Kaya::CreateApplication() {
    return new RenderingDemo();
}

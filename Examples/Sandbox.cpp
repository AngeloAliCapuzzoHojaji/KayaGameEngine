#include <Kaya.h>
#include <iostream>
#include <vector>

class SandboxApp : public Kaya::Application {
public:
    SandboxApp() : Application("Kaya Sandbox") {}

    void OnInit() override {
        // Initialize camera
        m_Camera = std::make_unique<Kaya::Camera>(
            45.0f,
            GetWindow().GetAspectRatio(),
            0.1f,
            1000.0f
        );
        m_Camera->SetPosition(glm::vec3(0.0f, 5.0f, 15.0f));
        m_Camera->SetRotation(glm::vec3(-20.0f, 0.0f, 0.0f));

        // Initialize physics
        m_PhysicsSystem = std::make_unique<Kaya::PhysicsSystem>();
        m_PhysicsSystem->Initialize();

        // Create ground (static box)
        m_GroundBody = m_PhysicsSystem->CreateBox(
            glm::vec3(0.0f, -1.0f, 0.0f),
            glm::vec3(20.0f, 2.0f, 20.0f),
            false // static
        );

        // Create dynamic boxes
        for (int i = 0; i < 5; ++i) {
            JPH::BodyID box = m_PhysicsSystem->CreateBox(
                glm::vec3(i * 2.0f - 4.0f, 10.0f + i * 3.0f, 0.0f),
                glm::vec3(1.0f, 1.0f, 1.0f),
                true // dynamic
            );
            m_DynamicBodies.push_back(box);
        }

        // Create dynamic spheres
        for (int i = 0; i < 3; ++i) {
            JPH::BodyID sphere = m_PhysicsSystem->CreateSphere(
                glm::vec3(i * 3.0f - 3.0f, 20.0f, 3.0f),
                0.5f,
                true // dynamic
            );
            m_DynamicBodies.push_back(sphere);
        }

        std::cout << "Sandbox initialized with " << m_DynamicBodies.size() << " dynamic bodies" << std::endl;
    }

    void OnUpdate(float deltaTime) override {
        // Update physics
        m_PhysicsSystem->Update(deltaTime);

        // Simple camera controls
        float cameraSpeed = 5.0f * deltaTime;
        glm::vec3 position = m_Camera->GetPosition();

        if (Kaya::Input::IsKeyPressed(Kaya::KeyCode::W))
            position += m_Camera->GetForward() * cameraSpeed;
        if (Kaya::Input::IsKeyPressed(Kaya::KeyCode::S))
            position -= m_Camera->GetForward() * cameraSpeed;
        if (Kaya::Input::IsKeyPressed(Kaya::KeyCode::A))
            position -= m_Camera->GetRight() * cameraSpeed;
        if (Kaya::Input::IsKeyPressed(Kaya::KeyCode::D))
            position += m_Camera->GetRight() * cameraSpeed;
        if (Kaya::Input::IsKeyPressed(Kaya::KeyCode::Space))
            position.y += cameraSpeed;
        if (Kaya::Input::IsKeyPressed(Kaya::KeyCode::LeftShift))
            position.y -= cameraSpeed;

        m_Camera->SetPosition(position);

        // Spawn new box on mouse click
        static bool wasPressed = false;
        bool isPressed = Kaya::Input::IsMouseButtonPressed(Kaya::MouseButton::Left);
        if (isPressed && !wasPressed) {
            glm::vec3 spawnPos = m_Camera->GetPosition() + m_Camera->GetForward() * 5.0f;
            JPH::BodyID newBox = m_PhysicsSystem->CreateBox(spawnPos, glm::vec3(1.0f), true);
            m_DynamicBodies.push_back(newBox);
            
            // Add impulse forward
            m_PhysicsSystem->SetBodyVelocity(newBox, m_Camera->GetForward() * 10.0f);
        }
        wasPressed = isPressed;
    }

    void OnRender() override {
        Kaya::Renderer::BeginScene(*m_Camera);

        // Draw ground
        glm::vec3 groundPos = m_PhysicsSystem->GetBodyPosition(m_GroundBody);
        Kaya::Renderer::DrawCube(groundPos, glm::vec3(20.0f, 2.0f, 20.0f), glm::vec4(0.3f, 0.5f, 0.3f, 1.0f));

        // Draw dynamic bodies
        for (size_t i = 0; i < m_DynamicBodies.size(); ++i) {
            glm::vec3 pos = m_PhysicsSystem->GetBodyPosition(m_DynamicBodies[i]);
            
            // Alternate between cubes and spheres for visual distinction
            if (i < 5) {
                // Boxes
                Kaya::Renderer::DrawCube(pos, glm::vec3(1.0f), glm::vec4(0.8f, 0.3f, 0.2f, 1.0f));
            } else {
                // Spheres
                Kaya::Renderer::DrawSphere(pos, 0.5f, glm::vec4(0.2f, 0.3f, 0.8f, 1.0f));
            }
        }

        Kaya::Renderer::EndScene();
    }

    void OnShutdown() override {
        // Clean up physics
        for (auto bodyID : m_DynamicBodies) {
            m_PhysicsSystem->RemoveBody(bodyID);
        }
        m_PhysicsSystem->RemoveBody(m_GroundBody);
        
        m_PhysicsSystem->Shutdown();
    }

private:
    std::unique_ptr<Kaya::Camera> m_Camera;
    std::unique_ptr<Kaya::PhysicsSystem> m_PhysicsSystem;
    
    JPH::BodyID m_GroundBody;
    std::vector<JPH::BodyID> m_DynamicBodies;
};

Kaya::Application* Kaya::CreateApplication() {
    return new SandboxApp();
}

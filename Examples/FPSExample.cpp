#include <Kaya.h>
#include <GLFW/glfw3.h>
#include <iostream>

class FPSExample : public Kaya::Application {
public:
    FPSExample() : Application("FPS Example") {}

    void OnInit() override {
        // Initialize FPS camera at player height
        m_Camera = std::make_unique<Kaya::Camera>(75.0f, GetWindow().GetAspectRatio());
        m_Camera->SetPosition(glm::vec3(0.0f, 1.7f, 5.0f)); // Eye level height
        
        // Initialize physics
        m_PhysicsSystem = std::make_unique<Kaya::PhysicsSystem>();
        m_PhysicsSystem->Initialize();

        // Create ground plane
        JPH::BodyID ground = m_PhysicsSystem->CreateBox(
            glm::vec3(0.0f, -0.5f, 0.0f),
            glm::vec3(50.0f, 1.0f, 50.0f),
            false
        );

        // Create some walls/obstacles to navigate around
        // Wall 1
        m_PhysicsSystem->CreateBox(
            glm::vec3(-10.0f, 2.0f, 0.0f),
            glm::vec3(1.0f, 4.0f, 10.0f),
            false
        );

        // Wall 2
        m_PhysicsSystem->CreateBox(
            glm::vec3(10.0f, 2.0f, 0.0f),
            glm::vec3(1.0f, 4.0f, 10.0f),
            false
        );

        // Some boxes to see
        for (int i = 0; i < 5; ++i) {
            for (int j = 0; j < 5; ++j) {
                if ((i + j) % 2 == 0) {
                    m_PhysicsSystem->CreateBox(
                        glm::vec3(i * 4.0f - 8.0f, 0.5f, j * 4.0f - 8.0f),
                        glm::vec3(0.5f, 1.0f, 0.5f),
                        false
                    );
                }
            }
        }

        // Hide mouse cursor for FPS feel
        glfwSetInputMode(static_cast<GLFWwindow*>(GetWindow().GetNativeWindow()), 
                        GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        
        // Center mouse
        int width, height;
        glfwGetWindowSize(static_cast<GLFWwindow*>(GetWindow().GetNativeWindow()), 
                         &width, &height);
        m_LastMouseX = width / 2.0f;
        m_LastMouseY = height / 2.0f;
        glfwSetCursorPos(static_cast<GLFWwindow*>(GetWindow().GetNativeWindow()), 
                        m_LastMouseX, m_LastMouseY);
    }

    void OnUpdate(float deltaTime) override {
        // Update physics
        m_PhysicsSystem->Update(deltaTime);

        // FPS Camera Controls
        UpdateFPSCamera(deltaTime);
    }

    void OnRender() override {
        // Clear screen
        Kaya::Renderer::Clear(glm::vec4(0.53f, 0.81f, 0.92f, 1.0f)); // Sky blue

        // Begin scene
        Kaya::Renderer::BeginScene(*m_Camera);

        // Render ground
        Kaya::Renderer::DrawCube(
            glm::vec3(0.0f, -0.5f, 0.0f),
            glm::vec3(50.0f, 1.0f, 50.0f),
            glm::vec4(0.3f, 0.6f, 0.3f, 1.0f)
        );

        // Render walls
        Kaya::Renderer::DrawCube(
            glm::vec3(-10.0f, 2.0f, 0.0f),
            glm::vec3(1.0f, 4.0f, 10.0f),
            glm::vec4(0.5f, 0.5f, 0.5f, 1.0f)
        );

        Kaya::Renderer::DrawCube(
            glm::vec3(10.0f, 2.0f, 0.0f),
            glm::vec3(1.0f, 4.0f, 10.0f),
            glm::vec4(0.5f, 0.5f, 0.5f, 1.0f)
        );

        // Render obstacle boxes
        for (int i = 0; i < 5; ++i) {
            for (int j = 0; j < 5; ++j) {
                if ((i + j) % 2 == 0) {
                    Kaya::Renderer::DrawCube(
                        glm::vec3(i * 4.0f - 8.0f, 0.5f, j * 4.0f - 8.0f),
                        glm::vec3(0.5f, 1.0f, 0.5f),
                        glm::vec4(0.8f, 0.4f, 0.2f, 1.0f)
                    );
                }
            }
        }

        Kaya::Renderer::EndScene();
    }

    void OnShutdown() override {
        // Show cursor again
        glfwSetInputMode(static_cast<GLFWwindow*>(GetWindow().GetNativeWindow()), 
                        GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        
        m_PhysicsSystem->Shutdown();
    }

private:
    void UpdateFPSCamera(float deltaTime) {
        // Mouse look
        glm::vec2 mousePos = Kaya::Input::GetMousePosition();
        float deltaX = mousePos.x - m_LastMouseX;
        float deltaY = mousePos.y - m_LastMouseY;
        
        // Only update camera if mouse actually moved
        if (deltaX != 0.0f || deltaY != 0.0f) {
            // Update camera rotation based on mouse movement
            float sensitivity = 0.1f;
            float yaw = m_Camera->GetYaw() + deltaX * sensitivity;
            float pitch = m_Camera->GetPitch() + deltaY * sensitivity; // Inverted

            // Clamp pitch to prevent camera flipping
            if (pitch > 89.0f) pitch = 89.0f;
            if (pitch < -89.0f) pitch = -89.0f;

            m_Camera->SetYaw(yaw);
            m_Camera->SetPitch(pitch);
        }
        
        m_LastMouseX = mousePos.x;
        m_LastMouseY = mousePos.y;

        // WASD movement
        glm::vec3 forward = m_Camera->GetForward();
        glm::vec3 right = m_Camera->GetRight();
        
        // Flatten forward and right vectors for ground-based movement
        forward.y = 0.0f;
        right.y = 0.0f;
        if (glm::length(forward) > 0.0f) forward = glm::normalize(forward);
        if (glm::length(right) > 0.0f) right = glm::normalize(right);

        glm::vec3 movement(0.0f);
        float moveSpeed = m_MoveSpeed;

        // Sprint with Shift
        if (Kaya::Input::IsKeyPressed(Kaya::KeyCode::LeftShift)) {
            moveSpeed *= 2.0f;
        }

        if (Kaya::Input::IsKeyPressed(Kaya::KeyCode::W))
            movement += forward;
        if (Kaya::Input::IsKeyPressed(Kaya::KeyCode::S))
            movement -= forward;
        if (Kaya::Input::IsKeyPressed(Kaya::KeyCode::D))
            movement += right;
        if (Kaya::Input::IsKeyPressed(Kaya::KeyCode::A))
            movement -= right;

        // Vertical movement (jump/crouch)
        if (Kaya::Input::IsKeyPressed(Kaya::KeyCode::Space))
            movement.y += 1.0f;
        if (Kaya::Input::IsKeyPressed(Kaya::KeyCode::LeftControl))
            movement.y -= 1.0f;

        if (glm::length(movement) > 0.0f) {
            movement = glm::normalize(movement);
            m_Camera->SetPosition(m_Camera->GetPosition() + movement * moveSpeed * deltaTime);
        }

        // Exit on ESC
        if (Kaya::Input::IsKeyPressed(Kaya::KeyCode::Escape)) {
            GetWindow().Close();
        }
    }

    std::unique_ptr<Kaya::Camera> m_Camera;
    std::unique_ptr<Kaya::PhysicsSystem> m_PhysicsSystem;
    
    float m_MoveSpeed = 5.0f;
    float m_LastMouseX = 0.0f;
    float m_LastMouseY = 0.0f;
};

Kaya::Application* Kaya::CreateApplication() {
    return new FPSExample();
}

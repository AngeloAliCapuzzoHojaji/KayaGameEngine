// Core
#include "Core/Application.h"
#include "Core/Window.h"
#include "Core/HotReload.h"

// Rendering
#include "Rendering/Renderer.h"
#include "Rendering/Camera.h"

// Physics
#include "Physics/PhysicsSystem.h"

// Input
#include "Input/Input.h"

// Math
#include <glm/glm.hpp>

#include <GLFW/glfw3.h>
#include <iostream>

class FPSGame : public Kaya::IGameModule {
public:
    void OnLoad(Kaya::GameState* state) override {
        std::cout << "FPS Game: OnLoad" << std::endl;
        
        // Initialize camera if first load
        if (!state->Camera) {
            state->Camera = std::make_shared<Kaya::Camera>(75.0f, 1280.0f / 720.0f);
            state->Camera->SetPosition(glm::vec3(0.0f, 1.7f, 5.0f));
        }
        
        // Initialize physics if first load
        if (!state->Physics) {
            state->Physics = std::make_shared<Kaya::PhysicsSystem>();
            state->Physics->Initialize();
            
            // Create ground plane
            state->Physics->CreateBox(
                glm::vec3(0.0f, -0.5f, 0.0f),
                glm::vec3(50.0f, 1.0f, 50.0f),
                false
            );
            
            // Create walls
            state->Physics->CreateBox(
                glm::vec3(-10.0f, 2.0f, 0.0f),
                glm::vec3(1.0f, 4.0f, 10.0f),
                false
            );
            
            state->Physics->CreateBox(
                glm::vec3(10.0f, 2.0f, 0.0f),
                glm::vec3(1.0f, 4.0f, 10.0f),
                false
            );
            
            // Some boxes
            for (int i = 0; i < 5; ++i) {
                for (int j = 0; j < 5; ++j) {
                    if ((i + j) % 2 == 0) {
                        state->Physics->CreateBox(
                            glm::vec3(i * 4.0f - 8.0f, 0.5f, j * 4.0f - 8.0f),
                            glm::vec3(0.5f, 1.0f, 0.5f),
                            false
                        );
                    }
                }
            }
        }
        
        // Hide and capture cursor
        if (state->Window) {
            glfwSetInputMode(
                static_cast<GLFWwindow*>(state->Window->GetNativeWindow()),
                GLFW_CURSOR,
                GLFW_CURSOR_DISABLED
            );
        }
        
        // Initialize mouse position
        m_LastMousePos = Kaya::Input::GetMousePosition();
    }
    
    void OnUpdate(Kaya::GameState* state) override {
        // Update physics
        state->Physics->Update(state->DeltaTime);
        
        // FPS camera controls
        UpdateFPSCamera(state);
    }
    
    void OnRender(Kaya::GameState* state) override {
        // Clear
        Kaya::Renderer::Clear(glm::vec4(0.53f, 0.81f, 0.92f, 1.0f));
        
        // Begin scene
        Kaya::Renderer::BeginScene(*state->Camera);
        
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
        
        // Render obstacle boxes - CHANGED COLOR FOR TESTING HOT RELOAD
        for (int i = 0; i < 5; ++i) {
            for (int j = 0; j < 5; ++j) {
                if ((i + j) % 2 == 0) {
                    Kaya::Renderer::DrawCube(
                        glm::vec3(i * 4.0f - 8.0f, 0.5f, j * 4.0f - 8.0f),
                        glm::vec3(0.5f, 1.0f, 0.5f),
                        glm::vec4(1.0f, 0.0f, 1.0f, 1.0f) // Magenta - HOT RELOADED!
                    );
                }
            }
        }
        
        Kaya::Renderer::EndScene();
    }
    
    void OnUnload(Kaya::GameState* state) override {
        std::cout << "FPS Game: OnUnload" << std::endl;
        // State is preserved, so we don't clean up
    }
    
    void OnReload(Kaya::GameState* state) override {
        std::cout << "FPS Game: OnReload - New code loaded!" << std::endl;
        m_LastMousePos = Kaya::Input::GetMousePosition();
    }
    
private:
    void UpdateFPSCamera(Kaya::GameState* state) {
        // Mouse look
        glm::vec2 mousePos = Kaya::Input::GetMousePosition();
        float deltaX = mousePos.x - m_LastMousePos.x;
        float deltaY = mousePos.y - m_LastMousePos.y;
        
        if (deltaX != 0.0f || deltaY != 0.0f) {
            float sensitivity = 0.1f;
            float yaw = state->Camera->GetYaw() + deltaX * sensitivity;
            float pitch = state->Camera->GetPitch() + deltaY * sensitivity; // Inverted
            
            // Clamp pitch
            if (pitch > 89.0f) pitch = 89.0f;
            if (pitch < -89.0f) pitch = -89.0f;
            
            state->Camera->SetYaw(yaw);
            state->Camera->SetPitch(pitch);
        }
        
        m_LastMousePos = mousePos;
        
        // WASD movement
        glm::vec3 forward = state->Camera->GetForward();
        glm::vec3 right = state->Camera->GetRight();
        
        forward.y = 0.0f;
        right.y = 0.0f;
        if (glm::length(forward) > 0.0f) forward = glm::normalize(forward);
        if (glm::length(right) > 0.0f) right = glm::normalize(right);
        
        glm::vec3 movement(0.0f);
        float moveSpeed = m_MoveSpeed;
        
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
        
        if (Kaya::Input::IsKeyPressed(Kaya::KeyCode::Space))
            movement.y += 1.0f;
        if (Kaya::Input::IsKeyPressed(Kaya::KeyCode::LeftControl))
            movement.y -= 1.0f;
        
        if (glm::length(movement) > 0.0f) {
            movement = glm::normalize(movement);
            state->Camera->SetPosition(
                state->Camera->GetPosition() + movement * moveSpeed * state->DeltaTime
            );
        }
    }
    
    float m_MoveSpeed = 5.0f;
    glm::vec2 m_LastMousePos = glm::vec2(0.0f);
};

// Export the game module
KAYA_GAME_MODULE(FPSGame)

#include "Kaya.h"
#include <GLFW/glfw3.h>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

struct Bullet {
    JPH::BodyID bodyId;
    float lifetime;
};

struct Enemy {
    JPH::BodyID bodyId;
    int health = 3;  // Enemies take 3 hits to destroy
    glm::vec3 color;
    glm::vec3 targetPos;  // For smooth movement
};

struct Debris {
    JPH::BodyID bodyId;
    float lifetime;
    glm::vec3 color;
};

class FPSExample : public Kaya::Application {
public:
    FPSExample() : Application("FPS Example") {}

    void OnInit() override {
        // Seed random number generator for debris
        srand(static_cast<unsigned>(time(nullptr)));

        // Initialize ImGui for HUD
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        // Disable ImGui mouse/keyboard capture so it doesn't interfere with game input
        io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
        io.WantCaptureKeyboard = false;
        io.WantCaptureMouse = false;
        ImGui::StyleColorsDark();
        
        GLFWwindow* window = GetWindow().GetNativeWindow();
        // Pass false to not install GLFW callbacks (we handle input ourselves)
        ImGui_ImplGlfw_InitForOpenGL(window, false);
        ImGui_ImplOpenGL3_Init("#version 460");

        // Initialize systems
        m_Camera = std::make_unique<Kaya::Camera>(45.0f, 1280.0f / 720.0f, 0.1f, 1000.0f);
        m_Camera->SetPosition(glm::vec3(0.0f, 2.0f, 5.0f));

        m_PhysicsSystem = std::make_unique<Kaya::PhysicsSystem>();
        m_PhysicsSystem->Initialize();

        // Disable cursor for FPS controls
        glfwSetInputMode(GetWindow().GetNativeWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        
        // Initialize mouse position
        double mouseX, mouseY;
        glfwGetCursorPos(GetWindow().GetNativeWindow(), &mouseX, &mouseY);
        m_LastMouseX = static_cast<float>(mouseX);
        m_LastMouseY = static_cast<float>(mouseY);

        // Create physics-based player (sphere collider)
        m_PlayerBodyId = m_PhysicsSystem->CreateSphere(
            glm::vec3(0.0f, 2.0f, 5.0f),
            0.5f,  // radius
            true   // is dynamic
        );

        // Create ground
        m_GroundBodyId = m_PhysicsSystem->CreateBox(
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(50.0f, 0.5f, 50.0f),
            false  // not dynamic
        );

        // Create some obstacles
        m_PhysicsSystem->CreateBox(
            glm::vec3(5.0f, 1.0f, 0.0f),
            glm::vec3(1.0f, 2.0f, 1.0f),
            false
        );

        m_PhysicsSystem->CreateBox(
            glm::vec3(-5.0f, 1.0f, -5.0f),
            glm::vec3(1.0f, 2.0f, 1.0f),
            false
        );

        // Spawn enemies around the map
        SpawnEnemy(glm::vec3(10.0f, 1.5f, 10.0f), glm::vec3(1.0f, 0.2f, 0.2f));
        SpawnEnemy(glm::vec3(-10.0f, 1.5f, 8.0f), glm::vec3(0.2f, 1.0f, 0.2f));
        SpawnEnemy(glm::vec3(8.0f, 1.5f, -12.0f), glm::vec3(0.2f, 0.2f, 1.0f));
        SpawnEnemy(glm::vec3(-7.0f, 1.5f, -10.0f), glm::vec3(1.0f, 0.5f, 0.0f));
        SpawnEnemy(glm::vec3(15.0f, 1.5f, 0.0f), glm::vec3(1.0f, 0.0f, 1.0f));
        SpawnEnemy(glm::vec3(0.0f, 1.5f, 15.0f), glm::vec3(0.0f, 1.0f, 1.0f));
        
        // Initialize gun position
        m_GunPosition = m_Camera->GetPosition() + m_Camera->GetForward() * 0.6f + m_Camera->GetRight() * 0.3f - m_Camera->GetUp() * 0.3f;
    }

    void OnUpdate(float deltaTime) override {
        // Update camera aspect ratio if window resized
        float currentAspect = GetWindow().GetAspectRatio();
        if (std::abs(currentAspect - m_LastAspectRatio) > 0.01f) {
            m_Camera->SetAspectRatio(currentAspect);
            m_LastAspectRatio = currentAspect;
        }

        // Update physics
        m_PhysicsSystem->Update(deltaTime);

        // Handle input
        HandleInput(deltaTime);

        // Get player position from physics
        glm::vec3 playerPos = m_PhysicsSystem->GetBodyPosition(m_PlayerBodyId);
        
        // Update camera to follow player (position above the sphere so it doesn't block view)
        m_Camera->SetPosition(playerPos + glm::vec3(0.0f, 0.6f, 0.0f));
        
        // Update gun position with smooth interpolation for viewmodel sway
        glm::vec3 targetGunPos = m_Camera->GetPosition() 
            + m_Camera->GetForward() * 0.6f    // Forward from camera
            + m_Camera->GetRight() * 0.3f      // Right offset
            - m_Camera->GetUp() * 0.3f;        // Down offset
        m_GunPosition = glm::mix(m_GunPosition, targetGunPos, deltaTime * m_GunSmoothSpeed);

        // Update bullets
        for (auto it = m_Bullets.begin(); it != m_Bullets.end(); ) {
            it->lifetime -= deltaTime;
            if (it->lifetime <= 0.0f) {
                m_PhysicsSystem->RemoveBody(it->bodyId);
                it = m_Bullets.erase(it);
            } else {
                ++it;
            }
        }

        // Check bullet-enemy collisions
        CheckBulletEnemyCollisions();

        // Update enemy movement - crawl towards player
        UpdateEnemies(deltaTime);

        // Check enemy-player collisions (damage player)
        CheckEnemyPlayerCollisions();

        // Update reload timer
        if (m_IsReloading) {
            m_ReloadTimer -= deltaTime;
            if (m_ReloadTimer <= 0.0f) {
                // Reload complete
                int ammoNeeded = m_MaxAmmo - m_Ammo;
                int ammoToReload = std::min(ammoNeeded, m_ReserveAmmo);
                m_Ammo += ammoToReload;
                m_ReserveAmmo -= ammoToReload;
                m_IsReloading = false;
            }
        }

        // Update debris lifetime
        for (auto it = m_Debris.begin(); it != m_Debris.end(); ) {
            it->lifetime -= deltaTime;
            if (it->lifetime <= 0.0f) {
                m_PhysicsSystem->RemoveBody(it->bodyId);
                it = m_Debris.erase(it);
            } else {
                ++it;
            }
        }
    }

    void OnRender() override {
        // Clear screen
        Kaya::Renderer::Clear(glm::vec4(0.2f, 0.3f, 0.3f, 1.0f));

        // Begin scene with camera
        Kaya::Renderer::BeginScene(*m_Camera);

        // Render ground
        glm::vec3 groundPos = m_PhysicsSystem->GetBodyPosition(m_GroundBodyId);
        Kaya::Renderer::DrawCube(groundPos, glm::vec3(50.0f, 0.5f, 50.0f), glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));

        // Render obstacles
        Kaya::Renderer::DrawCube(glm::vec3(5.0f, 1.0f, 0.0f), glm::vec3(1.0f, 2.0f, 1.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
        Kaya::Renderer::DrawCube(glm::vec3(-5.0f, 1.0f, -5.0f), glm::vec3(1.0f, 2.0f, 1.0f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));

        // Don't render player body - we're in first person!

        // Render gun using smoothed position (calculated in OnUpdate)
        glm::vec3 forward = m_Camera->GetForward();
        glm::vec3 right = m_Camera->GetRight();
        glm::vec3 up = m_Camera->GetUp();
        
        // Gun barrel - multiple segments along forward direction to simulate rotation
        for (int i = 0; i < 8; i++) {
            glm::vec3 segmentPos = m_GunPosition + forward * (0.1f + i * 0.06f);
            Kaya::Renderer::DrawSphere(segmentPos, 0.025f, glm::vec4(0.15f, 0.15f, 0.15f, 1.0f));
        }
        
        // Muzzle at end of barrel
        glm::vec3 muzzlePos = m_GunPosition + forward * 0.6f;
        Kaya::Renderer::DrawSphere(muzzlePos, 0.03f, glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));
        
        // Gun body/receiver (main box)
        Kaya::Renderer::DrawCube(m_GunPosition, glm::vec3(0.08f, 0.08f, 0.15f), glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
        
        // Gun grip (angled down)
        glm::vec3 gripPos = m_GunPosition - forward * 0.05f - up * 0.12f;
        Kaya::Renderer::DrawCube(gripPos, glm::vec3(0.06f, 0.15f, 0.08f), glm::vec4(0.18f, 0.18f, 0.18f, 1.0f));
        
        // Trigger guard area
        glm::vec3 triggerPos = m_GunPosition - up * 0.05f;
        Kaya::Renderer::DrawCube(triggerPos, glm::vec3(0.04f, 0.03f, 0.06f), glm::vec4(0.25f, 0.25f, 0.25f, 1.0f));

        // Render bullets
        for (const auto& bullet : m_Bullets) {
            glm::vec3 bulletPos = m_PhysicsSystem->GetBodyPosition(bullet.bodyId);
            Kaya::Renderer::DrawSphere(bulletPos, 0.1f, glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
        }

        // Render enemies
        for (const auto& enemy : m_Enemies) {
            glm::vec3 enemyPos = m_PhysicsSystem->GetBodyPosition(enemy.bodyId);
            // Main body
            Kaya::Renderer::DrawCube(enemyPos, glm::vec3(1.0f, 2.0f, 1.0f), glm::vec4(enemy.color, 1.0f));
            // Head
            Kaya::Renderer::DrawSphere(enemyPos + glm::vec3(0.0f, 1.3f, 0.0f), 0.4f, glm::vec4(enemy.color * 0.8f, 1.0f));
            // Eyes
            glm::vec3 eyeOffset = glm::normalize(m_Camera->GetPosition() - enemyPos);
            eyeOffset.y = 0.0f;
            if (glm::length(eyeOffset) > 0.01f) eyeOffset = glm::normalize(eyeOffset);
            Kaya::Renderer::DrawSphere(enemyPos + glm::vec3(0.0f, 1.4f, 0.0f) + eyeOffset * 0.25f + glm::vec3(-0.15f, 0.0f, 0.0f), 0.08f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
            Kaya::Renderer::DrawSphere(enemyPos + glm::vec3(0.0f, 1.4f, 0.0f) + eyeOffset * 0.25f + glm::vec3(0.15f, 0.0f, 0.0f), 0.08f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        }

        // Render debris
        for (const auto& debris : m_Debris) {
            glm::vec3 debrisPos = m_PhysicsSystem->GetBodyPosition(debris.bodyId);
            Kaya::Renderer::DrawCube(debrisPos, glm::vec3(0.2f, 0.2f, 0.2f), glm::vec4(debris.color, 1.0f));
        }

        // End 3D scene
        Kaya::Renderer::EndScene();

        // === IMGUI HUD RENDERING ===
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Get window size for positioning
        int windowWidth, windowHeight;
        glfwGetWindowSize(GetWindow().GetNativeWindow(), &windowWidth, &windowHeight);

        // Health Bar (bottom left)
        ImGui::SetNextWindowPos(ImVec2(20, windowHeight - 80), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(300, 60), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.5f);
        ImGui::Begin("Health", nullptr, 
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoInputs);
        
        // Health text
        ImGui::PushFont(nullptr);
        ImGui::SetWindowFontScale(1.5f);
        
        float healthPercent = (float)m_Health / m_MaxHealth;
        ImVec4 healthColor = healthPercent > 0.5f ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) :
                             healthPercent > 0.25f ? ImVec4(1.0f, 1.0f, 0.0f, 1.0f) :
                             ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
        
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "HEALTH");
        ImGui::SameLine();
        ImGui::TextColored(healthColor, "%d / %d", m_Health, m_MaxHealth);
        
        // Health bar
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, healthColor);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.0f, 0.0f, 0.8f));
        ImGui::ProgressBar(healthPercent, ImVec2(280, 20), "");
        ImGui::PopStyleColor(2);
        
        ImGui::PopFont();
        ImGui::End();

        // Ammo Counter (bottom right)
        ImGui::SetNextWindowPos(ImVec2(windowWidth - 220, windowHeight - 100), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(200, 80), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.5f);
        ImGui::Begin("Ammo", nullptr, 
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoInputs);
        
        ImGui::SetWindowFontScale(1.5f);
        
        // Ammo display: Current / Max | Reserve
        ImVec4 ammoColor = m_Ammo > 3 ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) :
                           m_Ammo > 0 ? ImVec4(1.0f, 0.8f, 0.0f, 1.0f) :
                           ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
        
        ImGui::TextColored(ammoColor, "%d", m_Ammo);
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "/ %d", m_MaxAmmo);
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "| %d", m_ReserveAmmo);
        
        // Reload indicator
        if (m_IsReloading) {
            float reloadProgress = 1.0f - (m_ReloadTimer / m_ReloadTime);
            ImGui::TextColored(ImVec4(0.0f, 0.8f, 1.0f, 1.0f), "RELOADING...");
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.0f, 0.7f, 1.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 0.8f));
            ImGui::ProgressBar(reloadProgress, ImVec2(180, 15), "");
            ImGui::PopStyleColor(2);
        } else if (m_Ammo == 0 && m_ReserveAmmo > 0) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Press R to Reload");
        } else if (m_Ammo == 0 && m_ReserveAmmo == 0) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "NO AMMO!");
        }
        
        ImGui::End();

        // Crosshair (center of screen)
        ImGui::SetNextWindowPos(ImVec2(windowWidth / 2.0f - 10, windowHeight / 2.0f - 10), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(20, 20), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::Begin("Crosshair", nullptr, 
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground);
        
        ImGui::SetWindowFontScale(2.0f);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.8f), "+");
        
        ImGui::End();

        // Enemies remaining (top right)
        ImGui::SetNextWindowPos(ImVec2(windowWidth - 180, 20), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(160, 40), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.5f);
        ImGui::Begin("Enemies", nullptr, 
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoInputs);
        
        ImGui::SetWindowFontScale(1.3f);
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "ENEMIES: %d", (int)m_Enemies.size());
        
        ImGui::End();

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void OnShutdown() override {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        m_PhysicsSystem.reset();
    }

private:
    void HandleInput(float deltaTime) {
        GLFWwindow* window = GetWindow().GetNativeWindow();

        // ESC to close window
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        // Mouse look
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        float deltaX = static_cast<float>(mouseX) - m_LastMouseX;
        float deltaY = static_cast<float>(mouseY) - m_LastMouseY;

        m_LastMouseX = static_cast<float>(mouseX);
        m_LastMouseY = static_cast<float>(mouseY);

        const float sensitivity = 0.1f;
        m_Yaw -= deltaX * sensitivity;  // Inverted for natural mouse control
        m_Pitch -= deltaY * sensitivity;  // Y-axis controls pitch (- for natural control)

        // Clamp pitch
        if (m_Pitch > 89.0f) m_Pitch = 89.0f;
        if (m_Pitch < -89.0f) m_Pitch = -89.0f;

        m_Camera->SetYaw(m_Yaw);
        m_Camera->SetPitch(m_Pitch);

        // Movement input
        glm::vec3 moveDir(0.0f);
        const float moveSpeed = 5.0f;  // Walking speed

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            glm::vec3 forward = m_Camera->GetForward();
            forward.y = 0.0f;  // Keep movement horizontal
            if (glm::length(forward) > 0.0f)
                forward = glm::normalize(forward);
            moveDir += forward;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            glm::vec3 forward = m_Camera->GetForward();
            forward.y = 0.0f;
            if (glm::length(forward) > 0.0f)
                forward = glm::normalize(forward);
            moveDir -= forward;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            moveDir -= m_Camera->GetRight();
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            moveDir += m_Camera->GetRight();
        }

        // Apply movement velocity (preserve Y velocity for gravity/jumping)
        glm::vec3 currentVel = m_PhysicsSystem->GetBodyVelocity(m_PlayerBodyId);
        if (glm::length(moveDir) > 0.0f) {
            moveDir = glm::normalize(moveDir);
            glm::vec3 newVel = moveDir * moveSpeed;
            newVel.y = currentVel.y;  // Preserve vertical velocity
            m_PhysicsSystem->SetBodyVelocity(m_PlayerBodyId, newVel);
        } else {
            // Apply friction when not moving - slow down horizontal velocity
            glm::vec3 horizontalVel(currentVel.x, 0.0f, currentVel.z);
            float friction = 0.85f;  // Friction coefficient (lower = more friction)
            glm::vec3 newVel = horizontalVel * friction;
            newVel.y = currentVel.y;  // Preserve vertical velocity
            m_PhysicsSystem->SetBodyVelocity(m_PlayerBodyId, newVel);
        }

        // Jump
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !m_JumpPressed) {
            glm::vec3 vel = m_PhysicsSystem->GetBodyVelocity(m_PlayerBodyId);
            vel.y = 5.0f;  // Jump velocity
            m_PhysicsSystem->SetBodyVelocity(m_PlayerBodyId, vel);
            m_JumpPressed = true;
        }
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) {
            m_JumpPressed = false;
        }

        // Shooting (only if not reloading and has ammo)
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !m_ShootPressed) {
            if (!m_IsReloading && m_Ammo > 0) {
                Shoot();
                m_Ammo--;
            }
            m_ShootPressed = true;
        }
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) {
            m_ShootPressed = false;
        }

        // Reload (R key)
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !m_ReloadPressed) {
            if (!m_IsReloading && m_Ammo < m_MaxAmmo && m_ReserveAmmo > 0) {
                m_IsReloading = true;
                m_ReloadTimer = m_ReloadTime;
            }
            m_ReloadPressed = true;
        }
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE) {
            m_ReloadPressed = false;
        }
    }

    void Shoot() {
        // Create bullet physics body
        glm::vec3 spawnPos = m_Camera->GetPosition() + m_Camera->GetForward() * 1.5f;
        JPH::BodyID bulletBodyId = m_PhysicsSystem->CreateSphere(
            spawnPos,
            0.1f,   // radius
            true    // is dynamic
        );

        // Give bullet velocity in camera forward direction
        glm::vec3 bulletVelocity = m_Camera->GetForward() * 50.0f;
        m_PhysicsSystem->SetBodyVelocity(bulletBodyId, bulletVelocity);

        // Add bullet to tracking list
        m_Bullets.push_back({ bulletBodyId, 5.0f });  // 5 second lifetime
    }

    void SpawnEnemy(const glm::vec3& position, const glm::vec3& color) {
        JPH::BodyID bodyId = m_PhysicsSystem->CreateBox(
            position,
            glm::vec3(0.5f, 1.0f, 0.5f),  // Collision box (smaller than visual)
            true  // Dynamic so they can move
        );
        m_Enemies.push_back({ bodyId, 3, color, position });
    }

    void UpdateEnemies(float deltaTime) {
        glm::vec3 playerPos = m_PhysicsSystem->GetBodyPosition(m_PlayerBodyId);
        const float enemySpeed = 2.0f;  // Slow crawl speed

        for (auto& enemy : m_Enemies) {
            glm::vec3 enemyPos = m_PhysicsSystem->GetBodyPosition(enemy.bodyId);
            
            // Calculate direction to player (horizontal only)
            glm::vec3 toPlayer = playerPos - enemyPos;
            toPlayer.y = 0.0f;  // Keep movement horizontal
            
            if (glm::length(toPlayer) > 0.1f) {
                glm::vec3 moveDir = glm::normalize(toPlayer);
                
                // Set velocity towards player, preserve Y for gravity
                glm::vec3 currentVel = m_PhysicsSystem->GetBodyVelocity(enemy.bodyId);
                glm::vec3 newVel = moveDir * enemySpeed;
                newVel.y = currentVel.y;  // Preserve vertical velocity
                m_PhysicsSystem->SetBodyVelocity(enemy.bodyId, newVel);
            }
        }
    }

    void CheckEnemyPlayerCollisions() {
        const float damageRadius = 1.5f;
        const float damageCooldown = 1.0f;  // 1 second between damage ticks

        glm::vec3 playerPos = m_PhysicsSystem->GetBodyPosition(m_PlayerBodyId);

        for (const auto& enemy : m_Enemies) {
            glm::vec3 enemyPos = m_PhysicsSystem->GetBodyPosition(enemy.bodyId);
            float distance = glm::length(playerPos - enemyPos);

            if (distance < damageRadius && m_DamageCooldown <= 0.0f) {
                m_Health -= 10;  // Take 10 damage
                m_DamageCooldown = damageCooldown;
                
                // Knockback player away from enemy
                glm::vec3 knockback = glm::normalize(playerPos - enemyPos);
                knockback.y = 0.3f;  // Slight upward
                glm::vec3 vel = m_PhysicsSystem->GetBodyVelocity(m_PlayerBodyId);
                vel += knockback * 8.0f;
                m_PhysicsSystem->SetBodyVelocity(m_PlayerBodyId, vel);

                if (m_Health <= 0) {
                    m_Health = 0;
                    // Could add game over logic here
                }
                break;  // Only one enemy can damage per frame
            }
        }

        // Update damage cooldown
        if (m_DamageCooldown > 0.0f) {
            m_DamageCooldown -= 1.0f / 60.0f;  // Approximate frame time
        }
    }

    void CheckBulletEnemyCollisions() {
        const float hitRadius = 1.5f;  // Distance threshold for hit detection

        for (auto bulletIt = m_Bullets.begin(); bulletIt != m_Bullets.end(); ) {
            glm::vec3 bulletPos = m_PhysicsSystem->GetBodyPosition(bulletIt->bodyId);
            bool bulletHit = false;

            for (auto enemyIt = m_Enemies.begin(); enemyIt != m_Enemies.end(); ) {
                glm::vec3 enemyPos = m_PhysicsSystem->GetBodyPosition(enemyIt->bodyId);
                float distance = glm::length(bulletPos - enemyPos);

                if (distance < hitRadius) {
                    // Bullet hit enemy!
                    enemyIt->health--;
                    
                    // Remove bullet
                    m_PhysicsSystem->RemoveBody(bulletIt->bodyId);
                    bulletHit = true;

                    if (enemyIt->health <= 0) {
                        // Destroy enemy and spawn debris
                        DestroyEnemy(enemyIt);
                        enemyIt = m_Enemies.erase(enemyIt);
                    } else {
                        ++enemyIt;
                    }
                    break;  // Bullet can only hit one enemy
                } else {
                    ++enemyIt;
                }
            }

            if (bulletHit) {
                bulletIt = m_Bullets.erase(bulletIt);
            } else {
                ++bulletIt;
            }
        }
    }

    void DestroyEnemy(std::vector<Enemy>::iterator enemyIt) {
        glm::vec3 enemyPos = m_PhysicsSystem->GetBodyPosition(enemyIt->bodyId);
        glm::vec3 enemyColor = enemyIt->color;

        // Remove enemy physics body
        m_PhysicsSystem->RemoveBody(enemyIt->bodyId);

        // Spawn debris pieces flying outward
        const int numDebris = 12;
        for (int i = 0; i < numDebris; i++) {
            // Random offset around enemy position
            float angle = (float)i / numDebris * 6.28318f;  // Evenly distributed angle
            float heightOffset = ((float)(i % 3) - 1.0f) * 0.5f;  // Vary height
            
            glm::vec3 debrisPos = enemyPos + glm::vec3(
                cos(angle) * 0.3f,
                heightOffset + 1.0f,
                sin(angle) * 0.3f
            );

            JPH::BodyID debrisBodyId = m_PhysicsSystem->CreateBox(
                debrisPos,
                glm::vec3(0.1f, 0.1f, 0.1f),
                true  // Dynamic
            );

            // Give debris outward velocity with some upward component
            glm::vec3 debrisVel = glm::vec3(
                cos(angle) * 8.0f + ((rand() % 100) / 100.0f - 0.5f) * 4.0f,
                5.0f + ((rand() % 100) / 100.0f) * 5.0f,
                sin(angle) * 8.0f + ((rand() % 100) / 100.0f - 0.5f) * 4.0f
            );
            m_PhysicsSystem->SetBodyVelocity(debrisBodyId, debrisVel);

            // Track debris with lifetime
            m_Debris.push_back({ debrisBodyId, 3.0f, enemyColor });
        }
    }

    std::unique_ptr<Kaya::PhysicsSystem> m_PhysicsSystem;
    std::unique_ptr<Kaya::Camera> m_Camera;
    
    JPH::BodyID m_PlayerBodyId;
    JPH::BodyID m_GroundBodyId;
    
    std::vector<Bullet> m_Bullets;
    std::vector<Enemy> m_Enemies;
    std::vector<Debris> m_Debris;
    
    float m_LastMouseX = 0.0f;
    float m_LastMouseY = 0.0f;
    float m_Yaw = -90.0f;
    float m_Pitch = 0.0f;
    float m_LastAspectRatio = 1280.0f / 720.0f;
    
    // Gun sway/smoothing
    glm::vec3 m_GunPosition = glm::vec3(0.0f);
    float m_GunSmoothSpeed = 8.0f;  // Higher = snappier, lower = more sway
    
    bool m_JumpPressed = false;
    bool m_ShootPressed = false;
    bool m_ReloadPressed = false;

    // Health system
    int m_Health = 100;
    int m_MaxHealth = 100;
    float m_DamageCooldown = 0.0f;

    // Ammo system
    int m_Ammo = 12;         // Current ammo in magazine
    int m_MaxAmmo = 12;      // Magazine capacity
    int m_ReserveAmmo = 48;  // Reserve ammo
    bool m_IsReloading = false;
    float m_ReloadTimer = 0.0f;
    float m_ReloadTime = 1.5f;  // Seconds to reload
};

Kaya::Application* Kaya::CreateApplication() {
    return new FPSExample();
}

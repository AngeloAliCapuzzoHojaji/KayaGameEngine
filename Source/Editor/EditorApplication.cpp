#include "EditorApplication.h"
#include "../Input/Input.h"
#include "../Rendering/Renderer.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>

namespace Kaya {
namespace Editor {

    EditorApplication::EditorApplication()
        : Application("Kaya Editor")
    {
    }

    void EditorApplication::OnInit() {
        // Initialize ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        
        #ifdef IMGUI_HAS_DOCK
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        #endif
        
        #ifdef IMGUI_HAS_VIEWPORT
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        #endif

        // Setup style
        ImGui::StyleColorsDark();

        #ifdef IMGUI_HAS_VIEWPORT
        // When viewports are enabled we tweak WindowRounding/WindowBg
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }
        #endif

        // Setup Platform/Renderer backends
        GLFWwindow* window = static_cast<GLFWwindow*>(GetWindow().GetNativeWindow());
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 460");

        // Create scene
        m_Scene = std::make_unique<Scene>("Editor Scene");
        
        // Initialize camera
        m_Camera = std::make_unique<Camera>(45.0f, GetWindow().GetAspectRatio());
        m_Camera->SetPosition(glm::vec3(0, 5, 10));

        // Initialize physics
        m_Physics = std::make_unique<PhysicsSystem>();
        m_Physics->Initialize();

        // Setup editor panels
        m_SceneHierarchyPanel.SetContext(m_Scene.get());

        // Create a default ground plane
        Entity* ground = m_Scene->CreateEntity("Ground");
        ground->GetTransform().Position = glm::vec3(0, -1, 0);
        ground->GetTransform().Scale = glm::vec3(20, 1, 20);
        ground->GetRender().GeometryType = RenderComponent::Type::Cube;
        ground->GetRender().Color = glm::vec4(0.3f, 0.5f, 0.3f, 1.0f);
        
        // Create physics body for ground
        JPH::BodyID groundBodyID = m_Physics->CreateBox(
            ground->GetTransform().Position,
            ground->GetTransform().Scale,
            false
        );
        ground->GetPhysics().BodyID = groundBodyID;
        ground->GetPhysics().IsDynamic = false;

        // Create a couple of example entities
        Entity* cube = m_Scene->CreateEntity("Cube");
        cube->GetTransform().Position = glm::vec3(0, 3, 0);
        cube->GetRender().GeometryType = RenderComponent::Type::Cube;
        cube->GetRender().Color = glm::vec4(0.8f, 0.3f, 0.2f, 1.0f);

        Entity* sphere = m_Scene->CreateEntity("Sphere");
        sphere->GetTransform().Position = glm::vec3(2, 5, 0);
        sphere->GetRender().GeometryType = RenderComponent::Type::Sphere;
        sphere->GetRender().Color = glm::vec4(0.2f, 0.4f, 0.9f, 1.0f);
    }

    void EditorApplication::OnUpdate(float deltaTime) {
        // Update viewport state
        m_ViewportFocused = m_ViewportPanel.IsFocused();
        m_ViewportHovered = m_ViewportPanel.IsHovered();

        // Update camera only if viewport is focused/hovered
        if (m_ViewportFocused || m_ViewportHovered) {
            UpdateCamera(deltaTime);
        }

        // Update camera aspect ratio based on viewport size
        glm::vec2 viewportSize = m_ViewportPanel.GetViewportSize();
        if (viewportSize.x > 0 && viewportSize.y > 0) {
            float aspectRatio = viewportSize.x / viewportSize.y;
            m_Camera->SetAspectRatio(aspectRatio);
        }

        // Update physics
        m_Physics->Update(deltaTime);

        // Update scene (sync with physics)
        m_Scene->Update(deltaTime, m_Physics.get());

        // Update properties panel with selected entity
        Entity* selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();
        m_PropertiesPanel.SetSelectedEntity(selectedEntity);
    }

    void EditorApplication::OnRender() {
        RenderScene();
    }

    void EditorApplication::OnImGuiRender() {
        SetupDockspace();
        RenderMenuBar();

        // Render editor panels
        m_SceneHierarchyPanel.OnImGuiRender();
        m_PropertiesPanel.OnImGuiRender();
        m_ViewportPanel.OnImGuiRender();

        // Render gizmos
        HandleGizmos();
    }

    void EditorApplication::OnShutdown() {
        // Cleanup ImGui
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        // Cleanup physics
        m_Physics->Shutdown();
    }

    void EditorApplication::SetupDockspace() {
        #ifdef IMGUI_HAS_DOCK
        static bool dockspaceOpen = true;
        
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar;
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace", &dockspaceOpen, window_flags);
        ImGui::PopStyleVar(3);

        // DockSpace
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
        }

        ImGui::End();
        #endif
    }

    void EditorApplication::RenderMenuBar() {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
                    // TODO: New scene
                }
                if (ImGui::MenuItem("Open Scene", "Ctrl+O")) {
                    // TODO: Open scene
                }
                if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
                    // TODO: Save scene
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit")) {
                    GetWindow().Close();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
                    // TODO: Undo
                }
                if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
                    // TODO: Redo
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Scene Hierarchy", nullptr, true);
                ImGui::MenuItem("Properties", nullptr, true);
                ImGui::MenuItem("Viewport", nullptr, true);
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }
    }

    void EditorApplication::RenderScene() {
        // Bind framebuffer
        m_ViewportPanel.GetFramebuffer()->Bind();

        // Clear
        Renderer::Clear(glm::vec4(0.1f, 0.1f, 0.15f, 1.0f));

        // Begin scene
        Renderer::BeginScene(*m_Camera);

        // Render all entities
        for (const auto& entity : m_Scene->GetEntities()) {
            if (!entity->GetRender().Visible)
                continue;

            const auto& transform = entity->GetTransform();
            const auto& render = entity->GetRender();

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
                default:
                    break;
            }
        }

        Renderer::EndScene();
        
        // Draw skybox last (if present)
        if (m_Scene->GetSkybox()) {
            Renderer::DrawSkybox(m_Scene->GetSkybox());
        }

        // Unbind framebuffer
        m_ViewportPanel.GetFramebuffer()->Unbind();
    }

    void EditorApplication::UpdateCamera(float deltaTime) {
        // Camera movement (WASD + Space/Shift)
        glm::vec3 forward = m_Camera->GetForward();
        glm::vec3 right = m_Camera->GetRight();
        glm::vec3 up = glm::vec3(0, 1, 0);

        glm::vec3 movement(0.0f);

        if (Input::IsKeyPressed(KeyCode::W))
            movement += forward;
        if (Input::IsKeyPressed(KeyCode::S))
            movement -= forward;
        if (Input::IsKeyPressed(KeyCode::D))
            movement += right;
        if (Input::IsKeyPressed(KeyCode::A))
            movement -= right;
        if (Input::IsKeyPressed(KeyCode::Space))
            movement += up;
        if (Input::IsKeyPressed(KeyCode::LeftShift))
            movement -= up;

        if (glm::length(movement) > 0.0f) {
            movement = glm::normalize(movement);
            m_Camera->SetPosition(m_Camera->GetPosition() + movement * m_CameraSpeed * deltaTime);
        }

        // Camera rotation with right mouse button
        if (Input::IsMouseButtonPressed(MouseCode::Right)) {
            glm::vec2 mousePos = Input::GetMousePosition();
            glm::vec2 delta = (mousePos - m_LastMousePos) * 0.1f;

            if (glm::length(delta) > 0.0f) {
                m_Camera->SetYaw(m_Camera->GetYaw() + delta.x);
                m_Camera->SetPitch(m_Camera->GetPitch() - delta.y);
            }

            m_LastMousePos = mousePos;
        } else {
            m_LastMousePos = Input::GetMousePosition();
        }
    }

    void EditorApplication::HandleGizmos() {
        Entity* selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();
        if (!selectedEntity)
            return;

        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();

        ImGuizmo::SetRect(
            ImGui::GetWindowPos().x,
            ImGui::GetWindowPos().y,
            ImGui::GetWindowWidth(),
            ImGui::GetWindowHeight()
        );

        // Camera matrices
        glm::mat4 view = m_Camera->GetViewMatrix();
        glm::mat4 projection = m_Camera->GetProjectionMatrix();

        // Entity transform
        auto& transform = selectedEntity->GetTransform();
        glm::mat4 entityTransform = transform.GetTransform();

        // Gizmo manipulation
        bool snap = Input::IsKeyPressed(KeyCode::LeftControl);
        float snapValue = 0.5f; // Snap to 0.5 units for translate/scale
        if (m_GizmoOperation == 7) // Rotate
            snapValue = 45.0f;

        float snapValues[3] = { snapValue, snapValue, snapValue };

        ImGuizmo::Manipulate(
            glm::value_ptr(view),
            glm::value_ptr(projection),
            (ImGuizmo::OPERATION)m_GizmoOperation,
            ImGuizmo::LOCAL,
            glm::value_ptr(entityTransform),
            nullptr,
            snap ? snapValues : nullptr
        );

        if (ImGuizmo::IsUsing()) {
            // Decompose matrix to get new transform values
            glm::vec3 position, rotation, scale;
            ImGuizmo::DecomposeMatrixToComponents(
                glm::value_ptr(entityTransform),
                glm::value_ptr(position),
                glm::value_ptr(rotation),
                glm::value_ptr(scale)
            );

            transform.Position = position;
            transform.Rotation = rotation;
            transform.Scale = scale;
        }

        // Toolbar for gizmo operation selection
        ImGui::Begin("Viewport");
        
        if (ImGui::RadioButton("Translate", m_GizmoOperation == 7))
            m_GizmoOperation = 7; // ImGuizmo::TRANSLATE
        ImGui::SameLine();
        if (ImGui::RadioButton("Rotate", m_GizmoOperation == 120))
            m_GizmoOperation = 120; // ImGuizmo::ROTATE
        ImGui::SameLine();
        if (ImGui::RadioButton("Scale", m_GizmoOperation == 896))
            m_GizmoOperation = 896; // ImGuizmo::SCALE

        ImGui::End();
    }

}}

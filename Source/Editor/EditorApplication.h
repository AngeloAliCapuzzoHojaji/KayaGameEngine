#pragma once

#include "../Core/Application.h"
#include "../Core/Scene.h"
#include "../Rendering/Camera.h"
#include "../Physics/PhysicsSystem.h"
#include "SceneHierarchyPanel.h"
#include "PropertiesPanel.h"
#include "ViewportPanel.h"
#include <memory>

namespace Kaya {
namespace Editor {

    class EditorApplication : public Application {
    public:
        EditorApplication();
        virtual ~EditorApplication() = default;

    protected:
        void OnInit() override;
        void OnUpdate(float deltaTime) override;
        void OnRender() override;
        void OnShutdown() override;
        void OnImGuiRender() override;

    private:
        void SetupDockspace();
        void RenderMenuBar();
        void RenderScene();
        void UpdateCamera(float deltaTime);
        void HandleGizmos();

    private:
        // Scene
        std::unique_ptr<Scene> m_Scene;
        std::unique_ptr<Camera> m_Camera;
        std::unique_ptr<PhysicsSystem> m_Physics;

        // Editor panels
        SceneHierarchyPanel m_SceneHierarchyPanel;
        PropertiesPanel m_PropertiesPanel;
        ViewportPanel m_ViewportPanel;

        // Editor state
        bool m_ViewportFocused = false;
        bool m_ViewportHovered = false;
        int m_GizmoOperation = 7; // Translate (ImGuizmo::OPERATION)
        float m_CameraSpeed = 5.0f;
        glm::vec2 m_LastMousePos = { 0.0f, 0.0f };
    };

}}

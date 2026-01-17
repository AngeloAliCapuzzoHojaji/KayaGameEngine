#include "SceneHierarchyPanel.h"
#include <imgui.h>

namespace Kaya {
namespace Editor {

    void SceneHierarchyPanel::SetContext(Scene* scene) {
        m_Context = scene;
        m_SelectionContext = nullptr;
    }

    void SceneHierarchyPanel::OnImGuiRender() {
        ImGui::Begin("Scene Hierarchy");

        if (m_Context) {
            // Right-click menu to create entities
            if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
                if (ImGui::MenuItem("Create Empty Entity")) {
                    m_Context->CreateEntity("Empty Entity");
                }
                if (ImGui::MenuItem("Create Cube")) {
                    Entity* entity = m_Context->CreateEntity("Cube");
                    entity->GetRender().GeometryType = RenderComponent::Type::Cube;
                }
                if (ImGui::MenuItem("Create Sphere")) {
                    Entity* entity = m_Context->CreateEntity("Sphere");
                    entity->GetRender().GeometryType = RenderComponent::Type::Sphere;
                }
                ImGui::EndPopup();
            }

            // Display all entities
            for (const auto& entity : m_Context->GetEntities()) {
                DrawEntityNode(entity.get());
            }

            // Click on blank space to deselect
            if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered()) {
                m_SelectionContext = nullptr;
                m_Context->ClearSelection();
            }
        }

        ImGui::End();
    }

    void SceneHierarchyPanel::DrawEntityNode(Entity* entity) {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        
        if (entity->IsSelected())
            flags |= ImGuiTreeNodeFlags_Selected;

        // Leaf node (no children for now)
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

        bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity->GetID(), flags, "%s", entity->GetName().c_str());

        if (ImGui::IsItemClicked()) {
            m_SelectionContext = entity;
            m_Context->SetSelectedEntity(entity);
        }

        // Right-click menu for entity
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Delete Entity")) {
                if (m_SelectionContext == entity)
                    m_SelectionContext = nullptr;
                m_Context->RemoveEntity(entity);
                ImGui::EndPopup();
                return;
            }
            ImGui::EndPopup();
        }
    }

}}

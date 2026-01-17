#include "PropertiesPanel.h"
#include <imgui.h>

namespace Kaya {
namespace Editor {

    void PropertiesPanel::SetSelectedEntity(Entity* entity) {
        m_SelectedEntity = entity;
    }

    void PropertiesPanel::OnImGuiRender() {
        ImGui::Begin("Properties");

        if (m_SelectedEntity) {
            // Entity name
            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            strcpy_s(buffer, sizeof(buffer), m_SelectedEntity->GetName().c_str());
            if (ImGui::InputText("##Name", buffer, sizeof(buffer))) {
                m_SelectedEntity->SetName(std::string(buffer));
            }

            ImGui::Separator();

            // Components
            DrawTransformComponent(m_SelectedEntity);
            DrawRenderComponent(m_SelectedEntity);
            
            if (m_SelectedEntity->HasPhysics()) {
                DrawPhysicsComponent(m_SelectedEntity);
            }
        }

        ImGui::End();
    }

    void PropertiesPanel::DrawTransformComponent(Entity* entity) {
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& transform = entity->GetTransform();

            ImGui::DragFloat3("Position", &transform.Position.x, 0.1f);
            ImGui::DragFloat3("Rotation", &transform.Rotation.x, 1.0f);
            ImGui::DragFloat3("Scale", &transform.Scale.x, 0.1f, 0.01f, 100.0f);
        }
    }

    void PropertiesPanel::DrawRenderComponent(Entity* entity) {
        if (ImGui::CollapsingHeader("Render", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& render = entity->GetRender();

            ImGui::Checkbox("Visible", &render.Visible);

            // Geometry type
            const char* geometryTypes[] = { "None", "Cube", "Sphere" };
            int currentType = (int)render.GeometryType;
            if (ImGui::Combo("Geometry", &currentType, geometryTypes, IM_ARRAYSIZE(geometryTypes))) {
                render.GeometryType = (RenderComponent::Type)currentType;
            }

            // Color picker
            ImGui::ColorEdit4("Color", &render.Color.x);
        }
    }

    void PropertiesPanel::DrawPhysicsComponent(Entity* entity) {
        if (ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& physics = entity->GetPhysics();

            ImGui::Text("Body ID: %u", physics.BodyID.GetIndexAndSequenceNumber());
            ImGui::Checkbox("Dynamic", &physics.IsDynamic);
            
            if (physics.IsDynamic) {
                ImGui::DragFloat("Mass", &physics.Mass, 0.1f, 0.1f, 1000.0f);
            }

            ImGui::TextDisabled("(Physics component is read-only)");
        }
    }

}}

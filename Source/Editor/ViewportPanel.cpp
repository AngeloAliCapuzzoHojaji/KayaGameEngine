#include "ViewportPanel.h"
#include <imgui.h>
#include <glm/glm.hpp>

namespace Kaya {
namespace Editor {

    ViewportPanel::ViewportPanel() {
        m_Framebuffer = std::make_unique<Framebuffer>(1280, 720);
    }

    void ViewportPanel::OnImGuiRender() {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Viewport");

        m_IsFocused = ImGui::IsWindowFocused();
        m_IsHovered = ImGui::IsWindowHovered();

        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        
        // Resize framebuffer if viewport size changed
        if (m_ViewportSize.x != viewportPanelSize.x || m_ViewportSize.y != viewportPanelSize.y) {
            if (viewportPanelSize.x > 0 && viewportPanelSize.y > 0) {
                m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };
                m_Framebuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
            }
        }

        // Display the framebuffer texture
        uint64_t textureID = m_Framebuffer->GetColorAttachment();
        ImGui::Image((void*)textureID, ImVec2(m_ViewportSize.x, m_ViewportSize.y), ImVec2(0, 1), ImVec2(1, 0));

        ImGui::End();
        ImGui::PopStyleVar();
    }

}}

#pragma once

#include <imgui.h>
#include <memory>
#include <glm/glm.hpp>
#include "../Rendering/Framebuffer.h"

namespace Kaya {
namespace Editor {

    class ViewportPanel {
    public:
        ViewportPanel();
        ~ViewportPanel() = default;

        void OnImGuiRender();
        
        Framebuffer* GetFramebuffer() const { return m_Framebuffer.get(); }
        bool IsFocused() const { return m_IsFocused; }
        bool IsHovered() const { return m_IsHovered; }
        
        glm::vec2 GetViewportSize() const { return m_ViewportSize; }

    private:
        std::unique_ptr<Framebuffer> m_Framebuffer;
        glm::vec2 m_ViewportSize = { 1280, 720 };
        bool m_IsFocused = false;
        bool m_IsHovered = false;
    };

}}

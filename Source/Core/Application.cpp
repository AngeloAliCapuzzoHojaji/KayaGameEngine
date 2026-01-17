#include "Core/Application.h"
#include "Rendering/Renderer.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>

namespace Kaya {

Application* Application::s_Instance = nullptr;

Application::Application(const std::string& name) {
    s_Instance = this;
    
    // Create window
    WindowProps props;
    props.Title = name;
    m_Window = std::make_unique<Window>(props);
    
    // Initialize renderer
    Renderer::Init();
    Renderer::SetViewport(0, 0, m_Window->GetWidth(), m_Window->GetHeight());
    
    std::cout << name << " initialized" << std::endl;
}

Application::~Application() {
    Renderer::Shutdown();
}

void Application::Run() {
    OnInit();
    
    m_Running = true;
    m_LastFrameTime = static_cast<float>(glfwGetTime());
    
    while (m_Running && !m_Window->ShouldClose()) {
        float time = static_cast<float>(glfwGetTime());
        float deltaTime = time - m_LastFrameTime;
        m_LastFrameTime = time;
        
        // Update
        OnUpdate(deltaTime);
        
        // Render
        Renderer::Clear();
        OnRender();
        
        // ImGui Render (optional, overridden by editor)
        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        OnImGuiRender();
        
        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        #ifdef IMGUI_HAS_VIEWPORT
        // Update and Render additional Platform Windows
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
        #endif
        
        // Swap buffers and poll events
        m_Window->OnUpdate();
    }
    
    OnShutdown();
}

} // namespace Kaya

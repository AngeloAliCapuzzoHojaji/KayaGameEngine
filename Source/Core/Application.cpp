#include "Core/Application.h"
#include "Rendering/Renderer.h"
#include "Rendering/GPUMetrics.h"
#include "Input/Input.h"
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
    
    // Initialize GPU metrics
    GPUMetricsManager::Init();
    
    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    
    GLFWwindow* window = static_cast<GLFWwindow*>(m_Window->GetNativeWindow());
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");
    
    std::cout << name << " initialized" << std::endl;
}

Application::~Application() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    GPUMetricsManager::Shutdown();
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
        
        // Toggle GPU metrics with F3
        static bool f3WasPressed = false;
        bool f3Pressed = glfwGetKey(static_cast<GLFWwindow*>(m_Window->GetNativeWindow()), GLFW_KEY_F3) == GLFW_PRESS;
        if (f3Pressed && !f3WasPressed)
            GPUMetricsManager::SetVisible(!GPUMetricsManager::IsVisible());
        f3WasPressed = f3Pressed;

        // Toggle debug culling visualization with F4
        static bool f4WasPressed = false;
        bool f4Pressed = glfwGetKey(static_cast<GLFWwindow*>(m_Window->GetNativeWindow()), GLFW_KEY_F4) == GLFW_PRESS;
        if (f4Pressed && !f4WasPressed) {
            Renderer::SetDebugCullingMode(!Renderer::IsDebugCullingMode());
            GPUMetricsManager::SetDebugCullingActive(Renderer::IsDebugCullingMode());
        }
        f4WasPressed = f4Pressed;

        // Render
        GPUMetricsManager::BeginFrame();
        Renderer::Clear();
        OnRender();
        GPUMetricsManager::EndFrame(deltaTime);
        
        // ImGui Render (optional, overridden by editor)
        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        OnImGuiRender();
        GPUMetricsManager::RenderOverlay();
        
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

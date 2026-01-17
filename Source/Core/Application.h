#pragma once

#include "Core/Window.h"
#include <memory>

namespace Kaya {

class Application {
public:
    Application(const std::string& name = "Kaya Application");
    virtual ~Application();

    void Run();
    
    virtual void OnInit() {}
    virtual void OnShutdown() {}
    virtual void OnUpdate(float deltaTime) {}
    virtual void OnRender() {}
    virtual void OnImGuiRender() {}

    Window& GetWindow() { return *m_Window; }
    
    static Application& Get() { return *s_Instance; }

private:
    std::unique_ptr<Window> m_Window;
    bool m_Running = true;
    float m_LastFrameTime = 0.0f;

    static Application* s_Instance;
};

// To be defined by client
Application* CreateApplication();

} // namespace Kaya

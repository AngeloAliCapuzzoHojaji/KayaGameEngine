#pragma once

#include <string>
#include <memory>
#include <functional>

struct GLFWwindow;

namespace Kaya {

struct WindowProps {
    std::string Title;
    unsigned int Width;
    unsigned int Height;
    bool VSync;

    WindowProps(const std::string& title = "Kaya Game Engine",
                unsigned int width = 1280,
                unsigned int height = 720,
                bool vsync = true)
        : Title(title), Width(width), Height(height), VSync(vsync) {}
};

class Window {
public:
    using EventCallbackFn = std::function<void(class Event&)>;

    Window(const WindowProps& props);
    ~Window();

    void OnUpdate();
    
    unsigned int GetWidth() const { return m_Data.Width; }
    unsigned int GetHeight() const { return m_Data.Height; }
    float GetAspectRatio() const { return static_cast<float>(m_Data.Width) / static_cast<float>(m_Data.Height); }

    void SetEventCallback(const EventCallbackFn& callback) { m_Data.EventCallback = callback; }
    void SetVSync(bool enabled);
    bool IsVSync() const { return m_Data.VSync; }

    GLFWwindow* GetNativeWindow() const { return m_Window; }

    bool ShouldClose() const;
    void Close();

private:
    void Init(const WindowProps& props);
    void Shutdown();

private:
    GLFWwindow* m_Window;

    struct WindowData {
        std::string Title;
        unsigned int Width, Height;
        bool VSync;
        EventCallbackFn EventCallback;
    };

    WindowData m_Data;
};

} // namespace Kaya

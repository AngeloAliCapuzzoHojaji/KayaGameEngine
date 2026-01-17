#include <Kaya.h>
#include <Editor/EditorApplication.h>

// Entry point
Kaya::Application* Kaya::CreateApplication() {
    return new Kaya::Editor::EditorApplication();
}

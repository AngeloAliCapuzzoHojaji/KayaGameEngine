#include "Input/Input.h"
#include "Core/Application.h"
#include <GLFW/glfw3.h>

namespace Kaya {

bool Input::IsKeyPressed(KeyCode keycode) {
    auto window = Application::Get().GetWindow().GetNativeWindow();
    auto state = glfwGetKey(window, static_cast<int>(keycode));
    return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool Input::IsMouseButtonPressed(MouseButton button) {
    auto window = Application::Get().GetWindow().GetNativeWindow();
    auto state = glfwGetMouseButton(window, static_cast<int>(button));
    return state == GLFW_PRESS;
}

glm::vec2 Input::GetMousePosition() {
    auto window = Application::Get().GetWindow().GetNativeWindow();
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    return { static_cast<float>(xpos), static_cast<float>(ypos) };
}

float Input::GetMouseX() {
    return GetMousePosition().x;
}

float Input::GetMouseY() {
    return GetMousePosition().y;
}

} // namespace Kaya

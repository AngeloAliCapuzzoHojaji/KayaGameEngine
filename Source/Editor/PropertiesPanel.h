#pragma once

#include <imgui.h>
#include "../Core/Entity.h"

namespace Kaya {
namespace Editor {

    class PropertiesPanel {
    public:
        PropertiesPanel() = default;
        ~PropertiesPanel() = default;

        void SetSelectedEntity(Entity* entity);
        void OnImGuiRender();

    private:
        void DrawTransformComponent(Entity* entity);
        void DrawRenderComponent(Entity* entity);
        void DrawPhysicsComponent(Entity* entity);

    private:
        Entity* m_SelectedEntity = nullptr;
    };

}}

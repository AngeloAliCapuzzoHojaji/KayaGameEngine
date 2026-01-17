#pragma once

#include <imgui.h>
#include "../Core/Scene.h"

namespace Kaya {
namespace Editor {

    class SceneHierarchyPanel {
    public:
        SceneHierarchyPanel() = default;
        ~SceneHierarchyPanel() = default;

        void SetContext(Scene* scene);
        void OnImGuiRender();

        Entity* GetSelectedEntity() const { return m_SelectionContext; }

    private:
        void DrawEntityNode(Entity* entity);

    private:
        Scene* m_Context = nullptr;
        Entity* m_SelectionContext = nullptr;
    };

}}

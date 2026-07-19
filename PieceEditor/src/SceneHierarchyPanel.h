#pragma once

#include <core/Core.h>

#include <scene/Entity.h>

#include <filesystem>
#include <string>

namespace Piece {

class Scene;

class SceneHierarchyPanel {
public:
    SceneHierarchyPanel() = default;
    explicit SceneHierarchyPanel(const Ref<Scene>& scene);

    void SetContext(const Ref<Scene>& scene);
    void OnImGuiRender();

private:
    void DrawSceneTools();
    void DrawLookDevTools();
    bool SpawnObjFromPath(const std::filesystem::path& sourcePath, const std::string& displayName);
    bool MergeAllChildren(Entity rootEntity);
    bool RestoreImportedChildren(Entity rootEntity);
    void DrawEntityNode(Entity entity);
    void DrawProperties(Entity entity);

private:
    Ref<Scene> m_Context;
    Entity m_SelectionContext;
    std::string m_ObjStatusMessage;
    bool m_ObjStatusIsError = false;
};

} // namespace Piece

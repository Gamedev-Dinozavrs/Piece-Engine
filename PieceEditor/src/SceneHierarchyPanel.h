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
    void SelectEntityByUUID(UUID uuid);
    Entity GetSelectedEntity() const { return m_SelectionContext; }
    void SetSelectedEntity(const Entity& entity) { m_SelectionContext = entity; }
    void OnImGuiRender();

private:
    bool SpawnObjFromPath(const std::filesystem::path& sourcePath, const std::string& displayName);
    bool MergeAllChildren(Entity rootEntity);
    bool RestoreImportedChildren(Entity rootEntity);
    void DrawEntityNode(Entity entity);
    void DrawProperties(Entity entity);
    void BeginRenameEntity(Entity entity);

private:
    Ref<Scene> m_Context;
    Entity m_SelectionContext;
    Entity m_RenameEntity;
    std::string m_ObjStatusMessage;
    bool m_ObjStatusIsError = false;
    uint32_t m_SurfaceFactorMaterialId = 0;
    bool m_EditSurfaceFactors = false;
    char m_RenameBuffer[256] = {};
    bool m_OpenRenamePopup = false;
};

} // namespace Piece

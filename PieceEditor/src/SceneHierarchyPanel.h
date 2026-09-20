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
    void SelectHierarchyRootByUUID(UUID uuid);
    bool SpawnModelFromPath(const std::filesystem::path& sourcePath);
    Entity GetSelectedEntity() const { return m_SelectionContext; }
    void SetSelectedEntity(const Entity& entity) { m_SelectionContext = entity; }
    void OnImGuiRender();

private:
    bool SpawnObjFromPath(const std::filesystem::path& sourcePath, const std::string& displayName);
    bool MergeAllChildren(Entity rootEntity);
    bool RestoreImportedChildren(Entity rootEntity);
    void DrawEntityNode(Entity entity);
    void DrawProperties(Entity entity);
    void DrawAnimatorGraph(Entity entity);
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

    int32_t m_AnimatorSelectedState = -1;
    int32_t m_AnimatorSelectedTransition = -1;
    bool m_AnimatorLinkMode = false;
    int32_t m_AnimatorLinkFromState = -1;
    int32_t m_AnimatorDraggingState = -1;
    float m_AnimatorDragOffsetX = 0.0f;
    float m_AnimatorDragOffsetY = 0.0f;
    char m_AnimatorNewParamName[64] = "Param";
};

} // namespace Piece

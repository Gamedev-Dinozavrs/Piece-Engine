#include "SceneHierarchyPanel.h"

#include "EditorPlacement.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <assets/AssetImporter.h>
#include <core/Log.h>
#include <renderer/Renderer.h>
#include <scene/Components.h>
#include <scene/Scene.h>
#include <scene/World.h>
#include <utils/platform/WindowsUtils.h>

#include <cstdio>
#include <exception>
#include <filesystem>

namespace Piece {

namespace {

const char* PrimitiveTypeLabel(PrimitiveType primitiveType) {
    switch (primitiveType) {
    case PrimitiveType::Quad:
        return "Quad";
    case PrimitiveType::Cube:
        return "Cube";
    case PrimitiveType::Sphere:
        return "Sphere";
    default:
        return "Unknown";
    }
}

void DrawVec3Control(const char* label, glm::vec3& values, float resetValue = 0.0f) {
    ImGui::PushID(label);
    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, 100.0f);
    ImGui::TextUnformatted(label);
    ImGui::NextColumn();

    const float controlWidth = ImGui::CalcItemWidth() / 3.0f - 6.0f;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

    const float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
    const ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

    if (ImGui::Button("X", buttonSize)) values.x = resetValue;
    ImGui::SameLine();
    ImGui::PushItemWidth(controlWidth);
    ImGui::DragFloat("##X", &values.x, 0.1f);
    ImGui::PopItemWidth();
    ImGui::SameLine();

    if (ImGui::Button("Y", buttonSize)) values.y = resetValue;
    ImGui::SameLine();
    ImGui::PushItemWidth(controlWidth);
    ImGui::DragFloat("##Y", &values.y, 0.1f);
    ImGui::PopItemWidth();
    ImGui::SameLine();

    if (ImGui::Button("Z", buttonSize)) values.z = resetValue;
    ImGui::SameLine();
    ImGui::PushItemWidth(controlWidth);
    ImGui::DragFloat("##Z", &values.z, 0.1f);
    ImGui::PopItemWidth();

    ImGui::PopStyleVar();
    ImGui::Columns(1);
    ImGui::PopID();
}

const char* NormalSourceLabel(NormalSource source) {
    switch (source) {
    case NormalSource::Derivative:
        return "Derivative";
    case NormalSource::Vertex:
        return "Vertex";
    default:
        return "Unknown";
    }
}

const char* AATechniqueLabel(AATechnique technique) {
    switch (technique) {
    case AATechnique::Off:
        return "Off";
    case AATechnique::MSAA:
        return "MSAA";
    default:
        return "Unknown";
    }
}

const char* ImportGroupModeLabel(ImportGroupMode mode) {
    switch (mode) {
    case ImportGroupMode::PreserveGroups:
        return "Preserve Groups";
    case ImportGroupMode::GroupByMaterial:
        return "Group By Material";
    case ImportGroupMode::MergeAll:
        return "Merge All";
    default:
        return "Unknown";
    }
}

bool FileExists(const std::string& path) {
    return !path.empty() && std::filesystem::exists(path);
}

std::string GetDisplayFileName(const std::string& path) {
    if (path.empty()) {
        return "None";
    }
    return std::filesystem::path(path).filename().string();
}

Entity FindEntityByUUID(const Ref<Scene>& scene, const UUID& uuid) {
    if (!scene) {
        return {};
    }

    auto view = scene->GetAllEntitiesViewWith<TagComponent>();
    for (auto entityID : view) {
        if (view.get<TagComponent>(entityID).id == uuid) {
            return Entity{entityID, scene.get()};
        }
    }

    return {};
}

Entity FindEntityById(const Ref<Scene>& scene, uint32_t entityId) {
    if (!scene) {
        return {};
    }

    auto view = scene->GetAllEntitiesViewWith<TagComponent>();
    for (auto entityHandle : view) {
        if (static_cast<uint32_t>(entityHandle) == entityId) {
            return Entity{entityHandle, scene.get()};
        }
    }

    return {};
}

bool IsDescendant(const Ref<Scene>& scene, Entity ancestor, Entity maybeDescendant) {
    if (!scene || !ancestor || !maybeDescendant || !maybeDescendant.HasComponent<HierarchyComponent>()) {
        return false;
    }

    const UUID ancestorUuid = ancestor.GetComponent<TagComponent>().id;
    UUID parentUuid = maybeDescendant.GetComponent<HierarchyComponent>().parent;
    while (static_cast<uint64_t>(parentUuid) != 0) {
        if (parentUuid == ancestorUuid) {
            return true;
        }
        Entity parent = FindEntityByUUID(scene, parentUuid);
        if (!parent || !parent.HasComponent<HierarchyComponent>()) {
            break;
        }
        parentUuid = parent.GetComponent<HierarchyComponent>().parent;
    }

    return false;
}

} // namespace

SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& scene) {
    SetContext(scene);
}

void SceneHierarchyPanel::SetContext(const Ref<Scene>& scene) {
    if (m_Context.get() != scene.get()) {
        m_Context = scene;
        m_SelectionContext = {};
    }
}

void SceneHierarchyPanel::SelectEntityByUUID(UUID uuid) {
    m_SelectionContext = FindEntityByUUID(m_Context, uuid);
}

void SceneHierarchyPanel::OnImGuiRender() {
    ImGui::Begin("Hierarchy");

    const ImVec2 windowPos = ImGui::GetWindowPos();
    const ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
    const ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
    const ImVec2 dropMin(windowPos.x + contentMin.x, windowPos.y + contentMin.y);
    const ImVec2 dropMax(windowPos.x + contentMax.x, windowPos.y + contentMax.y);

    if (ImGui::BeginDragDropTargetCustom(ImRect(dropMin, dropMax), ImGui::GetID("##obj-drop-target"))) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("UPLOADED_OBJ_TEMPLATE")) {
            const char* pathData = static_cast<const char*>(payload->Data);
            if (pathData && payload->DataSize > 0) {
                std::filesystem::path sourcePath(pathData);
                SpawnObjFromPath(sourcePath, sourcePath.stem().string());
            }
        }
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_ENTITY")) {
            if (payload->DataSize == sizeof(uint32_t)) {
                const uint32_t draggedEntityId = *static_cast<const uint32_t*>(payload->Data);
                if (World::SetEntityParent(draggedEntityId, 0)) {
                    m_ObjStatusMessage = "Moved entity to root.";
                    m_ObjStatusIsError = false;
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (!m_ObjStatusMessage.empty()) {
        if (m_ObjStatusIsError) {
            ImGui::TextColored(ImVec4(0.95f, 0.3f, 0.3f, 1.0f), "%s", m_ObjStatusMessage.c_str());
        } else {
            ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "%s", m_ObjStatusMessage.c_str());
        }
    }

    if (m_Context) {
        if (m_SelectionContext) {
            auto tagView = m_Context->GetAllEntitiesViewWith<TagComponent>();
            if (!tagView.contains(static_cast<entt::entity>(m_SelectionContext))) {
                m_SelectionContext = {};
            }
        }

        auto view = m_Context->GetAllEntitiesViewWith<TagComponent, HierarchyComponent>();
        for (auto entityID : view) {
            Entity entity{entityID, m_Context.get()};
            const auto& hierarchy = entity.GetComponent<HierarchyComponent>();
            const UUID parentUuid = hierarchy.parent;
            if (static_cast<uint64_t>(parentUuid) == 0 || !FindEntityByUUID(m_Context, parentUuid)) {
                DrawEntityNode(entity);
            }
        }

        if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered()) {
            m_SelectionContext = {};
        }

        if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_F2) && m_SelectionContext) {
            BeginRenameEntity(m_SelectionContext);
        }
    }

    if (ImGui::BeginPopupContextWindow("HierarchyCreateContext", ImGuiPopupFlags_NoOpenOverItems)) {
        EditorPlacement::DrawCreateMenu();
        ImGui::EndPopup();
    }

    if (m_OpenRenamePopup) {
        ImGui::OpenPopup("Rename Object");
        m_OpenRenamePopup = false;
    }
    if (ImGui::BeginPopupModal("Rename Object", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("Name", m_RenameBuffer, sizeof(m_RenameBuffer));
        if (ImGui::Button("Rename")) {
            if (m_RenameEntity && m_RenameEntity.HasComponent<TagComponent>() && m_RenameBuffer[0] != '\0') {
                m_RenameEntity.GetComponent<TagComponent>().tag = m_RenameBuffer;
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::End();

    ImGui::Begin("Properties");
    if (m_SelectionContext) {
        DrawProperties(m_SelectionContext);
    }
    ImGui::End();
}

void SceneHierarchyPanel::BeginRenameEntity(Entity entity) {
    if (!entity || !entity.HasComponent<TagComponent>()) {
        return;
    }

    m_RenameEntity = entity;
    std::snprintf(m_RenameBuffer, sizeof(m_RenameBuffer), "%s", entity.GetComponent<TagComponent>().tag.c_str());
    m_OpenRenamePopup = true;
}

bool SceneHierarchyPanel::SpawnObjFromPath(const std::filesystem::path& sourcePath, const std::string& displayName) {
    if (!std::filesystem::exists(sourcePath)) {
        m_ObjStatusMessage = "Create failed: source model missing.";
        m_ObjStatusIsError = true;
        return false;
    }

    ImportedModelData model;
    std::string error;
    if (!AssetImporter::ImportModel(sourcePath.string(), model, &error)) {
        m_ObjStatusMessage = "Create failed: " + error;
        m_ObjStatusIsError = true;
        return false;
    }

    std::unordered_map<int, uint32_t> importerMatToWorldMat;
    importerMatToWorldMat.reserve(model.materials.size());
    for (size_t i = 0; i < model.materials.size(); ++i) {
        const ImportedMaterialData& material = model.materials[i];
        const uint32_t materialId = World::CreateMaterial(material.name.empty() ? "Imported Material" : material.name);
        World::SetMaterialSurfaceFactors(materialId, material.roughnessFactor, material.metallicFactor);
        World::SetMaterialColors(materialId, MaterialColors{material.baseColor, material.emissiveColor, material.emissiveEnabled});
        if (FileExists(material.albedoPath)) {
            World::SetMaterialTexturePath(materialId, TextureSlot::Albedo, material.albedoPath);
        }
        if (FileExists(material.normalPath)) {
            World::SetMaterialTexturePath(materialId, TextureSlot::Normal, material.normalPath);
        }
        if (FileExists(material.roughnessPath)) {
            World::SetMaterialTexturePath(materialId, TextureSlot::Roughness, material.roughnessPath);
        }
        if (FileExists(material.ambientOcclusionPath)) {
            World::SetMaterialTexturePath(materialId, TextureSlot::AmbientOcclusion, material.ambientOcclusionPath);
        }
        if (FileExists(material.emissivePath)) {
            World::SetMaterialTexturePath(materialId, TextureSlot::Emissive, material.emissivePath);
        }
        importerMatToWorldMat.emplace(static_cast<int>(i), materialId);
    }

    const std::string rootName = displayName.empty() ? sourcePath.stem().string() : displayName;
    const uint32_t rootEntityId = World::CreateEmptyObject(rootName);
    m_Context = World::GetActiveScene();
    Entity rootEntity = FindEntityById(m_Context, rootEntityId);
    if (!rootEntity || !rootEntity.HasComponent<HierarchyComponent>()) {
        m_ObjStatusMessage = "Create failed: root object not found.";
        m_ObjStatusIsError = true;
        return false;
    }

    if (!rootEntity.HasComponent<ImportedModelComponent>()) {
        rootEntity.AddComponent<ImportedModelComponent>();
    }
    auto& importedModel = rootEntity.GetComponent<ImportedModelComponent>();
    importedModel.sourcePath = sourcePath.string();
    importedModel.mode = ImportGroupMode::PreserveGroups;
    importedModel.grouped = true;

    auto& rootHierarchy = rootEntity.GetComponent<HierarchyComponent>();
    rootHierarchy.children.clear();
    const UUID rootUuid = rootEntity.GetComponent<TagComponent>().id;

    uint32_t spawnedCount = 0;
    for (const auto& meshData : model.meshes) {
        try {
            Ref<Mesh> mesh = CreateRef<Mesh>(Renderer::GetDevice(), meshData.vertices, meshData.indices);
            SpawnTransform spawn{};
            const uint32_t entityId = World::SpawnMesh(mesh, spawn, meshData.name.empty() ? rootName : meshData.name, meshData.hasNormals);
            Entity childEntity = FindEntityById(m_Context, entityId);
            if (childEntity && childEntity.HasComponent<HierarchyComponent>() && childEntity.HasComponent<TagComponent>()) {
                auto& childHierarchy = childEntity.GetComponent<HierarchyComponent>();
                childHierarchy.parent = rootUuid;
                rootHierarchy.children.push_back(childEntity.GetComponent<TagComponent>().id);
                auto itMat = importerMatToWorldMat.find(meshData.materialIndex);
                if (itMat != importerMatToWorldMat.end()) {
                    World::SetEntityMaterial(entityId, itMat->second);
                }
                ++spawnedCount;
            }
        } catch (const std::exception& ex) {
            PIECE_ERROR("Failed to create GPU mesh for {}: {}", meshData.name, ex.what());
        }
    }

    if (spawnedCount == 0) {
        World::DestroyEntity(rootEntityId);
        m_ObjStatusMessage = "Create failed: no meshes were spawned.";
        m_ObjStatusIsError = true;
        return false;
    }

    m_ObjStatusMessage = "Created model root with " + std::to_string(spawnedCount) + " mesh child(ren).";
    m_ObjStatusIsError = false;
    return true;
}

bool SceneHierarchyPanel::MergeAllChildren(Entity rootEntity) {
    if (!rootEntity || !rootEntity.HasComponent<HierarchyComponent>()) {
        return false;
    }

    if (!rootEntity.HasComponent<ImportedModelComponent>()) {
        m_ObjStatusMessage = "Merge failed: selected root is not an imported model.";
        m_ObjStatusIsError = true;
        return false;
    }

    const auto& imported = rootEntity.GetComponent<ImportedModelComponent>();
    if (imported.sourcePath.empty()) {
        m_ObjStatusMessage = "Merge failed: imported source path is missing.";
        m_ObjStatusIsError = true;
        return false;
    }

    ImportedModelData model;
    std::string importError;
    if (!AssetImporter::ImportModel(imported.sourcePath, model, &importError)) {
        m_ObjStatusMessage = "Merge failed: " + importError;
        m_ObjStatusIsError = true;
        return false;
    }

    struct MergeBucket {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        bool hasNormals{false};
    };

    std::unordered_map<int, MergeBucket> buckets;
    std::vector<int> bucketOrder;
    for (const auto& meshData : model.meshes) {
        auto [it, inserted] = buckets.emplace(meshData.materialIndex, MergeBucket{});
        if (inserted) {
            bucketOrder.push_back(meshData.materialIndex);
        }

        MergeBucket& bucket = it->second;
        const uint32_t baseVertex = static_cast<uint32_t>(bucket.vertices.size());
        bucket.vertices.insert(bucket.vertices.end(), meshData.vertices.begin(), meshData.vertices.end());
        for (uint32_t index : meshData.indices) {
            bucket.indices.push_back(baseVertex + index);
        }
        bucket.hasNormals = bucket.hasNormals || meshData.hasNormals;
    }

    if (bucketOrder.empty()) {
        m_ObjStatusMessage = "Merge failed: model has no mesh data to merge.";
        m_ObjStatusIsError = true;
        return false;
    }

    std::unordered_map<int, uint32_t> importerMatToWorldMat;
    importerMatToWorldMat.reserve(model.materials.size());
    for (size_t i = 0; i < model.materials.size(); ++i) {
        const ImportedMaterialData& material = model.materials[i];
        const uint32_t materialId = World::CreateMaterial(material.name.empty() ? "Imported Material" : material.name);
        World::SetMaterialSurfaceFactors(materialId, material.roughnessFactor, material.metallicFactor);
        World::SetMaterialColors(materialId, MaterialColors{material.baseColor, material.emissiveColor, material.emissiveEnabled});
        if (FileExists(material.albedoPath)) {
            World::SetMaterialTexturePath(materialId, TextureSlot::Albedo, material.albedoPath);
        }
        if (FileExists(material.normalPath)) {
            World::SetMaterialTexturePath(materialId, TextureSlot::Normal, material.normalPath);
        }
        if (FileExists(material.roughnessPath)) {
            World::SetMaterialTexturePath(materialId, TextureSlot::Roughness, material.roughnessPath);
        }
        if (FileExists(material.ambientOcclusionPath)) {
            World::SetMaterialTexturePath(materialId, TextureSlot::AmbientOcclusion, material.ambientOcclusionPath);
        }
        if (FileExists(material.emissivePath)) {
            World::SetMaterialTexturePath(materialId, TextureSlot::Emissive, material.emissivePath);
        }
        importerMatToWorldMat.emplace(static_cast<int>(i), materialId);
    }

    auto& rootHierarchy = rootEntity.GetComponent<HierarchyComponent>();
    std::vector<UUID> childUuids = rootHierarchy.children;
    rootHierarchy.children.clear();
    for (const UUID& childUuid : childUuids) {
        Entity child = FindEntityByUUID(m_Context, childUuid);
        if (child) {
            World::DestroyEntity(static_cast<uint32_t>(child));
        }
    }

    const UUID rootUuid = rootEntity.GetComponent<TagComponent>().id;
    uint32_t mergedChildCount = 0;
    for (int materialIndex : bucketOrder) {
        auto itBucket = buckets.find(materialIndex);
        if (itBucket == buckets.end()) {
            continue;
        }

        const MergeBucket& bucket = itBucket->second;
        if (bucket.vertices.empty() || bucket.indices.empty()) {
            continue;
        }

        Ref<Mesh> mergedMesh;
        try {
            mergedMesh = CreateRef<Mesh>(Renderer::GetDevice(), bucket.vertices, bucket.indices);
        } catch (const std::exception& ex) {
            m_ObjStatusMessage = std::string("Merge failed: ") + ex.what();
            m_ObjStatusIsError = true;
            return false;
        }

        std::string mergedName = rootEntity.GetComponent<TagComponent>().tag + "_Merged";
        if (materialIndex >= 0 && materialIndex < static_cast<int>(model.materials.size())) {
            mergedName += " [" + model.materials[materialIndex].name + "]";
        }

        const uint32_t mergedEntityId = World::SpawnMesh(mergedMesh, SpawnTransform{}, mergedName, bucket.hasNormals);
        Entity mergedEntity = FindEntityById(m_Context, mergedEntityId);
        if (!mergedEntity || !mergedEntity.HasComponent<HierarchyComponent>() || !mergedEntity.HasComponent<TagComponent>()) {
            m_ObjStatusMessage = "Merge failed: could not create merged child entity.";
            m_ObjStatusIsError = true;
            return false;
        }

        auto& mergedHierarchy = mergedEntity.GetComponent<HierarchyComponent>();
        mergedHierarchy.parent = rootUuid;
        rootHierarchy.children.push_back(mergedEntity.GetComponent<TagComponent>().id);

        auto itMat = importerMatToWorldMat.find(materialIndex);
        if (itMat != importerMatToWorldMat.end()) {
            World::SetEntityMaterial(mergedEntityId, itMat->second);
        }
        ++mergedChildCount;
    }

    if (mergedChildCount == 0) {
        m_ObjStatusMessage = "Merge failed: no merged child meshes were created.";
        m_ObjStatusIsError = true;
        return false;
    }

    if (rootEntity.HasComponent<ImportedModelComponent>()) {
        auto& rootImported = rootEntity.GetComponent<ImportedModelComponent>();
        rootImported.mode = (mergedChildCount > 1) ? ImportGroupMode::GroupByMaterial : ImportGroupMode::MergeAll;
        rootImported.grouped = true;
    }

    if (mergedChildCount > 1) {
        m_ObjStatusMessage = "Merged children by material to preserve textures (" + std::to_string(mergedChildCount) + " merged child meshes).";
    } else {
        m_ObjStatusMessage = "Merged children into one child mesh under root.";
    }
    m_ObjStatusIsError = false;
    return true;
}

bool SceneHierarchyPanel::RestoreImportedChildren(Entity rootEntity) {
    if (!rootEntity || !rootEntity.HasComponent<HierarchyComponent>() || !rootEntity.HasComponent<ImportedModelComponent>()) {
        return false;
    }

    const auto& imported = rootEntity.GetComponent<ImportedModelComponent>();
    if (imported.sourcePath.empty()) {
        m_ObjStatusMessage = "Unmerge failed: imported source path is missing.";
        m_ObjStatusIsError = true;
        return false;
    }

    ImportedModelData model;
    std::string error;
    if (!AssetImporter::ImportModel(imported.sourcePath, model, &error)) {
        m_ObjStatusMessage = "Unmerge failed: " + error;
        m_ObjStatusIsError = true;
        return false;
    }

    auto& rootHierarchy = rootEntity.GetComponent<HierarchyComponent>();
    std::vector<UUID> currentChildren = rootHierarchy.children;
    rootHierarchy.children.clear();
    for (const UUID& childUuid : currentChildren) {
        Entity child = FindEntityByUUID(m_Context, childUuid);
        if (child) {
            World::DestroyEntity(static_cast<uint32_t>(child));
        }
    }

    std::unordered_map<int, uint32_t> importerMatToWorldMat;
    importerMatToWorldMat.reserve(model.materials.size());
    for (size_t i = 0; i < model.materials.size(); ++i) {
        const ImportedMaterialData& material = model.materials[i];
        const uint32_t materialId = World::CreateMaterial(material.name.empty() ? "Imported Material" : material.name);
        World::SetMaterialSurfaceFactors(materialId, material.roughnessFactor, material.metallicFactor);
        World::SetMaterialColors(materialId, MaterialColors{material.baseColor, material.emissiveColor, material.emissiveEnabled});
        if (FileExists(material.albedoPath)) {
            World::SetMaterialTexturePath(materialId, TextureSlot::Albedo, material.albedoPath);
        }
        if (FileExists(material.normalPath)) {
            World::SetMaterialTexturePath(materialId, TextureSlot::Normal, material.normalPath);
        }
        if (FileExists(material.roughnessPath)) {
            World::SetMaterialTexturePath(materialId, TextureSlot::Roughness, material.roughnessPath);
        }
        if (FileExists(material.ambientOcclusionPath)) {
            World::SetMaterialTexturePath(materialId, TextureSlot::AmbientOcclusion, material.ambientOcclusionPath);
        }
        if (FileExists(material.emissivePath)) {
            World::SetMaterialTexturePath(materialId, TextureSlot::Emissive, material.emissivePath);
        }
        importerMatToWorldMat.emplace(static_cast<int>(i), materialId);
    }

    const UUID rootUuid = rootEntity.GetComponent<TagComponent>().id;
    const std::string rootName = rootEntity.GetComponent<TagComponent>().tag;
    uint32_t spawnedCount = 0;
    for (const auto& meshData : model.meshes) {
        try {
            Ref<Mesh> mesh = CreateRef<Mesh>(Renderer::GetDevice(), meshData.vertices, meshData.indices);
            const uint32_t entityId = World::SpawnMesh(mesh, SpawnTransform{}, meshData.name.empty() ? rootName : meshData.name, meshData.hasNormals);
            Entity childEntity = FindEntityById(m_Context, entityId);
            if (!childEntity || !childEntity.HasComponent<HierarchyComponent>() || !childEntity.HasComponent<TagComponent>()) {
                continue;
            }

            auto& childHierarchy = childEntity.GetComponent<HierarchyComponent>();
            childHierarchy.parent = rootUuid;
            rootHierarchy.children.push_back(childEntity.GetComponent<TagComponent>().id);

            auto itMat = importerMatToWorldMat.find(meshData.materialIndex);
            if (itMat != importerMatToWorldMat.end()) {
                World::SetEntityMaterial(entityId, itMat->second);
            }
            ++spawnedCount;
        } catch (const std::exception& ex) {
            PIECE_ERROR("Failed to recreate child mesh during unmerge for {}: {}", meshData.name, ex.what());
        }
    }

    if (spawnedCount == 0) {
        m_ObjStatusMessage = "Unmerge failed: no child meshes were restored.";
        m_ObjStatusIsError = true;
        return false;
    }

    auto& importedMutable = rootEntity.GetComponent<ImportedModelComponent>();
    importedMutable.mode = ImportGroupMode::PreserveGroups;
    importedMutable.grouped = true;
    m_ObjStatusMessage = "Unmerged: restored " + std::to_string(spawnedCount) + " grouped child mesh(es).";
    m_ObjStatusIsError = false;
    return true;
}

void SceneHierarchyPanel::DrawEntityNode(Entity entity) {
    auto& tag = entity.GetComponent<TagComponent>().tag;
    const uint32_t thisEntityId = static_cast<uint32_t>(entity);
    const auto& hierarchy = entity.GetComponent<HierarchyComponent>();
    const bool hasChildren = !hierarchy.children.empty();
    ImGuiTreeNodeFlags flags = ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0)
        | ImGuiTreeNodeFlags_OpenOnArrow
        | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (!hasChildren) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    const bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, "%s", tag.c_str());

    if (ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload("HIERARCHY_ENTITY", &thisEntityId, sizeof(uint32_t));
        ImGui::Text("Move: %s", tag.c_str());
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_ENTITY")) {
            if (payload->DataSize == sizeof(uint32_t)) {
                const uint32_t draggedEntityId = *static_cast<const uint32_t*>(payload->Data);
                if (draggedEntityId != thisEntityId) {
                    Entity dragged = FindEntityById(m_Context, draggedEntityId);
                    const bool cycle = dragged && IsDescendant(m_Context, dragged, entity);
                    if (!cycle) {
                        if (World::SetEntityParent(draggedEntityId, thisEntityId)) {
                            m_ObjStatusMessage = "Reparented entity.";
                            m_ObjStatusIsError = false;
                        }
                    } else {
                        m_ObjStatusMessage = "Reparent failed: cannot parent to a descendant.";
                        m_ObjStatusIsError = true;
                    }
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (ImGui::IsItemClicked()) {
        m_SelectionContext = entity;
    }

    bool entityDeleted = false;
    bool doMergeAllChildren = false;
    bool doRestoreChildren = false;
    if (ImGui::BeginPopupContextItem()) {
        EditorPlacement::DrawCreateMenu();
        ImGui::Separator();
        if (entity.HasComponent<ImportedModelComponent>() && entity.HasComponent<HierarchyComponent>()) {
            const auto& imported = entity.GetComponent<ImportedModelComponent>();
            const auto& hierarchy = entity.GetComponent<HierarchyComponent>();
            if (!hierarchy.children.empty()) {
                if (ImGui::MenuItem("Merge All Children")) {
                    doMergeAllChildren = true;
                }
                if (imported.mode != ImportGroupMode::PreserveGroups) {
                    if (ImGui::MenuItem("Unmerge (Restore Groups)")) {
                        doRestoreChildren = true;
                    }
                }
                ImGui::Separator();
            }
        }

        if (ImGui::MenuItem("Delete")) {
            entityDeleted = true;
        }
        ImGui::EndPopup();
    }

    if (doMergeAllChildren) {
        MergeAllChildren(entity);
    }
    if (doRestoreChildren) {
        RestoreImportedChildren(entity);
    }

    if (opened && hasChildren) {
        for (const UUID& childUuid : hierarchy.children) {
            Entity child = FindEntityByUUID(m_Context, childUuid);
            if (child) {
                DrawEntityNode(child);
            }
        }
        ImGui::TreePop();
    }

    if (entityDeleted) {
        if (m_SelectionContext == entity) {
            m_SelectionContext = {};
        }
        World::DestroyEntity(static_cast<uint32_t>(entity));
    }
}

void SceneHierarchyPanel::DrawProperties(Entity entity) {
    if (entity.HasComponent<TagComponent>()) {
        auto& tag = entity.GetComponent<TagComponent>().tag;

        char buffer[256] = {};
        std::snprintf(buffer, sizeof(buffer), "%s", tag.c_str());
        if (ImGui::InputText("Tag", buffer, sizeof(buffer))) {
            tag = std::string(buffer);
        }
    }

    if (entity.HasComponent<TransformComponent>()) {
        if (ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
            auto& tc = entity.GetComponent<TransformComponent>();
            DrawVec3Control("Position", tc.position);
            DrawVec3Control("Rotation", tc.rotation);
            DrawVec3Control("Scale", tc.scale, 1.0f);
            ImGui::TreePop();
        }
    }

    if (entity.HasComponent<ImportedModelComponent>()) {
        if (ImGui::TreeNodeEx("Imported Model", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
            const auto& imported = entity.GetComponent<ImportedModelComponent>();
            ImGui::Text("Mode: %s", ImportGroupModeLabel(imported.mode));
            ImGui::Text("Grouped: %s", imported.grouped ? "Yes" : "No");
            ImGui::TextWrapped("Source: %s", imported.sourcePath.c_str());

            const bool hasChildren = entity.HasComponent<HierarchyComponent>()
                && !entity.GetComponent<HierarchyComponent>().children.empty();
            if (hasChildren) {
                if (ImGui::Button("Merge All Children")) {
                    MergeAllChildren(entity);
                }
                if (imported.mode != ImportGroupMode::PreserveGroups) {
                    ImGui::SameLine();
                    if (ImGui::Button("Unmerge (Restore Groups)")) {
                        RestoreImportedChildren(entity);
                    }
                }
            }

            ImGui::TreePop();
        }
    }

    if (entity.HasComponent<MeshRendererComponent>()) {
        if (ImGui::TreeNodeEx("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
            auto& mr = entity.GetComponent<MeshRendererComponent>();
            ImGui::Text("Primitive: %s", PrimitiveTypeLabel(mr.primitiveType));
            ImGui::Text("Normal Source: %s", NormalSourceLabel(mr.normalSource));
            ImGui::Text("Custom Mesh: %s", mr.mesh ? "Yes" : "No");

            if (!entity.HasComponent<MaterialComponent>()) {
                ImGui::TextDisabled("No material assigned");
                if (ImGui::Button("Assign Default Material")) {
                    World::SetEntityMaterial(static_cast<uint32_t>(entity), World::GetDefaultMaterialId());
                }
            }

            ImGui::TreePop();
        }
    }

    if (entity.HasComponent<MaterialComponent>()) {
        if (ImGui::TreeNodeEx("Material", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
            auto& materialComponent = entity.GetComponent<MaterialComponent>();

            auto materials = World::GetMaterials();
            if (materials.empty()) {
                if (ImGui::Button("Assign Default Material")) {
                    const uint32_t materialId = World::GetDefaultMaterialId();
                    materialComponent.materialId = materialId;
                    World::SetEntityMaterial(static_cast<uint32_t>(entity), materialId);
                }
            } else {
                int currentMaterialIndex = -1;
                for (size_t i = 0; i < materials.size(); ++i) {
                    if (materials[i].id == materialComponent.materialId) {
                        currentMaterialIndex = static_cast<int>(i);
                        break;
                    }
                }

                const char* preview = "None";
                if (currentMaterialIndex >= 0) {
                    preview = materials[static_cast<size_t>(currentMaterialIndex)].name.c_str();
                }

                ImGui::TextUnformatted("Material Slot");
                if (ImGui::BeginCombo("##MaterialSlot", preview)) {
                    for (size_t i = 0; i < materials.size(); ++i) {
                        const bool selected = currentMaterialIndex == static_cast<int>(i);
                        if (ImGui::Selectable(materials[i].name.c_str(), selected)) {
                            materialComponent.materialId = materials[i].id;
                            World::SetEntityMaterial(static_cast<uint32_t>(entity), materials[i].id);
                        }
                    }
                    ImGui::EndCombo();
                }
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MATERIAL_ASSET")) {
                        if (payload->DataSize == sizeof(uint32_t)) {
                            const uint32_t materialId = *static_cast<const uint32_t*>(payload->Data);
                            materialComponent.materialId = materialId;
                            World::SetEntityMaterial(static_cast<uint32_t>(entity), materialId);
                        }
                    }
                    ImGui::EndDragDropTarget();
                }
                ImGui::SameLine();
                ImGui::TextDisabled("Drop a material card here");

                if (currentMaterialIndex >= 0) {
                    MaterialView& selectedMaterial = materials[static_cast<size_t>(currentMaterialIndex)];
                    const char* imageFilter = "Image Files\0*.png;*.jpg;*.jpeg;*.bmp;*.tga;*.psd\0All Files\0*.*\0";

                    ImGui::TextDisabled("Surface response comes from material textures.");
                    glm::vec3 baseColor = materialComponent.colors.baseColor;
                    glm::vec3 emissiveColor = materialComponent.colors.emissiveColor;
                    float roughnessFactor = selectedMaterial.surfaceFactors.roughnessFactor;
                    float metallicFactor = selectedMaterial.surfaceFactors.metallicFactor;
                    if (m_SurfaceFactorMaterialId != selectedMaterial.id) {
                        m_SurfaceFactorMaterialId = selectedMaterial.id;
                        m_EditSurfaceFactors = false;
                    }
                    bool emissiveEnabled = materialComponent.colors.emissiveEnabled;
                    bool hdrBloomEnabled = materialComponent.colors.hdrBloomEnabled;
                    bool emissiveBloomEnabled = materialComponent.colors.emissiveBloomEnabled;
                    bool colorsChanged = ImGui::ColorEdit3("Base Color", &baseColor.x);
                    colorsChanged |= ImGui::Checkbox("Emissive Enabled", &emissiveEnabled);
                    float bloomThreshold = materialComponent.colors.bloomThreshold;
                    float bloomIntensity = materialComponent.colors.bloomIntensity;
                    float bloomRadius = materialComponent.colors.bloomRadius;
                    colorsChanged |= ImGui::Checkbox("HDR Bloom Enabled", &hdrBloomEnabled);
                    colorsChanged |= ImGui::DragFloat("Bloom Threshold", &bloomThreshold, 0.01f, 0.0f, 10.0f);
                    colorsChanged |= ImGui::DragFloat("Bloom Intensity", &bloomIntensity, 0.01f, 0.0f, 5.0f);
                    colorsChanged |= ImGui::DragFloat("Bloom Radius", &bloomRadius, 0.05f, 0.0f, 8.0f);
                    ImGui::Checkbox("Edit Surface Factors", &m_EditSurfaceFactors);
                    if (m_EditSurfaceFactors) {
                        if (ImGui::DragFloat("Roughness Factor", &roughnessFactor, 0.01f, 0.0f, 1.0f)) {
                            World::SetMaterialSurfaceFactors(selectedMaterial.id, roughnessFactor, metallicFactor);
                        }
                        if (ImGui::DragFloat("Metallic Factor", &metallicFactor, 0.01f, 0.0f, 1.0f)) {
                            World::SetMaterialSurfaceFactors(selectedMaterial.id, roughnessFactor, metallicFactor);
                        }
                    }
                    if (emissiveEnabled) {
                        colorsChanged |= ImGui::Checkbox("Emissive Bloom Enabled", &emissiveBloomEnabled);
                        colorsChanged |= ImGui::ColorEdit3("Emissive Color", &emissiveColor.x);
                    }
                    if (colorsChanged) {
                        materialComponent.colors = MaterialColors{
                            baseColor,
                            emissiveColor,
                            emissiveEnabled,
                            hdrBloomEnabled,
                            emissiveBloomEnabled,
                            bloomThreshold,
                            bloomIntensity,
                            bloomRadius};
                    }
                    ImGui::Separator();

                    const auto drawTextureSlot = [&](const char* label, const std::string& path, TextureSlot slot) {
                        ImGui::PushID(label);
                        ImGui::Text("%s", label);
                        ImGui::SameLine();
                        ImGui::TextDisabled("%s", GetDisplayFileName(path).c_str());
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Browse")) {
                            const uint32_t materialId = selectedMaterial.id;
                            Platform::OpenFileDialogAsync(imageFilter, [materialId, slot](std::string selectedPath) {
                                World::SetMaterialTexturePath(materialId, slot, selectedPath);
                                if (slot == TextureSlot::Metallic) {
                                    const MaterialSurfaceFactors factors = World::ResolveMaterialSurfaceFactors(materialId);
                                    World::SetMaterialSurfaceFactors(materialId, factors.roughnessFactor, 1.0f);
                                }
                            });
                        }
                        ImGui::PopID();
                    };

                    drawTextureSlot("Albedo", selectedMaterial.textures.albedoPath, TextureSlot::Albedo);
                    drawTextureSlot("Normal", selectedMaterial.textures.normalPath, TextureSlot::Normal);
                    drawTextureSlot("Height", selectedMaterial.textures.heightPath, TextureSlot::Height);
                    drawTextureSlot("Roughness", selectedMaterial.textures.roughnessPath, TextureSlot::Roughness);
                    drawTextureSlot("Metallic", selectedMaterial.textures.metallicPath, TextureSlot::Metallic);
                    drawTextureSlot("Ambient Occlusion", selectedMaterial.textures.ambientOcclusionPath, TextureSlot::AmbientOcclusion);
                }
            }

            ImGui::TreePop();
        }
    }

    if (entity.HasComponent<DirectionalLightComponent>()) {
        if (ImGui::TreeNodeEx("Directional Light", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
            auto& light = entity.GetComponent<DirectionalLightComponent>();
            ImGui::DragFloat3("Direction", &light.direction.x, 0.02f, -1.0f, 1.0f);
            ImGui::ColorEdit3("Color", &light.color.x);
            ImGui::DragFloat("Intensity", &light.intensity, 0.05f, 0.0f, 100.0f);
            ImGui::TreePop();
        }
    }

    if (entity.HasComponent<PointLightComponent>()) {
        if (ImGui::TreeNodeEx("Point Light", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
            auto& light = entity.GetComponent<PointLightComponent>();
            ImGui::ColorEdit3("Color", &light.color.x);
            ImGui::DragFloat("Intensity", &light.intensity, 0.05f, 0.0f, 100.0f);
            ImGui::DragFloat("Outer Radius", &light.radius, 0.05f, 0.01f, 100.0f);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Outer distance where point-light influence fades to zero.");
            }
            ImGui::TreePop();
        }
    }

    if (entity.HasComponent<SpotLightComponent>()) {
        if (ImGui::TreeNodeEx("Spot Light", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
            auto& light = entity.GetComponent<SpotLightComponent>();
            ImGui::DragFloat3("Direction", &light.direction.x, 0.02f, -1.0f, 1.0f);
            ImGui::ColorEdit3("Color", &light.color.x);
            ImGui::DragFloat("Intensity", &light.intensity, 0.05f, 0.0f, 100.0f);
            ImGui::DragFloat("Inner Cutoff", &light.innerCutoffDegrees, 0.1f, 0.0f, 90.0f);
            ImGui::DragFloat("Outer Cutoff", &light.outerCutoffDegrees, 0.1f, 0.0f, 90.0f);
            ImGui::TreePop();
        }
    }

    if (entity.HasComponent<CameraComponent>()) {
        if (ImGui::TreeNodeEx("Camera", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
            auto& camera = entity.GetComponent<CameraComponent>();
            ImGui::Checkbox("Primary", &camera.primary);
            ImGui::Checkbox("Fixed Aspect", &camera.fixedAspectRatio);
            ImGui::TreePop();
        }
    }
}

} // namespace Piece

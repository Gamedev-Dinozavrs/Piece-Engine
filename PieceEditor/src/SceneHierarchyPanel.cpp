#include "SceneHierarchyPanel.h"

#include "EditorPlacement.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <assets/AssetImporter.h>
#include <core/KeyCodes.h>
#include <core/Log.h>
#include <renderer/Renderer.h>
#include <scene/Components.h>
#include <scene/Scene.h>
#include <scene/World.h>
#include <scripting/ScriptComponent.h>
#include <utils/platform/WindowsUtils.h>

#include <cstdio>
#include <exception>
#include <filesystem>
#include <algorithm>
#include <cmath>
#include <vector>

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

void SceneHierarchyPanel::SelectHierarchyRootByUUID(UUID uuid) {
    Entity entity = FindEntityByUUID(m_Context, uuid);
    while (entity && entity.HasComponent<HierarchyComponent>()) {
        const UUID parentUuid = entity.GetComponent<HierarchyComponent>().parent;
        if (static_cast<uint64_t>(parentUuid) == 0) {
            break;
        }
        Entity parent = FindEntityByUUID(m_Context, parentUuid);
        if (!parent) {
            break;
        }
        entity = parent;
    }
    m_SelectionContext = entity;
}

bool SceneHierarchyPanel::SpawnModelFromPath(const std::filesystem::path& sourcePath) {
    return SpawnObjFromPath(sourcePath, sourcePath.stem().string());
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

    if (model.meshes.empty() && !model.animations.empty()) {
        m_ObjStatusMessage = "Animation asset: create an Animator clip slot and drag this asset onto it.";
        m_ObjStatusIsError = false;
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
    if (!model.joints.empty()) {
        World::SetEntityAnimationData(rootEntityId, model.joints, model.animations);
    }

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
                World::SetEntityImportedModelInfo(entityId, sourcePath.string(), meshData.name);
                if (!model.joints.empty()) {
                    World::SetEntityAnimationData(entityId, model.joints, model.animations);
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
    m_SelectionContext = rootEntity;
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
            World::SetEntityImportedModelInfo(entityId, imported.sourcePath, meshData.name);
            if (!model.joints.empty()) {
                World::SetEntityAnimationData(entityId, model.joints, model.animations);
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

                    float normalScale = selectedMaterial.surfaceFactors.normalScale;
                    float occlusionStrength = selectedMaterial.surfaceFactors.occlusionStrength;
                    if (ImGui::DragFloat("Normal Strength", &normalScale, 0.01f, 0.0f, 2.0f)) {
                        World::SetMaterialSurfaceFactors(selectedMaterial.id, roughnessFactor, metallicFactor, normalScale, occlusionStrength);
                    }
                    if (ImGui::DragFloat("AO Strength", &occlusionStrength, 0.01f, 0.0f, 1.0f)) {
                        World::SetMaterialSurfaceFactors(selectedMaterial.id, roughnessFactor, metallicFactor, normalScale, occlusionStrength);
                    }

                    MaterialRenderSettings renderSettings = selectedMaterial.renderSettings;
                    const char* alphaModeLabels[] = {"Opaque", "Mask", "Blend"};
                    int alphaMode = static_cast<int>(renderSettings.alphaMode);
                    if (ImGui::Combo("Alpha Mode", &alphaMode, alphaModeLabels, 3)) {
                        renderSettings.alphaMode = static_cast<MaterialAlphaMode>(alphaMode);
                        World::SetMaterialRenderSettings(selectedMaterial.id, renderSettings);
                    }
                    if (renderSettings.alphaMode == MaterialAlphaMode::Mask
                        && ImGui::DragFloat("Alpha Cutoff", &renderSettings.alphaCutoff, 0.01f, 0.0f, 1.0f)) {
                        World::SetMaterialRenderSettings(selectedMaterial.id, renderSettings);
                    }
                    if (ImGui::Checkbox("Double Sided", &renderSettings.doubleSided)) {
                        World::SetMaterialRenderSettings(selectedMaterial.id, renderSettings);
                    }
                    if (ImGui::Checkbox("Unlit", &renderSettings.unlit)) {
                        World::SetMaterialRenderSettings(selectedMaterial.id, renderSettings);
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

    if (entity.HasComponent<EnvironmentComponent>()) {
        if (ImGui::TreeNodeEx("Environment", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
            auto& environment = entity.GetComponent<EnvironmentComponent>();
            ImGui::Checkbox("Enabled", &environment.enabled);
            ImGui::DragFloat("Intensity", &environment.intensity, 0.01f, 0.0f, 8.0f);
            ImGui::DragFloat("Diffuse Strength", &environment.diffuseStrength, 0.01f, 0.0f, 4.0f);
            ImGui::DragFloat("Specular Strength", &environment.specularStrength, 0.01f, 0.0f, 4.0f);
            ImGui::DragFloat("Ambient Strength", &environment.ambientStrength, 0.01f, 0.0f, 4.0f);
            ImGui::Text("HDRI: %s", GetDisplayFileName(environment.hdrPath).c_str());
            if (ImGui::SmallButton("Browse##EnvironmentHDR")) {
                Platform::OpenFileDialogAsync(
                    "Environment Maps\0*.hdr;*.exr;*.png;*.jpg;*.jpeg\0All Files\0*.*\0",
                    [](std::string path) {
                        if (!path.empty()) {
                            EnvironmentSettings updated = World::GetEnvironmentSettings();
                            updated.hdrPath = std::move(path);
                            updated.enabled = true;
                            World::SetEnvironmentSettings(updated);
                        }
                    });
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Clear##EnvironmentHDR")) {
                environment.hdrPath.clear();
            }
            ImGui::TreePop();
        }
    }

    if (entity.HasComponent<AnimatorComponent>()) {
        if (ImGui::TreeNodeEx("Animator", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
            auto& animator = entity.GetComponent<AnimatorComponent>();
            ImGui::Text("Joints: %zu", animator.joints.size());
            ImGui::Text("Clips: %zu", animator.clips.size());
            bool playing = animator.playing;
            if (ImGui::Checkbox("Playing", &playing)) {
                World::SetEntityAnimationPlayback(static_cast<uint32_t>(entity), playing, animator.speed);
            }
            float speed = animator.speed;
            if (ImGui::DragFloat("Speed", &speed, 0.01f, -4.0f, 4.0f)) {
                World::SetEntityAnimationPlayback(static_cast<uint32_t>(entity), animator.playing, speed);
            }

            if (animator.controller.states.empty() && !animator.clips.empty()) {
                ImGui::TextUnformatted("Quick Preview Clip (no graph yet)");
                animator.currentClip = std::min(
                    animator.currentClip,
                    static_cast<uint32_t>(animator.clips.size() - 1));
                std::vector<const char*> clipNames;
                clipNames.reserve(animator.clips.size());
                for (const auto& clip : animator.clips) {
                    clipNames.push_back(clip.name.c_str());
                }
                int selectedClip = static_cast<int>(animator.currentClip);
                if (ImGui::Combo("Clip", &selectedClip, clipNames.data(), static_cast<int>(clipNames.size()))) {
                    World::SetEntityAnimationPreviewClip(
                        static_cast<uint32_t>(entity), animator.clips[static_cast<size_t>(selectedClip)].name);
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("Remove Selected")) {
                    const std::string clipName = animator.clips[animator.currentClip].name;
                    World::RemoveEntityAnimationClip(static_cast<uint32_t>(entity), clipName);
                }
                if (!animator.clips.empty()) {
                    ImGui::Text("Time: %.2f / %.2f", animator.time, animator.clips[animator.currentClip].duration);
                }
            }

            DrawAnimatorGraph(entity);

            ImGui::TreePop();
        }
    }

    if (entity.HasComponent<CameraComponent>()) {
        if (ImGui::TreeNodeEx("Camera", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
            auto& cameraComponent = entity.GetComponent<CameraComponent>();
            auto& camera = cameraComponent.camera;
            ImGui::Checkbox("Primary", &cameraComponent.primary);
            ImGui::Checkbox("Fixed Aspect", &cameraComponent.fixedAspectRatio);

            const char* projectionLabels[] = { "Perspective", "Orthographic" };
            int projectionIndex = static_cast<int>(camera.getProjectionType());
            if (ImGui::Combo("Projection", &projectionIndex, projectionLabels, 2)) {
                const float aspect = camera.getAspectRatio();
                if (projectionIndex == 0) {
                    camera.setPerspective(camera.getFovY(), aspect, camera.getNearClip(), camera.getFarClip());
                } else {
                    camera.setOrthographic(camera.getOrthographicSize(), aspect, camera.getNearClip(), camera.getFarClip());
                }
            }

            if (camera.getProjectionType() == ProjectionType::Perspective) {
                float fovY = camera.getFovY();
                if (ImGui::DragFloat("Field of View", &fovY, 0.5f, 1.0f, 179.0f)) {
                    camera.setPerspective(fovY, camera.getAspectRatio(), camera.getNearClip(), camera.getFarClip());
                }
            } else {
                float orthoSize = camera.getOrthographicSize();
                if (ImGui::DragFloat("Size", &orthoSize, 0.1f, 0.01f, 1000.0f)) {
                    camera.setOrthographic(orthoSize, camera.getAspectRatio(), camera.getNearClip(), camera.getFarClip());
                }
            }

            float nearClip = camera.getNearClip();
            float farClip = camera.getFarClip();
            bool clipChanged = ImGui::DragFloat("Near Clip", &nearClip, 0.01f, 0.001f, farClip - 0.01f);
            clipChanged |= ImGui::DragFloat("Far Clip", &farClip, 1.0f, nearClip + 0.01f, 10000.0f);
            if (clipChanged) {
                if (camera.getProjectionType() == ProjectionType::Perspective) {
                    camera.setPerspective(camera.getFovY(), camera.getAspectRatio(), nearClip, farClip);
                } else {
                    camera.setOrthographic(camera.getOrthographicSize(), camera.getAspectRatio(), nearClip, farClip);
                }
            }

            ImGui::TreePop();
        }
    }

    if (entity.HasComponent<ScriptComponent>()) {
        if (ImGui::TreeNodeEx("Script", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
            auto& script = entity.GetComponent<ScriptComponent>();

            char classNameBuffer[256] = {};
            std::snprintf(classNameBuffer, sizeof(classNameBuffer), "%s", script.className.c_str());
            if (ImGui::InputText("Class Name", classNameBuffer, sizeof(classNameBuffer))) {
                script.className = std::string(classNameBuffer);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Fully qualified type name, e.g. PieceEngine.Examples.LogScript");
            }

            ImGui::Checkbox("Enabled", &script.enabled);

            const bool removeRequested = ImGui::SmallButton("Remove Script Component");
            ImGui::TreePop();
            if (removeRequested) {
                entity.RemoveComponent<ScriptComponent>();
            }
        }
    } else {
        if (ImGui::Button("Add Script Component")) {
            entity.AddComponent<ScriptComponent>();
        }
    }
}

void SceneHierarchyPanel::DrawAnimatorGraph(Entity entity) {
    auto& animator = entity.GetComponent<AnimatorComponent>();
    AnimatorController& controller = animator.controller;

    ImGui::Separator();
    ImGui::TextUnformatted("Parameters");
    for (size_t i = 0; i < controller.parameters.size();) {
        ImGui::PushID(static_cast<int>(100 + i));
        AnimationParameter& param = controller.parameters[i];
        char nameBuffer[64];
        std::snprintf(nameBuffer, sizeof(nameBuffer), "%s", param.name.c_str());
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::InputText("##ParamName", nameBuffer, sizeof(nameBuffer))) {
            param.name = nameBuffer;
        }
        ImGui::SameLine();
        const char* typeLabels[] = {"Float", "Bool", "Trigger", "Key Pressed"};
        int typeIndex = static_cast<int>(param.type);
        ImGui::SetNextItemWidth(90.0f);
        if (ImGui::Combo("##ParamType", &typeIndex, typeLabels, 4)) {
            param.type = static_cast<AnimationParameterType>(typeIndex);
        }
        ImGui::SameLine();
        switch (param.type) {
        case AnimationParameterType::Float:
            ImGui::SetNextItemWidth(90.0f);
            ImGui::DragFloat("##ParamValue", &param.floatValue, 0.01f);
            break;
        case AnimationParameterType::Bool:
            ImGui::Checkbox("##ParamValue", &param.boolValue);
            break;
        case AnimationParameterType::Trigger:
            if (ImGui::SmallButton(param.triggerValue ? "Armed" : "Fire")) {
                param.triggerValue = true;
            }
            break;
        case AnimationParameterType::KeyPressed: {
            static const KeyCode kBindableKeys[] = {
                KeyCode::Space, KeyCode::W, KeyCode::A, KeyCode::S, KeyCode::D,
                KeyCode::Q, KeyCode::E, KeyCode::R, KeyCode::F, KeyCode::C,
                KeyCode::LeftShift, KeyCode::LeftControl, KeyCode::LeftAlt,
                KeyCode::Enter, KeyCode::Escape, KeyCode::Tab,
                KeyCode::Up, KeyCode::Down, KeyCode::Left, KeyCode::Right};
            constexpr size_t kBindableKeyCount = sizeof(kBindableKeys) / sizeof(kBindableKeys[0]);
            int keySelection = 0;
            for (size_t k = 0; k < kBindableKeyCount; ++k) {
                if (static_cast<int32_t>(kBindableKeys[k]) == param.keyCode) {
                    keySelection = static_cast<int>(k);
                    break;
                }
            }
            ImGui::SetNextItemWidth(90.0f);
            if (ImGui::BeginCombo("##ParamKey", ToString(kBindableKeys[keySelection]))) {
                for (size_t k = 0; k < kBindableKeyCount; ++k) {
                    const bool isSelected = keySelection == static_cast<int>(k);
                    if (ImGui::Selectable(ToString(kBindableKeys[k]), isSelected)) {
                        param.keyCode = static_cast<int32_t>(kBindableKeys[k]);
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            ImGui::TextColored(param.boolValue ? ImVec4(0.4f, 0.9f, 0.5f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                param.boolValue ? "Down" : "Up");
            break;
        }
        }
        ImGui::SameLine();
        const bool removeParam = ImGui::SmallButton("X");
        ImGui::PopID();
        if (removeParam) {
            controller.parameters.erase(controller.parameters.begin() + static_cast<std::ptrdiff_t>(i));
            continue;
        }
        ++i;
    }

    ImGui::SetNextItemWidth(150.0f);
    ImGui::InputText("##NewParamName", m_AnimatorNewParamName, sizeof(m_AnimatorNewParamName));
    ImGui::SameLine();
    if (ImGui::SmallButton("Add Parameter") && m_AnimatorNewParamName[0] != '\0') {
        AnimationParameter param{};
        param.name = m_AnimatorNewParamName;
        controller.parameters.push_back(param);
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Animation Clips");
    ImGui::SameLine();
    if (ImGui::SmallButton("+##AddClipSlot")) {
        World::AddEntityAnimationClipSlot(static_cast<uint32_t>(entity));
    }
    for (size_t i = 0; i < animator.clips.size();) {
        ImGui::PushID(static_cast<int>(1000 + i));
        char clipNameBuffer[64];
        std::snprintf(clipNameBuffer, sizeof(clipNameBuffer), "%s", animator.clips[i].name.c_str());
        ImGui::SetNextItemWidth(160.0f);
        if (ImGui::InputText("##ClipName", clipNameBuffer, sizeof(clipNameBuffer))) {
            const std::string oldName = animator.clips[i].name;
            World::RenameEntityAnimationClip(static_cast<uint32_t>(entity), oldName, clipNameBuffer);
        }
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("UPLOADED_OBJ_TEMPLATE")) {
                const char* pathData = static_cast<const char*>(payload->Data);
                if (pathData != nullptr && payload->DataSize > 0) {
                    ImportedModelData imported;
                    std::string error;
                    if (AssetImporter::ImportModel(pathData, imported, &error) && !imported.animations.empty()) {
                        World::SetEntityAnimationClipSlot(
                            static_cast<uint32_t>(entity), i, imported.animations.front());
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }
        ImGui::SameLine();
        if (ImGui::Button("Drag", ImVec2(48.0f, 0.0f))) {
            // no-op target for the drag handle; dragging is initiated below
        }
        if (ImGui::BeginDragDropSource()) {
            uint32_t clipIndex = static_cast<uint32_t>(i);
            ImGui::SetDragDropPayload("ANIMATOR_CLIP_INDEX", &clipIndex, sizeof(clipIndex));
            ImGui::TextUnformatted(animator.clips[i].name.c_str());
            ImGui::EndDragDropSource();
        }
        ImGui::SameLine();
        const bool removeClip = ImGui::SmallButton("X");
        ImGui::PopID();
        if (removeClip) {
            const std::string clipName = animator.clips[i].name;
            World::RemoveEntityAnimationClip(static_cast<uint32_t>(entity), clipName);
            continue;
        }
        ++i;
    }
    if (animator.clips.empty()) {
        ImGui::TextDisabled("Press + to create a slot, then drag an animation asset onto it.");
    }

    ImGui::Separator();
    ImGui::TextUnformatted("State Graph");
    if (ImGui::Button(m_AnimatorLinkMode ? "Cancel Link" : "Link States")) {
        m_AnimatorLinkMode = !m_AnimatorLinkMode;
        m_AnimatorLinkFromState = -1;
    }
    ImGui::SameLine();
    ImGui::TextDisabled(m_AnimatorLinkMode ? "Click a source state, then a target state." : "Drag a clip in; click a node to edit it.");

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.045f, 0.065f, 0.09f, 1.0f));
    ImGui::BeginChild("##AnimatorCanvas", ImVec2(0.0f, 260.0f), true, ImGuiWindowFlags_NoScrollWithMouse);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 canvasOrigin = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    if (canvasSize.x < 1.0f) canvasSize.x = 1.0f;
    if (canvasSize.y < 1.0f) canvasSize.y = 1.0f;

    ImGui::SetCursorScreenPos(canvasOrigin);
    ImGui::InvisibleButton("##AnimatorCanvasBg", canvasSize);
    const bool canvasRightClicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ANIMATOR_CLIP_INDEX")) {
            const uint32_t clipIndex = *static_cast<const uint32_t*>(payload->Data);
            if (clipIndex < animator.clips.size()) {
                const ImVec2 dropPos = ImGui::GetMousePos();
                AnimationState newState{};
                newState.name = animator.clips[clipIndex].name;
                newState.clipName = animator.clips[clipIndex].name;
                newState.canvasX = dropPos.x - canvasOrigin.x;
                newState.canvasY = dropPos.y - canvasOrigin.y;
                controller.states.push_back(newState);
                if (controller.entryState < 0) {
                    controller.entryState = static_cast<int32_t>(controller.states.size()) - 1;
                }
            }
        }
        ImGui::EndDragDropTarget();
    }
    if (canvasRightClicked) {
        ImGui::OpenPopup("AnimatorCanvasContext");
    }
    if (ImGui::BeginPopup("AnimatorCanvasContext")) {
        const ImVec2 popupOpenPos = ImGui::GetMousePosOnOpeningCurrentPopup();
        if (ImGui::MenuItem("Add Empty State")) {
            AnimationState newState{};
            newState.canvasX = popupOpenPos.x - canvasOrigin.x;
            newState.canvasY = popupOpenPos.y - canvasOrigin.y;
            controller.states.push_back(newState);
            if (controller.entryState < 0) {
                controller.entryState = static_cast<int32_t>(controller.states.size()) - 1;
            }
        }
        ImGui::EndPopup();
    }

    constexpr ImVec2 kNodeSize(120.0f, 38.0f);
    constexpr float kPortRadius = 5.0f;
    const ImVec2 mousePos = ImGui::GetMousePos();
    const bool mouseClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    const bool mouseReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
    const bool mouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    bool linkCompleted = false;
    for (size_t ti = 0; ti < controller.transitions.size(); ++ti) {
        const AnimationTransition& transition = controller.transitions[ti];
        if (transition.fromState < 0 || transition.fromState >= static_cast<int32_t>(controller.states.size())) continue;
        if (transition.toState < 0 || transition.toState >= static_cast<int32_t>(controller.states.size())) continue;

        const AnimationState& fromNode = controller.states[static_cast<size_t>(transition.fromState)];
        const AnimationState& toNode = controller.states[static_cast<size_t>(transition.toState)];
        const ImVec2 p1(canvasOrigin.x + fromNode.canvasX + kNodeSize.x * 0.5f, canvasOrigin.y + fromNode.canvasY + kNodeSize.y * 0.5f);
        const ImVec2 p2(canvasOrigin.x + toNode.canvasX + kNodeSize.x * 0.5f, canvasOrigin.y + toNode.canvasY + kNodeSize.y * 0.5f);
        const bool selected = static_cast<int32_t>(ti) == m_AnimatorSelectedTransition;
        drawList->AddLine(p1, p2, selected ? IM_COL32(255, 139, 46, 255) : IM_COL32(104, 124, 146, 210), selected ? 3.0f : 2.0f);

        ImVec2 dir(p2.x - p1.x, p2.y - p1.y);
        const float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (len > 1.0f) {
            dir.x /= len;
            dir.y /= len;
            const ImVec2 mid((p1.x + p2.x) * 0.5f, (p1.y + p2.y) * 0.5f);
            const ImVec2 perp(-dir.y, dir.x);
            const ImVec2 tip(mid.x + dir.x * 7.0f, mid.y + dir.y * 7.0f);
            const ImVec2 a(mid.x - dir.x * 7.0f + perp.x * 5.0f, mid.y - dir.y * 7.0f + perp.y * 5.0f);
            const ImVec2 b(mid.x - dir.x * 7.0f - perp.x * 5.0f, mid.y - dir.y * 7.0f - perp.y * 5.0f);
            const ImU32 arrowColor = selected ? IM_COL32(255, 139, 46, 255) : IM_COL32(104, 124, 146, 230);
            drawList->AddTriangleFilled(tip, a, b, arrowColor);

            ImGui::PushID(static_cast<int>(2000 + ti));
            ImGui::SetCursorScreenPos(ImVec2(mid.x - 8.0f, mid.y - 8.0f));
            if (ImGui::InvisibleButton("##TransitionHit", ImVec2(16.0f, 16.0f))) {
                m_AnimatorSelectedTransition = static_cast<int32_t>(ti);
                m_AnimatorSelectedState = -1;
            }
            ImGui::PopID();
        }
    }

    for (size_t si = 0; si < controller.states.size(); ++si) {
        AnimationState& state = controller.states[si];
        ImGui::PushID(static_cast<int>(3000 + si));
        const ImVec2 nodePos(canvasOrigin.x + state.canvasX, canvasOrigin.y + state.canvasY);
        const ImVec2 nodeMax(nodePos.x + kNodeSize.x, nodePos.y + kNodeSize.y);
        const ImVec2 inputPort(nodePos.x, nodePos.y + kNodeSize.y * 0.5f);
        const ImVec2 outputPort(nodeMax.x, nodePos.y + kNodeSize.y * 0.5f);
        const auto isNear = [&](const ImVec2& point) {
            const float dx = mousePos.x - point.x;
            const float dy = mousePos.y - point.y;
            return dx * dx + dy * dy <= 100.0f;
        };
        const bool inputHovered = isNear(inputPort);
        const bool outputHovered = isNear(outputPort);
        const bool nodeHovered = mousePos.x >= nodePos.x && mousePos.x <= nodeMax.x
            && mousePos.y >= nodePos.y && mousePos.y <= nodeMax.y
            && !inputHovered && !outputHovered;

        if (outputHovered && mouseClicked) {
            m_AnimatorLinkMode = true;
            m_AnimatorLinkFromState = static_cast<int32_t>(si);
        } else if (nodeHovered && mouseClicked) {
            if (m_AnimatorLinkMode) {
                if (m_AnimatorLinkFromState < 0) {
                    m_AnimatorLinkFromState = static_cast<int32_t>(si);
                } else if (m_AnimatorLinkFromState != static_cast<int32_t>(si)) {
                    AnimationTransition newTransition{};
                    newTransition.fromState = m_AnimatorLinkFromState;
                    newTransition.toState = static_cast<int32_t>(si);
                    controller.transitions.push_back(newTransition);
                    m_AnimatorLinkFromState = -1;
                    m_AnimatorLinkMode = false;
                }
            } else {
                m_AnimatorSelectedState = static_cast<int32_t>(si);
                m_AnimatorSelectedTransition = -1;
                m_AnimatorDraggingState = static_cast<int32_t>(si);
                m_AnimatorDragOffsetX = mousePos.x - nodePos.x;
                m_AnimatorDragOffsetY = mousePos.y - nodePos.y;
            }
        }

        if (m_AnimatorDraggingState == static_cast<int32_t>(si) && mouseDown) {
            state.canvasX = mousePos.x - canvasOrigin.x - m_AnimatorDragOffsetX;
            state.canvasY = mousePos.y - canvasOrigin.y - m_AnimatorDragOffsetY;
        }
        if (m_AnimatorLinkMode && m_AnimatorLinkFromState >= 0 && inputHovered
            && mouseReleased
            && m_AnimatorLinkFromState != static_cast<int32_t>(si)) {
            const bool duplicate = std::any_of(controller.transitions.begin(), controller.transitions.end(), [&](const AnimationTransition& transition) {
                return transition.fromState == m_AnimatorLinkFromState && transition.toState == static_cast<int32_t>(si);
            });
            if (!duplicate) {
                AnimationTransition newTransition{};
                newTransition.fromState = m_AnimatorLinkFromState;
                newTransition.toState = static_cast<int32_t>(si);
                controller.transitions.push_back(newTransition);
                m_AnimatorSelectedTransition = static_cast<int32_t>(controller.transitions.size()) - 1;
                m_AnimatorSelectedState = -1;
            }
            linkCompleted = true;
            m_AnimatorLinkFromState = -1;
            m_AnimatorLinkMode = false;
        }

        const bool isEntry = static_cast<int32_t>(si) == controller.entryState;
        const bool isCurrent = static_cast<int32_t>(si) == animator.currentState;
        const bool isSelected = static_cast<int32_t>(si) == m_AnimatorSelectedState;
        const bool isLinkSource = m_AnimatorLinkMode && m_AnimatorLinkFromState == static_cast<int32_t>(si);

        const ImU32 fill = isCurrent ? IM_COL32(151, 67, 22, 255) : (isSelected ? IM_COL32(39, 57, 78, 255) : IM_COL32(22, 33, 47, 255));
        const ImU32 outline = isLinkSource ? IM_COL32(255, 139, 46, 255) : (isSelected ? IM_COL32(244, 118, 34, 255) : IM_COL32(66, 84, 104, 220));
        drawList->AddRectFilled(nodePos, nodeMax, fill, 5.0f);
        drawList->AddRect(nodePos, nodeMax, outline, 5.0f, 0, isSelected ? 2.5f : 1.2f);
        drawList->AddCircleFilled(inputPort, kPortRadius, inputHovered ? IM_COL32(255, 155, 65, 255) : IM_COL32(214, 92, 29, 255));
        drawList->AddCircleFilled(outputPort, kPortRadius, isLinkSource ? IM_COL32(255, 155, 65, 255) : IM_COL32(214, 92, 29, 255));
        drawList->AddText(ImVec2(nodePos.x + 6.0f, nodePos.y + 4.0f), IM_COL32(235, 240, 245, 255), state.name.c_str());
        drawList->AddText(ImVec2(nodePos.x + 6.0f, nodePos.y + 20.0f), IM_COL32(180, 195, 205, 255),
            state.clipName.empty() ? "(no clip)" : state.clipName.c_str());
        if (isEntry) {
            drawList->AddText(ImVec2(nodeMax.x - 14.0f, nodePos.y + 4.0f), IM_COL32(255, 155, 65, 255), "E");
        }
        ImGui::PopID();
    }
    if (m_AnimatorLinkMode && m_AnimatorLinkFromState >= 0
        && m_AnimatorLinkFromState < static_cast<int32_t>(controller.states.size())) {
        const AnimationState& source = controller.states[static_cast<size_t>(m_AnimatorLinkFromState)];
        const ImVec2 sourcePort(
            canvasOrigin.x + source.canvasX + kNodeSize.x,
            canvasOrigin.y + source.canvasY + kNodeSize.y * 0.5f);
        drawList->AddLine(sourcePort, ImGui::GetMousePos(), IM_COL32(255, 139, 46, 240), 2.0f);
        if (mouseReleased && !linkCompleted) {
            m_AnimatorLinkFromState = -1;
            m_AnimatorLinkMode = false;
        }
    }
    if (mouseReleased) {
        m_AnimatorDraggingState = -1;
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Separator();
    ImGui::TextUnformatted("Transitions");
    if (ImGui::SmallButton("Add Transition")) {
        AnimationTransition newTransition{};
        newTransition.fromState = -1;
        newTransition.toState = controller.states.empty() ? -1 : 0;
        controller.transitions.push_back(newTransition);
        m_AnimatorSelectedTransition = static_cast<int32_t>(controller.transitions.size()) - 1;
        m_AnimatorSelectedState = -1;
    }
    for (size_t ti = 0; ti < controller.transitions.size();) {
        ImGui::PushID(static_cast<int>(5000 + ti));
        AnimationTransition& transition = controller.transitions[ti];

        std::vector<std::string> fromLabelStorage;
        fromLabelStorage.emplace_back("Any State");
        for (const auto& state : controller.states) {
            fromLabelStorage.push_back(state.name);
        }
        std::vector<const char*> fromLabels;
        for (const auto& label : fromLabelStorage) {
            fromLabels.push_back(label.c_str());
        }
        int fromSelection = transition.fromState + 1;
        ImGui::SetNextItemWidth(110.0f);
        if (ImGui::Combo("##FromState", &fromSelection, fromLabels.data(), static_cast<int>(fromLabels.size()))) {
            transition.fromState = fromSelection - 1;
        }

        ImGui::SameLine();
        ImGui::TextUnformatted("->");
        ImGui::SameLine();

        std::vector<const char*> toLabels;
        for (const auto& state : controller.states) {
            toLabels.push_back(state.name.c_str());
        }
        int toSelection = toLabels.empty()
            ? -1
            : std::clamp(transition.toState, 0, static_cast<int32_t>(toLabels.size()) - 1);
        ImGui::SetNextItemWidth(110.0f);
        if (!toLabels.empty() && ImGui::Combo("##ToState", &toSelection, toLabels.data(), static_cast<int>(toLabels.size()))) {
            transition.toState = toSelection;
        }

        ImGui::SameLine();
        if (ImGui::SmallButton("Edit")) {
            m_AnimatorSelectedTransition = static_cast<int32_t>(ti);
            m_AnimatorSelectedState = -1;
        }
        ImGui::SameLine();
        const bool removeTransition = ImGui::SmallButton("X");
        ImGui::PopID();
        if (removeTransition) {
            controller.transitions.erase(controller.transitions.begin() + static_cast<std::ptrdiff_t>(ti));
            if (m_AnimatorSelectedTransition == static_cast<int32_t>(ti)) {
                m_AnimatorSelectedTransition = -1;
            }
            continue;
        }
        ++ti;
    }
    if (controller.transitions.empty()) {
        ImGui::TextDisabled("No transitions yet. Add one, or link two nodes above.");
    }

    if (m_AnimatorSelectedState >= 0 && m_AnimatorSelectedState < static_cast<int32_t>(controller.states.size())) {
        AnimationState& state = controller.states[static_cast<size_t>(m_AnimatorSelectedState)];
        ImGui::Separator();
        ImGui::Text("State: %s", state.name.c_str());
        char nameBuffer[64];
        std::snprintf(nameBuffer, sizeof(nameBuffer), "%s", state.name.c_str());
        if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
            state.name = nameBuffer;
        }

        std::vector<std::string> clipLabelStorage;
        clipLabelStorage.reserve(animator.clips.size() + 1);
        clipLabelStorage.emplace_back("(none)");
        for (const auto& clip : animator.clips) {
            clipLabelStorage.push_back(clip.name);
        }
        std::vector<const char*> clipLabels;
        clipLabels.reserve(clipLabelStorage.size());
        for (const auto& label : clipLabelStorage) {
            clipLabels.push_back(label.c_str());
        }
        int selectedClip = 0;
        for (size_t ci = 0; ci < animator.clips.size(); ++ci) {
            if (animator.clips[ci].name == state.clipName) {
                selectedClip = static_cast<int>(ci) + 1;
                break;
            }
        }
        if (ImGui::Combo("Clip", &selectedClip, clipLabels.data(), static_cast<int>(clipLabels.size()))) {
            state.clipName = selectedClip == 0 ? std::string() : animator.clips[static_cast<size_t>(selectedClip - 1)].name;
        }
        ImGui::DragFloat("State Speed", &state.speed, 0.01f, -4.0f, 4.0f);
        ImGui::Checkbox("Loop", &state.loop);
        if (ImGui::Button("Set As Entry")) {
            controller.entryState = m_AnimatorSelectedState;
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete State")) {
            const int32_t deleted = m_AnimatorSelectedState;
            controller.states.erase(controller.states.begin() + deleted);
            for (auto it = controller.transitions.begin(); it != controller.transitions.end();) {
                if (it->fromState == deleted || it->toState == deleted) {
                    it = controller.transitions.erase(it);
                    continue;
                }
                if (it->fromState > deleted) --it->fromState;
                if (it->toState > deleted) --it->toState;
                ++it;
            }
            if (controller.entryState == deleted) {
                controller.entryState = controller.states.empty() ? -1 : 0;
            } else if (controller.entryState > deleted) {
                --controller.entryState;
            }
            if (animator.currentState == deleted) {
                animator.currentState = -1;
            } else if (animator.currentState > deleted) {
                --animator.currentState;
            }
            m_AnimatorSelectedState = -1;
        }
    }

    if (m_AnimatorSelectedTransition >= 0 && m_AnimatorSelectedTransition < static_cast<int32_t>(controller.transitions.size())) {
        AnimationTransition& transition = controller.transitions[static_cast<size_t>(m_AnimatorSelectedTransition)];
        ImGui::Separator();
        const char* fromName = (transition.fromState >= 0 && transition.fromState < static_cast<int32_t>(controller.states.size()))
            ? controller.states[static_cast<size_t>(transition.fromState)].name.c_str() : "?";
        const char* toName = (transition.toState >= 0 && transition.toState < static_cast<int32_t>(controller.states.size()))
            ? controller.states[static_cast<size_t>(transition.toState)].name.c_str() : "?";
        ImGui::Text("Transition: %s -> %s", fromName, toName);
        ImGui::Checkbox("Has Exit Time", &transition.hasExitTime);
        if (transition.hasExitTime) {
            ImGui::SliderFloat("Exit Time", &transition.exitTime, 0.0f, 1.0f);
        }
        ImGui::DragFloat("Blend Duration", &transition.blendDuration, 0.01f, 0.0f, 2.0f);

        ImGui::TextUnformatted("Conditions");
        for (size_t ci = 0; ci < transition.conditions.size();) {
            ImGui::PushID(static_cast<int>(4000 + ci));
            AnimationCondition& condition = transition.conditions[ci];

            std::vector<const char*> paramNames;
            paramNames.reserve(controller.parameters.size());
            for (const auto& param : controller.parameters) {
                paramNames.push_back(param.name.c_str());
            }
            int paramIndex = 0;
            for (size_t pi = 0; pi < controller.parameters.size(); ++pi) {
                if (controller.parameters[pi].name == condition.parameterName) {
                    paramIndex = static_cast<int>(pi);
                    break;
                }
            }
            if (!paramNames.empty()) {
                ImGui::SetNextItemWidth(110.0f);
                if (ImGui::Combo("##ConditionParam", &paramIndex, paramNames.data(), static_cast<int>(paramNames.size()))) {
                    condition.parameterName = controller.parameters[static_cast<size_t>(paramIndex)].name;
                }
            } else {
                ImGui::TextDisabled("No parameters yet");
            }
            ImGui::SameLine();
            const char* comparisonLabels[] = {"==", "!=", ">", "<", ">=", "<="};
            int comparisonIndex = static_cast<int>(condition.comparison);
            ImGui::SetNextItemWidth(55.0f);
            if (ImGui::Combo("##ConditionComparison", &comparisonIndex, comparisonLabels, 6)) {
                condition.comparison = static_cast<AnimationComparison>(comparisonIndex);
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(70.0f);
            ImGui::DragFloat("##ConditionThreshold", &condition.threshold, 0.01f);
            ImGui::SameLine();
            const bool removeCondition = ImGui::SmallButton("X");
            ImGui::PopID();
            if (removeCondition) {
                transition.conditions.erase(transition.conditions.begin() + static_cast<std::ptrdiff_t>(ci));
                continue;
            }
            ++ci;
        }
        if (ImGui::SmallButton("Add Condition") && !controller.parameters.empty()) {
            AnimationCondition condition{};
            condition.parameterName = controller.parameters.front().name;
            transition.conditions.push_back(condition);
        }
        if (ImGui::Button("Delete Transition")) {
            controller.transitions.erase(controller.transitions.begin() + m_AnimatorSelectedTransition);
            m_AnimatorSelectedTransition = -1;
        }
    }

    World::SetEntityAnimatorController(static_cast<uint32_t>(entity), controller);
}

} // namespace Piece

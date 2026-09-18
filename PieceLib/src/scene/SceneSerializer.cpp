#include <PiecePCH.h>

#include <scene/SceneSerializer.h>

#include <assets/AssetImporter.h>
#include <core/Log.h>
#include <core/UUID.h>
#include <renderer/Renderer.h>
#include <scene/Components.h>
#include <scene/Entity.h>
#include <scene/Mesh.h>
#include <scene/Scene.h>
#include <scene/World.h>

#include <yaml-cpp/yaml.h>

#include <fstream>
#include <sstream>

namespace YAML {

template <>
struct convert<glm::vec3> {
    static Node encode(const glm::vec3& rhs) {
        Node node;
        node.push_back(rhs.x);
        node.push_back(rhs.y);
        node.push_back(rhs.z);
        return node;
    }

    static bool decode(const Node& node, glm::vec3& rhs) {
        if (!node.IsSequence() || node.size() != 3) {
            return false;
        }
        rhs.x = node[0].as<float>();
        rhs.y = node[1].as<float>();
        rhs.z = node[2].as<float>();
        return true;
    }
};

} // namespace YAML

namespace Piece {

namespace {

YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v) {
    out << YAML::Flow << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
    return out;
}

std::string NormalSourceToString(NormalSource source) {
    return source == NormalSource::Vertex ? "Vertex" : "Derivative";
}

NormalSource StringToNormalSource(const std::string& value) {
    return value == "Vertex" ? NormalSource::Vertex : NormalSource::Derivative;
}

std::string PrimitiveTypeToString(PrimitiveType type) {
    switch (type) {
    case PrimitiveType::Quad: return "Quad";
    case PrimitiveType::Cube: return "Cube";
    case PrimitiveType::Sphere: return "Sphere";
    default: return "Unknown";
    }
}

PrimitiveType StringToPrimitiveType(const std::string& value) {
    if (value == "Quad") return PrimitiveType::Quad;
    if (value == "Cube") return PrimitiveType::Cube;
    if (value == "Sphere") return PrimitiveType::Sphere;
    return PrimitiveType::Unknown;
}

void SerializeMaterials(YAML::Emitter& out) {
    out << YAML::Key << "DefaultMaterialId" << YAML::Value << World::GetDefaultMaterialId();
    out << YAML::Key << "Materials" << YAML::Value << YAML::BeginSeq;
    for (const auto& material : World::GetMaterials()) {
        out << YAML::BeginMap;
        out << YAML::Key << "Id" << YAML::Value << material.id;
        out << YAML::Key << "Name" << YAML::Value << material.name;
        out << YAML::Key << "AlbedoPath" << YAML::Value << material.textures.albedoPath;
        out << YAML::Key << "NormalPath" << YAML::Value << material.textures.normalPath;
        out << YAML::Key << "HeightPath" << YAML::Value << material.textures.heightPath;
        out << YAML::Key << "RoughnessPath" << YAML::Value << material.textures.roughnessPath;
        out << YAML::Key << "MetallicPath" << YAML::Value << material.textures.metallicPath;
        out << YAML::Key << "AmbientOcclusionPath" << YAML::Value << material.textures.ambientOcclusionPath;
        out << YAML::Key << "EmissivePath" << YAML::Value << material.textures.emissivePath;
        out << YAML::Key << "RoughnessFactor" << YAML::Value << material.surfaceFactors.roughnessFactor;
        out << YAML::Key << "MetallicFactor" << YAML::Value << material.surfaceFactors.metallicFactor;
        out << YAML::Key << "BaseColor" << YAML::Value << material.colors.baseColor;
        out << YAML::Key << "EmissiveColor" << YAML::Value << material.colors.emissiveColor;
        out << YAML::Key << "EmissiveEnabled" << YAML::Value << material.colors.emissiveEnabled;
        out << YAML::Key << "HdrBloomEnabled" << YAML::Value << material.colors.hdrBloomEnabled;
        out << YAML::Key << "EmissiveBloomEnabled" << YAML::Value << material.colors.emissiveBloomEnabled;
        out << YAML::Key << "BloomThreshold" << YAML::Value << material.colors.bloomThreshold;
        out << YAML::Key << "BloomIntensity" << YAML::Value << material.colors.bloomIntensity;
        out << YAML::Key << "BloomRadius" << YAML::Value << material.colors.bloomRadius;
        out << YAML::EndMap;
    }
    out << YAML::EndSeq;
}

void DeserializeMaterials(const YAML::Node& root) {
    World::ClearMaterials();

    auto materials = root["Materials"];
    if (materials) {
        for (auto materialNode : materials) {
            MaterialTextures textures{};
            textures.albedoPath = materialNode["AlbedoPath"].as<std::string>("");
            textures.normalPath = materialNode["NormalPath"].as<std::string>("");
            textures.heightPath = materialNode["HeightPath"].as<std::string>("");
            textures.roughnessPath = materialNode["RoughnessPath"].as<std::string>("");
            textures.metallicPath = materialNode["MetallicPath"].as<std::string>("");
            textures.ambientOcclusionPath = materialNode["AmbientOcclusionPath"].as<std::string>("");
            textures.emissivePath = materialNode["EmissivePath"].as<std::string>("");

            MaterialSurfaceFactors surfaceFactors{};
            surfaceFactors.roughnessFactor = materialNode["RoughnessFactor"].as<float>(0.75f);
            surfaceFactors.metallicFactor = materialNode["MetallicFactor"].as<float>(0.0f);

            MaterialColors colors{};
            colors.baseColor = materialNode["BaseColor"].as<glm::vec3>(glm::vec3(1.0f));
            colors.emissiveColor = materialNode["EmissiveColor"].as<glm::vec3>(glm::vec3(0.0f));
            colors.emissiveEnabled = materialNode["EmissiveEnabled"].as<bool>(false);
            colors.hdrBloomEnabled = materialNode["HdrBloomEnabled"].as<bool>(false);
            colors.emissiveBloomEnabled = materialNode["EmissiveBloomEnabled"].as<bool>(true);
            colors.bloomThreshold = materialNode["BloomThreshold"].as<float>(0.8f);
            colors.bloomIntensity = materialNode["BloomIntensity"].as<float>(0.35f);
            colors.bloomRadius = materialNode["BloomRadius"].as<float>(2.0f);

            const uint32_t id = materialNode["Id"].as<uint32_t>();
            const std::string name = materialNode["Name"].as<std::string>("Material");
            World::RestoreMaterial(id, name, textures, surfaceFactors, colors);
        }
    }

    World::SetDefaultMaterialId(root["DefaultMaterialId"].as<uint32_t>(0));
}

void SerializeWorldSettings(YAML::Emitter& out) {
    const World::SpecularSettings specular = World::GetSpecularSettings();
    out << YAML::Key << "Specular" << YAML::Value;
    out << YAML::BeginMap;
    out << YAML::Key << "Strength" << YAML::Value << specular.strength;
    out << YAML::Key << "ShininessMin" << YAML::Value << specular.shininessMin;
    out << YAML::Key << "ShininessMax" << YAML::Value << specular.shininessMax;
    out << YAML::EndMap;

    const EnvironmentSettings environment = World::GetEnvironmentSettings();
    out << YAML::Key << "Environment" << YAML::Value;
    out << YAML::BeginMap;
    out << YAML::Key << "Enabled" << YAML::Value << environment.enabled;
    out << YAML::Key << "DiffuseMapPath" << YAML::Value << environment.diffuseMapPath;
    out << YAML::Key << "SpecularMapPath" << YAML::Value << environment.specularMapPath;
    out << YAML::Key << "Intensity" << YAML::Value << environment.intensity;
    out << YAML::Key << "DiffuseStrength" << YAML::Value << environment.diffuseStrength;
    out << YAML::Key << "SpecularStrength" << YAML::Value << environment.specularStrength;
    out << YAML::Key << "AmbientStrength" << YAML::Value << environment.ambientStrength;
    out << YAML::Key << "AATechnique" << YAML::Value << static_cast<int>(environment.aaTechnique);
    out << YAML::Key << "MsaaSampleCount" << YAML::Value << environment.msaaSampleCount;
    out << YAML::EndMap;
}

void DeserializeWorldSettings(const YAML::Node& root) {
    auto specularNode = root["Specular"];
    if (specularNode) {
        World::SpecularSettings specular{};
        specular.strength = specularNode["Strength"].as<float>(1.0f);
        specular.shininessMin = specularNode["ShininessMin"].as<float>(8.0f);
        specular.shininessMax = specularNode["ShininessMax"].as<float>(128.0f);
        World::SetSpecularSettings(specular);
    }

    auto environmentNode = root["Environment"];
    if (environmentNode) {
        EnvironmentSettings environment{};
        environment.enabled = environmentNode["Enabled"].as<bool>(false);
        environment.diffuseMapPath = environmentNode["DiffuseMapPath"].as<std::string>("");
        environment.specularMapPath = environmentNode["SpecularMapPath"].as<std::string>("");
        environment.intensity = environmentNode["Intensity"].as<float>(1.0f);
        environment.diffuseStrength = environmentNode["DiffuseStrength"].as<float>(1.0f);
        environment.specularStrength = environmentNode["SpecularStrength"].as<float>(1.0f);
        environment.ambientStrength = environmentNode["AmbientStrength"].as<float>(0.08f);
        environment.aaTechnique = static_cast<AATechnique>(environmentNode["AATechnique"].as<int>(1));
        environment.msaaSampleCount = environmentNode["MsaaSampleCount"].as<uint32_t>(4);
        World::SetEnvironmentSettings(environment);
    }
}

void SerializeEntity(YAML::Emitter& out, Entity entity) {
    out << YAML::BeginMap;
    out << YAML::Key << "Entity" << YAML::Value << static_cast<uint64_t>(entity.getUUID());

    const auto& tag = entity.GetComponent<TagComponent>();
    out << YAML::Key << "TagComponent";
    out << YAML::BeginMap;
    out << YAML::Key << "Tag" << YAML::Value << tag.tag;
    out << YAML::EndMap;

    if (entity.HasComponent<HierarchyComponent>()) {
        const auto& hierarchy = entity.GetComponent<HierarchyComponent>();
        out << YAML::Key << "HierarchyComponent";
        out << YAML::BeginMap;
        out << YAML::Key << "Parent" << YAML::Value << static_cast<uint64_t>(hierarchy.parent);
        out << YAML::Key << "Children" << YAML::Value << YAML::BeginSeq;
        for (const UUID& child : hierarchy.children) {
            out << static_cast<uint64_t>(child);
        }
        out << YAML::EndSeq;
        out << YAML::EndMap;
    }

    if (entity.HasComponent<TransformComponent>()) {
        const auto& transform = entity.GetComponent<TransformComponent>();
        out << YAML::Key << "TransformComponent";
        out << YAML::BeginMap;
        out << YAML::Key << "Position" << YAML::Value << transform.position;
        out << YAML::Key << "Rotation" << YAML::Value << transform.rotation;
        out << YAML::Key << "Scale" << YAML::Value << transform.scale;
        out << YAML::EndMap;
    }

    if (entity.HasComponent<CameraComponent>()) {
        const auto& camera = entity.GetComponent<CameraComponent>();
        out << YAML::Key << "CameraComponent";
        out << YAML::BeginMap;
        out << YAML::Key << "Primary" << YAML::Value << camera.primary;
        out << YAML::Key << "FixedAspectRatio" << YAML::Value << camera.fixedAspectRatio;
        out << YAML::EndMap;
    }

    if (entity.HasComponent<MeshRendererComponent>()) {
        const auto& meshRenderer = entity.GetComponent<MeshRendererComponent>();
        out << YAML::Key << "MeshRendererComponent";
        out << YAML::BeginMap;
        out << YAML::Key << "PrimitiveType" << YAML::Value << PrimitiveTypeToString(meshRenderer.primitiveType);
        out << YAML::Key << "NormalSource" << YAML::Value << NormalSourceToString(meshRenderer.normalSource);
        out << YAML::Key << "MaterialId" << YAML::Value << meshRenderer.materialId;
        out << YAML::EndMap;
    }

    if (entity.HasComponent<ImportedModelComponent>()) {
        const auto& imported = entity.GetComponent<ImportedModelComponent>();
        out << YAML::Key << "ImportedModelComponent";
        out << YAML::BeginMap;
        out << YAML::Key << "SourcePath" << YAML::Value << imported.sourcePath;
        out << YAML::Key << "MeshName" << YAML::Value << imported.meshName;
        out << YAML::EndMap;
    }

    if (entity.HasComponent<MaterialComponent>()) {
        const auto& material = entity.GetComponent<MaterialComponent>();
        out << YAML::Key << "MaterialComponent";
        out << YAML::BeginMap;
        out << YAML::Key << "MaterialId" << YAML::Value << material.materialId;
        out << YAML::Key << "BaseColor" << YAML::Value << material.colors.baseColor;
        out << YAML::Key << "EmissiveColor" << YAML::Value << material.colors.emissiveColor;
        out << YAML::Key << "EmissiveEnabled" << YAML::Value << material.colors.emissiveEnabled;
        out << YAML::Key << "HdrBloomEnabled" << YAML::Value << material.colors.hdrBloomEnabled;
        out << YAML::Key << "EmissiveBloomEnabled" << YAML::Value << material.colors.emissiveBloomEnabled;
        out << YAML::Key << "BloomThreshold" << YAML::Value << material.colors.bloomThreshold;
        out << YAML::Key << "BloomIntensity" << YAML::Value << material.colors.bloomIntensity;
        out << YAML::Key << "BloomRadius" << YAML::Value << material.colors.bloomRadius;
        out << YAML::EndMap;
    }

    if (entity.HasComponent<DirectionalLightComponent>()) {
        const auto& light = entity.GetComponent<DirectionalLightComponent>();
        out << YAML::Key << "DirectionalLightComponent";
        out << YAML::BeginMap;
        out << YAML::Key << "Direction" << YAML::Value << light.direction;
        out << YAML::Key << "Color" << YAML::Value << light.color;
        out << YAML::Key << "Intensity" << YAML::Value << light.intensity;
        out << YAML::EndMap;
    }

    if (entity.HasComponent<PointLightComponent>()) {
        const auto& light = entity.GetComponent<PointLightComponent>();
        out << YAML::Key << "PointLightComponent";
        out << YAML::BeginMap;
        out << YAML::Key << "Color" << YAML::Value << light.color;
        out << YAML::Key << "Intensity" << YAML::Value << light.intensity;
        out << YAML::Key << "Radius" << YAML::Value << light.radius;
        out << YAML::EndMap;
    }

    if (entity.HasComponent<SpotLightComponent>()) {
        const auto& light = entity.GetComponent<SpotLightComponent>();
        out << YAML::Key << "SpotLightComponent";
        out << YAML::BeginMap;
        out << YAML::Key << "Direction" << YAML::Value << light.direction;
        out << YAML::Key << "Color" << YAML::Value << light.color;
        out << YAML::Key << "Intensity" << YAML::Value << light.intensity;
        out << YAML::Key << "InnerCutoffDegrees" << YAML::Value << light.innerCutoffDegrees;
        out << YAML::Key << "OuterCutoffDegrees" << YAML::Value << light.outerCutoffDegrees;
        out << YAML::EndMap;
    }

    out << YAML::EndMap;
}

Ref<Mesh> ReimportMesh(const std::string& sourcePath, const std::string& meshName) {
    ImportedModelData model{};
    std::string error;
    if (!AssetImporter::ImportModel(sourcePath, model, &error)) {
        PIECE_CORE_ERROR("SceneSerializer: failed to re-import '{0}': {1}", sourcePath, error);
        return nullptr;
    }

    for (const auto& meshData : model.meshes) {
        if (meshData.name == meshName) {
            return CreateRef<Mesh>(Renderer::GetDevice(), meshData.vertices, meshData.indices);
        }
    }

    PIECE_CORE_WARN("SceneSerializer: mesh '{0}' not found in '{1}'", meshName, sourcePath);
    return nullptr;
}

} // namespace

SceneSerializer::SceneSerializer(const Ref<Scene>& scene)
    : m_Scene(scene) {}

void SceneSerializer::Serialize(const std::string& filepath) {
    if (!m_Scene) {
        return;
    }

    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "Scene" << YAML::Value << "Untitled";

    SerializeMaterials(out);
    SerializeWorldSettings(out);

    out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
    auto view = m_Scene->GetAllEntitiesViewWith<TagComponent>();
    for (auto handle : view) {
        Entity entity{handle, m_Scene.get()};
        SerializeEntity(out, entity);
    }
    out << YAML::EndSeq;

    out << YAML::EndMap;

    std::ofstream fout(filepath);
    fout << out.c_str();
}

bool SceneSerializer::Deserialize(const std::string& filepath) {
    YAML::Node root;
    try {
        root = YAML::LoadFile(filepath);
    } catch (const YAML::Exception& e) {
        PIECE_CORE_ERROR("SceneSerializer: {0}", e.what());
        return false;
    }

    if (!root["Scene"]) {
        PIECE_CORE_WARN("SceneSerializer: no 'Scene' node found in {0}", filepath);
        return false;
    }

    m_Scene->Clear();
    DeserializeMaterials(root);
    DeserializeWorldSettings(root);

    auto entities = root["Entities"];
    if (entities) {
        for (auto entityNode : entities) {
            const uint64_t uuid = entityNode["Entity"].as<uint64_t>();
            std::string name = "Entity";
            if (auto tagNode = entityNode["TagComponent"]) {
                name = tagNode["Tag"].as<std::string>("Entity");
            }

            Entity deserializedEntity = m_Scene->CreateEntity(name, UUID(uuid));

            if (auto hierarchyNode = entityNode["HierarchyComponent"]) {
                auto& hierarchy = deserializedEntity.GetComponent<HierarchyComponent>();
                hierarchy.parent = UUID(hierarchyNode["Parent"].as<uint64_t>(0));
                hierarchy.children.clear();
                if (auto childrenNode = hierarchyNode["Children"]) {
                    for (auto childNode : childrenNode) {
                        hierarchy.children.emplace_back(childNode.as<uint64_t>());
                    }
                }
            }

            if (auto transformNode = entityNode["TransformComponent"]) {
                auto& transform = deserializedEntity.GetComponent<TransformComponent>();
                transform.position = transformNode["Position"].as<glm::vec3>(glm::vec3(0.0f));
                transform.rotation = transformNode["Rotation"].as<glm::vec3>(glm::vec3(0.0f));
                transform.scale = transformNode["Scale"].as<glm::vec3>(glm::vec3(1.0f));
            }

            if (auto cameraNode = entityNode["CameraComponent"]) {
                auto& camera = deserializedEntity.AddComponent<CameraComponent>();
                camera.primary = cameraNode["Primary"].as<bool>(true);
                camera.fixedAspectRatio = cameraNode["FixedAspectRatio"].as<bool>(false);
            }

            std::string importedSourcePath;
            std::string importedMeshName;
            if (auto importedNode = entityNode["ImportedModelComponent"]) {
                auto& imported = deserializedEntity.AddComponent<ImportedModelComponent>();
                imported.sourcePath = importedNode["SourcePath"].as<std::string>("");
                imported.meshName = importedNode["MeshName"].as<std::string>("");
                importedSourcePath = imported.sourcePath;
                importedMeshName = imported.meshName;
            }

            if (auto meshRendererNode = entityNode["MeshRendererComponent"]) {
                const PrimitiveType primitiveType = StringToPrimitiveType(meshRendererNode["PrimitiveType"].as<std::string>("Unknown"));
                const NormalSource normalSource = StringToNormalSource(meshRendererNode["NormalSource"].as<std::string>("Derivative"));
                auto& meshRenderer = deserializedEntity.AddComponent<MeshRendererComponent>(primitiveType, normalSource);
                meshRenderer.materialId = meshRendererNode["MaterialId"].as<uint32_t>(0);

                if (primitiveType == PrimitiveType::Unknown && !importedSourcePath.empty()) {
                    meshRenderer.mesh = ReimportMesh(importedSourcePath, importedMeshName);
                }
            }

            if (auto materialNode = entityNode["MaterialComponent"]) {
                auto& material = deserializedEntity.AddComponent<MaterialComponent>(materialNode["MaterialId"].as<uint32_t>(0));
                material.colors.baseColor = materialNode["BaseColor"].as<glm::vec3>(glm::vec3(1.0f));
                material.colors.emissiveColor = materialNode["EmissiveColor"].as<glm::vec3>(glm::vec3(0.0f));
                material.colors.emissiveEnabled = materialNode["EmissiveEnabled"].as<bool>(false);
                material.colors.hdrBloomEnabled = materialNode["HdrBloomEnabled"].as<bool>(false);
                material.colors.emissiveBloomEnabled = materialNode["EmissiveBloomEnabled"].as<bool>(true);
                material.colors.bloomThreshold = materialNode["BloomThreshold"].as<float>(0.8f);
                material.colors.bloomIntensity = materialNode["BloomIntensity"].as<float>(0.35f);
                material.colors.bloomRadius = materialNode["BloomRadius"].as<float>(2.0f);
            }

            if (auto dirLightNode = entityNode["DirectionalLightComponent"]) {
                auto& light = deserializedEntity.AddComponent<DirectionalLightComponent>();
                light.direction = dirLightNode["Direction"].as<glm::vec3>(glm::vec3(-0.4f, -1.0f, -0.2f));
                light.color = dirLightNode["Color"].as<glm::vec3>(glm::vec3(1.0f));
                light.intensity = dirLightNode["Intensity"].as<float>(1.2f);
            }

            if (auto pointLightNode = entityNode["PointLightComponent"]) {
                auto& light = deserializedEntity.AddComponent<PointLightComponent>();
                light.color = pointLightNode["Color"].as<glm::vec3>(glm::vec3(1.0f));
                light.intensity = pointLightNode["Intensity"].as<float>(1.0f);
                light.radius = pointLightNode["Radius"].as<float>(1.0f);
            }

            if (auto spotLightNode = entityNode["SpotLightComponent"]) {
                auto& light = deserializedEntity.AddComponent<SpotLightComponent>();
                light.direction = spotLightNode["Direction"].as<glm::vec3>(glm::vec3(0.0f, -1.0f, 0.0f));
                light.color = spotLightNode["Color"].as<glm::vec3>(glm::vec3(1.0f));
                light.intensity = spotLightNode["Intensity"].as<float>(1.0f);
                light.innerCutoffDegrees = spotLightNode["InnerCutoffDegrees"].as<float>(15.0f);
                light.outerCutoffDegrees = spotLightNode["OuterCutoffDegrees"].as<float>(20.0f);
            }
        }
    }

    World::SetActiveScene(m_Scene);
    return true;
}

} // namespace Piece

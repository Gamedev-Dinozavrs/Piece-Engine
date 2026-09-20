#include <PiecePCH.h>

#include <assets/MaterialAssetSerializer.h>

#include <yaml-cpp/yaml.h>

#include <fstream>

namespace Piece {

namespace {

YAML::Node EncodeVec3(const glm::vec3& value) {
    YAML::Node node;
    node.push_back(value.x);
    node.push_back(value.y);
    node.push_back(value.z);
    return node;
}

glm::vec3 DecodeVec3(const YAML::Node& node, const glm::vec3& fallback) {
    if (!node || !node.IsSequence() || node.size() != 3) {
        return fallback;
    }
    return {node[0].as<float>(), node[1].as<float>(), node[2].as<float>()};
}

} // namespace

bool MaterialAssetSerializer::Serialize(const MaterialView& material, const std::string& filepath) {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "Material" << YAML::Value << material.name;
    out << YAML::Key << "AlbedoPath" << YAML::Value << material.textures.albedoPath;
    out << YAML::Key << "NormalPath" << YAML::Value << material.textures.normalPath;
    out << YAML::Key << "HeightPath" << YAML::Value << material.textures.heightPath;
    out << YAML::Key << "RoughnessPath" << YAML::Value << material.textures.roughnessPath;
    out << YAML::Key << "MetallicPath" << YAML::Value << material.textures.metallicPath;
    out << YAML::Key << "AmbientOcclusionPath" << YAML::Value << material.textures.ambientOcclusionPath;
    out << YAML::Key << "EmissivePath" << YAML::Value << material.textures.emissivePath;
    out << YAML::Key << "BaseColor" << YAML::Value << EncodeVec3(material.colors.baseColor);
    out << YAML::Key << "EmissiveColor" << YAML::Value << EncodeVec3(material.colors.emissiveColor);
    out << YAML::Key << "EmissiveEnabled" << YAML::Value << material.colors.emissiveEnabled;
    out << YAML::Key << "RoughnessFactor" << YAML::Value << material.surfaceFactors.roughnessFactor;
    out << YAML::Key << "MetallicFactor" << YAML::Value << material.surfaceFactors.metallicFactor;
    out << YAML::Key << "NormalScale" << YAML::Value << material.surfaceFactors.normalScale;
    out << YAML::Key << "OcclusionStrength" << YAML::Value << material.surfaceFactors.occlusionStrength;
    out << YAML::Key << "AlphaMode" << YAML::Value << static_cast<int>(material.renderSettings.alphaMode);
    out << YAML::Key << "AlphaCutoff" << YAML::Value << material.renderSettings.alphaCutoff;
    out << YAML::Key << "DoubleSided" << YAML::Value << material.renderSettings.doubleSided;
    out << YAML::Key << "Unlit" << YAML::Value << material.renderSettings.unlit;
    out << YAML::EndMap;

    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    file << out.c_str();
    return file.good();
}

bool MaterialAssetSerializer::Deserialize(const std::string& filepath, MaterialView& material) {
    YAML::Node root;
    try {
        root = YAML::LoadFile(filepath);
    } catch (const YAML::Exception&) {
        return false;
    }

    if (!root["Material"]) {
        return false;
    }

    material = {};
    material.name = root["Material"].as<std::string>("Material");
    material.assetPath = filepath;
    material.textures.albedoPath = root["AlbedoPath"].as<std::string>("");
    material.textures.normalPath = root["NormalPath"].as<std::string>("");
    material.textures.heightPath = root["HeightPath"].as<std::string>("");
    material.textures.roughnessPath = root["RoughnessPath"].as<std::string>("");
    material.textures.metallicPath = root["MetallicPath"].as<std::string>("");
    material.textures.ambientOcclusionPath = root["AmbientOcclusionPath"].as<std::string>("");
    material.textures.emissivePath = root["EmissivePath"].as<std::string>("");
    material.colors.baseColor = DecodeVec3(root["BaseColor"], glm::vec3(1.0f));
    material.colors.emissiveColor = DecodeVec3(root["EmissiveColor"], glm::vec3(0.0f));
    material.colors.emissiveEnabled = root["EmissiveEnabled"].as<bool>(false);
    material.surfaceFactors.roughnessFactor = root["RoughnessFactor"].as<float>(0.75f);
    material.surfaceFactors.metallicFactor = root["MetallicFactor"].as<float>(0.0f);
    material.surfaceFactors.normalScale = root["NormalScale"].as<float>(1.0f);
    material.surfaceFactors.occlusionStrength = root["OcclusionStrength"].as<float>(1.0f);
    material.renderSettings.alphaMode = static_cast<MaterialAlphaMode>(root["AlphaMode"].as<int>(0));
    material.renderSettings.alphaCutoff = root["AlphaCutoff"].as<float>(0.5f);
    material.renderSettings.doubleSided = root["DoubleSided"].as<bool>(false);
    material.renderSettings.unlit = root["Unlit"].as<bool>(false);
    return true;
}

} // namespace Piece
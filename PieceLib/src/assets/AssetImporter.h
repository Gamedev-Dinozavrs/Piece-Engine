#pragma once

#include <scene/Mesh.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cstdint>
#include <string>
#include <vector>

namespace Piece {

struct ImportedJoint {
    std::string name;
    int32_t parentIndex{-1};
    glm::mat4 inverseBindMatrix{1.0f};
    glm::vec3 bindTranslation{0.0f};
    glm::vec4 bindRotation{0.0f, 0.0f, 0.0f, 1.0f};
    glm::vec3 bindScale{1.0f};
};

struct ImportedAnimationChannel {
    int32_t jointIndex{-1};
    std::string jointName;
    std::vector<float> times;
    std::vector<glm::vec4> values;
    bool stepInterpolation{false};
    enum class Path : uint8_t {
        Translation,
        Rotation,
        Scale
    } path{Path::Translation};
};

struct ImportedAnimationClip {
    std::string name;
    float duration{0.0f};
    std::vector<ImportedAnimationChannel> channels;
};

struct ImportedMeshData {
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    bool hasNormals{false};
    int materialIndex{-1};
    std::string materialName;
};

struct ImportedMaterialData {
    std::string name;
    std::string albedoPath;
    std::string normalPath;
    std::string roughnessPath;
    std::string ambientOcclusionPath;
    std::string emissivePath;
    glm::vec3 baseColor{1.0f};
    glm::vec3 emissiveColor{0.0f};
    bool emissiveEnabled{false};
    float roughnessFactor{0.75f};
    float metallicFactor{1.0f};
};

struct ImportedModelData {
    std::string sourcePath;
    std::vector<ImportedMeshData> meshes;
    std::vector<ImportedMaterialData> materials;
    std::vector<ImportedJoint> joints;
    std::vector<ImportedAnimationClip> animations;
};

class AssetImporter {
public:
    static bool ImportOBJ(const std::string& path, ImportedModelData& outModel, std::string* outError = nullptr);
    static bool ImportGLTF(const std::string& path, ImportedModelData& outModel, std::string* outError = nullptr);
    static bool ImportFBX(const std::string& path, ImportedModelData& outModel, std::string* outError = nullptr);
    static bool ImportModel(const std::string& path, ImportedModelData& outModel, std::string* outError = nullptr);
};

} // namespace Piece

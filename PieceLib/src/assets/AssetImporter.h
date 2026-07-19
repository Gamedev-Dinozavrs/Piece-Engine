#pragma once

#include <scene/Mesh.h>

#include <string>
#include <vector>

namespace Piece {

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
};

struct ImportedModelData {
    std::string sourcePath;
    std::vector<ImportedMeshData> meshes;
    std::vector<ImportedMaterialData> materials;
};

class AssetImporter {
public:
    static bool ImportOBJ(const std::string& path, ImportedModelData& outModel, std::string* outError = nullptr);
    static bool ImportGLTF(const std::string& path, ImportedModelData& outModel, std::string* outError = nullptr);
    static bool ImportModel(const std::string& path, ImportedModelData& outModel, std::string* outError = nullptr);
};

} // namespace Piece

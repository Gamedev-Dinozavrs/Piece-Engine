#include <PiecePCH.h>

#include <assets/AssetImporter.h>

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <filesystem>
#include <fstream>
#include <algorithm>
#include <array>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>

namespace Piece {

namespace {

struct ObjVertexKey {
    int vertexIndex{-1};
    int normalIndex{-1};
    int uvIndex{-1};

    bool operator==(const ObjVertexKey& other) const {
        return vertexIndex == other.vertexIndex && normalIndex == other.normalIndex && uvIndex == other.uvIndex;
    }
};

struct ObjVertexKeyHash {
    size_t operator()(const ObjVertexKey& key) const {
        size_t h = std::hash<int>{}(key.vertexIndex);
        h ^= (std::hash<int>{}(key.normalIndex) << 1);
        h ^= (std::hash<int>{}(key.uvIndex) << 2);
        return h;
    }
};

struct MeshBuilder {
    ImportedMeshData mesh;
    std::unordered_map<ObjVertexKey, uint32_t, ObjVertexKeyHash> uniqueVertices;
};

std::string Trim(std::string value) {
    const char* whitespace = " \t\r\n";
    const size_t begin = value.find_first_not_of(whitespace);
    if (begin == std::string::npos) {
        return {};
    }
    const size_t end = value.find_last_not_of(whitespace);
    return value.substr(begin, end - begin + 1);
}

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::string ResolveTexturePath(const std::filesystem::path& baseDir, const std::string& texturePath);

std::string ResolveGltfTexturePath(const fastgltf::Asset& asset, const std::filesystem::path& baseDir, const fastgltf::Optional<fastgltf::TextureInfo>& textureInfo) {
    if (!textureInfo.has_value()) {
        return {};
    }

    const std::size_t textureIndex = textureInfo->textureIndex;
    if (textureIndex >= asset.textures.size()) {
        return {};
    }

    const fastgltf::Texture& texture = asset.textures[textureIndex];
    std::size_t imageIndex = std::numeric_limits<std::size_t>::max();
    if (texture.imageIndex.has_value()) {
        imageIndex = texture.imageIndex.value();
    } else if (texture.webpImageIndex.has_value()) {
        imageIndex = texture.webpImageIndex.value();
    } else if (texture.basisuImageIndex.has_value()) {
        imageIndex = texture.basisuImageIndex.value();
    }

    if (imageIndex == std::numeric_limits<std::size_t>::max() || imageIndex >= asset.images.size()) {
        return {};
    }

    const fastgltf::Image& image = asset.images[imageIndex];
    const auto* uriSource = std::get_if<fastgltf::sources::URI>(&image.data);
    if (uriSource == nullptr || !uriSource->uri.isLocalPath()) {
        return {};
    }

    return ResolveTexturePath(baseDir, uriSource->uri.fspath().string());
}

std::string ResolveGltfTexturePath(const fastgltf::Asset& asset, const std::filesystem::path& baseDir, const fastgltf::Optional<fastgltf::NormalTextureInfo>& textureInfo) {
    if (!textureInfo.has_value()) {
        return {};
    }

    const std::size_t textureIndex = textureInfo->textureIndex;
    if (textureIndex >= asset.textures.size()) {
        return {};
    }

    const fastgltf::Texture& texture = asset.textures[textureIndex];
    std::size_t imageIndex = std::numeric_limits<std::size_t>::max();
    if (texture.imageIndex.has_value()) {
        imageIndex = texture.imageIndex.value();
    } else if (texture.webpImageIndex.has_value()) {
        imageIndex = texture.webpImageIndex.value();
    } else if (texture.basisuImageIndex.has_value()) {
        imageIndex = texture.basisuImageIndex.value();
    }

    if (imageIndex == std::numeric_limits<std::size_t>::max() || imageIndex >= asset.images.size()) {
        return {};
    }

    const fastgltf::Image& image = asset.images[imageIndex];
    const auto* uriSource = std::get_if<fastgltf::sources::URI>(&image.data);
    if (uriSource == nullptr || !uriSource->uri.isLocalPath()) {
        return {};
    }

    return ResolveTexturePath(baseDir, uriSource->uri.fspath().string());
}

std::string ResolveGltfTexturePath(const fastgltf::Asset& asset, const std::filesystem::path& baseDir, const fastgltf::Optional<fastgltf::OcclusionTextureInfo>& textureInfo) {
    if (!textureInfo.has_value()) {
        return {};
    }

    const std::size_t textureIndex = textureInfo->textureIndex;
    if (textureIndex >= asset.textures.size()) {
        return {};
    }

    const fastgltf::Texture& texture = asset.textures[textureIndex];
    std::size_t imageIndex = std::numeric_limits<std::size_t>::max();
    if (texture.imageIndex.has_value()) {
        imageIndex = texture.imageIndex.value();
    } else if (texture.webpImageIndex.has_value()) {
        imageIndex = texture.webpImageIndex.value();
    } else if (texture.basisuImageIndex.has_value()) {
        imageIndex = texture.basisuImageIndex.value();
    }

    if (imageIndex == std::numeric_limits<std::size_t>::max() || imageIndex >= asset.images.size()) {
        return {};
    }

    const fastgltf::Image& image = asset.images[imageIndex];
    const auto* uriSource = std::get_if<fastgltf::sources::URI>(&image.data);
    if (uriSource == nullptr || !uriSource->uri.isLocalPath()) {
        return {};
    }

    return ResolveTexturePath(baseDir, uriSource->uri.fspath().string());
}

void FillVertexAttributesFromGltf(
    const fastgltf::Asset& asset,
    const fastgltf::Primitive& primitive,
    ImportedMeshData& meshData,
    std::string* outError) {
    const auto posIt = primitive.findAttribute("POSITION");
    if (posIt == primitive.attributes.end()) {
        if (outError) {
            *outError = "glTF primitive missing POSITION attribute";
        }
        return;
    }

    if (posIt->accessorIndex >= asset.accessors.size()) {
        if (outError) {
            *outError = "glTF POSITION accessor index out of bounds";
        }
        return;
    }

    const auto& positionAccessor = asset.accessors[posIt->accessorIndex];
    if (positionAccessor.count == 0) {
        if (outError) {
            *outError = "glTF POSITION accessor has zero vertices";
        }
        return;
    }

    std::vector<fastgltf::math::fvec3> positions(positionAccessor.count);
    fastgltf::copyFromAccessor<fastgltf::math::fvec3>(asset, positionAccessor, positions.data());

    std::vector<fastgltf::math::fvec3> normals;
    if (const auto normalIt = primitive.findAttribute("NORMAL"); normalIt != primitive.attributes.end() && normalIt->accessorIndex < asset.accessors.size()) {
        const auto& normalAccessor = asset.accessors[normalIt->accessorIndex];
        normals.resize(normalAccessor.count);
        fastgltf::copyFromAccessor<fastgltf::math::fvec3>(asset, normalAccessor, normals.data());
    }

    std::vector<fastgltf::math::fvec2> uvs;
    if (const auto uvIt = primitive.findAttribute("TEXCOORD_0"); uvIt != primitive.attributes.end() && uvIt->accessorIndex < asset.accessors.size()) {
        const auto& uvAccessor = asset.accessors[uvIt->accessorIndex];
        uvs.resize(uvAccessor.count);
        fastgltf::copyFromAccessor<fastgltf::math::fvec2>(asset, uvAccessor, uvs.data());
    }

    meshData.vertices.resize(positions.size());
    meshData.hasNormals = normals.size() == positions.size();
    for (std::size_t i = 0; i < positions.size(); ++i) {
        Vertex vertex{};
        vertex.position = {positions[i][0], positions[i][1], positions[i][2]};
        vertex.color = {1.0f, 1.0f, 1.0f};
        if (i < uvs.size()) {
            vertex.uv = {uvs[i][0], uvs[i][1]};
        }
        if (meshData.hasNormals) {
            vertex.normal = {normals[i][0], normals[i][1], normals[i][2]};
        } else {
            vertex.normal = {0.0f, 0.0f, 1.0f};
        }
        meshData.vertices[i] = vertex;
    }

    if (primitive.indicesAccessor.has_value()) {
        const std::size_t indexAccessorIdx = primitive.indicesAccessor.value();
        if (indexAccessorIdx >= asset.accessors.size()) {
            if (outError) {
                *outError = "glTF index accessor index out of bounds";
            }
            return;
        }

        const auto& indexAccessor = asset.accessors[indexAccessorIdx];
        meshData.indices.resize(indexAccessor.count);
        fastgltf::copyFromAccessor<uint32_t>(asset, indexAccessor, meshData.indices.data());
    } else {
        meshData.indices.resize(meshData.vertices.size());
        for (std::size_t i = 0; i < meshData.vertices.size(); ++i) {
            meshData.indices[i] = static_cast<uint32_t>(i);
        }
    }
}

std::filesystem::path TryResolveRelativeTexture(const std::filesystem::path& baseDir, const std::filesystem::path& texPath) {
    static const std::filesystem::path kCommonSubdirs[] = {
        "",
        "Textures",
        "Texture",
        "textures",
        "texture",
        "Materials",
        "Material",
        "materials",
        "material"
    };

    const std::filesystem::path fileName = texPath.filename();
    const std::string fileNameLower = ToLower(fileName.string());

    for (const auto& subdir : kCommonSubdirs) {
        const std::filesystem::path candidate = (baseDir / subdir / texPath).lexically_normal();
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }

        const std::filesystem::path candidateByName = (baseDir / subdir / fileName).lexically_normal();
        if (std::filesystem::exists(candidateByName)) {
            return candidateByName;
        }
    }

    std::error_code ec;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(baseDir, ec)) {
        if (ec) {
            break;
        }
        if (!entry.is_regular_file()) {
            continue;
        }

        const std::filesystem::path currentFile = entry.path().filename();
        if (ToLower(currentFile.string()) == fileNameLower) {
            return entry.path().lexically_normal();
        }
    }

    return {};
}

std::string ResolveTexturePath(const std::filesystem::path& baseDir, const std::string& texNameRaw) {
    const std::string texName = Trim(texNameRaw);
    if (texName.empty()) {
        return {};
    }

    std::filesystem::path texPath(texName);
    if (texPath.is_absolute()) {
        return std::filesystem::exists(texPath) ? texPath.lexically_normal().string() : std::string{};
    }

    if (const std::filesystem::path resolved = TryResolveRelativeTexture(baseDir, texPath); !resolved.empty()) {
        return resolved.string();
    }

    return (baseDir / texPath).lexically_normal().string();
}

glm::vec3 ReadPosition(const tinyobj::attrib_t& attrib, int vertexIndex) {
    if (vertexIndex < 0) {
        return {0.0f, 0.0f, 0.0f};
    }
    const int base = 3 * vertexIndex;
    if (base + 2 >= static_cast<int>(attrib.vertices.size())) {
        return {0.0f, 0.0f, 0.0f};
    }
    return {attrib.vertices[base + 0], attrib.vertices[base + 1], attrib.vertices[base + 2]};
}

glm::vec3 ReadNormal(const tinyobj::attrib_t& attrib, int normalIndex, bool& hasNormals) {
    if (normalIndex < 0) {
        hasNormals = false;
        return {0.0f, 0.0f, 1.0f};
    }
    const int base = 3 * normalIndex;
    if (base + 2 >= static_cast<int>(attrib.normals.size())) {
        hasNormals = false;
        return {0.0f, 0.0f, 1.0f};
    }
    return {attrib.normals[base + 0], attrib.normals[base + 1], attrib.normals[base + 2]};
}

glm::vec2 ReadUV(const tinyobj::attrib_t& attrib, int uvIndex) {
    if (uvIndex < 0) {
        return {0.0f, 0.0f};
    }
    const int base = 2 * uvIndex;
    if (base + 1 >= static_cast<int>(attrib.texcoords.size())) {
        return {0.0f, 0.0f};
    }
    return {attrib.texcoords[base + 0], attrib.texcoords[base + 1]};
}

std::optional<std::filesystem::path> FindMtlWithSpacesFromObj(const std::filesystem::path& objPath) {
    std::ifstream in(objPath);
    if (!in.is_open()) {
        return std::nullopt;
    }

    std::string line;
    while (std::getline(in, line)) {
        const std::string prefix = "mtllib ";
        if (line.rfind(prefix, 0) != 0) {
            continue;
        }

        const std::string rawValue = Trim(line.substr(prefix.size()));
        if (rawValue.empty()) {
            continue;
        }

        std::filesystem::path mtlPath = objPath.parent_path() / rawValue;
        if (std::filesystem::exists(mtlPath)) {
            return mtlPath.lexically_normal();
        }
    }

    return std::nullopt;
}

bool TryParseObjWithMtllibAlias(
    const std::filesystem::path& objPath,
    const std::filesystem::path& mtlPath,
    tinyobj::ObjReader& outReader)
{
    const std::filesystem::path parentDir = objPath.parent_path();
    const std::filesystem::path aliasMtl = parentDir / "__piece_mtl_alias__.mtl";
    const std::filesystem::path tempObj = parentDir / "__piece_obj_alias__.obj";

    std::error_code ec;
    std::filesystem::copy_file(mtlPath, aliasMtl, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        return false;
    }

    std::ifstream in(objPath);
    if (!in.is_open()) {
        std::filesystem::remove(aliasMtl, ec);
        return false;
    }

    std::ofstream out(tempObj, std::ios::trunc);
    if (!out.is_open()) {
        std::filesystem::remove(aliasMtl, ec);
        return false;
    }

    std::string line;
    while (std::getline(in, line)) {
        const std::string prefix = "mtllib ";
        if (line.rfind(prefix, 0) == 0) {
            out << "mtllib " << aliasMtl.filename().string() << "\n";
        } else {
            out << line << "\n";
        }
    }
    out.close();

    tinyobj::ObjReaderConfig retryConfig{};
    retryConfig.mtl_search_path = parentDir.string();
    const bool parsed = outReader.ParseFromFile(tempObj.string(), retryConfig);

    std::filesystem::remove(tempObj, ec);
    std::filesystem::remove(aliasMtl, ec);
    return parsed;
}

} // namespace

bool AssetImporter::ImportOBJ(const std::string& path, ImportedModelData& outModel, std::string* outError) {
    outModel = {};
    outModel.sourcePath = path;

    const std::filesystem::path sourcePath(path);
    const std::filesystem::path sourceDir = sourcePath.parent_path();

    if (!std::filesystem::exists(path)) {
        if (outError) {
            *outError = "File does not exist: " + path;
        }
        return false;
    }

    tinyobj::ObjReaderConfig config{};
    config.mtl_search_path = std::filesystem::path(path).parent_path().string();

    tinyobj::ObjReader reader;
    bool parsed = false;

    if (auto mtlCandidate = FindMtlWithSpacesFromObj(sourcePath)) {
        tinyobj::ObjReader fallbackReader;
        if (TryParseObjWithMtllibAlias(sourcePath, *mtlCandidate, fallbackReader)) {
            reader = std::move(fallbackReader);
            parsed = true;
        }
    }

    if (!parsed) {
        parsed = reader.ParseFromFile(path, config);
    }

    if (!parsed) {
        if (outError) {
            *outError = reader.Error().empty() ? "tinyobjloader parse failure" : reader.Error();
        }
        return false;
    }

    if (!reader.Warning().empty()) {
        PIECE_CORE_WARN("OBJ import warning for {}: {}", path, reader.Warning());
    }

    const tinyobj::attrib_t& attrib = reader.GetAttrib();
    const std::vector<tinyobj::shape_t>& shapes = reader.GetShapes();
    const std::vector<tinyobj::material_t>& materials = reader.GetMaterials();

    if (shapes.empty()) {
        if (outError) {
            *outError = "OBJ has no shapes";
        }
        return false;
    }

    outModel.materials.reserve(materials.size());
    for (size_t i = 0; i < materials.size(); ++i) {
        const tinyobj::material_t& material = materials[i];
        ImportedMaterialData imported{};
        imported.name = material.name.empty() ? ("Material " + std::to_string(i)) : std::string(material.name);
        imported.albedoPath = ResolveTexturePath(sourceDir, material.diffuse_texname);
        imported.normalPath = ResolveTexturePath(sourceDir,
            !material.normal_texname.empty() ? material.normal_texname : material.bump_texname);
        imported.roughnessPath = ResolveTexturePath(sourceDir, material.roughness_texname);
        imported.ambientOcclusionPath = ResolveTexturePath(sourceDir, material.ambient_texname);
        imported.emissivePath = ResolveTexturePath(sourceDir, material.emissive_texname);
        outModel.materials.push_back(std::move(imported));
    }

    outModel.meshes.reserve(shapes.size());

    for (const tinyobj::shape_t& shape : shapes) {
        std::unordered_map<int, MeshBuilder> builders;
        std::vector<int> materialOrder;

        size_t indexOffset = 0;
        for (size_t face = 0; face < shape.mesh.num_face_vertices.size(); ++face) {
            const int fv = static_cast<int>(shape.mesh.num_face_vertices[face]);
            int materialIndex = -1;
            if (face < shape.mesh.material_ids.size()) {
                materialIndex = shape.mesh.material_ids[face];
            }

            auto [itBuilder, inserted] = builders.emplace(materialIndex, MeshBuilder{});
            if (inserted) {
                materialOrder.push_back(materialIndex);
                itBuilder->second.mesh.name = shape.name.empty() ? "Imported Mesh" : shape.name;
                itBuilder->second.mesh.hasNormals = true;
                itBuilder->second.mesh.materialIndex = materialIndex;
                if (materialIndex >= 0 && materialIndex < static_cast<int>(outModel.materials.size())) {
                    itBuilder->second.mesh.materialName = outModel.materials[materialIndex].name;
                }
                itBuilder->second.uniqueVertices.reserve(shape.mesh.indices.size());
            }

            MeshBuilder& builder = itBuilder->second;

            for (int v = 0; v < fv; ++v) {
                const tinyobj::index_t& idx = shape.mesh.indices[indexOffset + static_cast<size_t>(v)];

                ObjVertexKey key{};
                key.vertexIndex = idx.vertex_index;
                key.normalIndex = idx.normal_index;
                key.uvIndex = idx.texcoord_index;

                auto itVertex = builder.uniqueVertices.find(key);
                if (itVertex != builder.uniqueVertices.end()) {
                    builder.mesh.indices.push_back(itVertex->second);
                    continue;
                }

                Vertex vertex{};
                vertex.position = ReadPosition(attrib, idx.vertex_index);
                vertex.color = {1.0f, 1.0f, 1.0f};
                vertex.uv = ReadUV(attrib, idx.texcoord_index);
                vertex.normal = ReadNormal(attrib, idx.normal_index, builder.mesh.hasNormals);

                const uint32_t newIndex = static_cast<uint32_t>(builder.mesh.vertices.size());
                builder.mesh.vertices.push_back(vertex);
                builder.mesh.indices.push_back(newIndex);
                builder.uniqueVertices.emplace(key, newIndex);
            }

            indexOffset += static_cast<size_t>(fv);
        }

        for (int materialIndex : materialOrder) {
            auto it = builders.find(materialIndex);
            if (it == builders.end()) {
                continue;
            }

            ImportedMeshData& mesh = it->second.mesh;
            if (!mesh.vertices.empty() && !mesh.indices.empty()) {
                if (!mesh.materialName.empty()) {
                    mesh.name += " [" + mesh.materialName + "]";
                }
                outModel.meshes.push_back(std::move(mesh));
            }
        }
    }

    if (outModel.meshes.empty()) {
        if (outError) {
            *outError = "OBJ parsing succeeded but produced no renderable meshes";
        }
        return false;
    }

    return true;
}

bool AssetImporter::ImportGLTF(const std::string& path, ImportedModelData& outModel, std::string* outError) {
    outModel = {};
    outModel.sourcePath = path;

    const std::filesystem::path sourcePath(path);
    const std::filesystem::path sourceDir = sourcePath.parent_path();
    if (!std::filesystem::exists(sourcePath)) {
        if (outError) {
            *outError = "File does not exist: " + path;
        }
        return false;
    }

    auto dataBuffer = fastgltf::GltfDataBuffer::FromPath(sourcePath);
    if (dataBuffer.error() != fastgltf::Error::None) {
        if (outError) {
            *outError = std::string(fastgltf::getErrorName(dataBuffer.error())) + ": " + std::string(fastgltf::getErrorMessage(dataBuffer.error()));
        }
        return false;
    }

    fastgltf::Parser parser(
        fastgltf::Extensions::KHR_texture_transform |
        fastgltf::Extensions::KHR_materials_unlit |
        fastgltf::Extensions::KHR_mesh_quantization);

    constexpr fastgltf::Options kOptions =
        fastgltf::Options::LoadExternalBuffers |
        fastgltf::Options::LoadExternalImages |
        fastgltf::Options::GenerateMeshIndices;

    auto parsedAsset = parser.loadGltf(dataBuffer.get(), sourceDir, kOptions, fastgltf::Category::All);
    if (parsedAsset.error() != fastgltf::Error::None) {
        if (outError) {
            *outError = std::string(fastgltf::getErrorName(parsedAsset.error())) + ": " + std::string(fastgltf::getErrorMessage(parsedAsset.error()));
        }
        return false;
    }

    const fastgltf::Asset& asset = parsedAsset.get();

    outModel.materials.reserve(asset.materials.size());
    for (std::size_t i = 0; i < asset.materials.size(); ++i) {
        const fastgltf::Material& material = asset.materials[i];
        ImportedMaterialData imported{};
        imported.name = material.name.empty() ? ("Material " + std::to_string(i)) : std::string(material.name);
        imported.albedoPath = ResolveGltfTexturePath(asset, sourceDir, material.pbrData.baseColorTexture);
        imported.normalPath = ResolveGltfTexturePath(asset, sourceDir, material.normalTexture);
        imported.roughnessPath = ResolveGltfTexturePath(asset, sourceDir, material.pbrData.metallicRoughnessTexture);
        imported.ambientOcclusionPath = ResolveGltfTexturePath(asset, sourceDir, material.occlusionTexture);
        imported.emissivePath = ResolveGltfTexturePath(asset, sourceDir, material.emissiveTexture);
        imported.roughnessFactor = std::clamp(static_cast<float>(material.pbrData.roughnessFactor), 0.0f, 1.0f);
        imported.metallicFactor = std::clamp(static_cast<float>(material.pbrData.metallicFactor), 0.0f, 1.0f);
        outModel.materials.push_back(std::move(imported));
    }

    for (std::size_t meshIndex = 0; meshIndex < asset.meshes.size(); ++meshIndex) {
        const fastgltf::Mesh& mesh = asset.meshes[meshIndex];
        for (std::size_t primitiveIndex = 0; primitiveIndex < mesh.primitives.size(); ++primitiveIndex) {
            const fastgltf::Primitive& primitive = mesh.primitives[primitiveIndex];
            if (primitive.type != fastgltf::PrimitiveType::Triangles) {
                continue;
            }

            ImportedMeshData meshData{};
            const std::string baseName = mesh.name.empty() ? ("glTF Mesh " + std::to_string(meshIndex)) : std::string(mesh.name);
            meshData.name = baseName + "#" + std::to_string(primitiveIndex);

            if (primitive.materialIndex.has_value()) {
                meshData.materialIndex = static_cast<int>(primitive.materialIndex.value());
                if (meshData.materialIndex >= 0 && meshData.materialIndex < static_cast<int>(outModel.materials.size())) {
                    meshData.materialName = outModel.materials[meshData.materialIndex].name;
                }
            }

            FillVertexAttributesFromGltf(asset, primitive, meshData, outError);
            if (meshData.vertices.empty() || meshData.indices.empty()) {
                if (outError && outError->empty()) {
                    *outError = "glTF primitive had no renderable data";
                }
                return false;
            }

            outModel.meshes.push_back(std::move(meshData));
        }
    }

    if (outModel.meshes.empty()) {
        if (outError) {
            *outError = "glTF parsing succeeded but produced no triangle primitives";
        }
        return false;
    }

    return true;
}

bool AssetImporter::ImportModel(const std::string& path, ImportedModelData& outModel, std::string* outError) {
    const std::string extension = ToLower(std::filesystem::path(path).extension().string());
    if (extension == ".obj") {
        return ImportOBJ(path, outModel, outError);
    }
    if (extension == ".gltf" || extension == ".glb") {
        return ImportGLTF(path, outModel, outError);
    }

    if (outError) {
        *outError = "Unsupported model format: " + extension;
    }
    return false;
}

} // namespace Piece

#include <PiecePCH.h>

#include <assets/AssetImporter.h>

#include <ufbx.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace Piece {

namespace {

std::string ToString(ufbx_string value) {
    return value.data == nullptr ? std::string{} : std::string(value.data, value.length);
}

glm::mat4 ToGlmMatrix(const ufbx_matrix& matrix) {
    glm::mat4 result(1.0f);
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 3; ++row) {
            result[column][row] = static_cast<float>(matrix.cols[column].v[row]);
        }
    }
    return result;
}

glm::vec3 ToGlmVec3(const ufbx_vec3& value) {
    return {
        static_cast<float>(value.x),
        static_cast<float>(value.y),
        static_cast<float>(value.z)};
}

glm::vec4 ToGlmQuat(const ufbx_quat& value) {
    return {
        static_cast<float>(value.x),
        static_cast<float>(value.y),
        static_cast<float>(value.z),
        static_cast<float>(value.w)};
}

struct FbxSkeleton {
    std::unordered_map<const ufbx_node*, int32_t> nodeToJoint;
};

bool BuildSkeleton(const ufbx_scene& scene, ImportedModelData& outModel, FbxSkeleton& outSkeleton, std::string* outError) {
    const ufbx_skin_deformer* skin = nullptr;
    for (size_t nodeIndex = 0; nodeIndex < scene.nodes.count && skin == nullptr; ++nodeIndex) {
        const ufbx_node* node = scene.nodes.data[nodeIndex];
        if (node == nullptr || node->mesh == nullptr || node->mesh->skin_deformers.count == 0) {
            continue;
        }
        skin = node->mesh->skin_deformers.data[0];
    }

    if (skin == nullptr) {
        for (size_t nodeIndex = 0; nodeIndex < scene.nodes.count; ++nodeIndex) {
            const ufbx_node* node = scene.nodes.data[nodeIndex];
            if (node == nullptr || node->bone == nullptr) {
                continue;
            }

            ImportedJoint joint{};
            joint.name = ToString(node->name);
            joint.bindTranslation = ToGlmVec3(node->local_transform.translation);
            joint.bindRotation = ToGlmQuat(node->local_transform.rotation);
            joint.bindScale = ToGlmVec3(node->local_transform.scale);
            outSkeleton.nodeToJoint.emplace(node, static_cast<int32_t>(outModel.joints.size()));
            outModel.joints.push_back(std::move(joint));
        }

        for (const auto& entry : outSkeleton.nodeToJoint) {
            const ufbx_node* node = entry.first;
            const auto parentIt = outSkeleton.nodeToJoint.find(node->parent);
            if (parentIt != outSkeleton.nodeToJoint.end()) {
                outModel.joints[static_cast<size_t>(entry.second)].parentIndex = parentIt->second;
            }
        }
        return true;
    }
    if (skin->clusters.count > 128) {
        if (outError) {
            *outError = "FBX skin has more than the supported 128 joints";
        }
        return false;
    }

    outModel.joints.reserve(skin->clusters.count);
    for (size_t clusterIndex = 0; clusterIndex < skin->clusters.count; ++clusterIndex) {
        const ufbx_skin_cluster* cluster = skin->clusters.data[clusterIndex];
        if (cluster == nullptr || cluster->bone_node == nullptr) {
            continue;
        }

        ImportedJoint joint{};
        joint.name = ToString(cluster->bone_node->name);
        if (joint.name.empty()) {
            joint.name = "Joint " + std::to_string(outModel.joints.size());
        }
        joint.inverseBindMatrix = ToGlmMatrix(cluster->geometry_to_bone);
        joint.bindTranslation = ToGlmVec3(cluster->bone_node->local_transform.translation);
        joint.bindRotation = ToGlmQuat(cluster->bone_node->local_transform.rotation);
        joint.bindScale = ToGlmVec3(cluster->bone_node->local_transform.scale);
        outSkeleton.nodeToJoint.emplace(cluster->bone_node, static_cast<int32_t>(outModel.joints.size()));
        outModel.joints.push_back(std::move(joint));
    }

    for (size_t jointIndex = 0; jointIndex < outModel.joints.size(); ++jointIndex) {
        const ufbx_skin_cluster* cluster = skin->clusters.data[jointIndex];
        if (cluster == nullptr || cluster->bone_node == nullptr || cluster->bone_node->parent == nullptr) {
            continue;
        }
        const auto parentIt = outSkeleton.nodeToJoint.find(cluster->bone_node->parent);
        if (parentIt != outSkeleton.nodeToJoint.end()) {
            outModel.joints[jointIndex].parentIndex = parentIt->second;
        }
    }

    return true;
}

void FillSkinWeights(
    const ufbx_mesh& mesh,
    const ufbx_skin_deformer* skin,
    const FbxSkeleton& skeleton,
    uint32_t logicalVertexIndex,
    Vertex& vertex) {
    if (skin == nullptr || logicalVertexIndex >= skin->vertices.count) {
        return;
    }

    const ufbx_skin_vertex skinVertex = skin->vertices.data[logicalVertexIndex];
    float totalWeight = 0.0f;
    uint32_t influenceCount = 0;
    for (uint32_t weightIndex = 0; weightIndex < skinVertex.num_weights && influenceCount < 4; ++weightIndex) {
        const uint32_t sourceIndex = skinVertex.weight_begin + weightIndex;
        if (sourceIndex >= skin->weights.count) {
            break;
        }

        const ufbx_skin_weight weight = skin->weights.data[sourceIndex];
        if (weight.cluster_index >= skin->clusters.count) {
            continue;
        }
        const ufbx_skin_cluster* cluster = skin->clusters.data[weight.cluster_index];
        if (cluster == nullptr || cluster->bone_node == nullptr) {
            continue;
        }
        const auto jointIt = skeleton.nodeToJoint.find(cluster->bone_node);
        if (jointIt == skeleton.nodeToJoint.end()) {
            continue;
        }

        vertex.jointIds[influenceCount] = static_cast<uint32_t>(jointIt->second);
        vertex.jointWeights[influenceCount] = static_cast<float>(weight.weight);
        totalWeight += vertex.jointWeights[influenceCount];
        ++influenceCount;
    }

    if (totalWeight > 0.0f) {
        vertex.jointWeights /= totalWeight;
    }
}

void ImportMeshNode(const ufbx_node& node, const FbxSkeleton& skeleton, ImportedModelData& outModel) {
    const ufbx_mesh& mesh = *node.mesh;
    const ufbx_skin_deformer* skin = mesh.skin_deformers.count > 0 ? mesh.skin_deformers.data[0] : nullptr;
    ImportedMeshData meshData{};
    meshData.name = ToString(node.name);
    if (meshData.name.empty()) {
        meshData.name = "FBX Mesh";
    }
    meshData.hasNormals = mesh.vertex_normal.exists;

    std::vector<uint32_t> triangleIndices(std::max<size_t>(mesh.max_face_triangles * 3, 3));
    for (size_t faceIndex = 0; faceIndex < mesh.faces.count; ++faceIndex) {
        const ufbx_face face = mesh.faces.data[faceIndex];
        const uint32_t triangleCount = ufbx_triangulate_face(
            triangleIndices.data(), triangleIndices.size(), &mesh, face);
        for (uint32_t triangleIndex = 0; triangleIndex < triangleCount * 3; ++triangleIndex) {
            const uint32_t sourceIndex = triangleIndices[triangleIndex];
            if (sourceIndex >= mesh.vertex_indices.count) {
                continue;
            }

            Vertex vertex{};
            vertex.position = ToGlmVec3(ufbx_get_vertex_vec3(&mesh.vertex_position, sourceIndex));
            vertex.color = glm::vec3(1.0f);
            vertex.normal = mesh.vertex_normal.exists
                ? ToGlmVec3(ufbx_get_vertex_vec3(&mesh.vertex_normal, sourceIndex))
                : glm::vec3(0.0f, 0.0f, 1.0f);
            vertex.uv = mesh.vertex_uv.exists
                ? glm::vec2(
                    static_cast<float>(ufbx_get_vertex_vec2(&mesh.vertex_uv, sourceIndex).x),
                    static_cast<float>(ufbx_get_vertex_vec2(&mesh.vertex_uv, sourceIndex).y))
                : glm::vec2(0.0f);

            FillSkinWeights(mesh, skin, skeleton, mesh.vertex_indices.data[sourceIndex], vertex);
            meshData.indices.push_back(static_cast<uint32_t>(meshData.vertices.size()));
            meshData.vertices.push_back(vertex);
        }
    }

    if (!meshData.vertices.empty() && !meshData.indices.empty()) {
        outModel.meshes.push_back(std::move(meshData));
    }
}

void ImportAnimations(const ufbx_scene& scene, const FbxSkeleton& skeleton, ImportedModelData& outModel) {
    constexpr double kSampleRate = 30.0;
    constexpr size_t kMaximumSamples = 3000;

    for (size_t stackIndex = 0; stackIndex < scene.anim_stacks.count; ++stackIndex) {
        const ufbx_anim_stack* stack = scene.anim_stacks.data[stackIndex];
        if (stack == nullptr || stack->anim == nullptr || stack->time_end <= stack->time_begin) {
            continue;
        }

        const double duration = stack->time_end - stack->time_begin;
        const size_t sampleCount = std::min(
            kMaximumSamples,
            std::max<size_t>(2, static_cast<size_t>(std::ceil(duration * kSampleRate)) + 1));
        ImportedAnimationClip clip{};
        clip.name = ToString(stack->name);
        if (clip.name.empty()) {
            clip.name = "FBX Animation " + std::to_string(stackIndex);
        }
        clip.duration = static_cast<float>(duration);

        for (const auto& entry : skeleton.nodeToJoint) {
            const ufbx_node* node = entry.first;
            ImportedAnimationChannel translation{};
            ImportedAnimationChannel rotation{};
            ImportedAnimationChannel scale{};
            translation.jointIndex = entry.second;
            rotation.jointIndex = entry.second;
            scale.jointIndex = entry.second;
            translation.jointName = ToString(node->name);
            rotation.jointName = translation.jointName;
            scale.jointName = translation.jointName;
            translation.path = ImportedAnimationChannel::Path::Translation;
            rotation.path = ImportedAnimationChannel::Path::Rotation;
            scale.path = ImportedAnimationChannel::Path::Scale;
            translation.times.reserve(sampleCount);
            rotation.times.reserve(sampleCount);
            scale.times.reserve(sampleCount);
            translation.values.reserve(sampleCount);
            rotation.values.reserve(sampleCount);
            scale.values.reserve(sampleCount);

            for (size_t sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
                const double alpha = static_cast<double>(sampleIndex) / static_cast<double>(sampleCount - 1);
                const double absoluteTime = stack->time_begin + duration * alpha;
                const ufbx_transform transform = ufbx_evaluate_transform(stack->anim, node, absoluteTime);
                const float relativeTime = static_cast<float>(duration * alpha);
                translation.times.push_back(relativeTime);
                rotation.times.push_back(relativeTime);
                scale.times.push_back(relativeTime);
                translation.values.emplace_back(
                    static_cast<float>(transform.translation.x),
                    static_cast<float>(transform.translation.y),
                    static_cast<float>(transform.translation.z),
                    0.0f);
                rotation.values.emplace_back(
                    static_cast<float>(transform.rotation.x),
                    static_cast<float>(transform.rotation.y),
                    static_cast<float>(transform.rotation.z),
                    static_cast<float>(transform.rotation.w));
                scale.values.emplace_back(
                    static_cast<float>(transform.scale.x),
                    static_cast<float>(transform.scale.y),
                    static_cast<float>(transform.scale.z),
                    0.0f);
            }

            clip.channels.push_back(std::move(translation));
            clip.channels.push_back(std::move(rotation));
            clip.channels.push_back(std::move(scale));
        }

        if (!clip.channels.empty()) {
            outModel.animations.push_back(std::move(clip));
        }
    }
}

} // namespace

bool AssetImporter::ImportFBX(const std::string& path, ImportedModelData& outModel, std::string* outError) {
    outModel = {};
    if (outError) {
        outError->clear();
    }

    if (!std::filesystem::exists(path)) {
        if (outError) {
            *outError = "File does not exist: " + path;
        }
        return false;
    }

    ufbx_load_opts options{};
    options.load_external_files = true;
    options.ignore_missing_external_files = true;
    options.generate_missing_normals = true;
    options.clean_skin_weights = true;

    ufbx_error error{};
    ufbx_scene* scene = ufbx_load_file(path.c_str(), &options, &error);
    if (scene == nullptr) {
        if (outError) {
            *outError = ToString(error.description);
        }
        return false;
    }

    outModel.sourcePath = path;
    FbxSkeleton skeleton;
    const bool skeletonLoaded = BuildSkeleton(*scene, outModel, skeleton, outError);
    if (!skeletonLoaded) {
        ufbx_free_scene(scene);
        return false;
    }

    for (size_t nodeIndex = 0; nodeIndex < scene->nodes.count; ++nodeIndex) {
        const ufbx_node* node = scene->nodes.data[nodeIndex];
        if (node != nullptr && node->mesh != nullptr) {
            ImportMeshNode(*node, skeleton, outModel);
        }
    }
    ImportAnimations(*scene, skeleton, outModel);
    ufbx_free_scene(scene);

    // Mixamo (and many other exporters) always name the animation stack "mixamo.com" or
    // "Take 001", which makes every imported clip collide. Prefer the source file name instead.
    const std::string fileStem = std::filesystem::path(path).stem().string();
    if (outModel.animations.size() == 1) {
        outModel.animations.front().name = fileStem;
    } else {
        for (ImportedAnimationClip& clip : outModel.animations) {
            clip.name = fileStem + " - " + clip.name;
        }
    }

    if (outModel.meshes.empty() && outModel.animations.empty()) {
        if (outError) {
            *outError = "FBX parsing succeeded but produced no renderable meshes";
        }
        return false;
    }
    return true;
}

} // namespace Piece

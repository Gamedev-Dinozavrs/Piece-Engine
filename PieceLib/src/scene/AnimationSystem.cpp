#include <PiecePCH.h>

#include <scene/AnimationSystem.h>

#include <scene/Components.h>
#include <scene/Entity.h>
#include <scene/Scene.h>

#include <algorithm>
#include <cmath>

namespace Piece::AnimationSystem {

namespace {

struct LocalPose {
    glm::vec3 translation{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
};

glm::vec4 SampleChannel(const ImportedAnimationChannel& channel, float time) {
    if (channel.times.empty() || channel.values.empty()) {
        return {};
    }
    if (time <= channel.times.front()) {
        return channel.values.front();
    }
    if (time >= channel.times.back()) {
        return channel.values.back();
    }

    const auto upper = std::upper_bound(channel.times.begin(), channel.times.end(), time);
    const std::size_t nextIndex = static_cast<std::size_t>(upper - channel.times.begin());
    const std::size_t previousIndex = nextIndex - 1;
    if (channel.stepInterpolation) {
        return channel.values[previousIndex];
    }

    const float range = channel.times[nextIndex] - channel.times[previousIndex];
    const float alpha = range > 0.0f ? (time - channel.times[previousIndex]) / range : 0.0f;
    return glm::mix(channel.values[previousIndex], channel.values[nextIndex], alpha);
}

void EvaluateAnimator(AnimatorComponent& animator, float deltaTime) {
    if (animator.joints.empty()) {
        return;
    }

    if (animator.boneMatrices.size() != animator.joints.size()) {
        animator.boneMatrices.resize(animator.joints.size(), glm::mat4(1.0f));
    }
    if (animator.clips.empty()) {
        for (std::size_t i = 0; i < animator.joints.size(); ++i) {
            const ImportedJoint& joint = animator.joints[i];
            const glm::mat4 local = glm::translate(glm::mat4(1.0f), joint.bindTranslation)
                * glm::toMat4(glm::quat(joint.bindRotation.w, joint.bindRotation.x, joint.bindRotation.y, joint.bindRotation.z))
                * glm::scale(glm::mat4(1.0f), joint.bindScale);
            const glm::mat4 parent = joint.parentIndex >= 0 ? animator.boneMatrices[static_cast<std::size_t>(joint.parentIndex)] : glm::mat4(1.0f);
            animator.boneMatrices[i] = parent * local * joint.inverseBindMatrix;
        }
        return;
    }

    animator.currentClip = std::min(animator.currentClip, static_cast<uint32_t>(animator.clips.size() - 1));
    const ImportedAnimationClip& clip = animator.clips[animator.currentClip];
    if (animator.playing && clip.duration > 0.0f) {
        animator.time = std::fmod(animator.time + deltaTime * animator.speed, clip.duration);
        if (animator.time < 0.0f) {
            animator.time += clip.duration;
        }
    }

    std::vector<LocalPose> pose(animator.joints.size());
    for (std::size_t i = 0; i < animator.joints.size(); ++i) {
        const ImportedJoint& joint = animator.joints[i];
        pose[i].translation = joint.bindTranslation;
        pose[i].rotation = glm::quat(joint.bindRotation.w, joint.bindRotation.x, joint.bindRotation.y, joint.bindRotation.z);
        pose[i].scale = joint.bindScale;
    }

    for (const ImportedAnimationChannel& channel : clip.channels) {
        if (channel.jointIndex < 0 || static_cast<std::size_t>(channel.jointIndex) >= pose.size()) {
            continue;
        }
        const glm::vec4 value = SampleChannel(channel, animator.time);
        LocalPose& jointPose = pose[static_cast<std::size_t>(channel.jointIndex)];
        switch (channel.path) {
        case ImportedAnimationChannel::Path::Translation:
            jointPose.translation = glm::vec3(value);
            break;
        case ImportedAnimationChannel::Path::Rotation:
            jointPose.rotation = glm::normalize(glm::quat(value.w, value.x, value.y, value.z));
            break;
        case ImportedAnimationChannel::Path::Scale:
            jointPose.scale = glm::vec3(value);
            break;
        }
    }

    std::vector<glm::mat4> globalMatrices(animator.joints.size(), glm::mat4(1.0f));
    std::vector<bool> evaluated(animator.joints.size(), false);
    std::function<glm::mat4(std::size_t)> evaluateGlobal = [&](std::size_t jointIndex) {
        if (evaluated[jointIndex]) {
            return globalMatrices[jointIndex];
        }

        const LocalPose& jointPose = pose[jointIndex];
        const glm::mat4 local = glm::translate(glm::mat4(1.0f), jointPose.translation)
            * glm::toMat4(jointPose.rotation)
            * glm::scale(glm::mat4(1.0f), jointPose.scale);
        const int32_t parentIndex = animator.joints[jointIndex].parentIndex;
        globalMatrices[jointIndex] = parentIndex >= 0
            && static_cast<std::size_t>(parentIndex) < animator.joints.size()
            ? evaluateGlobal(static_cast<std::size_t>(parentIndex)) * local
            : local;
        evaluated[jointIndex] = true;
        return globalMatrices[jointIndex];
    };

    for (std::size_t i = 0; i < animator.joints.size(); ++i) {
        animator.boneMatrices[i] = evaluateGlobal(i) * animator.joints[i].inverseBindMatrix;
    }
}

} // namespace

void Update(Scene& scene, Timestep timestep) {
    auto view = scene.GetAllEntitiesViewWith<AnimatorComponent>();
    for (auto entityHandle : view) {
        auto& animator = view.get<AnimatorComponent>(entityHandle);
        EvaluateAnimator(animator, static_cast<float>(timestep));
    }
}

} // namespace Piece::AnimationSystem
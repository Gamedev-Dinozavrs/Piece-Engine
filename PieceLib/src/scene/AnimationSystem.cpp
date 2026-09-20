#include <PiecePCH.h>

#include <scene/AnimationSystem.h>

#include <scene/Components.h>
#include <scene/Entity.h>
#include <scene/Scene.h>
#include <core/Input.h>
#include <core/KeyCodes.h>

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

std::vector<LocalPose> BuildBindLocalPoses(const std::vector<ImportedJoint>& joints) {
    std::vector<LocalPose> pose(joints.size());
    for (std::size_t i = 0; i < joints.size(); ++i) {
        pose[i].translation = joints[i].bindTranslation;
        pose[i].rotation = glm::quat(joints[i].bindRotation.w, joints[i].bindRotation.x, joints[i].bindRotation.y, joints[i].bindRotation.z);
        pose[i].scale = joints[i].bindScale;
    }
    return pose;
}

std::vector<LocalPose> BuildClipLocalPoses(const std::vector<ImportedJoint>& joints, const ImportedAnimationClip& clip, float time) {
    std::vector<LocalPose> pose = BuildBindLocalPoses(joints);
    for (const ImportedAnimationChannel& channel : clip.channels) {
        int32_t jointIndex = channel.jointIndex;
        const bool storedIndexMatches = jointIndex >= 0
            && static_cast<std::size_t>(jointIndex) < joints.size()
            && (channel.jointName.empty() || joints[static_cast<std::size_t>(jointIndex)].name == channel.jointName);
        if (!storedIndexMatches && !channel.jointName.empty()) {
            const auto jointIt = std::find_if(joints.begin(), joints.end(), [&](const ImportedJoint& joint) {
                return joint.name == channel.jointName;
            });
            jointIndex = jointIt == joints.end() ? -1 : static_cast<int32_t>(jointIt - joints.begin());
        }
        if (jointIndex < 0 || static_cast<std::size_t>(jointIndex) >= pose.size()) {
            continue;
        }
        const glm::vec4 value = SampleChannel(channel, time);
        LocalPose& jointPose = pose[static_cast<std::size_t>(jointIndex)];
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
    return pose;
}

std::vector<glm::mat4> EvaluateFromLocalPoses(const std::vector<ImportedJoint>& joints, const std::vector<LocalPose>& pose) {
    std::vector<glm::mat4> globalMatrices(joints.size(), glm::mat4(1.0f));
    std::vector<bool> evaluated(joints.size(), false);
    std::function<glm::mat4(std::size_t)> evaluateGlobal = [&](std::size_t jointIndex) {
        if (evaluated[jointIndex]) {
            return globalMatrices[jointIndex];
        }

        const LocalPose& jointPose = pose[jointIndex];
        const glm::mat4 local = glm::translate(glm::mat4(1.0f), jointPose.translation)
            * glm::toMat4(jointPose.rotation)
            * glm::scale(glm::mat4(1.0f), jointPose.scale);
        const int32_t parentIndex = joints[jointIndex].parentIndex;
        globalMatrices[jointIndex] = parentIndex >= 0
            && static_cast<std::size_t>(parentIndex) < joints.size()
            ? evaluateGlobal(static_cast<std::size_t>(parentIndex)) * local
            : local;
        evaluated[jointIndex] = true;
        return globalMatrices[jointIndex];
    };

    std::vector<glm::mat4> boneMatrices(joints.size(), glm::mat4(1.0f));
    for (std::size_t i = 0; i < joints.size(); ++i) {
        boneMatrices[i] = evaluateGlobal(i) * joints[i].inverseBindMatrix;
    }
    return boneMatrices;
}

int32_t FindClipIndexByName(const AnimatorComponent& animator, const std::string& name) {
    for (std::size_t i = 0; i < animator.clips.size(); ++i) {
        if (animator.clips[i].name == name) {
            return static_cast<int32_t>(i);
        }
    }
    return -1;
}

// Legacy/quick-preview playback: plays animator.clips[currentClip] on a loop. Used only
// while the entity has no animator-controller states defined yet.
void EvaluateLegacyClip(AnimatorComponent& animator, float deltaTime) {
    if (animator.clips.empty()) {
        animator.boneMatrices = EvaluateFromLocalPoses(animator.joints, BuildBindLocalPoses(animator.joints));
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

    animator.boneMatrices = EvaluateFromLocalPoses(animator.joints, BuildClipLocalPoses(animator.joints, clip, animator.time));
}

bool ConditionPasses(const AnimatorController& controller, const AnimationCondition& condition) {
    for (const AnimationParameter& param : controller.parameters) {
        if (param.name != condition.parameterName) {
            continue;
        }

        switch (param.type) {
        case AnimationParameterType::Float:
            switch (condition.comparison) {
            case AnimationComparison::Equals: return std::abs(param.floatValue - condition.threshold) < 0.0001f;
            case AnimationComparison::NotEquals: return std::abs(param.floatValue - condition.threshold) >= 0.0001f;
            case AnimationComparison::Greater: return param.floatValue > condition.threshold;
            case AnimationComparison::Less: return param.floatValue < condition.threshold;
            case AnimationComparison::GreaterOrEqual: return param.floatValue >= condition.threshold;
            case AnimationComparison::LessOrEqual: return param.floatValue <= condition.threshold;
            }
            return false;
        case AnimationParameterType::Bool: {
            const bool target = condition.threshold != 0.0f;
            return condition.comparison == AnimationComparison::NotEquals ? (param.boolValue != target) : (param.boolValue == target);
        }
        case AnimationParameterType::Trigger:
            return param.triggerValue;
        case AnimationParameterType::KeyPressed: {
            const bool target = condition.threshold != 0.0f;
            return condition.comparison == AnimationComparison::NotEquals ? (param.boolValue != target) : (param.boolValue == target);
        }
        }
        return false;
    }
    return false;
}

bool TransitionPasses(const AnimatorController& controller, const AnimationTransition& transition, float normalizedTime) {
    if (transition.hasExitTime && normalizedTime < transition.exitTime) {
        return false;
    }
    for (const AnimationCondition& condition : transition.conditions) {
        if (!ConditionPasses(controller, condition)) {
            return false;
        }
    }
    return true;
}

void ConsumeTriggers(AnimatorController& controller, const AnimationTransition& transition) {
    for (const AnimationCondition& condition : transition.conditions) {
        for (AnimationParameter& param : controller.parameters) {
            if (param.name == condition.parameterName && param.type == AnimationParameterType::Trigger) {
                param.triggerValue = false;
            }
        }
    }
}

// Drives the per-entity animator graph: advances the active state's clip, evaluates
// transitions against the entity's own parameters, and cross-fades bone matrices on switch.
void EvaluateStateMachine(AnimatorComponent& animator, float deltaTime) {
    AnimatorController& controller = animator.controller;

    for (AnimationParameter& param : controller.parameters) {
        if (param.type == AnimationParameterType::KeyPressed) {
            param.boolValue = Input::IsKeyPressed(static_cast<KeyCode>(param.keyCode));
        }
    }

    if (animator.currentState < 0 || animator.currentState >= static_cast<int32_t>(controller.states.size())) {
        animator.currentState = (controller.entryState >= 0 && controller.entryState < static_cast<int32_t>(controller.states.size()))
            ? controller.entryState
            : 0;
        animator.stateTime = 0.0f;
        animator.previousState = -1;
    }

    const AnimationState* state = &controller.states[static_cast<std::size_t>(animator.currentState)];
    int32_t clipIndex = FindClipIndexByName(animator, state->clipName);
    const ImportedAnimationClip* clip = clipIndex >= 0 ? &animator.clips[static_cast<std::size_t>(clipIndex)] : nullptr;

    if (animator.playing && clip != nullptr && clip->duration > 0.0f) {
        animator.stateTime += deltaTime * animator.speed * state->speed;
        if (state->loop) {
            animator.stateTime = std::fmod(animator.stateTime, clip->duration);
            if (animator.stateTime < 0.0f) {
                animator.stateTime += clip->duration;
            }
        } else {
            animator.stateTime = std::min(animator.stateTime, clip->duration);
        }
    }

    const float normalizedTime = (clip != nullptr && clip->duration > 0.0f) ? (animator.stateTime / clip->duration) : 0.0f;

    for (const AnimationTransition& transition : controller.transitions) {
        const bool fromMatches = transition.fromState == animator.currentState || transition.fromState == -1;
        if (!fromMatches || transition.toState == animator.currentState) {
            continue;
        }
        if (transition.toState < 0 || transition.toState >= static_cast<int32_t>(controller.states.size())) {
            continue;
        }
        if (!TransitionPasses(controller, transition, normalizedTime)) {
            continue;
        }

        ConsumeTriggers(controller, transition);
        animator.previousBoneMatrices = animator.boneMatrices;
        animator.previousState = animator.currentState;
        animator.currentState = transition.toState;
        animator.stateTime = 0.0f;
        animator.blendElapsed = 0.0f;
        animator.activeBlendDuration = std::max(0.0f, transition.blendDuration);
        break;
    }

    state = &controller.states[static_cast<std::size_t>(animator.currentState)];
    clipIndex = FindClipIndexByName(animator, state->clipName);
    clip = clipIndex >= 0 ? &animator.clips[static_cast<std::size_t>(clipIndex)] : nullptr;

    std::vector<glm::mat4> newPose = clip != nullptr
        ? EvaluateFromLocalPoses(animator.joints, BuildClipLocalPoses(animator.joints, *clip, animator.stateTime))
        : EvaluateFromLocalPoses(animator.joints, BuildBindLocalPoses(animator.joints));

    if (animator.previousState >= 0 && animator.activeBlendDuration > 0.0f && animator.blendElapsed < animator.activeBlendDuration) {
        const float blendAlpha = std::clamp(animator.blendElapsed / animator.activeBlendDuration, 0.0f, 1.0f);
        std::vector<glm::mat4> blended(newPose.size(), glm::mat4(1.0f));
        for (std::size_t i = 0; i < newPose.size(); ++i) {
            const glm::mat4 previous = i < animator.previousBoneMatrices.size() ? animator.previousBoneMatrices[i] : glm::mat4(1.0f);
            blended[i] = previous + (newPose[i] - previous) * blendAlpha;
        }
        animator.boneMatrices = std::move(blended);
        animator.blendElapsed += deltaTime;
        if (animator.blendElapsed >= animator.activeBlendDuration) {
            animator.previousState = -1;
        }
    } else {
        animator.boneMatrices = std::move(newPose);
        animator.previousState = -1;
    }
}

} // namespace

void Update(Scene& scene, Timestep timestep) {
    auto view = scene.GetAllEntitiesViewWith<AnimatorComponent>();
    for (auto entityHandle : view) {
        auto& animator = view.get<AnimatorComponent>(entityHandle);
        if (animator.joints.empty()) {
            continue;
        }
        if (animator.boneMatrices.size() != animator.joints.size()) {
            animator.boneMatrices.resize(animator.joints.size(), glm::mat4(1.0f));
        }

        if (!animator.controller.states.empty()) {
            EvaluateStateMachine(animator, static_cast<float>(timestep));
        } else {
            EvaluateLegacyClip(animator, static_cast<float>(timestep));
        }
    }
}

} // namespace Piece::AnimationSystem

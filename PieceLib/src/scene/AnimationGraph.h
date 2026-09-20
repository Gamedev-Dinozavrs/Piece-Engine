#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Piece {

enum class AnimationParameterType : uint8_t {
    Float = 0,
    Bool,
    Trigger,
    KeyPressed
};

// Named runtime value read by transition conditions. Owned per-entity by AnimatorController,
// so two entities sharing the same clips never share parameter state.
struct AnimationParameter {
    std::string name;
    AnimationParameterType type{AnimationParameterType::Float};
    float floatValue{0.0f};
    bool boolValue{false};
    bool triggerValue{false}; // consumed (reset to false) once a transition using it fires
    int32_t keyCode{0}; // KeyCode used when type == KeyPressed
};

enum class AnimationComparison : uint8_t {
    Equals = 0,
    NotEquals,
    Greater,
    Less,
    GreaterOrEqual,
    LessOrEqual
};

struct AnimationCondition {
    std::string parameterName;
    AnimationComparison comparison{AnimationComparison::Equals};
    float threshold{0.0f};
};

// A node in the animator graph. References a clip by name (not index) so clips can be
// reloaded/reordered without invalidating the graph.
struct AnimationState {
    std::string name{"State"};
    std::string clipName;
    float speed{1.0f};
    bool loop{true};
    float canvasX{20.0f};
    float canvasY{20.0f};
};

// An edge in the animator graph. fromState == -1 means "any state".
struct AnimationTransition {
    int32_t fromState{-1};
    int32_t toState{-1};
    std::vector<AnimationCondition> conditions;
    bool hasExitTime{false};
    float exitTime{1.0f}; // normalized clip time [0..1]
    float blendDuration{0.15f};
};

// Per-entity animation state machine definition. Kept as plain data (no shared asset file yet)
// so each entity's graph is fully independent of every other entity's.
struct AnimatorController {
    std::vector<AnimationState> states;
    std::vector<AnimationTransition> transitions;
    std::vector<AnimationParameter> parameters;
    int32_t entryState{-1};
};

} // namespace Piece

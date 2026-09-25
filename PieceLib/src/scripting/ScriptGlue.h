#pragma once

#include <cstdint>

namespace Piece {

// Native-side functions exposed to managed scripts via ScriptEngine::RegisterEngineCallbacks.
// Signatures must stay blittable (plain numeric types/pointers) to match the C# delegates
// wrapped in Piece.ScriptCore.EngineBridge.
namespace ScriptGlue {

void GetPosition(int64_t entityId, float* outX, float* outY, float* outZ);
void SetPosition(int64_t entityId, float x, float y, float z);

// Rotation is in degrees, matching TransformComponent and the editor's Transform panel.
void GetRotation(int64_t entityId, float* outX, float* outY, float* outZ);
void SetRotation(int64_t entityId, float x, float y, float z);

void GetScale(int64_t entityId, float* outX, float* outY, float* outZ);
void SetScale(int64_t entityId, float x, float y, float z);

int32_t IsKeyPressed(int32_t keyCode);

} // namespace ScriptGlue
} // namespace Piece

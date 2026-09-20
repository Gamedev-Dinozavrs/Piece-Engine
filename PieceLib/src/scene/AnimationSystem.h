#pragma once

#include <core/Timestep.h>

namespace Piece {

class Scene;

namespace AnimationSystem {

void Update(Scene& scene, Timestep timestep);

} // namespace AnimationSystem

} // namespace Piece
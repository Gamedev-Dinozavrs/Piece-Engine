#pragma once

#include <layer/Layer.h>
#include <core/Timestep.h>

namespace Piece {

// The actual gameplay layer for the shipped game: loads/builds the starting scene
// and always runs it in "play" mode (there is no editor here).
class GameLayer : public Layer {
public:
    GameLayer();
    ~GameLayer() override = default;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(Timestep ts) override;
};

} // namespace Piece

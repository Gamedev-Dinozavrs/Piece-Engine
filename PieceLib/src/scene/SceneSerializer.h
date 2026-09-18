#pragma once

#include <core/Core.h>

#include <string>

namespace Piece {

class Scene;

class SceneSerializer {
public:
    explicit SceneSerializer(const Ref<Scene>& scene);

    void Serialize(const std::string& filepath);
    bool Deserialize(const std::string& filepath);

private:
    Ref<Scene> m_Scene;
};

} // namespace Piece

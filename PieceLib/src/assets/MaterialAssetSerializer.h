#pragma once

#include <scene/World.h>

#include <string>

namespace Piece {

class MaterialAssetSerializer {
public:
    static bool Serialize(const MaterialView& material, const std::string& filepath);
    static bool Deserialize(const std::string& filepath, MaterialView& material);
};

} // namespace Piece
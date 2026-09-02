#pragma once

#include "rendering/scene.h"
#include <string>

namespace xe {

class SceneLoader {
public:
    static Scene LoadFromFile(const std::string& path);
    static bool SaveToFile(const Scene& scene, const std::string& path);

    static std::string ToJson(const Scene& scene);
    static Scene FromJson(const std::string& text);
};

}  // namespace xe
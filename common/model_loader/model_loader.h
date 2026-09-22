#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "common/abstractions/asset_manager.h"
#include "common/util/resource_handles.h"
#include "lib/buffer/buffer.h"

namespace common {

struct ImageID {
  std::shared_ptr<const AssetManager::ImageData> imageData;
  std::string path;
};

struct AssetData {
  std::shared_ptr<const AssetManager::VertexData> vertexData;
  glm::mat4 model;

  ImageID diffuseTexture;
  ImageID normalTexture;
  ImageID metallicRoughnessTexture;
};

class ModelLoader {
public:
  ~ModelLoader() = default;
};

}  // namespace common

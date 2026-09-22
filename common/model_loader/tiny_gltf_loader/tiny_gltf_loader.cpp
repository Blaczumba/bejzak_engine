#include "tiny_gltf_loader.h"

#ifdef __ANDROID__
  #include <android/asset_manager.h>
#endif
#define TINYGLTF_IMPLEMENTATION
#include <cstdint>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <span>
#include <string>
#include <tinygltf/tiny_gltf.h>
#include <vector>

#include "common/abstractions/asset_manager.h"
#include "common/file/file.h"
#include "common/model_loader/model_loader.h"
#include "common/util/engine_exception.h"
#include "common/util/geometry.h"
#include "lib/buffer/buffer.h"

namespace common {

#ifdef __ANDROID__
void setAssetmanager(AAssetManager* assetManager) {
  tinygltf::asset_manager = assetManager;
}
#endif

namespace {

struct SharedData {
  tinygltf::Model model;
  std::vector<lib::Buffer<glm::vec3>> tangents;
};

glm::mat4 GetNodeTransform(const tinygltf::Node& node) {
  glm::mat4 mat(1.0f);

  if (node.matrix.size() == 16) {
    mat = glm::make_mat4(node.matrix.data());
  } else {
    if (node.translation.size() == 3) {
      mat = glm::translate(
          mat, glm::vec3(node.translation[0], node.translation[1], node.translation[2]));
    }
    if (node.rotation.size() == 4) {
      glm::quat quat(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
      mat *= glm::mat4_cast(quat);
    }
    if (node.scale.size() == 3) {
      mat = glm::scale(mat, glm::vec3(node.scale[0], node.scale[1], node.scale[2]));
    }
  }

  return mat;
}

std::span<const unsigned char> processAttribute(
    const tinygltf::Model& model, std::map<std::string, int> attributes,
    const std::string& attribute) {
  auto it = attributes.find(attribute);
  if (it == attributes.cend()) {
    return {};
  }

  const tinygltf::Accessor& accessor = model.accessors[it->second];
  const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
  const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
  return std::span<const unsigned char>(
      &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count);
}

std::span<const std::byte> getIndices(
    const tinygltf::Model& model, const tinygltf::Primitive& primitive, uint8_t* indexSize) {
  const tinygltf::Accessor& accessor = model.accessors[primitive.indices];
  const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
  const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

  size_t indicesCount = accessor.count;
  switch (accessor.componentType) {
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
      *indexSize = 1;
      break;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
      *indexSize = 2;
      break;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
      *indexSize = 4;
      break;
  }
  const size_t offset = accessor.byteOffset + bufferView.byteOffset;

  return std::span<const std::byte>(
      reinterpret_cast<const std::byte*>(&buffer.data[offset]), indicesCount * *indexSize);
}

ImageID getOrLoadTexture(
    std::shared_ptr<SharedData>& sharedData, const FileLoader& fileLoader, std::string_view baseDir,
    int textureIndex, AssetManager& assetManager,
    std::unordered_map<std::string, std::shared_ptr<const AssetManager::ImageData>>&
        textureCollisionMap) {
  if (textureIndex < 0) {
    return ImageID{{}, ""};
  }

  const tinygltf::Texture& tex = sharedData->model.textures[textureIndex];
  const tinygltf::Image& img = sharedData->model.images[tex.source];

  std::string key = !img.uri.empty() ? joinPaths(baseDir, img.uri) :
                                       joinPaths(baseDir, std::to_string(img.bufferView));

  auto [it, inserted] = textureCollisionMap.try_emplace(key);
  if (inserted) {
    if (!img.image.empty()) {
      ImageResource imageResource = {
        .width = static_cast<uint32_t>(img.width),
        .height = static_cast<uint32_t>(img.height),
        .mipLevels =
            static_cast<uint32_t>(std::floor(std::log2(std::max(img.width, img.height)))) + 1,
        .layerCount = 1,
        .subresources = lib::Buffer<ImageSubresource>{ImageSubresource{
          .layerCount = 1,
          .width = static_cast<uint32_t>(img.width),
          .height = static_cast<uint32_t>(img.height),
          .depth = 1,
        }},
        .data = img.image.data(),
        .size = img.image.size(),
      };
      it->second = assetManager.loadImageAsync(sharedData, std::move(imageResource));
    } else if (!img.uri.empty()) {
      it->second = assetManager.loadImageAsync([&, filePath = joinPaths(baseDir, img.uri)]() {
        return loadImage(fileLoader.loadFileToBuffer(filePath), filePath);
      });
    }
  }
  return ImageID{it->second, std::move(key)};
};

std::string getTextureUri(const tinygltf::Model& model, const tinygltf::ParameterMap& values,
                          const std::string& textureType) {
  auto it = values.find(textureType);
  if (it == values.cend()) {
    return std::string{};
  }
  const tinygltf::Texture& texture = model.textures[it->second.TextureIndex()];
  const tinygltf::Image& image = model.images[texture.source];
  return image.uri;
}

void processNode(
    common::AssetManager& assetManager, const FileLoader& fileLoader,
    std::shared_ptr<SharedData>& sharedData, const tinygltf::Node& node,
    const glm::mat4& parentTransform, std::vector<AssetData>& assets,
    std::unordered_map<std::string, std::shared_ptr<const AssetManager::ImageData>>&
        textureCollisionMap,
    const std::string& baseDir) {
  const glm::mat4 currentTransform = parentTransform * GetNodeTransform(node);

  if (node.mesh < 0) {
    for (int childIndex : node.children) {
      processNode(assetManager, fileLoader, sharedData, sharedData->model.nodes[childIndex],
                  currentTransform, assets, textureCollisionMap, baseDir);
    }
    return;
  }

  for (const tinygltf::Primitive& primitive : sharedData->model.meshes[node.mesh].primitives) {
    const std::map<std::string, int>& attributes = primitive.attributes;

    std::span<const unsigned char> positionsData =
        processAttribute(sharedData->model, attributes, "POSITION");
    std::span<const unsigned char> textureCoordsData =
        processAttribute(sharedData->model, attributes, "TEXCOORD_0");
    std::span<const unsigned char> normalsData =
        processAttribute(sharedData->model, attributes, "NORMAL");

    if (primitive.indices <= 0) {
      continue;
    }
    uint8_t indexSize;
    std::span<const std::byte> indicesBytes = getIndices(sharedData->model, primitive, &indexSize);

    ImageID diffuseID, normalID, metallicRoughnessID;
    if (primitive.material >= 0) {
      const tinygltf::Material& mat = sharedData->model.materials[primitive.material];
      diffuseID = getOrLoadTexture(
          sharedData, fileLoader, baseDir, mat.pbrMetallicRoughness.baseColorTexture.index,
          assetManager, textureCollisionMap);
      metallicRoughnessID = getOrLoadTexture(
          sharedData, fileLoader, baseDir, mat.pbrMetallicRoughness.metallicRoughnessTexture.index,
          assetManager, textureCollisionMap);
      normalID = getOrLoadTexture(sharedData, fileLoader, baseDir, mat.normalTexture.index,
                                  assetManager, textureCollisionMap);
    }

    if (diffuseID.path.empty() || normalID.path.empty() || metallicRoughnessID.path.empty()) {
      continue;
    }

    sharedData->tangents.push_back(createTangents(
        indexSize, indicesBytes,
        std::span(reinterpret_cast<const glm::vec3*>(positionsData.data()), positionsData.size()),
        std::span(reinterpret_cast<const glm::vec2*>(textureCoordsData.data()),
                  textureCoordsData.size())));

    static std::pair<std::string, std::string> orders[] = {
      {"PTNT", "0123"},
      {"P",    "0"   }
    };

    const std::array attributeDescriptions = common::createAttributeDescriptions(
        std::span(reinterpret_cast<const glm::vec3*>(positionsData.data()), positionsData.size()),
        std::span(
            reinterpret_cast<const glm::vec2*>(textureCoordsData.data()), textureCoordsData.size()),
        std::span(reinterpret_cast<const glm::vec3*>(normalsData.data()), normalsData.size()),
        std::span(reinterpret_cast<const glm::vec3*>(sharedData->tangents.back().data()),
                  sharedData->tangents.back().size()));

    std::shared_ptr<const AssetManager::VertexData> vertexResourceID =
        assetManager.loadVertexDataInterleavingAsync(
            sharedData, indicesBytes, getIndexType(indexSize),
            common::analyzeConfig(orders, attributeDescriptions));
    assets.push_back(AssetData{
      .vertexData = std::move(vertexResourceID),
      .model = currentTransform,
      .diffuseTexture = std::move(diffuseID),
      .normalTexture = std::move(normalID),
      .metallicRoughnessTexture = std::move(metallicRoughnessID)});
  }

  for (int childIndex : node.children) {
    processNode(assetManager, fileLoader, sharedData, sharedData->model.nodes[childIndex],
                currentTransform, assets, textureCollisionMap, baseDir);
  }
}

}  // namespace

std::vector<AssetData> LoadGltfFromFile(
    common::AssetManager& assetManager, const FileLoader& fileLoader, const std::string& filePath) {
  auto sharedData = std::make_shared<SharedData>();
  tinygltf::TinyGLTF loader;
  //  if (!std::filesystem::exists(std::filesystem::path(filePath))) {
  //    throw EngineException(std::format("{} does not exists in the filesystem.", filePath));
  //  }

  // loader.SetImageLoader(nullptr, nullptr);
  if (filePath.ends_with(".glb")) {
    loader.LoadBinaryFromFile(&sharedData->model, nullptr, nullptr, filePath);
  } else if (filePath.ends_with(".gltf")) {
    loader.LoadASCIIFromFile(&sharedData->model, nullptr, nullptr, filePath);
  } else {
    throw EngineException(
        std::format("GLTF loader cannot load {} which is not .gltf or .glb format.", filePath));
  }

  const std::string baseDir = std::filesystem::path(filePath).parent_path().string();
  std::vector<AssetData> assets;
  std::unordered_map<std::string, std::shared_ptr<const AssetManager::ImageData>>
      textureCollisionMap;
  for (const tinygltf::Scene& scene : sharedData->model.scenes) {
    for (int nodeIndex : scene.nodes) {
      const tinygltf::Node& node = sharedData->model.nodes[nodeIndex];
      processNode(assetManager, fileLoader, sharedData, node, glm::mat4(1.0f), assets,
                  textureCollisionMap, baseDir);
    }
  }
  return assets;
}

std::vector<AssetData> LoadGltfFromString(
    common::AssetManager& assetManager, const FileLoader& fileLoader, const std::string& dataString,
    const std::string& baseDir) {
  auto sharedData = std::make_shared<SharedData>();
  tinygltf::TinyGLTF loader;
  std::string error, warning;

  loader.SetImageLoader(nullptr, nullptr);
  loader.LoadASCIIFromString(
      &sharedData->model, &error, &warning, dataString.data(), dataString.size(), baseDir);

  std::vector<AssetData> assets;
  std::unordered_map<std::string, std::shared_ptr<const AssetManager::ImageData>>
      textureCollisionMap;
  for (const tinygltf::Scene& scene : sharedData->model.scenes) {
    for (int nodeIndex : scene.nodes) {
      const tinygltf::Node& node = sharedData->model.nodes[nodeIndex];
      processNode(assetManager, fileLoader, sharedData, node, glm::mat4(1.0f), assets,
                  textureCollisionMap, baseDir);
    }
  }
  return assets;
}

}  // namespace common

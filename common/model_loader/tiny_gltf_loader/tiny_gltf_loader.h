#pragma once

#include <any>
#include <map>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <tinygltf/tiny_gltf.h>
#include <vector>

#include "common/model_loader/model_loader.h"
#include "common/status/status.h"
#include "common/util/asset_manager.h"
#include "common/util/geometry.h"
#include "common/util/primitives.h"
#include "lib/buffer/shared_buffer.h"

namespace {

template <typename AssetManagerImpl>
Status processNode(common::AssetManager<AssetManagerImpl>& assetManager, std::any& model,
                   const tinygltf::Node& node, const glm::mat4& parentTransform,
                   std::vector<VertexData>& vertexDataList, const std::string& baseDir);

}  // namespace

template <typename AssetManagerImpl>
ErrorOr<std::vector<VertexData>> LoadGltfFromFile(
    common::AssetManager<AssetManagerImpl>& assetManager, const std::string& filePath) {
  std::any model = std::make_shared<tinygltf::Model>();
  tinygltf::Model& modelRef = *std::any_cast<std::shared_ptr<tinygltf::Model>&>(model);
  tinygltf::TinyGLTF loader;

  if (filePath.ends_with(".glb")) {
    loader.LoadBinaryFromFile(&modelRef, nullptr, nullptr, filePath);
  } else if (filePath.ends_with(".gltf")) {
    loader.LoadASCIIFromFile(&modelRef, nullptr, nullptr, filePath);
  } else {
    return Error(EngineError::LOAD_FAILURE);
  }

  const std::string baseDir = std::filesystem::path(filePath).parent_path().string();
  std::vector<VertexData> vertexDataList;
  for (const tinygltf::Scene& scene : modelRef.scenes) {
    for (int nodeIndex : scene.nodes) {
      const tinygltf::Node& node = modelRef.nodes[nodeIndex];
      RETURN_IF_ERROR(processNode(assetManager, model, node, glm::mat4(1.0f), vertexDataList, baseDir));
    }
  }
  return vertexDataList;
}

template <typename AssetManagerImpl>
ErrorOr<std::vector<VertexData>> LoadGltfFromString(
    common::AssetManager<AssetManagerImpl>& assetManager, const std::string& dataString,
    const std::string& baseDir) {
  std::any model = std::make_shared<tinygltf::Model>();
  tinygltf::Model& modelRef = *std::any_cast<std::shared_ptr<tinygltf::Model>&>(model);
  tinygltf::TinyGLTF loader;
  std::string error, warning;

  loader.LoadASCIIFromString(
      &modelRef, &error, &warning, dataString.data(), dataString.size(), baseDir);

  std::vector<VertexData> vertexDataList;
  for (const tinygltf::Scene& scene : modelRef.scenes) {
    for (int nodeIndex : scene.nodes) {
      const tinygltf::Node& node = modelRef.nodes[nodeIndex];
      RETURN_IF_ERROR(
          processNode(assetManager, model, node, glm::mat4(1.0f), vertexDataList, baseDir));
    }
  }
  return vertexDataList;
}

namespace {

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

template <typename AssetManagerImpl>
Status processNode(common::AssetManager<AssetManagerImpl>& assetManager, std::any& model,
                   const tinygltf::Node& node, const glm::mat4& parentTransform,
                   std::vector<VertexData>& vertexDataList, const std::string& baseDir) {
  tinygltf::Model& modelRef = *std::any_cast<std::shared_ptr<tinygltf::Model>&>(model);
  const glm::mat4 currentTransform = parentTransform * GetNodeTransform(node);

  if (node.mesh < 0) {
    for (int childIndex : node.children) {
      processNode(assetManager, model, modelRef.nodes[childIndex], currentTransform, vertexDataList,
                  baseDir);
    }
    return StatusOk();
  }

  for (const tinygltf::Primitive& primitive : modelRef.meshes[node.mesh].primitives) {
    const std::map<std::string, int>& attributes = primitive.attributes;

    std::span<const unsigned char> positionsData =
        processAttribute(modelRef, attributes, "POSITION");
    lib::SharedBuffer<glm::vec3> positions(
        reinterpret_cast<const glm::vec3*>(positionsData.data()), positionsData.size());

    std::span<const unsigned char> textureCoordsData =
        processAttribute(modelRef, attributes, "TEXCOORD_0");
    std::span<const unsigned char> normalsData = processAttribute(modelRef, attributes, "NORMAL");

    if (primitive.indices <= 0) {
      continue;
    }
    uint8_t indexSize;
    std::span<const std::byte> indicesBytes = getIndices(modelRef, primitive, &indexSize);

    std::string diffuseTexture;
    std::string metallicRoughnessTexture;
    std::string normalTexture;
    if (primitive.material >= 0) {
      const tinygltf::Material& material = modelRef.materials[primitive.material];
      diffuseTexture = getTextureUri(modelRef, material.values, "baseColorTexture");
      metallicRoughnessTexture =
          getTextureUri(modelRef, material.values, "metallicRoughnessTexture");
      normalTexture = getTextureUri(modelRef, material.additionalValues, "normalTexture");
    }

    if (diffuseTexture.empty() || metallicRoughnessTexture.empty() || normalTexture.empty()) {
      continue;
    }

    // TODO: refactor
    static int objectCounter = 0;

    assetManager.loadVertexDataInterleavingAsync(
        model, std::to_string(objectCounter), indicesBytes, indexSize,
        std::span(reinterpret_cast<const glm::vec3*>(positionsData.data()), positionsData.size()),
        std::span(reinterpret_cast<const glm::vec2*>(textureCoordsData.data()), textureCoordsData.size()),
        std::span(reinterpret_cast<const glm::vec3*>(normalsData.data()), normalsData.size()));

    assetManager.loadImageAsync(baseDir + '/' + diffuseTexture);
    assetManager.loadImageAsync(baseDir + '/' + metallicRoughnessTexture);
    assetManager.loadImageAsync(baseDir + '/' + normalTexture);

    vertexDataList.emplace_back(
        std::move(positions), lib::SharedBuffer<glm::vec2>{}, lib::SharedBuffer<glm::vec3>{},
        lib::SharedBuffer<std::byte>{},
        indexSize, currentTransform, std::move(diffuseTexture), std::move(normalTexture), std::move(metallicRoughnessTexture),
        std::to_string(objectCounter++));
  }

  for (int childIndex : node.children) {
    RETURN_IF_ERROR(processNode(assetManager, model, modelRef.nodes[childIndex], currentTransform,
                                vertexDataList, baseDir));
  }
  return StatusOk();
}

}  // namespace

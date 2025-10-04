#pragma once

#include <any>
#include <map>
#include <span>
#include <string>
#include <vector>
#include <memory>
#include <tinygltf/tiny_gltf.h>

#include "common/model_loader/model_loader.h"
#include "common/status/status.h"
#include "common/util/asset_manager.h"
#include "common/util/primitives.h"
#include "lib/buffer/shared_buffer.h"
#include "common/util/geometry.h"

namespace {

template <typename AssetManagerImpl>
Status processNode(common::AssetManager<AssetManagerImpl>& assetManager, std::shared_ptr<tinygltf::Model>& model,
                 const tinygltf::Node& node,
                 const glm::mat4& parentTransform, std::vector<VertexData>& vertexDataList);

}  // namespace

template <typename AssetManagerImpl>
ErrorOr<std::vector<VertexData>> LoadGltfFromFile(
    common::AssetManager<AssetManagerImpl>& assetManager, const std::string& filePath) {
  std::shared_ptr<tinygltf::Model> model = std::make_shared<tinygltf::Model>();
  tinygltf::TinyGLTF loader;

  if (filePath.ends_with(".glb")) {
    loader.LoadBinaryFromFile(model.get(), nullptr, nullptr, filePath);
  } else if (filePath.ends_with(".gltf")) {
    loader.LoadASCIIFromFile(model.get(), nullptr, nullptr, filePath);
  } else {
    return Error(EngineError::LOAD_FAILURE);
  }

  std::vector<VertexData> vertexDataList;
  for (const tinygltf::Scene& scene : model->scenes) {
    for (int nodeIndex : scene.nodes) {
      const tinygltf::Node& node = model->nodes[nodeIndex];
      RETURN_IF_ERROR(processNode(assetManager, model, node, glm::mat4(1.0f), vertexDataList));
    }
  }
  return vertexDataList;
}

template <typename AssetManagerImpl>
ErrorOr<std::vector<VertexData>> LoadGltfFromString(
    common::AssetManager<AssetManagerImpl>& assetManager, const std::string& dataString,
    const std::string& baseDir) {
  std::shared_ptr<tinygltf::Model> model = std::make_shared<tinygltf::Model>();
  tinygltf::TinyGLTF loader;
  std::string error, warning;

  loader.LoadASCIIFromString(
      model.get(), &error, &warning, dataString.data(), dataString.size(), baseDir);

  std::vector<VertexData> vertexDataList;
  for (const tinygltf::Scene& scene : model->scenes) {
    for (int nodeIndex : scene.nodes) {
      const tinygltf::Node& node = model->nodes[nodeIndex];
      RETURN_IF_ERROR(processNode(assetManager, model, node, glm::mat4(1.0f), vertexDataList));
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

lib::Buffer<std::byte> createIndices(
    const tinygltf::Model& model, const tinygltf::Primitive& primitive, uint8_t* indexType) {
  const tinygltf::Accessor& accessor = model.accessors[primitive.indices];
  const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
  const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

  size_t indicesCount = accessor.count;
  *indexType = getMatchingIndexSize(indicesCount);
  const size_t offset = accessor.byteOffset + bufferView.byteOffset;

  switch (accessor.componentType) {
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
      return processIndices(
          std::span(reinterpret_cast<const uint8_t*>(&buffer.data[offset]), indicesCount),
          *indexType);
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
      return processIndices(
          std::span(reinterpret_cast<const uint16_t*>(&buffer.data[offset]), indicesCount),
          *indexType);
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
      return processIndices(
          std::span(reinterpret_cast<const uint32_t*>(&buffer.data[offset]), indicesCount),
          *indexType);
  }
  return lib::Buffer<std::byte>{};
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
Status processNode(common::AssetManager<AssetManagerImpl>& assetManager,
                 std::shared_ptr<tinygltf::Model>& model, const tinygltf::Node& node,
                 const glm::mat4& parentTransform, std::vector<VertexData>& vertexDataList) {
  const glm::mat4 currentTransform = parentTransform * GetNodeTransform(node);

  if (node.mesh < 0) {
    for (int childIndex : node.children) {
      processNode(assetManager, model, model->nodes[childIndex], currentTransform, vertexDataList);
    }
    return StatusOk();
  }

  for (const tinygltf::Primitive& primitive : model->meshes[node.mesh].primitives) {
    const std::map<std::string, int>& attributes = primitive.attributes;

    lib::SharedBuffer<glm::vec3> positions;
    lib::SharedBuffer<glm::vec2> texCoords;
    lib::SharedBuffer<glm::vec3> normals;

    if (std::span<const unsigned char> positionsData =
            processAttribute(*model, attributes, "POSITION");
        positionsData.data()) {
      const glm::vec3* data = reinterpret_cast<const glm::vec3*>(positionsData.data());
      positions = lib::SharedBuffer<glm::vec3>(data, data + positionsData.size());
    }

    if (std::span<const unsigned char> textureCoordsData =
            processAttribute(*model, attributes, "TEXCOORD_0");
        textureCoordsData.data()) {
      const glm::vec2* data = reinterpret_cast<const glm::vec2*>(textureCoordsData.data());
      texCoords = lib::SharedBuffer<glm::vec2>(data, data + textureCoordsData.size());
    }

    if (std::span<const unsigned char> normalsData = processAttribute(*model, attributes, "NORMAL");
        normalsData.data()) {
      const glm::vec3* data = reinterpret_cast<const glm::vec3*>(normalsData.data());
      normals = lib::SharedBuffer<glm::vec3>(data, data + normalsData.size());
    }

    // Load indices
    lib::SharedBuffer<std::byte> indicesBytes;
    uint8_t indexSize;
    if (primitive.indices >= 0) {
      indicesBytes = createIndices(*model, primitive, &indexSize);
    }

    // Load textures
    std::string diffuseTexture;
    std::string metallicRoughnessTexture;
    std::string normalTexture;
    if (primitive.material >= 0) {
      const tinygltf::Material& material = model->materials[primitive.material];
      diffuseTexture = getTextureUri(*model, material.values, "baseColorTexture");
      metallicRoughnessTexture = getTextureUri(*model, material.values, "metallicRoughnessTexture");
      normalTexture = getTextureUri(*model, material.additionalValues, "normalTexture");
    }
    vertexDataList.emplace_back(
        std::move(positions), std::move(texCoords), std::move(normals),
        std::move(indicesBytes), indexSize, currentTransform, std::move(diffuseTexture),
        std::move(normalTexture), std::move(metallicRoughnessTexture));
  }

  for (int childIndex : node.children) {
    RETURN_IF_ERROR(processNode(assetManager, model, model->nodes[childIndex], currentTransform, vertexDataList));
  }
  return StatusOk();
}

}  // namespace

#pragma once

#include <string>
#include <vector>

#include "common/model_loader/model_loader.h"
#include "common/status/status.h"
#include "common/util/asset_manager.h"

#include "tinygltf/tiny_gltf.h"

void ProcessNode(const tinygltf::Model& model, const tinygltf::Node& node,
                 const glm::mat4& parentTransform, std::vector<VertexData>& vertexDataList);

template<typename AssetManagerImpl>
ErrorOr<std::vector<VertexData>> LoadGltfFromFile(
    common::AssetManager<AssetManagerImpl>& assetManager, const std::string& filePath) {
  tinygltf::Model model;
  tinygltf::TinyGLTF loader;

  if (filePath.ends_with(".glb")) {
    loader.LoadBinaryFromFile(&model, nullptr, nullptr, filePath);
  } else if (filePath.ends_with(".gltf")) {
    loader.LoadASCIIFromFile(&model, nullptr, nullptr, filePath);
  } else {
    return Error(EngineError::LOAD_FAILURE);
  }

  std::vector<VertexData> vertexDataList;
  for (const tinygltf::Scene& scene : model.scenes) {
    for (int nodeIndex : scene.nodes) {
      const tinygltf::Node& node = model.nodes[nodeIndex];
      ProcessNode(model, node, glm::mat4(1.0f), vertexDataList);
    }
  }
  return vertexDataList;
}

template <typename AssetManagerImpl>
ErrorOr<std::vector<VertexData>> LoadGltfFromString(
    common::AssetManager<AssetManagerImpl>& assetManager, 
    const std::string& dataString, const std::string& baseDir){
  tinygltf::Model model;
  tinygltf::TinyGLTF loader;
  std::string error, warning;

  loader.LoadASCIIFromString(
      &model, &error, &warning, dataString.data(), dataString.size(), baseDir);

  std::vector<VertexData> vertexDataList;
  for (const tinygltf::Scene& scene : model.scenes) {
    for (int nodeIndex : scene.nodes) {
      const tinygltf::Node& node = model.nodes[nodeIndex];
      ProcessNode(model, node, glm::mat4(1.0f), vertexDataList);
    }
  }
  return vertexDataList;
}

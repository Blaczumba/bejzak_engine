#pragma once

#include <algorithm>
#include <iterator>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <tinyobjloader/tiny_obj_loader.h>
#include <unordered_map>

#include "common/model_loader/model_loader.h"
#include "common/status/status.h"
#include "common/util/asset_manager.h"
#include "common/util/primitives.h"
#include "lib/buffer/shared_buffer.h"

struct Indices {
  int a;
  int b;
  int c;

  struct Hash {
    std::size_t operator()(const Indices& triple) const {
      std::size_t hash = 0;
      hash ^= std::hash<int>{}(triple.a) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
      hash ^= std::hash<int>{}(triple.b) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
      hash ^= std::hash<int>{}(triple.c) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
      return hash;
    }
  };

  bool operator==(const Indices& other) const {
    return a == other.a && b == other.b && c == other.c;
  }
};

template <typename AssetManagerImpl>
ErrorOr<VertexData> loadObj(common::AssetManager<AssetManagerImpl>& assetManager,
                            const std::string& name, std::string& stringData) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warning, error;

  struct ModelData {
    std::vector<uint32_t> indices;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;
  };

  auto model = std::make_shared<ModelData>();

  std::istringstream dataStream(stringData);
  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warning, &error, &dataStream)) {
    return Error(EngineError::LOAD_FAILURE);
  }

  std::unordered_map<Indices, int, Indices::Hash> mp;
  for (const tinyobj::shape_t& shape : shapes) {
    for (const tinyobj::index_t& index : shape.mesh.indices) {
      const Indices idx = Indices{index.vertex_index, index.normal_index, index.texcoord_index};
      if (auto ptr = mp.find(idx); ptr != mp.cend()) {
        model->indices.push_back(ptr->second);
      } else {
        mp.insert({idx, static_cast<uint32_t>(model->positions.size())});

        model->indices.emplace_back(static_cast<uint32_t>(model->positions.size()));
        const int vertexIndex = 3 * index.vertex_index;
        const int texIndex = 2 * index.texcoord_index;
        const int normalIndex = 3 * index.normal_index;
        model->positions.emplace_back(
            attrib.vertices[vertexIndex], attrib.vertices[vertexIndex + 1],
            attrib.vertices[vertexIndex + 2]);
        model->texCoords.emplace_back(
            attrib.texcoords[texIndex], 1.0f - attrib.texcoords[texIndex + 1]);
        model->normals.emplace_back(attrib.normals[normalIndex], attrib.normals[normalIndex + 1],
                                    attrib.normals[normalIndex + 2]);
      }
    }
  }

  static constexpr uint8_t indexSize = 4;

  assetManager.loadVertexDataAsync(
      model, name,
      std::span(reinterpret_cast<const std::byte*>(model->indices.data()),
                model->indices.size() * indexSize),
      indexSize, std::span<const glm::vec3>(model->positions.data(), model->positions.size()));

  return VertexData{.indexSize = indexSize, .vertexResource = name};
}

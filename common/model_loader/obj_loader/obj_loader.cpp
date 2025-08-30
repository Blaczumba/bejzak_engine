#include "obj_loader.h"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <tinyobjloader/tiny_obj_loader.h>
#include <unordered_map>

#include "common/util/primitives.h"
#include "lib/buffer/shared_buffer.h"
#include "common/model_loader/model_loader.h"

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

ErrorOr<VertexData> loadObj(std::istringstream& data) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warning, error;

  std::vector<uint32_t> indices;
  std::vector<glm::vec3> positions;
  std::vector<glm::vec2> texCoords;
  std::vector<glm::vec3> normals;

  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warning, &error, &data)) {
    return Error(EngineError::LOAD_FAILURE);
  }

  std::unordered_map<Indices, int, Indices::Hash> mp;
  for (const tinyobj::shape_t& shape : shapes) {
    for (const tinyobj::index_t& index : shape.mesh.indices) {
      const Indices idx = Indices{index.vertex_index, index.normal_index, index.texcoord_index};
      if (auto ptr = mp.find(idx); ptr != mp.cend()) {
        indices.push_back(ptr->second);
      } else {
        mp.insert({idx, static_cast<uint32_t>(positions.size())});

        indices.emplace_back(static_cast<uint32_t>(positions.size()));
        const int vertexIndex = 3 * index.vertex_index;
        const int texIndex = 2 * index.texcoord_index;
        const int normalIndex = 3 * index.normal_index;
        positions.emplace_back(attrib.vertices[vertexIndex], attrib.vertices[vertexIndex + 1],
                               attrib.vertices[vertexIndex + 2]);
        texCoords.emplace_back(attrib.texcoords[texIndex], 1.0f - attrib.texcoords[texIndex + 1]);
        normals.emplace_back(attrib.normals[normalIndex], attrib.normals[normalIndex + 1],
                             attrib.normals[normalIndex + 2]);
      }
    }
  }
  IndexType indexType = getMatchingIndexType(indices.size());
  return VertexData{
    .positions = lib::SharedBuffer<glm::vec3>(positions.data(), positions.size()),
    .textureCoordinates = lib::SharedBuffer<glm::vec2>(texCoords.data(), texCoords.size()),
    .normals = lib::SharedBuffer<glm::vec3>(normals.data(), normals.size()),
    .indices = processIndices(std::span<const unsigned int>(indices), indexType),
    .indexType = indexType};
}

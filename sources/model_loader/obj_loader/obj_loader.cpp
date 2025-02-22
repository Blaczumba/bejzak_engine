#include "obj_loader.h"
#include "lib/buffer/buffer.h"
#include "model_loader/model_loader.h"
#include "primitives/primitives.h"

#include <tinyobjloader/tiny_obj_loader.h>

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <unordered_map>

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


VertexData loadObj(const std::string& filePath) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warning, error;

    std::vector<uint32_t> indices;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warning, &error, filePath.data())) {
        throw std::runtime_error(warning + error);
    }

    std::unordered_map<Indices, int, Indices::Hash> mp;
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Indices idx = Indices{ index.vertex_index, index.normal_index, index.texcoord_index };
            if (auto ptr = mp.find(idx); ptr != mp.cend()) {
                indices.push_back(ptr->second);
            }
            else {
                mp.insert({ idx, static_cast<uint32_t>(positions.size()) });

                indices.emplace_back(static_cast<uint32_t>(positions.size()));
                int vertexIndex = 3 * index.vertex_index;
                int texIndex = 2 * index.texcoord_index;
                int normalIndex = 3 * index.normal_index;
                positions.emplace_back(attrib.vertices[vertexIndex], attrib.vertices[vertexIndex + 1], attrib.vertices[vertexIndex + 2]);
                texCoords.emplace_back(attrib.texcoords[texIndex], 1.0f - attrib.texcoords[texIndex + 1]);
                normals.emplace_back(attrib.normals[normalIndex], attrib.normals[normalIndex + 1], attrib.normals[normalIndex + 2]);
            }
        }
    }
    IndexType indexType = getMatchingIndexType(indices.size());
    lib::Buffer<uint8_t> indicesBuffer(indices.size() * static_cast<size_t>(indexType));
    processIndices(indicesBuffer.data(), indices.data(), indices.size(), indexType);
    return VertexData{
        .positions = std::move(positions),
        .textureCoordinates = std::move(texCoords),
        .normals = std::move(normals),
        .indices = std::move(indicesBuffer),
        .indexType = indexType
    };
}

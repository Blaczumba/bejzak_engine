#pragma once

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


class TinyOBJLoaderVertex {
public:
    template<typename VertexT>
    static VertexData<VertexT> load(const std::string& filePath) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warning, error;
        
        std::vector<VertexT> vertices;
        std::vector<uint32_t> indices;
        
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
                    mp.insert({ idx, static_cast<uint32_t>(vertices.size()) });
        
                    VertexT vertex{};
                    if constexpr (VertexTraits<VertexT>::hasPosition) {
                        vertex.pos = {
                            attrib.vertices[3 * index.vertex_index + 0],
                            attrib.vertices[3 * index.vertex_index + 1],
                            attrib.vertices[3 * index.vertex_index + 2]
                        };
                    }
                    
                    if constexpr (VertexTraits<VertexT>::hasTexCoord) {
                        vertex.texCoord = {
                            attrib.texcoords[2 * index.texcoord_index + 0],
                            1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                        };
                    }
        
                    if constexpr (VertexTraits<VertexT>::hasNormal) {
                        vertex.normal = {
                            attrib.normals[3 * index.normal_index + 0],
                            attrib.normals[3 * index.normal_index + 1],
                            attrib.normals[3 * index.normal_index + 2]
                        };
                    }
        
                    indices.push_back(static_cast<uint32_t>(vertices.size()));
                    vertices.push_back(vertex);
                }
            }
        }
        lib::Buffer<glm::vec3> positions(vertices.size());
        lib::Buffer<glm::vec2> texCoords(vertices.size());
        lib::Buffer<glm::vec3> normals(vertices.size());
        for (size_t i = 0; i < vertices.size(); i++) {
            if constexpr (VertexTraits<VertexT>::hasPosition)
                positions[i] = vertices[i].pos;
            if constexpr (VertexTraits<VertexT>::hasTexCoord)
                texCoords[i] = vertices[i].texCoord;
            if constexpr (VertexTraits<VertexT>::hasNormal)
                normals[i] = vertices[i].normal;
        }
        IndexType indexType = getMatchingIndexType(indices.size());
        lib::Buffer<uint8_t> indicesBuffer(indices.size() * static_cast<size_t>(indexType));
        processIndices(indicesBuffer.data(), indices.data(), indices.size(), indexType);
        return VertexData<VertexT>{ 
            .vertices = std::move(vertices),
            .positions = std::move(positions),
            .textureCoordinates = std::move(texCoords),
            .normals = std::move(normals),
            .indices = std::move(indicesBuffer),
            .indexType = indexType
        };
    }
};



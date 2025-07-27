#include "tiny_gltf_loader.h"

#include "lib/buffer/shared_buffer.h"
#include "vulkan_wrapper/model_loader/model_loader.h"
#include "common/util/primitives.h"
#include "common/status/status.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tinygltf/tiny_gltf.h>

#include <map>
#include <span>
#include <string>
#include <vector>

glm::mat4 GetNodeTransform(const tinygltf::Node& node) {
    glm::mat4 mat(1.0f);

    if (node.matrix.size() == 16) {
        mat = glm::make_mat4(node.matrix.data());
    }
    else {
        if (node.translation.size() == 3) {
            mat = glm::translate(mat, glm::vec3(node.translation[0], node.translation[1], node.translation[2]));
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

std::span<const unsigned char> processAttribute(const tinygltf::Model& model, std::map<std::string, int> attributes, const std::string& attribute) {
    auto it = attributes.find(attribute);
    if (it == attributes.cend())
        return {};
    const tinygltf::Accessor& accessor = model.accessors[it->second];
    const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
    return std::span<const unsigned char>(&buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count);
}

lib::Buffer<uint8_t> createIndices(const tinygltf::Model& model, const tinygltf::Primitive& primitive, IndexType* indexType) {
    const tinygltf::Accessor& accessor = model.accessors[primitive.indices];
    const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

    size_t indicesCount = accessor.count;
    *indexType = getMatchingIndexType(indicesCount);
    const size_t offset = accessor.byteOffset + bufferView.byteOffset;

    switch (accessor.componentType) {
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
        return processIndices(std::span(reinterpret_cast<const uint8_t*>(&buffer.data[offset]), indicesCount), *indexType);
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
        return processIndices(std::span(reinterpret_cast<const uint16_t*>(&buffer.data[offset]), indicesCount), *indexType);
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
        return processIndices(std::span(reinterpret_cast<const uint32_t*>(&buffer.data[offset]), indicesCount), *indexType);
    }
    return lib::Buffer<uint8_t>{};
}

template<typename IndexType>
std::enable_if_t<std::is_unsigned<IndexType>::value, lib::Buffer<glm::vec3>> processTangents(std::span<const IndexType> indices, std::span<const glm::vec3> positions, std::span<const glm::vec2> texCoords) {
    lib::Buffer<glm::vec3> tangents(positions.size());
    for (size_t i = 0; i < indices.size(); i += 3) {
        const glm::vec3& pos0 = positions[indices[i]];
        glm::vec3 edge1 = positions[indices[i + 1]] - pos0;
        glm::vec3 edge2 = positions[indices[i + 2]] - pos0;

        const glm::vec2& texCoord0 = texCoords[indices[i]];
        glm::vec2 deltaUV1 = texCoords[indices[i + 1]] - texCoord0;
        glm::vec2 deltaUV2 = texCoords[indices[i + 2]] - texCoord0;

        // float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
        tangents[indices[i]] = tangents[indices[i + 1]] = tangents[indices[i + 2]] = glm::normalize(deltaUV2.y * edge1 - deltaUV1.y * edge2); // f scale
        //glm::vec3 bitangent = glm::normalize(-deltaUV2.x * edge1 + deltaUV1.x * edge2); // f scale
    }
    return tangents;
}

lib::Buffer<glm::vec3> createTangents(IndexType indexType, std::span<const uint8_t> indicesBytes, std::span<const glm::vec3> positions, std::span<const glm::vec2> texCoords) {
    const size_t indicesCount = indicesBytes.size() / static_cast<size_t>(indexType);
    switch (indexType) {
    case IndexType::UINT8:
        return processTangents(indicesBytes, positions, texCoords);
    case IndexType::UINT16:
        return processTangents(std::span(reinterpret_cast<const uint16_t*>(indicesBytes.data()), indicesCount), positions, texCoords);
    case IndexType::UINT32:
        return processTangents(std::span(reinterpret_cast<const uint32_t*>(indicesBytes.data()), indicesCount), positions, texCoords);
    }
    return lib::Buffer<glm::vec3>{};
}

std::string getTextureUri(const tinygltf::Model& model, const tinygltf::ParameterMap& values, const std::string& textureType) {
    auto it = values.find(textureType);
    if (it == values.cend()) {
        return std::string{};
    }
    const tinygltf::Texture& texture = model.textures[it->second.TextureIndex()];
    const tinygltf::Image& image = model.images[texture.source];
    return image.uri;
}

void ProcessNode(const tinygltf::Model& model, const tinygltf::Node& node, const glm::mat4& parentTransform, std::vector<VertexData>& vertexDataList) {
    const glm::mat4 currentTransform = parentTransform * GetNodeTransform(node);

    if (node.mesh < 0) {
        for (int childIndex : node.children) {
            ProcessNode(model, model.nodes[childIndex], currentTransform, vertexDataList);
        }
        return;
    }

    for (const tinygltf::Primitive& primitive : model.meshes[node.mesh].primitives) {
        const std::map<std::string, int>& attributes = primitive.attributes;

        lib::SharedBuffer<glm::vec3> positions;
        lib::SharedBuffer<glm::vec2> texCoords;
        lib::SharedBuffer<glm::vec3> normals;

        if (std::span<const unsigned char> positionsData = processAttribute(model, attributes, "POSITION"); positionsData.data()) {
            const glm::vec3* data = reinterpret_cast<const glm::vec3*>(positionsData.data());
            positions = lib::SharedBuffer<glm::vec3>(data, data + positionsData.size());
        }

        if (std::span<const unsigned char> textureCoordsData = processAttribute(model, attributes, "TEXCOORD_0");  textureCoordsData.data()) {
            const glm::vec2* data = reinterpret_cast<const glm::vec2*>(textureCoordsData.data());
            texCoords = lib::SharedBuffer<glm::vec2>(data, data + textureCoordsData.size());
        }

        if (std::span<const unsigned char> normalsData = processAttribute(model, attributes, "NORMAL");  normalsData.data()) {
            const glm::vec3* data = reinterpret_cast<const glm::vec3*>(normalsData.data());
            normals = lib::SharedBuffer<glm::vec3>(data, data + normalsData.size());
        }

        // Load indices
        lib::SharedBuffer<uint8_t> indicesBytes;
        IndexType indexType = IndexType::NONE;
        if (primitive.indices >= 0) {
            indicesBytes = createIndices(model, primitive, &indexType);
        }

        lib::SharedBuffer<glm::vec3> tangents = createTangents(indexType, indicesBytes, positions, texCoords);

        // Load textures
        std::string diffuseTexture;
        std::string metallicRoughnessTexture;
        std::string normalTexture;
        if (primitive.material >= 0) {
            const tinygltf::Material& material = model.materials[primitive.material];
            diffuseTexture = getTextureUri(model, material.values, "baseColorTexture");
            metallicRoughnessTexture = getTextureUri(model, material.values, "metallicRoughnessTexture");
            normalTexture = getTextureUri(model, material.additionalValues, "normalTexture");
        }
        vertexDataList.emplace_back(std::move(positions), std::move(texCoords), std::move(normals), std::move(tangents), std::move(indicesBytes), indexType, currentTransform, std::move(diffuseTexture), std::move(normalTexture), std::move(metallicRoughnessTexture));
    }

    for (int childIndex : node.children) {
        ProcessNode(model, model.nodes[childIndex], currentTransform, vertexDataList);
    }
}

ErrorOr<std::vector<VertexData>> LoadGltf(const std::string& filePath) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;

    if (filePath.ends_with(".glb") || filePath.ends_with(".bin"))
        loader.LoadBinaryFromFile(&model, nullptr, nullptr, filePath);
    else if (filePath.ends_with(".gltf"))
        loader.LoadASCIIFromFile(&model, nullptr, nullptr, filePath);
    else
        return Error(EngineError::LOAD_FAILURE);

    std::vector<VertexData> vertexDataList;
    for (const tinygltf::Scene& scene : model.scenes) {
        for (int nodeIndex : scene.nodes) {
            const tinygltf::Node& node = model.nodes[nodeIndex];
            ProcessNode(model, node, glm::mat4(1.0f), vertexDataList);
        }
    }
    return vertexDataList;
}
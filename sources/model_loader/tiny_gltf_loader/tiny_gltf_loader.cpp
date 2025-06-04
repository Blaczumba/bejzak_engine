#include "tiny_gltf_loader.h"

#include "lib/status/status.h"
#include "lib/buffer/shared_buffer.h"
#include "model_loader/model_loader.h"
#include "primitives/primitives.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tinygltf/tiny_gltf.h>

#include <expected>
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

template<typename IndexType>
std::enable_if_t<std::is_unsigned<IndexType>::value> processTangents(std::span<const IndexType> indices, std::span<const glm::vec3> positions, std::span<const glm::vec2> texCoords, std::span<glm::vec3> tangents) {
    for (size_t i = 0; i < indices.size(); i += 3) {
        const glm::vec3& pos0 = positions[indices[i]];
        const glm::vec3& pos1 = positions[indices[i + 1]];
        const glm::vec3& pos2 = positions[indices[i + 2]];

        const glm::vec2& texCoord0 = texCoords[indices[i]];
        const glm::vec2& texCoord1 = texCoords[indices[i + 1]];
        const glm::vec2& texCoord2 = texCoords[indices[i + 2]];

        glm::vec3 edge1 = pos1 - pos0;
        glm::vec3 edge2 = pos2 - pos0;
        glm::vec2 deltaUV1 = texCoord1 - texCoord0;
        glm::vec2 deltaUV2 = texCoord2 - texCoord0;

        // float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
        tangents[indices[i]] = tangents[indices[i + 1]] = tangents[indices[i + 2]] = glm::normalize(deltaUV2.y * edge1 - deltaUV1.y * edge2); // f scale
        //glm::vec3 bitangent = glm::normalize(-deltaUV2.x * edge1 + deltaUV1.x * edge2); // f scale
    }
}

std::string getTextureUri(const tinygltf::Model& model, const tinygltf::ParameterMap& values, const std::string& textureType) {
    auto it = values.find(textureType);
    if (it == values.end()) {
        return std::string{};
    }
    const tinygltf::Texture& texture = model.textures[it->second.TextureIndex()];
    const tinygltf::Image& image = model.images[texture.source];
    return image.uri;
}

void ProcessNode(const tinygltf::Model& model, const tinygltf::Node& node, glm::mat4 parentTransform, std::vector<VertexData>& vertexDataList) {
    glm::mat4 currentTransform = parentTransform * GetNodeTransform(node);

    if (node.mesh < 0) {
        for (const auto& childIndex : node.children) {
            ProcessNode(model, model.nodes[childIndex], currentTransform, vertexDataList);
        }
        return;
    }

    const auto& mesh = model.meshes[node.mesh];

    for (const auto& primitive : mesh.primitives) {
        const auto& attributes = primitive.attributes;

        lib::SharedBuffer<glm::vec3> positions;
        lib::SharedBuffer<glm::vec2> texCoords;
        lib::SharedBuffer<glm::vec3> normals;
        lib::SharedBuffer<glm::vec3> tangents;
        lib::SharedBuffer<glm::vec3> bitangents;

        lib::SharedBuffer<uint8_t> indices;
        uint32_t indicesCount;
        IndexType indexType;

        auto positionsData = processAttribute(model, attributes, "POSITION");
        if (positionsData.data()) {
            const glm::vec3* data = reinterpret_cast<const glm::vec3*>(positionsData.data());
            positions = lib::SharedBuffer<glm::vec3>(data, data + positionsData.size());
        }

        auto textureCoordsData = processAttribute(model, attributes, "TEXCOORD_0");
        if (textureCoordsData.data()) {
            const glm::vec2* data = reinterpret_cast<const glm::vec2*>(textureCoordsData.data());
            texCoords = lib::SharedBuffer<glm::vec2>(data, data + textureCoordsData.size());
        }

        auto normalsData = processAttribute(model, attributes, "NORMAL");
        if (normalsData.data()) {
            const glm::vec3* data = reinterpret_cast<const glm::vec3*>(normalsData.data());
            normals = lib::SharedBuffer<glm::vec3>(data, data + normalsData.size());
        }

        // Load indices
        if (primitive.indices >= 0) {
            const tinygltf::Accessor& accessor = model.accessors[primitive.indices];
            const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
            const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

            indicesCount = accessor.count;
            indexType = getMatchingIndexType(indicesCount);
            indices = lib::SharedBuffer<uint8_t>(indicesCount * static_cast<size_t>(indexType));

            // Determine the component type and process
            if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                const uint16_t* data = reinterpret_cast<const uint16_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                processIndices(indices.data(), data, indicesCount, indexType);
            }
            else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                const uint32_t* data = reinterpret_cast<const uint32_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                processIndices(indices.data(), data, indicesCount, indexType);
            }
            else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                const uint8_t* data = reinterpret_cast<const uint8_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                processIndices(indices.data(), data, indicesCount, indexType);
            }
        }

        tangents = lib::SharedBuffer<glm::vec3>(normals.size());
        switch (indexType) {
        case IndexType::UINT8:
            processTangents(std::span<const uint8_t>(indices), positions, texCoords, tangents);
            break;
        case IndexType::UINT16:
            processTangents(std::span(reinterpret_cast<const uint16_t*>(indices.data()), indicesCount), positions, texCoords, tangents);
            break;
        case IndexType::UINT32:
            processTangents(std::span(reinterpret_cast<const uint32_t*>(indices.data()), indicesCount), positions, texCoords, tangents);
        }

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
        vertexDataList.emplace_back(std::move(positions), std::move(texCoords), std::move(normals), std::move(tangents), std::move(indices), indexType, currentTransform, std::move(diffuseTexture), std::move(normalTexture), std::move(metallicRoughnessTexture));
    }

    for (const auto& childIndex : node.children) {
        ProcessNode(model, model.nodes[childIndex], currentTransform, vertexDataList);
    }
}

lib::ErrorOr<std::vector<VertexData>> LoadGltf(const std::string& filePath) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;
    bool ret;

    if (filePath.find(".glb") != std::string::npos || filePath.find(".bin") != std::string::npos)
        loader.LoadBinaryFromFile(&model, &err, &warn, filePath);
    else if (filePath.find(".gltf") != std::string::npos)
        loader.LoadASCIIFromFile(&model, &err, &warn, filePath);
    else
        return lib::Error("Failed to load gltf file.");

    std::vector<VertexData> vertexDataList;
    for (const auto& scene : model.scenes) {
        for (const auto& nodeIndex : scene.nodes) {
            const tinygltf::Node& node = model.nodes[nodeIndex];
            ProcessNode(model, node, glm::mat4(1.0f), vertexDataList);
        }
    }
    return vertexDataList;
}
#include "tiny_gltf_loader.h"

#ifdef __ANDROID__
  #include <android/asset_manager.h>
#endif

#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_IMPLEMENTATION

#include <algorithm>
#include <bit>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <tinygltf/tiny_gltf.h>
#include <unordered_map>
#include <utility>
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

// Data that must outlive LoadGltf*(): the async upload jobs read from it later.
// Every span handed to the AssetManager points directly into `model`'s immutable buffers,
// except tangents, which glTF may not provide in the vec3 layout the renderer uses.
struct SharedData {
  tinygltf::Model model;
  std::vector<lib::Buffer<glm::vec3>> tangents;
};

using TextureCache =
    std::unordered_map<std::string, std::shared_ptr<const AssetManager::ImageData>>;

struct PrimitiveAssets {
  std::shared_ptr<const AssetManager::VertexData> vertexData;
  ImageID diffuse;
  ImageID normal;
  ImageID metallicRoughness;
};

struct LoadContext {
  AssetManager& assetManager;
  const FileLoader& fileLoader;
  std::shared_ptr<SharedData> sharedData;
  std::string baseDir;
  std::string sourceName;  // file path (or base dir) used to key embedded images
  TextureCache textureCache{};
  std::map<std::pair<int, int>, std::optional<PrimitiveAssets>> primitiveCache{};
  std::vector<AssetData> assets{};
};

constexpr int kMaxNodeDepth = 256;  // guards against cyclic (malformed) node graphs

template <typename T>
bool inRange(const std::vector<T>& v, int index) {
  return index >= 0 && static_cast<size_t>(index) < v.size();
}

glm::mat4 getNodeTransform(const tinygltf::Node& node) {
  if (node.matrix.size() == 16) {
    return glm::mat4(glm::make_mat4(node.matrix.data()));
  }

  glm::mat4 transform(1.0f);
  if (node.translation.size() == 3) {
    transform = glm::translate(transform, glm::vec3(glm::make_vec3(node.translation.data())));
  }
  if (node.rotation.size() == 4) {
    // glTF stores (x, y, z, w); glm's constructor takes (w, x, y, z).
    const glm::quat rotation(
        static_cast<float>(node.rotation[3]), static_cast<float>(node.rotation[0]),
        static_cast<float>(node.rotation[1]), static_cast<float>(node.rotation[2]));
    transform *= glm::mat4_cast(rotation);
  }
  if (node.scale.size() == 3) {
    transform = glm::scale(transform, glm::vec3(glm::make_vec3(node.scale.data())));
  }
  return transform;  // T * R * S
}

struct AccessorView {
  const unsigned char* data = nullptr;
  size_t count = 0;
  size_t stride = 0;
  int componentType = -1;
  int componentCount = 0;
  bool normalized = false;
};

std::optional<AccessorView> viewAccessor(const tinygltf::Model& model, int accessorIndex) {
  if (!inRange(model.accessors, accessorIndex)) {
    return std::nullopt;
  }
  const tinygltf::Accessor& accessor = model.accessors[accessorIndex];
  if (accessor.sparse.isSparse || !inRange(model.bufferViews, accessor.bufferView)) {
    return std::nullopt;
  }
  const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
  if (!inRange(model.buffers, bufferView.buffer)) {
    return std::nullopt;
  }
  const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

  const int componentSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
  const int componentCount = tinygltf::GetNumComponentsInType(accessor.type);
  const int stride = accessor.ByteStride(bufferView);  // honours bufferView.byteStride
  if (componentSize <= 0 || componentCount <= 0 || stride <= 0) {
    return std::nullopt;
  }

  const size_t elementSize = static_cast<size_t>(componentSize) * componentCount;
  const size_t byteSpan =
      accessor.count == 0 ? 0 : (accessor.count - 1) * static_cast<size_t>(stride) + elementSize;
  const size_t start = bufferView.byteOffset + accessor.byteOffset;
  if (accessor.byteOffset + byteSpan > bufferView.byteLength
      || start + byteSpan > buffer.data.size()) {
    return std::nullopt;
  }

  return AccessorView{
    .data = buffer.data.data() + start,
    .count = accessor.count,
    .stride = static_cast<size_t>(stride),
    .componentType = accessor.componentType,
    .componentCount = componentCount,
    .normalized = accessor.normalized,
  };
}

// Returns a span pointing directly into the model's buffer. Only tightly packed float
// data maps onto glm vectors in place; anything else (interleaved buffer views,
// KHR_mesh_quantization integer formats) yields an empty span and the primitive is skipped.
template <typename Vec>
std::span<const Vec> viewAttribute(const tinygltf::Model& model, int accessorIndex) {
  const std::optional<AccessorView> view = viewAccessor(model, accessorIndex);
  if (!view || view->componentType != TINYGLTF_COMPONENT_TYPE_FLOAT
      || view->componentCount != Vec::length() || view->stride != sizeof(Vec)
      || reinterpret_cast<std::uintptr_t>(view->data) % alignof(Vec) != 0) {
    return {};
  }
  return {reinterpret_cast<const Vec*>(view->data), view->count};
}

int findAttribute(const tinygltf::Primitive& primitive, const std::string& name) {
  const auto it = primitive.attributes.find(name);
  return it == primitive.attributes.cend() ? -1 : it->second;
}

struct IndexData {
  std::span<const std::byte> bytes;
  uint8_t indexSize = 0;
};

template <std::unsigned_integral T>
bool allIndicesBelow(std::span<const std::byte> bytes, size_t limit) {
  const std::span<const T> indices(
      reinterpret_cast<const T*>(bytes.data()), bytes.size() / sizeof(T));
  return std::all_of(indices.begin(), indices.end(), [limit](T index) {
    return index < limit;
  });
}

// Non-indexed primitives (indices < 0) are not supported and yield std::nullopt.
std::optional<IndexData> readIndices(
    const tinygltf::Model& model, const tinygltf::Primitive& primitive, size_t vertexCount) {
  const std::optional<AccessorView> view = viewAccessor(model, primitive.indices);
  if (!view || view->componentCount != 1) {
    return std::nullopt;
  }

  uint8_t indexSize = 0;
  bool inBounds = false;
  const std::span<const std::byte> bytes(
      reinterpret_cast<const std::byte*>(view->data), view->count * view->stride);
  switch (view->componentType) {
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
      indexSize = 1;
      inBounds = allIndicesBelow<uint8_t>(bytes, vertexCount);
      break;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
      indexSize = 2;
      inBounds = allIndicesBelow<uint16_t>(bytes, vertexCount);
      break;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
      indexSize = 4;
      inBounds = allIndicesBelow<uint32_t>(bytes, vertexCount);
      break;
    default:
      return std::nullopt;
  }
  // The spec requires index data to be tightly packed; out-of-range indices would make
  // the GPU read past the vertex buffer.
  if (view->stride != indexSize || !inBounds) {
    return std::nullopt;
  }
  return IndexData{bytes, indexSize};
}

const tinygltf::Image* findImage(const tinygltf::Model& model, int textureIndex) {
  if (!inRange(model.textures, textureIndex)) {
    return nullptr;
  }
  const int source = model.textures[textureIndex].source;
  if (!inRange(model.images, source)) {
    return nullptr;  // e.g. only an extension source (KTX2 / WebP) is provided
  }
  const tinygltf::Image& image = model.images[source];
  const bool decoded = !image.image.empty() && image.width > 0 && image.height > 0;
  return decoded || !image.uri.empty() ? &image : nullptr;
}

ImageID loadTexture(LoadContext& ctx, int textureIndex) {
  const tinygltf::Model& model = ctx.sharedData->model;
  const tinygltf::Image* image = findImage(model, textureIndex);
  if (image == nullptr) {
    return ImageID{{}, ""};
  }

  // External images are keyed by path. Embedded ones (GLB buffer views, data URIs) are
  // keyed by source + image index; keying them by bufferView made every data-URI image
  // collide on "-1" and embedded images from different files in one folder collide.
  std::string key =
      image->uri.empty() ?
          std::format("{}#image{}", ctx.sourceName, model.textures[textureIndex].source) :
          joinPaths(ctx.baseDir, image->uri);

  auto [it, inserted] = ctx.textureCache.try_emplace(key);
  if (inserted) {
    if (!image->image.empty()) {
      const auto width = static_cast<uint32_t>(image->width);
      const auto height = static_cast<uint32_t>(image->height);
      ImageResource imageResource = {
        .width = width,
        .height = height,
        .mipLevels = static_cast<uint32_t>(std::bit_width(std::max(width, height))),
        .layerCount = 1,
        .subresources = lib::Buffer<ImageSubresource>{ImageSubresource{
          .layerCount = 1,
          .width = width,
          .height = height,
          .depth = 1,
        }},
        .data = image->image.data(),
        .size = image->image.size(),
      };
      it->second = ctx.assetManager.loadImageAsync(ctx.sharedData, std::move(imageResource));
    } else {
      it->second =
          ctx.assetManager.loadImageAsync([&fileLoader = ctx.fileLoader, filePath = key]() {
            return loadImage(fileLoader.loadFileToBuffer(filePath), filePath);
          });
    }
  }
  return ImageID{it->second, std::move(key)};
}

std::optional<PrimitiveAssets> buildPrimitive(
    LoadContext& ctx, const tinygltf::Primitive& primitive) {
  SharedData& shared = *ctx.sharedData;
  const tinygltf::Model& model = shared.model;

  // Only triangle lists are supported (tangent generation and the pipeline assume them).
  if (primitive.mode != TINYGLTF_MODE_TRIANGLES && primitive.mode != -1) {
    return std::nullopt;
  }

  // The renderer needs all three maps. Check before any upload starts so a rejected
  // primitive doesn't leave orphaned texture loads behind.
  if (!inRange(model.materials, primitive.material)) {
    return std::nullopt;
  }
  const tinygltf::Material& material = model.materials[primitive.material];
  const int diffuseIndex = material.pbrMetallicRoughness.baseColorTexture.index;
  const int metallicRoughnessIndex = material.pbrMetallicRoughness.metallicRoughnessTexture.index;
  const int normalIndex = material.normalTexture.index;
  if (!findImage(model, diffuseIndex) || !findImage(model, metallicRoughnessIndex)
      || !findImage(model, normalIndex)) {
    return std::nullopt;
  }

  const std::span<const glm::vec3> positions =
      viewAttribute<glm::vec3>(model, findAttribute(primitive, "POSITION"));
  const std::span<const glm::vec2> texCoords =
      viewAttribute<glm::vec2>(model, findAttribute(primitive, "TEXCOORD_0"));
  const std::span<const glm::vec3> normals =
      viewAttribute<glm::vec3>(model, findAttribute(primitive, "NORMAL"));

  const size_t vertexCount = positions.size();
  if (vertexCount == 0 || texCoords.size() != vertexCount || normals.size() != vertexCount) {
    return std::nullopt;
  }

  const std::optional<IndexData> indices = readIndices(model, primitive, vertexCount);
  if (!indices || indices->bytes.empty()) {
    return std::nullopt;
  }

  // Prefer authored tangents (usually MikkTSpace); generate them only when absent.
  // glTF tangents are vec4 (w = handedness), so their xyz is copied into the vec3 layout.
  const std::span<const glm::vec4> authoredTangents =
      viewAttribute<glm::vec4>(model, findAttribute(primitive, "TANGENT"));
  if (authoredTangents.size() == vertexCount) {
    lib::Buffer<glm::vec3> tangents(vertexCount);
    for (size_t v = 0; v < vertexCount; ++v) {
      tangents[v] = glm::vec3(authoredTangents[v]);
    }
    shared.tangents.push_back(std::move(tangents));
  } else {
    shared.tangents.push_back(
        createTangents(indices->indexSize, indices->bytes, positions, texCoords));
  }
  const std::span<const glm::vec3> tangents(
      reinterpret_cast<const glm::vec3*>(shared.tangents.back().data()),
      shared.tangents.back().size());

  static std::pair<std::string, std::string> orders[] = {
    {"PTNT", "0123"},
    {"P",    "0"   }
  };
  const std::array attributeDescriptions =
      common::createAttributeDescriptions(positions, texCoords, normals, tangents);

  PrimitiveAssets result;
  result.vertexData = ctx.assetManager.loadVertexDataInterleavingAsync(
      ctx.sharedData, indices->bytes, getIndexType(indices->indexSize),
      common::analyzeConfig(orders, attributeDescriptions));
  result.diffuse = loadTexture(ctx, diffuseIndex);
  result.normal = loadTexture(ctx, normalIndex);
  result.metallicRoughness = loadTexture(ctx, metallicRoughnessIndex);
  return result;
}

void processMesh(LoadContext& ctx, int meshIndex, const glm::mat4& transform) {
  const tinygltf::Mesh& mesh = ctx.sharedData->model.meshes[meshIndex];
  for (size_t p = 0; p < mesh.primitives.size(); ++p) {
    auto [it, inserted] = ctx.primitiveCache.try_emplace({meshIndex, static_cast<int>(p)});
    if (inserted) {
      it->second = buildPrimitive(ctx, mesh.primitives[p]);
    }
    if (!it->second) {
      continue;
    }
    const PrimitiveAssets& primitive = *it->second;
    ctx.assets.push_back(AssetData{
      .vertexData = primitive.vertexData,
      .model = transform,
      .diffuseTexture = primitive.diffuse,
      .normalTexture = primitive.normal,
      .metallicRoughnessTexture = primitive.metallicRoughness});
  }
}

void processNode(LoadContext& ctx, int nodeIndex, const glm::mat4& parentTransform, int depth) {
  const tinygltf::Model& model = ctx.sharedData->model;
  if (!inRange(model.nodes, nodeIndex) || depth > kMaxNodeDepth) {
    return;
  }
  const tinygltf::Node& node = model.nodes[nodeIndex];
  const glm::mat4 transform = parentTransform * getNodeTransform(node);

  if (inRange(model.meshes, node.mesh)) {
    processMesh(ctx, node.mesh, transform);
  }
  for (int child : node.children) {
    processNode(ctx, child, transform, depth + 1);
  }
}

std::vector<AssetData> collectAssets(
    AssetManager& assetManager, const FileLoader& fileLoader,
    std::shared_ptr<SharedData> sharedData, std::string baseDir, std::string sourceName) {
  LoadContext ctx{
    .assetManager = assetManager,
    .fileLoader = fileLoader,
    .sharedData = std::move(sharedData),
    .baseDir = std::move(baseDir),
    .sourceName = std::move(sourceName),
  };

  const tinygltf::Model& model = ctx.sharedData->model;
  if (model.scenes.empty()) {
    return {};
  }
  // Instantiate only the default scene; iterating every scene duplicated geometry
  // whenever a file contained more than one.
  const int sceneIndex = inRange(model.scenes, model.defaultScene) ? model.defaultScene : 0;
  for (int nodeIndex : model.scenes[sceneIndex].nodes) {
    processNode(ctx, nodeIndex, glm::mat4(1.0f), 0);
  }
  return std::move(ctx.assets);
}

std::string lowercaseExtension(const std::string& filePath) {
  std::string ext = std::filesystem::path(filePath).extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return ext;
}

}  // namespace

std::vector<AssetData> LoadGltfFromFile(
    common::AssetManager& assetManager, const FileLoader& fileLoader, const std::string& filePath) {
  auto sharedData = std::make_shared<SharedData>();
  tinygltf::TinyGLTF loader;
  std::string error, warning;

  const std::string extension = lowercaseExtension(filePath);
  bool loaded = false;
  if (extension == ".glb") {
    loaded = loader.LoadBinaryFromFile(&sharedData->model, &error, &warning, filePath);
  } else if (extension == ".gltf") {
    loaded = loader.LoadASCIIFromFile(&sharedData->model, &error, &warning, filePath);
  } else {
    throw EngineException(
        std::format("GLTF loader cannot load {} which is not .gltf or .glb format.", filePath));
  }
  if (!loaded) {
    throw EngineException(std::format("Failed to load glTF {}: {}", filePath, error));
  }

  std::string baseDir = std::filesystem::path(filePath).parent_path().string();
  return collectAssets(
      assetManager, fileLoader, std::move(sharedData), std::move(baseDir), filePath);
}

std::vector<AssetData> LoadGltfFromString(
    common::AssetManager& assetManager, const FileLoader& fileLoader, const std::string& dataString,
    const std::string& baseDir) {
  auto sharedData = std::make_shared<SharedData>();
  tinygltf::TinyGLTF loader;
  std::string error, warning;

  const bool loaded = loader.LoadASCIIFromString(
      &sharedData->model, &error, &warning, dataString.data(),
      static_cast<unsigned int>(dataString.size()), baseDir);
  if (!loaded) {
    throw EngineException(std::format("Failed to load glTF from string: {}", error));
  }

  return collectAssets(assetManager, fileLoader, std::move(sharedData), baseDir, baseDir);
}

}  // namespace common

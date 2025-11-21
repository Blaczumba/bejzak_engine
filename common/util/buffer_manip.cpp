#include "buffer_manip.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <span>

namespace {

size_t getMaxIndex(std::span<const std::byte> indicesBuffer, size_t indexSize) {
  switch (indexSize) {
    case sizeof(uint32_t):
      {
        return *std::max_element(reinterpret_cast<const uint32_t*>(indicesBuffer.data()),
                                 reinterpret_cast<const uint32_t*>(indicesBuffer.data())
                                     + indicesBuffer.size() / sizeof(uint32_t));
      }
    case sizeof(uint16_t):
      {
        return *std::max_element(reinterpret_cast<const uint16_t*>(indicesBuffer.data()),
                                 reinterpret_cast<const uint16_t*>(indicesBuffer.data())
                                     + indicesBuffer.size() / sizeof(uint16_t));
      }
    case sizeof(uint8_t):
      {
        return *std::max_element(reinterpret_cast<const uint8_t*>(indicesBuffer.data()),
                                 reinterpret_cast<const uint8_t*>(indicesBuffer.data())
                                     + indicesBuffer.size() / sizeof(uint8_t));
      }
    default:
      {
        return *std::max_element(reinterpret_cast<const uint64_t*>(indicesBuffer.data()),
                                 reinterpret_cast<const uint64_t*>(indicesBuffer.data())
                                     + indicesBuffer.size() / sizeof(uint64_t));
      }
  }
  return 0;
}

}  // namespace

/*
 * The format of config should follow:
 * (name, "numbers")
 *
 * Name is and identifier of buffer, and numbers should be
 * from 0 to descs.size() - 1. It tells the pattern of
 * interleaving the buffers from descs.
 *
 * For instance ("PTNP", "0120") means that we want to
 * create a buffer with interleaving buffers accordingly:
 * (descs[0], descs[1], descs[2], descs[0]).
 */
ErrorOr<std::vector<BufferDescription>> analyzeConfig(
    std::span<const std::pair<std::string, std::string>> orders,
    std::span<const AttributeDescription> descs) {
  std::vector<BufferDescription> descriptions;
  descriptions.reserve(descs.size());

  for (const auto& [name, config] : orders) {
    std::vector<AttributeDescription> orderedDescs;
    orderedDescs.reserve(config.size());

    size_t totalSize = 0;
    for (size_t i = 0; i < config.size(); ++i) {
      if (!std::isdigit(config[i])) [[unlikely]] {
        return Error(EngineError::LOAD_FAILURE);
      }

      orderedDescs.push_back(descs[static_cast<size_t>(config[i] - '0')]);
      totalSize += orderedDescs.back().size * orderedDescs.back().count;
    }

    descriptions.emplace_back(name, std::move(orderedDescs), totalSize);
  }

  return descriptions;
}

size_t getShrunkIndexSize(std::span<const std::byte> indicesBuffer, size_t indexSize) {
  const size_t maxIndex = getMaxIndex(indicesBuffer, indexSize);
  if (maxIndex <= std::numeric_limits<uint8_t>::max()) {
    return sizeof(uint8_t);
  } else if (maxIndex <= std::numeric_limits<uint16_t>::max()) {
    return sizeof(uint16_t);
  } else if (maxIndex <= std::numeric_limits<uint32_t>::max()) {
    return sizeof(uint32_t);
  } else {
    return sizeof(uint64_t);
  }
}

void copyAndShrinkIndices(void* dstIndices, size_t dstIndexSize, const void* srcIndices,
                          size_t srcIndexSize, size_t count) {
  std::byte* dstData = static_cast<std::byte*>(dstIndices);
  const std::byte* srcData = static_cast<const std::byte*>(srcIndices);
  for (size_t i = 0; i < count; i++) {
    std::memcpy(dstData, srcData, dstIndexSize);
    dstData += dstIndexSize;
    srcData += srcIndexSize;
  }
}

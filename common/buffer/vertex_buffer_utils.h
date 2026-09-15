#pragma once

#include <cstdint>
#include <vector>
#include <span>
#include <string>

namespace common {

struct AttributeDescription {
  void* data;
  size_t size;
  size_t count;
};

struct BufferDescription {
  std::string name;
  std::vector<AttributeDescription> attributes;
  size_t totalSize;
};

void copyDataInterleaving(
    std::span<std::byte> dst, std::span<const AttributeDescription> attributes);

template <typename... Type>
auto createAttributeDescriptions(std::span<const Type>... attributes) {
  return std::array<AttributeDescription, sizeof...(Type)>{
    AttributeDescription{(void*)attributes.data(), sizeof(Type), attributes.size()}
    ...
  };
}

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
std::vector<BufferDescription> analyzeConfig(
    std::span<const std::pair<std::string, std::string>> orders,
    std::span<const common::AttributeDescription> descs);

}  // namespace common



#include "common/buffer/vertex_buffer_lib.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <format>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace common {

size_t copyInterleavingDataAndGetStride(
    std::span<std::byte> dst, std::span<const AttributeDescription> attributes) {
  assert(!attributes.empty());

  const size_t count = attributes[0].count;

  if (std::any_of(std::cbegin(attributes), std::cend(attributes),
                  [count](const AttributeDescription& attribute) {
                    return attribute.count != count;
                  })) {
    throw std::runtime_error(
        "Buffers must have equal number of elements when copying buffers in an interleaving "
        "manner.");
  }

  std::vector<std::byte*> offsetMemory;
  offsetMemory.reserve(attributes.size());
  offsetMemory.push_back(dst.data());
  size_t stride = 0;
  std::transform(std::cbegin(attributes), std::prev(std::cend(attributes)),
                 std::back_inserter(offsetMemory), [&](const AttributeDescription& attribute) {
                   stride += attribute.size;
                   return dst.data() + stride;
                 });
  stride += attributes.back().size;

  for (size_t j = 0, running_offset = 0; j < count; j++, running_offset += stride) {
    for (const auto& [offset, attribute] : std::views::zip(offsetMemory, attributes)) {
      std::memcpy(offset + running_offset,
                  static_cast<uint8_t*>(attribute.data) + j * attribute.size, attribute.size);
    }
  }
  return stride;
}

std::vector<BufferDescription> analyzeConfig(
    std::span<const std::pair<std::string, std::string>> orders,
    std::span<const common::AttributeDescription> descs) {
  std::vector<BufferDescription> descriptions;
  descriptions.reserve(descs.size());

  for (const auto& [name, config] : orders) {
    std::vector<common::AttributeDescription> orderedDescs;
    orderedDescs.reserve(config.size());

    size_t totalSize = 0;
    for (const char digit : config) {
      assert(std::isdigit(digit));
      const common::AttributeDescription& description =
          orderedDescs.emplace_back(descs[static_cast<size_t>(digit - '0')]);
      totalSize += description.size * description.count;
    }

    descriptions.emplace_back(name, std::move(orderedDescs), totalSize);
  }

  return descriptions;
}

}  // namespace common

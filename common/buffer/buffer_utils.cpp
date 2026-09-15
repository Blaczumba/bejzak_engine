#include "common/buffer/buffer_utils.h"

#include <algorithm>
#include <format>
#include <ranges>
#include <span>
#include <vector>

#include "common/util/engine_exception.h"

namespace common {

void copyData(std::span<std::byte> dst, size_t dstOffset, std::span<const std::byte> src,
              size_t srcOffset, size_t size) {
  if (dst.size() < size + dstOffset) [[unlikely]] {
    throw EngineException(
        "Size of the destination buffer must be greater than copied buffer size + offset.");
  }

  if (src.size() > size + srcOffset) [[unlikely]] {
    throw EngineException(
        "Size of the source buffer must be greater than copied buffer size + offset.");
  }

  std::memcpy(dst.data() + dstOffset, src.data() + srcOffset, size);
}

void copyData(std::span<std::byte> dst, size_t dstOffset, std::span<const std::byte> src) {
  copyData(dst, dstOffset, src, 0, src.size());
}

void copyDataInterleaving(
    std::span<std::byte> dst, std::span<const AttributeDescription> attributes) {
  if (attributes.empty()) {
    throw EngineException("AttributeDescriptions cannot be empty.");
  }

  const size_t count = attributes[0].count;

  if (std::any_of(std::cbegin(attributes), std::cend(attributes),
                  [count](const AttributeDescription& attribute) {
                    return attribute.count != count;
                  })) {
    throw EngineException(
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

  for (size_t j = 0, running_stride = 0; j < count; j++, running_stride += stride) {
    for (const auto& [offset, attribute] : std::views::zip(offsetMemory, attributes)) {
      std::memcpy(offset + running_stride,
                  static_cast<uint8_t*>(attribute.data) + j * attribute.size, attribute.size);
    }
  }
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
      if (!std::isdigit(digit)) [[unlikely]] {
        throw EngineException(std::format(
            "The format of config string in analyzeConfig must contain digits only. Got: {}.",
            digit));
      }

      const common::AttributeDescription& description =
          orderedDescs.emplace_back(descs[static_cast<size_t>(digit - '0')]);
      totalSize += description.size * description.count;
    }

    descriptions.emplace_back(name, std::move(orderedDescs), totalSize);
  }

  return descriptions;
}

}  // namespace common

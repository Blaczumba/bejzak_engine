#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "common/status/status.h"

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

ErrorOr<std::vector<BufferDescription>> analyzeConfig(
    std::span<const std::pair<std::string, std::string>> orders,
    std::span<const AttributeDescription> descs);

size_t getShrunkIndexSize(std::span<const std::byte> indicesBuffer, size_t indexSize);

void copyAndShrinkIndices(void* dstIndices, size_t dstIndexSize, const void* srcIndices,
                          size_t srcIndexSize, size_t count);

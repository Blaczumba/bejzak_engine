#pragma once

#include <format>
#include <vulkan/vulkan.h>

#include "common/util/engine_exception.h"

constexpr uint32_t getIndexSize(VkIndexType type) {
  switch (type) {
    case VK_INDEX_TYPE_UINT8_EXT:
      return 1;
    case VK_INDEX_TYPE_UINT16:
      return 2;
    case VK_INDEX_TYPE_UINT32:
      return 4;
    default:
      throw EngineException(
          std::format("Unrecognized VkIndexType of {}.", static_cast<uint32_t>(type)));
  }
}

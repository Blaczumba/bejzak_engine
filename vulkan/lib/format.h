#pragma once

#include <vulkan/vulkan.h>

#include "common/format.h"

namespace vlkn {

constexpr VkFormat translateFormat(common::Format format) {
  switch (format) {
    case common::Format::FORMAT_B8G8R8A8_SRGB:
      return VK_FORMAT_B8G8R8A8_SRGB;
    case common::Format::FORMAT_R8G8B8A8_SRGB:
      return VK_FORMAT_R8G8B8A8_SRGB;
    case common::Format::FORMAT_B8G8R8A8_UNORM:
      return VK_FORMAT_B8G8R8A8_UNORM;
    case common::Format::FORMAT_R8G8B8A8_UNORM:
      return VK_FORMAT_R8G8B8A8_UNORM;
  }
  return VK_FORMAT_UNDEFINED;
}

}  // vlkn
#pragma once

#include <vulkan/vulkan.h>

#include "common/status/status.h"

#define CHECK_VKCMD(cmd) \
    if (VkResult result = cmd; result != VK_SUCCESS) [[unlikely]] return Error(result)

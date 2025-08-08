#pragma once

#include "common/status/status.h"

#include <vulkan/vulkan.h>

#define CHECK_VKCMD(cmd) \
    if (VkResult result = cmd; result != VK_SUCCESS) [[unlikely]] return Error(result)
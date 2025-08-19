#pragma once

#include <variant>

#include "common/status/status.h"
#include "vulkan_wrapper/memory_allocator/allocation.h"
#include "vulkan_wrapper/memory_allocator/memory_allocator.h"
#include "vulkan_wrapper/memory_objects/texture.h"
#include "vulkan_wrapper/physical_device/physical_device.h"

enum class QueueType : uint8_t {
  GRAPHICS = 0,
  PRESENT,
  COMPUTE,
  TRANSFER
};

class LogicalDevice {
  VkDevice _device = VK_NULL_HANDLE;

  const PhysicalDevice* _physicalDevice = nullptr;
  mutable MemoryAllocator _memoryAllocator;

  VkQueue _graphicsQueue = VK_NULL_HANDLE;
  VkQueue _presentQueue = VK_NULL_HANDLE;
  VkQueue _computeQueue = VK_NULL_HANDLE;
  VkQueue _transferQueue = VK_NULL_HANDLE;

  LogicalDevice(VkDevice logicalDevice, const PhysicalDevice& physicalDevice);

public:
  LogicalDevice() = default;

  static ErrorOr<LogicalDevice> create(const PhysicalDevice& physicalDevice);

  static ErrorOr<LogicalDevice> wrap(VkDevice device, const PhysicalDevice& physicalDevice);

  LogicalDevice(LogicalDevice&& logicalDevice) noexcept;

  LogicalDevice& operator=(LogicalDevice&& logicalDevice) noexcept;

  ~LogicalDevice();

  ErrorOr<VkSampler> createSampler(const SamplerParameters& params) const;

  ErrorOr<VkImageView> createImageView(VkImage image, const ImageParameters& params) const;

  VkDevice getVkDevice() const;

  const PhysicalDevice& getPhysicalDevice() const;

  MemoryAllocator& getMemoryAllocator() const;

  VkQueue getVkQueue(QueueType queueType) const;

  VkQueue getGraphicsVkQueue() const;

  VkQueue getPresentVkQueue() const;

  VkQueue getComputeVkQueue() const;

  VkQueue getTransferVkQueue() const;
};

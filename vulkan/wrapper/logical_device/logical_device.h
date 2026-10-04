#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <vulkan/vulkan.h>

#include "vulkan/wrapper/logical_device/resource_destroyer.h"
#include "vulkan/wrapper/memory_allocator/allocation.h"
#include "vulkan/wrapper/memory_allocator/memory_allocator.h"
#include "vulkan/wrapper/physical_device/physical_device.h"

enum class QueueType : uint8_t {
  GRAPHICS = 0,
  PRESENT,
  COMPUTE,
  TRANSFER
};

class LogicalDevice {
public:
  LogicalDevice() noexcept = default;

  static LogicalDevice create(
      const PhysicalDevice& physicalDevice, const VkPhysicalDeviceFeatures2& physicalDeviceFeatures,
      std::span<const char*> extensions,
      std::unique_ptr<ResourceDestroyer>&& resourceDestroyer = std::
          make_unique<ThreadedResourceDestroyer>());

  static std::unique_ptr<LogicalDevice> createPtr(
      const PhysicalDevice& physicalDevice, const VkPhysicalDeviceFeatures2& physicalDeviceFeatures,
      std::span<const char*> extensions,
      std::unique_ptr<ResourceDestroyer>&& resourceDestroyer = std::
          make_unique<ThreadedResourceDestroyer>());

  static LogicalDevice wrap(VkDevice device, const PhysicalDevice& physicalDevice,
                            std::unique_ptr<ResourceDestroyer>&& resourceDestroyer = std::
                                make_unique<ThreadedResourceDestroyer>());

  static std::unique_ptr<LogicalDevice> wrapPtr(
      VkDevice device, const PhysicalDevice& physicalDevice,
      std::unique_ptr<ResourceDestroyer>&& resourceDestroyer = std::
          make_unique<ThreadedResourceDestroyer>());

  LogicalDevice(LogicalDevice&& logicalDevice) noexcept;

  LogicalDevice& operator=(LogicalDevice&& logicalDevice) noexcept;

  ~LogicalDevice();

  void destroyResource(ResourceDestroyer::Job destroyResource) const;

  VkImageView createImageView(const VkImageViewCreateInfo& imageViewCreateInfo) const;

  VkDevice getVkDevice() const noexcept;

  const PhysicalDevice& getPhysicalDevice() const;

  MemoryAllocator& getMemoryAllocator() const;

  VkQueue getVkQueue(QueueType queueType) const noexcept;

  VkQueue getGraphicsVkQueue() const noexcept;

  VkQueue getPresentVkQueue() const noexcept;

  VkQueue getComputeVkQueue() const noexcept;

  VkQueue getTransferVkQueue() const noexcept;

private:
  LogicalDevice(VkDevice logicalDevice, const PhysicalDevice& physicalDevice,
                std::unique_ptr<ResourceDestroyer>&& resourceDestroyer) noexcept;

  VkDevice _device = VK_NULL_HANDLE;

  const PhysicalDevice* _physicalDevice = nullptr;
  MemoryAllocatorPtr _memoryAllocator;
  ResourceDestroyerPtr _resourceDestroyer;

  VkQueue _graphicsQueue = VK_NULL_HANDLE;
  VkQueue _presentQueue = VK_NULL_HANDLE;
  VkQueue _computeQueue = VK_NULL_HANDLE;
  VkQueue _transferQueue = VK_NULL_HANDLE;
};

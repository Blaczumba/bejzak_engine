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
  UNIVERSAL = 0,  // GRAPHICS, TRANSFER, COMPUTE
  DEDICATED_PRESENT,
  DEDICATED_TRANSFER,
  DEDICATED_COMPUTE
};

class LogicalDevice {
public:
  LogicalDevice() noexcept = default;

  static LogicalDevice create(
      const PhysicalDevice& physicalDevice, const VkDeviceCreateInfo& deviceCreateInfo,
      std::unique_ptr<ResourceDestroyer> resourceDestroyer);

  static std::unique_ptr<LogicalDevice> createPtr(
      const PhysicalDevice& physicalDevice, const VkDeviceCreateInfo& deviceCreateInfo,
      std::unique_ptr<ResourceDestroyer> resourceDestroyer);

  static LogicalDevice wrap(VkDevice device, const PhysicalDevice& physicalDevice,
                            std::unique_ptr<ResourceDestroyer> resourceDestroyer = std::
                                make_unique<ThreadedResourceDestroyer>());

  static std::unique_ptr<LogicalDevice> wrapPtr(
      VkDevice device, const PhysicalDevice& physicalDevice,
      std::unique_ptr<ResourceDestroyer> resourceDestroyer = std::
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

  VkQueue getUniversalVkQueue() const noexcept;

  VkQueue getDedicatedPresentQueue() const noexcept;

  VkQueue getDedicatedTransferVkQueue() const noexcept;

  VkQueue getDedicatedComputeVkQueue() const noexcept;

private:
  LogicalDevice(
      VkDevice logicalDevice, const PhysicalDevice& physicalDevice, VkQueue universalQueue,
      VkQueue dedicatedPresentQueue, VkQueue dedicatedTransferQueue, VkQueue dedicatedComputeQueue,
      std::unique_ptr<ResourceDestroyer> resourceDestroyer) noexcept;

  VkDevice _device = VK_NULL_HANDLE;

  const PhysicalDevice* _physicalDevice = nullptr;
  MemoryAllocatorPtr _memoryAllocator;
  ResourceDestroyerPtr _resourceDestroyer;

  VkQueue _universalQueue = VK_NULL_HANDLE;
  VkQueue _dedicatedPresentQueue = VK_NULL_HANDLE;
  VkQueue _dedicatedTransferQueue = VK_NULL_HANDLE;
  VkQueue _dedicatedComputeQueue = VK_NULL_HANDLE;
};

enum class ResourceDestroyerType : uint8_t {
  SYNCHRONOUS = 0,
  DEFERRED
};

class LogicalDeviceBuilder {
public:
  LogicalDeviceBuilder& withPhysicalDeviceFeatures2(
      const VkPhysicalDeviceFeatures2& physicalDeviceFeatures) noexcept;

  LogicalDeviceBuilder& withValidationLayers(lib::Buffer<const char*>&& validationLayers) noexcept;

  LogicalDeviceBuilder& withValidationLayers(
      std::span<const char* const> validationLayers) noexcept;

  LogicalDeviceBuilder& withExtensions(lib::Buffer<const char*>&& extensions) noexcept;

  LogicalDeviceBuilder& withExtensions(std::span<const char* const> extensions) noexcept;

  LogicalDeviceBuilder& withResourceDestroyerType(ResourceDestroyerType type) noexcept;

  LogicalDevice build(const PhysicalDevice& physicalDevice);

  std::unique_ptr<LogicalDevice> buildPtr(const PhysicalDevice& physicalDevice);

private:
  std::optional<VkPhysicalDeviceFeatures2> _physicalDeviceFeatures2;
  lib::Buffer<const char*> _validationLayers;
  lib::Buffer<const char*> _extensions;
  ResourceDestroyerType _resourceDestroyerType = ResourceDestroyerType::SYNCHRONOUS;
  VkDeviceCreateInfo _deviceCreateInfo{
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, .pNext = nullptr};
};

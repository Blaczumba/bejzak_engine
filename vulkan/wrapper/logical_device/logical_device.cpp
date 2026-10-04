#include "logical_device.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <set>
#include <utility>
#include <vulkan/vulkan.h>

#include "common/util/engine_exception.h"
#include "lib/buffer/buffer.h"
#include "vulkan/wrapper/instance/extensions.h"
#include "vulkan/wrapper/instance/validation_layers.h"
#include "vulkan/wrapper/logical_device/resource_destroyer.h"
#include "vulkan/wrapper/memory_allocator/allocation.h"
#include "vulkan/wrapper/memory_allocator/memory_allocator.h"
#include "vulkan/wrapper/physical_device/physical_device.h"
#include "vulkan/wrapper/util/check.h"

namespace {

struct VkQeueus {
  VkQueue graphicsQueue;
  VkQueue presentQueue;
  VkQueue computeQueue;
  VkQueue transferQueue;
};

VkQeueus getVkQueues(VkDevice device, const QueueFamilyIndices& indices) {
  VkQueue graphicsQueue, presentQueue, computeQueue, transferQueue;
  vkGetDeviceQueue(device, *indices.graphicsFamily, 0, &graphicsQueue);
  vkGetDeviceQueue(device, *indices.presentFamily, 0, &presentQueue);
  vkGetDeviceQueue(device, *indices.computeFamily, 0, &computeQueue);
  vkGetDeviceQueue(device, *indices.transferFamily, 0, &transferQueue);
  return {graphicsQueue, presentQueue, computeQueue, transferQueue};
}

lib::Buffer<VkDeviceQueueCreateInfo> getDeviceQueueCreateInfos(
    const QueueFamilyIndices& indices, float* queuePriority) {
  const std::set<uint32_t> uniqueQueueFamilies = {*indices.graphicsFamily, *indices.presentFamily,
                                                  *indices.computeFamily, *indices.transferFamily};
  lib::Buffer<VkDeviceQueueCreateInfo> queueCreateInfos(uniqueQueueFamilies.size());
  std::transform(uniqueQueueFamilies.cbegin(), uniqueQueueFamilies.cend(), queueCreateInfos.begin(),
                 [queuePriority](uint32_t queueFamilyIndex) {
                   return VkDeviceQueueCreateInfo{
                     .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                     .pNext = nullptr,
                     .flags = 0,
                     .queueFamilyIndex = queueFamilyIndex,
                     .queueCount = 1,
                     .pQueuePriorities = queuePriority};
                 });
  return queueCreateInfos;
}

std::unique_ptr<ResourceDestroyer> createResourceDestroyer(ResourceDestroyerType type) {
  switch (type) {
    case ResourceDestroyerType::SYNCHRONOUS:
      return std::make_unique<ImmediateResourceDestroyer>();
    case ResourceDestroyerType::DEFERRED:
      return std::make_unique<ThreadedResourceDestroyer>();
    default:
      throw EngineException("Unknown ResourceDestroyerType.");
  }
}

}  // namespace

LogicalDevice::LogicalDevice(
    VkDevice logicalDevice, const PhysicalDevice& physicalDevice, VkQueue graphicsQueue,
    VkQueue presentQueue, VkQueue computeQueue, VkQueue transferQueue,
    std::unique_ptr<ResourceDestroyer> resourceDestroyer) noexcept
  : _device(logicalDevice), _physicalDevice(&physicalDevice), _graphicsQueue(graphicsQueue),
    _presentQueue(presentQueue), _computeQueue(computeQueue), _transferQueue(transferQueue),
    _memoryAllocator(std::make_unique<MemoryAllocator>(
        std::in_place_type<VmaWrapper>, logicalDevice, physicalDevice.getVkPhysicalDevice(),
        physicalDevice.getInstance().getVkInstance())),
    _resourceDestroyer(std::move(resourceDestroyer)) {
  _resourceDestroyer->setupContext(_device, nullptr, _memoryAllocator.get());
  const QueueFamilyIndices& queueFamilyIndices = physicalDevice.getQueueFamilyIndices();
}

LogicalDevice::LogicalDevice(LogicalDevice&& logicalDevice) noexcept
  : _device(std::exchange(logicalDevice._device, VK_NULL_HANDLE)),
    _physicalDevice(std::exchange(logicalDevice._physicalDevice, nullptr)),
    _memoryAllocator(std::move(logicalDevice._memoryAllocator)),
    _resourceDestroyer(std::move(logicalDevice._resourceDestroyer)),
    _graphicsQueue(std::exchange(logicalDevice._graphicsQueue, VK_NULL_HANDLE)),
    _presentQueue(std::exchange(logicalDevice._presentQueue, VK_NULL_HANDLE)),
    _computeQueue(std::exchange(logicalDevice._computeQueue, VK_NULL_HANDLE)),
    _transferQueue(std::exchange(logicalDevice._transferQueue, VK_NULL_HANDLE)) {}

LogicalDevice& LogicalDevice::operator=(LogicalDevice&& logicalDevice) noexcept {
  if (this == &logicalDevice) {
    return *this;
  }
  // TODO what if _device != VK_NULL_HANDLE
  _device = std::exchange(logicalDevice._device, VK_NULL_HANDLE);
  _physicalDevice = std::exchange(logicalDevice._physicalDevice, nullptr);
  _memoryAllocator = std::move(logicalDevice._memoryAllocator);
  _resourceDestroyer = std::move(logicalDevice._resourceDestroyer);
  _graphicsQueue = std::exchange(logicalDevice._graphicsQueue, VK_NULL_HANDLE);
  _presentQueue = std::exchange(logicalDevice._presentQueue, VK_NULL_HANDLE);
  _computeQueue = std::exchange(logicalDevice._computeQueue, VK_NULL_HANDLE);
  _transferQueue = std::exchange(logicalDevice._transferQueue, VK_NULL_HANDLE);
  return *this;
}

LogicalDevice::~LogicalDevice() {
  if (_device != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(_device);
    _resourceDestroyer.reset();
    _memoryAllocator.reset();
    vkDestroyDevice(_device, nullptr);
  }
}

void LogicalDevice::destroyResource(ResourceDestroyer::Job destroyResource) const {
  _resourceDestroyer->destroyResource(std::move(destroyResource));
}

LogicalDevice LogicalDevice::create(
    const PhysicalDevice& physicalDevice, const VkDeviceCreateInfo& deviceCreateInfo,
    std::unique_ptr<ResourceDestroyer> resourceDestroyer) {
  VkDevice logicalDevice;
  CHECK_VKCMD(vkCreateDevice(
                  physicalDevice.getVkPhysicalDevice(), &deviceCreateInfo, nullptr, &logicalDevice),
              "Failed to create LogicalDevice!");
  auto [graphicsQueue, presentQueue, computeQueue, transferQueue] =
      getVkQueues(logicalDevice, physicalDevice.getQueueFamilyIndices());
  return LogicalDevice(logicalDevice, physicalDevice, graphicsQueue, presentQueue, computeQueue,
                       transferQueue, std::move(resourceDestroyer));
}

std::unique_ptr<LogicalDevice> LogicalDevice::createPtr(
    const PhysicalDevice& physicalDevice, const VkDeviceCreateInfo& deviceCreateInfo,
    std::unique_ptr<ResourceDestroyer> resourceDestroyer) {
  VkDevice logicalDevice;
  CHECK_VKCMD(vkCreateDevice(
                  physicalDevice.getVkPhysicalDevice(), &deviceCreateInfo, nullptr, &logicalDevice),
              "Failed to create LogicalDevice!");
  auto [graphicsQueue, presentQueue, computeQueue, transferQueue] =
      getVkQueues(logicalDevice, physicalDevice.getQueueFamilyIndices());
  return std::unique_ptr<LogicalDevice>(
      new LogicalDevice(logicalDevice, physicalDevice, graphicsQueue, presentQueue, computeQueue,
                        transferQueue, std::move(resourceDestroyer)));
}

LogicalDevice LogicalDevice::wrap(VkDevice logicalDevice, const PhysicalDevice& physicalDevice,
                                  std::unique_ptr<ResourceDestroyer> resourceDestroyer) {
  if (logicalDevice == VK_NULL_HANDLE) {
    throw EngineException("Cannot wrap VK_NULL_HANDLE around LogicalDevice.");
  }
  auto [graphicsQueue, presentQueue, computeQueue, transferQueue] =
      getVkQueues(logicalDevice, physicalDevice.getQueueFamilyIndices());
  return LogicalDevice(logicalDevice, physicalDevice, graphicsQueue, presentQueue, computeQueue,
                       transferQueue, std::move(resourceDestroyer));
}

std::unique_ptr<LogicalDevice> LogicalDevice::wrapPtr(
    VkDevice logicalDevice, const PhysicalDevice& physicalDevice,
    std::unique_ptr<ResourceDestroyer> resourceDestroyer) {
  if (logicalDevice == VK_NULL_HANDLE) {
    throw EngineException("Cannot wrap VK_NULL_HANDLE around LogicalDevice.");
  }
  auto [graphicsQueue, presentQueue, computeQueue, transferQueue] =
      getVkQueues(logicalDevice, physicalDevice.getQueueFamilyIndices());
  return std::unique_ptr<LogicalDevice>(
      new LogicalDevice(logicalDevice, physicalDevice, graphicsQueue, presentQueue, computeQueue,
                        transferQueue, std::move(resourceDestroyer)));
}

VkImageView LogicalDevice::createImageView(const VkImageViewCreateInfo& imageViewCreateInfo) const {
  VkImageView view;
  CHECK_VKCMD(vkCreateImageView(_device, &imageViewCreateInfo, nullptr, &view),
              "Failed to create VkImageView.");
  return view;
}

VkDevice LogicalDevice::getVkDevice() const noexcept {
  return _device;
}

const PhysicalDevice& LogicalDevice::getPhysicalDevice() const {
  return *_physicalDevice;
}

MemoryAllocator& LogicalDevice::getMemoryAllocator() const {
  return *_memoryAllocator;
}

VkQueue LogicalDevice::getVkQueue(QueueType queueType) const noexcept {
  switch (queueType) {
    case QueueType::GRAPHICS:
      return _graphicsQueue;
    case QueueType::PRESENT:
      return _presentQueue;
    case QueueType::COMPUTE:
      return _computeQueue;
    case QueueType::TRANSFER:
      return _transferQueue;
    default:
      return VK_NULL_HANDLE;
  }
}

VkQueue LogicalDevice::getGraphicsVkQueue() const noexcept {
  return _graphicsQueue;
}

VkQueue LogicalDevice::getPresentVkQueue() const noexcept {
  return _presentQueue;
}

VkQueue LogicalDevice::getComputeVkQueue() const noexcept {
  return _computeQueue;
}

VkQueue LogicalDevice::getTransferVkQueue() const noexcept {
  return _transferQueue;
}

LogicalDeviceBuilder& LogicalDeviceBuilder::withPhysicalDeviceFeatures2(
    const VkPhysicalDeviceFeatures2& physicalDeviceFeatures) noexcept {
  const bool isAlreadyChained = _physicalDeviceFeatures2.has_value();
  _physicalDeviceFeatures2 = physicalDeviceFeatures;
  if (!isAlreadyChained) {
    _deviceCreateInfo.pNext = &_physicalDeviceFeatures2.value();
  }
  return *this;
}

LogicalDeviceBuilder& LogicalDeviceBuilder::withValidationLayers(
    lib::Buffer<const const char*>&& validationLayers) noexcept {
  _validationLayers = std::move(validationLayers);
  _deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(_validationLayers.size());
  _deviceCreateInfo.ppEnabledLayerNames = _validationLayers.data();
  return *this;
}

LogicalDeviceBuilder& LogicalDeviceBuilder::withValidationLayers(
    std::span<const char* const> validationLayers) noexcept {
  _validationLayers = validationLayers;
  _deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(_validationLayers.size());
  _deviceCreateInfo.ppEnabledLayerNames = _validationLayers.data();
  return *this;
}

LogicalDeviceBuilder& LogicalDeviceBuilder::withExtensions(
    lib::Buffer<const char*>&& extensions) noexcept {
  _extensions = std::move(extensions);
  _deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(_extensions.size());
  _deviceCreateInfo.ppEnabledExtensionNames = _extensions.data();
  return *this;
}

LogicalDeviceBuilder& LogicalDeviceBuilder::withExtensions(
    std::span<const char* const> extensions) noexcept {
  _extensions = extensions;
  _deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(_extensions.size());
  _deviceCreateInfo.ppEnabledExtensionNames = _extensions.data();
  return *this;
}

LogicalDeviceBuilder& LogicalDeviceBuilder::withResourceDestroyerType(
    ResourceDestroyerType type) noexcept {
  _resourceDestroyerType = type;
  return *this;
}

LogicalDevice LogicalDeviceBuilder::build(const PhysicalDevice& physicalDevice) {
  float queuePriority = 1.0f;
  lib::Buffer<VkDeviceQueueCreateInfo> queueCreateInfos =
      getDeviceQueueCreateInfos(physicalDevice.getQueueFamilyIndices(), &queuePriority);
  _deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
  _deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
  return LogicalDevice::create(
      physicalDevice, _deviceCreateInfo, createResourceDestroyer(_resourceDestroyerType));
}

std::unique_ptr<LogicalDevice> LogicalDeviceBuilder::buildPtr(
    const PhysicalDevice& physicalDevice) {
  float queuePriority = 1.0f;
  lib::Buffer<VkDeviceQueueCreateInfo> queueCreateInfos =
      getDeviceQueueCreateInfos(physicalDevice.getQueueFamilyIndices(), &queuePriority);
  _deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
  _deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
  return LogicalDevice::createPtr(
      physicalDevice, _deviceCreateInfo, createResourceDestroyer(_resourceDestroyerType));
}

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
#include "vulkan/wrapper/logical_device/extensions_connector.h"
#include "vulkan/wrapper/logical_device/resource_destroyer.h"
#include "vulkan/wrapper/memory_allocator/allocation.h"
#include "vulkan/wrapper/memory_allocator/memory_allocator.h"
#include "vulkan/wrapper/physical_device/physical_device.h"
#include "vulkan/wrapper/util/check.h"

LogicalDevice::LogicalDevice(VkDevice logicalDevice, const PhysicalDevice& physicalDevice,
                             std::unique_ptr<ResourceDestroyer>&& resourceDestroyer) noexcept
  : _device(logicalDevice), _physicalDevice(&physicalDevice),
    _memoryAllocator(std::make_unique<MemoryAllocator>(
        std::in_place_type<VmaWrapper>, logicalDevice, physicalDevice.getVkPhysicalDevice(),
        physicalDevice.getInstance().getVkInstance())),
    _resourceDestroyer(std::move(resourceDestroyer)) {
  _resourceDestroyer->setupContext(_device, nullptr, _memoryAllocator.get());
  const QueueFamilyIndices& queueFamilyIndices = physicalDevice.getQueueFamilyIndices();
  vkGetDeviceQueue(logicalDevice, *queueFamilyIndices.graphicsFamily, 0, &_graphicsQueue);
  vkGetDeviceQueue(logicalDevice, *queueFamilyIndices.presentFamily, 0, &_presentQueue);
  vkGetDeviceQueue(logicalDevice, *queueFamilyIndices.computeFamily, 0, &_computeQueue);
  vkGetDeviceQueue(logicalDevice, *queueFamilyIndices.transferFamily, 0, &_transferQueue);
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

namespace {

VkDevice createVkDevice(const PhysicalDevice& physicalDevice) {
  const QueueFamilyIndices& indices = physicalDevice.getQueueFamilyIndices();
  const std::set<uint32_t> uniqueQueueFamilies = {*indices.graphicsFamily, *indices.presentFamily,
                                                  *indices.computeFamily, *indices.transferFamily};

  float queuePriority = 1.0f;
  lib::Buffer<VkDeviceQueueCreateInfo> queueCreateInfos(uniqueQueueFamilies.size());
  std::transform(uniqueQueueFamilies.cbegin(), uniqueQueueFamilies.cend(), queueCreateInfos.begin(),
                 [&queuePriority](uint32_t queueFamilyIndex) {
                   return VkDeviceQueueCreateInfo{
                     .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                     .pNext = nullptr,
                     .flags = 0,
                     .queueFamilyIndex = queueFamilyIndex,
                     .queueCount = 1,
                     .pQueuePriorities = &queuePriority};
                 });

  ExtensionsConnector extensionsConnector(physicalDevice);
  extensionsConnector.withDescriptorIndexingExtension()
      .withBufferDeviceAddressExtension()
      .withIndexTypeUint8Extension()
      .withInheritedViewportScissorExtension()
      .withMultiviewExtension()
      .withStorage8BitExtension()
      .withStorage16BitExtension()
      .withFragmentShadingRateExtension()
      .withFragmentDensityMapExtension()
      .withSynchronization2();

  const VkPhysicalDeviceFeatures2 deviceFeaturesInfo = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
    .pNext = extensionsConnector.getNext(),
    .features = VkPhysicalDeviceFeatures{
                                         .geometryShader = VK_TRUE,
                                         .tessellationShader = VK_TRUE,
                                         .sampleRateShading = VK_TRUE,
                                         .depthClamp = VK_TRUE,
                                         .samplerAnisotropy = VK_TRUE,
                                         .shaderInt16 = VK_TRUE}
  };

  const lib::Buffer<const char*> extensions = physicalDevice.getAvailableExtensions();

  const VkDeviceCreateInfo createInfo = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext = &deviceFeaturesInfo,
    .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
    .pQueueCreateInfos = queueCreateInfos.data(),
#ifdef VALIDATION_LAYERS_ENABLED
    .enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
    .ppEnabledLayerNames = validationLayers.data(),
#endif  // VALIDATION_LAYERS_ENABLED
    .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
    .ppEnabledExtensionNames = extensions.data(),
  };

  VkDevice logicalDevice;
  CHECK_VKCMD(
      vkCreateDevice(physicalDevice.getVkPhysicalDevice(), &createInfo, nullptr, &logicalDevice),
      "Failed to create LogicalDevice!");
  return logicalDevice;
}

}  // namespace

LogicalDevice LogicalDevice::create(
    const PhysicalDevice& physicalDevice, std::unique_ptr<ResourceDestroyer>&& resourceDestroyer) {
  return LogicalDevice(
      createVkDevice(physicalDevice), physicalDevice, std::move(resourceDestroyer));
}

std::unique_ptr<LogicalDevice> LogicalDevice::createPtr(
    const PhysicalDevice& physicalDevice, std::unique_ptr<ResourceDestroyer>&& resourceDestroyer) {
  return std::unique_ptr<LogicalDevice>(new LogicalDevice(
      createVkDevice(physicalDevice), physicalDevice, std::move(resourceDestroyer)));
}

LogicalDevice LogicalDevice::wrap(VkDevice device, const PhysicalDevice& physicalDevice,
                                  std::unique_ptr<ResourceDestroyer>&& resourceDestroyer) {
  if (device == VK_NULL_HANDLE) {
    throw EngineException("Cannot wrap VK_NULL_HANDLE around LogicalDevice.");
  }

  return LogicalDevice(device, physicalDevice, std::move(resourceDestroyer));
}

std::unique_ptr<LogicalDevice> LogicalDevice::wrapPtr(
    VkDevice device, const PhysicalDevice& physicalDevice,
    std::unique_ptr<ResourceDestroyer>&& resourceDestroyer) {
  if (device == VK_NULL_HANDLE) {
    throw EngineException("Cannot wrap VK_NULL_HANDLE around LogicalDevice.");
  }

  return std::unique_ptr<LogicalDevice>(
      new LogicalDevice(device, physicalDevice, std::move(resourceDestroyer)));
}

VkImageView LogicalDevice::createImageView(const VkImageViewCreateInfo& imageViewCreateInfo) const {
  VkImageView view;
  CHECK_VKCMD(vkCreateImageView(_device, &imageViewCreateInfo, nullptr, &view),
              "Failed to create VkImageView.");
  return view;
}

VkResult LogicalDevice::waitForFences(
    std::span<const VkFence> fences, VkBool32 waitAll, uint64_t timeout) {
  return vkWaitForFences(
      _device, static_cast<uint32_t>(fences.size()), fences.data(), waitAll, timeout);
}

VkResult LogicalDevice::waitForFences(
    std::initializer_list<VkFence> fences, VkBool32 waitAll, uint64_t timeout) {
  return vkWaitForFences(
      _device, static_cast<uint32_t>(fences.size()), fences.begin(), waitAll, timeout);
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

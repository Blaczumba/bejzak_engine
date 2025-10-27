#include "logical_device.h"

#include <algorithm>
#include <cstring>
#include <set>
#include <stdexcept>
#include <vulkan/vulkan.h>

#include "extensions_connector.h"
#include "lib/buffer/buffer.h"
#include "vulkan_wrapper/instance/extensions.h"
#include "vulkan_wrapper/util/check.h"

LogicalDevice::LogicalDevice(VkDevice logicalDevice, const PhysicalDevice& physicalDevice)
  : _device(logicalDevice), _physicalDevice(&physicalDevice),
    _memoryAllocator(
        std::in_place_type<VmaWrapper>, logicalDevice, physicalDevice.getVkPhysicalDevice(),
        physicalDevice.getInstance().getVkInstance()) {
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
  _graphicsQueue = std::exchange(logicalDevice._graphicsQueue, VK_NULL_HANDLE);
  _presentQueue = std::exchange(logicalDevice._presentQueue, VK_NULL_HANDLE);
  _computeQueue = std::exchange(logicalDevice._computeQueue, VK_NULL_HANDLE);
  _transferQueue = std::exchange(logicalDevice._transferQueue, VK_NULL_HANDLE);
  return *this;
}

LogicalDevice::~LogicalDevice() {
  // TODO refactor
  if (_device != VK_NULL_HANDLE) {
    std::get<VmaWrapper>(_memoryAllocator).destroy();
    vkDestroyDevice(_device, nullptr);
  }
}

ErrorOr<LogicalDevice> LogicalDevice::create(const PhysicalDevice& physicalDevice) {
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

  DeviceFeatures deviceFeatures;
  chainExtensionIndexTypeUint8(deviceFeatures, physicalDevice);
  chainExtensionBufferDeviceAddress(deviceFeatures, physicalDevice);
  chainExtensionInheritedViewportScissor(deviceFeatures, physicalDevice);
  chainExtensionDescriptorIndexing(deviceFeatures, physicalDevice);
  chainExtensionMultiview(deviceFeatures, physicalDevice);

  const VkPhysicalDeviceFeatures2 deviceFeaturesInfo = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
    .pNext = deviceFeatures.next,
    .features = {.geometryShader = VK_TRUE,
                 .tessellationShader = VK_TRUE,
                 .sampleRateShading = VK_TRUE,
                 .depthClamp = VK_TRUE,
                 .samplerAnisotropy = VK_TRUE}
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
      vkCreateDevice(physicalDevice.getVkPhysicalDevice(), &createInfo, nullptr, &logicalDevice));

  return LogicalDevice(logicalDevice, physicalDevice);
}

ErrorOr<LogicalDevice> LogicalDevice::wrap(VkDevice device, const PhysicalDevice& physicalDevice) {
  if (device == VK_NULL_HANDLE) {
    return Error(EngineError::NULLPTR_REFERENCE);
  }
  return LogicalDevice(device, physicalDevice);
}

ErrorOr<VkImageView> LogicalDevice::createImageView(
    VkImage image, VkImageViewType type, VkFormat format, VkImageAspectFlags aspect,
    uint32_t baseMipLevel, uint32_t mipLevels, uint32_t baseArrayLayer, uint32_t layerCount) const {
  const VkImageViewCreateInfo viewInfo = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .image = image,
    .viewType = type,
    .format = format,
    .subresourceRange = {.aspectMask = aspect,
                         .baseMipLevel = baseMipLevel,
                         .levelCount = mipLevels,
                         .baseArrayLayer = baseArrayLayer,
                         .layerCount = layerCount}
  };

  VkImageView view;
  CHECK_VKCMD(vkCreateImageView(_device, &viewInfo, nullptr, &view));
  return view;
}

ErrorOr<VkSampler> LogicalDevice::createSampler(const SamplerParameters& params) const {
  VkSamplerCreateInfo samplerInfo = {
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .magFilter = params.magFilter,
    .minFilter = params.minFilter,
    .mipmapMode = params.mipmapMode,
    .addressModeU = params.addressModeU,
    .addressModeV = params.addressModeV,
    .addressModeW = params.addressModeW,
    .mipLodBias = params.mipLodBias,
    .minLod = params.minLod,
    .maxLod = params.maxLod,
    .borderColor = params.borderColor,
    .unnormalizedCoordinates = params.unnormalizedCoordinates};

  if (params.maxAnisotropy.has_value()) {
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = *params.maxAnisotropy;
  }

  if (params.compareOp.has_value()) {
    samplerInfo.compareEnable = VK_TRUE;
    samplerInfo.compareOp = *params.compareOp;
  } else {
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
  }

  VkSampler sampler;
  CHECK_VKCMD(vkCreateSampler(_device, &samplerInfo, nullptr, &sampler));
  return sampler;
}

VkDevice LogicalDevice::getVkDevice() const {
  return _device;
}

const PhysicalDevice& LogicalDevice::getPhysicalDevice() const {
  return *_physicalDevice;
}

MemoryAllocator& LogicalDevice::getMemoryAllocator() const {
  return _memoryAllocator;
}

VkQueue LogicalDevice::getVkQueue(QueueType queueType) const {
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

VkQueue LogicalDevice::getGraphicsVkQueue() const {
  return _graphicsQueue;
}

VkQueue LogicalDevice::getPresentVkQueue() const {
  return _presentQueue;
}

VkQueue LogicalDevice::getComputeVkQueue() const {
  return _computeQueue;
}

VkQueue LogicalDevice::getTransferVkQueue() const {
  return _transferQueue;
}

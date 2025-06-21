#include "logical_device.h"

#include "config/config.h"
#include "lib/buffer/buffer.h"

#include <vulkan/vulkan.h>

#include <algorithm>
#include <cstring>
#include <set>
#include <stdexcept>

LogicalDevice::LogicalDevice(const VkDevice logicalDevice, const PhysicalDevice& physicalDevice, const VkQueue graphicsQueue, const VkQueue presentQueue, const VkQueue computeQueue, const VkQueue transferQueue)
    : _device(logicalDevice), _physicalDevice(physicalDevice),
    _memoryAllocator(std::in_place_type<VmaWrapper>, logicalDevice, physicalDevice.getVkPhysicalDevice(), _physicalDevice.getSurface().getInstance().getVkInstance()),
    _graphicsQueue(graphicsQueue), _presentQueue(presentQueue), _computeQueue(computeQueue), _transferQueue(transferQueue){
}

LogicalDevice::~LogicalDevice() {
    // TODO refactor
    std::get<VmaWrapper>(_memoryAllocator).destroy();
    vkDestroyDevice(_device, nullptr);
}

template<typename T> 
void chainExtensionFeature(void** next, T& feature, std::string_view extension, const std::unordered_set<std::string_view>& availableExtensions) {
    if (availableExtensions.contains(extension)) {
        feature.pNext = *next;
        *next = (void*)&feature;
    }
}

lib::ErrorOr<std::unique_ptr<LogicalDevice>> LogicalDevice::create(const PhysicalDevice& physicalDevice) {
    const QueueFamilyIndices& indices = physicalDevice.getPropertyManager().getQueueFamilyIndices();

    const std::set<uint32_t> uniqueQueueFamilies = {
        *indices.graphicsFamily,
        *indices.presentFamily,
        *indices.computeFamily,
        *indices.transferFamily
    };

    float queuePriority = 1.0f;
    lib::Buffer<VkDeviceQueueCreateInfo> queueCreateInfos(uniqueQueueFamilies.size());
    std::transform(uniqueQueueFamilies.cbegin(), uniqueQueueFamilies.cend(), queueCreateInfos.begin(), [&queuePriority](uint32_t queueFamilyIndex) {
            return VkDeviceQueueCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0, queueFamilyIndex, 1, &queuePriority }; 
        }
    );

    void* next = nullptr;
    const std::unordered_set<std::string_view>& availableExtensions = physicalDevice.getAvailableRequestedExtensions();

    VkPhysicalDeviceIndexTypeUint8FeaturesEXT uint8IndexFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INDEX_TYPE_UINT8_FEATURES_EXT,
        .indexTypeUint8 = VK_TRUE
    };

    chainExtensionFeature(&next, uint8IndexFeatures, VK_EXT_INDEX_TYPE_UINT8_EXTENSION_NAME, availableExtensions);

    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
        .bufferDeviceAddress = VK_TRUE
    };

    chainExtensionFeature(&next, bufferDeviceAddressFeatures, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, availableExtensions);

    VkPhysicalDeviceInheritedViewportScissorFeaturesNV viewportScissorsFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INHERITED_VIEWPORT_SCISSOR_FEATURES_NV,
        .inheritedViewportScissor2D = VK_FALSE,
    };

    chainExtensionFeature(&next, viewportScissorsFeatures, VK_NV_INHERITED_VIEWPORT_SCISSOR_EXTENSION_NAME, availableExtensions);

    VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
        .shaderUniformBufferArrayNonUniformIndexing = VK_TRUE,
        .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
        .shaderStorageBufferArrayNonUniformIndexing = VK_TRUE,
        .descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE,
        .descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
        .descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE
    };

    chainExtensionFeature(&next, descriptorIndexingFeatures, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, availableExtensions);

    const VkPhysicalDeviceFeatures2 deviceFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = next,
        .features = {
            .geometryShader = VK_TRUE,
            .tessellationShader = VK_TRUE,
            .sampleRateShading = VK_TRUE,
            .depthClamp = VK_TRUE,
            .samplerAnisotropy = VK_TRUE,
        }
    };

    lib::Buffer<const char*> extensions(availableExtensions.size());
    std::transform(availableExtensions.cbegin(), availableExtensions.cend(), extensions.begin(), [](std::string_view ext) { return ext.data(); });

    const VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &deviceFeatures,
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
    #ifdef VALIDATION_LAYERS_ENABLED
        .enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
        .ppEnabledLayerNames = validationLayers.data(),
    #else
        .enabledLayerCount = 0,
    #endif  // VALIDATION_LAYERS_ENABLED
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };

    VkDevice logicalDevice;
    if (vkCreateDevice(physicalDevice.getVkPhysicalDevice(), &createInfo, nullptr, &logicalDevice) != VK_SUCCESS) {
        return lib::Error("failed to create logical device!");
    }

    VkQueue graphicsQueue, presentQueue, computeQueue, transferQueue;
    vkGetDeviceQueue(logicalDevice, *indices.graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(logicalDevice, *indices.presentFamily, 0, &presentQueue);
    vkGetDeviceQueue(logicalDevice, *indices.computeFamily, 0, &computeQueue);
    vkGetDeviceQueue(logicalDevice, *indices.transferFamily, 0, &transferQueue);
    return std::unique_ptr<LogicalDevice>(new LogicalDevice(logicalDevice, physicalDevice, graphicsQueue, presentQueue, computeQueue, transferQueue));
}

const VkBuffer LogicalDevice::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage) const {
    // DEPRECATED
    const VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    VkBuffer buffer;
    if (vkCreateBuffer(_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }
    return buffer;
}

const VkDeviceMemory LogicalDevice::createBufferMemory(VkBuffer buffer, VkMemoryPropertyFlags properties) const {
    // DEPRECATED
    const auto& propertyManager = _physicalDevice.getPropertyManager();
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(_device, buffer, &memRequirements);

    const VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = propertyManager.findMemoryType(memRequirements.memoryTypeBits, properties)
    };

    VkDeviceMemory bufferMemory;
    if (vkAllocateMemory(_device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }
    vkBindBufferMemory(_device, buffer, bufferMemory, 0);
    return bufferMemory;
}

const VkImage LogicalDevice::createImage(const ImageParameters& params) const {
    // DEPRECATED
    VkImageCreateInfo imageInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = params.format,
        .extent = {
            .width = params.width,
            .height = params.height,
            .depth = 1
        },
        .mipLevels = params.mipLevels,
        .arrayLayers = params.layerCount,
        .samples = params.numSamples,
        .tiling = params.tiling,
        .usage = params.usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = params.layout
    };

    if (params.layerCount == 6)
        imageInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    VkImage image;
    if (vkCreateImage(_device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }
    return image;
}

const VkDeviceMemory LogicalDevice::createImageMemory(const VkImage image, const ImageParameters& params) const {
    // DEPRECATED
    const auto& propertyManager = _physicalDevice.getPropertyManager();

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(_device, image, &memRequirements);

    const VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = propertyManager.findMemoryType(memRequirements.memoryTypeBits, params.properties)
    };

    VkDeviceMemory memory;
    if (vkAllocateMemory(_device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(_device, image, memory, 0);
    return memory;
}

const VkImageView LogicalDevice::createImageView(const VkImage image, const ImageParameters& params) const {
    // DEPRECATED
    const VkImageSubresourceRange range = {
        .aspectMask = params.aspect,
        .baseMipLevel = 0,
        .levelCount = params.mipLevels,
        .baseArrayLayer = 0,
        .layerCount = params.layerCount
    };

    const VkImageViewType viewType = (params.layerCount == 6) ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;

    const VkImageViewCreateInfo viewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = viewType,
        .format = params.format,
        .subresourceRange = range
    };

    VkImageView view;
    if (vkCreateImageView(_device, &viewInfo, nullptr, &view) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image view!");
    }

    return view;
}

const VkSampler LogicalDevice::createSampler(const SamplerParameters& params) const {
    // DEPRECATED
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
        .unnormalizedCoordinates = params.unnormalizedCoordinates,
    };

    if (params.maxAnisotropy.has_value()) {
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = *params.maxAnisotropy;
    }

    if (params.compareOp.has_value()) {
        samplerInfo.compareEnable = VK_TRUE;
        samplerInfo.compareOp = *params.compareOp;
    }
    else {
        samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
    }

    VkSampler sampler;
    if (vkCreateSampler(_device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }

    return sampler;
}

void LogicalDevice::sendDataToMemory(const VkDeviceMemory memory, const void* data, size_t size) const {
    // DEPRECATED
    void* mappedMemory;
    vkMapMemory(_device, memory, 0, size, 0, &mappedMemory);
    std::memcpy(mappedMemory, data, size);
    // vkUnmapMemory(_device, memory);
}

const VkDevice LogicalDevice::getVkDevice() const {
    return _device;
}

const PhysicalDevice& LogicalDevice::getPhysicalDevice() const {
    return _physicalDevice;
}

MemoryAllocator& LogicalDevice::getMemoryAllocator() const {
    return _memoryAllocator;
}

const VkQueue LogicalDevice::getQueue(QueueType queueType) const {
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

const VkQueue LogicalDevice::getGraphicsQueue() const {
    return _graphicsQueue;
}

const VkQueue LogicalDevice::getPresentQueue() const {
    return _presentQueue;
}

const VkQueue LogicalDevice::getComputeQueue() const {
    return _computeQueue;
}

const VkQueue LogicalDevice::getTransferQueue() const {
    return _transferQueue;
}
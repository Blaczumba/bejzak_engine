#include "logical_device.h"

#include "vulkan_wrapper/instance/extensions.h"
#include "lib/buffer/buffer.h"

#include <vulkan/vulkan.h>

#include <algorithm>
#include <cstring>
#include <set>
#include <stdexcept>

LogicalDevice::LogicalDevice(VkDevice logicalDevice, const PhysicalDevice& physicalDevice, VkQueue graphicsQueue, VkQueue presentQueue, VkQueue computeQueue, VkQueue transferQueue)
    : _device(logicalDevice), _physicalDevice(&physicalDevice),
    _memoryAllocator(std::in_place_type<VmaWrapper>, logicalDevice, physicalDevice.getVkPhysicalDevice(), _physicalDevice->getSurface().getInstance().getVkInstance()),
    _graphicsQueue(graphicsQueue), _presentQueue(presentQueue), _computeQueue(computeQueue), _transferQueue(transferQueue){
}

LogicalDevice::LogicalDevice() : _device(VK_NULL_HANDLE), _physicalDevice(nullptr), _graphicsQueue(VK_NULL_HANDLE), _presentQueue(VK_NULL_HANDLE), _computeQueue(VK_NULL_HANDLE), _transferQueue(VK_NULL_HANDLE) { }

LogicalDevice::LogicalDevice(LogicalDevice&& logicalDevice) noexcept
	: _device(std::exchange(logicalDevice._device, VK_NULL_HANDLE)), _physicalDevice(std::exchange(logicalDevice._physicalDevice, nullptr)), _memoryAllocator(std::move(logicalDevice._memoryAllocator)),
    _graphicsQueue(std::exchange(logicalDevice._graphicsQueue, VK_NULL_HANDLE)), _presentQueue(std::exchange(logicalDevice._presentQueue, VK_NULL_HANDLE)),
    _computeQueue(std::exchange(logicalDevice._computeQueue, VK_NULL_HANDLE)), _transferQueue(std::exchange(logicalDevice._transferQueue, VK_NULL_HANDLE)) { }

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

template<typename T> 
void chainExtensionFeature(void** next, T& feature, std::string_view extension, const PhysicalDevice& physicalDevice) {
    if (physicalDevice.hasAvailableExtension(extension)) {
        feature.pNext = *next;
        *next = (void*)&feature;
    }
}

ErrorOr<LogicalDevice> LogicalDevice::create(const PhysicalDevice& physicalDevice) {
    const QueueFamilyIndices indices = physicalDevice.getQueueFamilyIndices();

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

    VkPhysicalDeviceIndexTypeUint8FeaturesEXT uint8IndexFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INDEX_TYPE_UINT8_FEATURES_EXT,
        .indexTypeUint8 = VK_TRUE
    };

    chainExtensionFeature(&next, uint8IndexFeatures, VK_EXT_INDEX_TYPE_UINT8_EXTENSION_NAME, physicalDevice);

    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
        .bufferDeviceAddress = VK_TRUE
    };

    chainExtensionFeature(&next, bufferDeviceAddressFeatures, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, physicalDevice);

    VkPhysicalDeviceInheritedViewportScissorFeaturesNV viewportScissorsFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INHERITED_VIEWPORT_SCISSOR_FEATURES_NV,
        .inheritedViewportScissor2D = VK_TRUE,
    };

    chainExtensionFeature(&next, viewportScissorsFeatures, VK_NV_INHERITED_VIEWPORT_SCISSOR_EXTENSION_NAME, physicalDevice);

    VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
        .shaderUniformBufferArrayNonUniformIndexing = VK_TRUE,
        .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
        .shaderStorageBufferArrayNonUniformIndexing = VK_TRUE,
        .descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE,
        .descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
        .descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE,
        .descriptorBindingPartiallyBound = VK_TRUE,
        .runtimeDescriptorArray = VK_TRUE,
    };

    chainExtensionFeature(&next, descriptorIndexingFeatures, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, physicalDevice);

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

    const lib::Buffer<const char*> extensions = physicalDevice.getAvailableExtensions();

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
    if (VkResult result = vkCreateDevice(physicalDevice.getVkPhysicalDevice(), &createInfo, nullptr, &logicalDevice); result != VK_SUCCESS) {
        return Error(result);
    }

    VkQueue graphicsQueue, presentQueue, computeQueue, transferQueue;
    vkGetDeviceQueue(logicalDevice, *indices.graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(logicalDevice, *indices.presentFamily, 0, &presentQueue);
    vkGetDeviceQueue(logicalDevice, *indices.computeFamily, 0, &computeQueue);
    vkGetDeviceQueue(logicalDevice, *indices.transferFamily, 0, &transferQueue);
    return LogicalDevice(logicalDevice, physicalDevice, graphicsQueue, presentQueue, computeQueue, transferQueue);
}

VkImageView LogicalDevice::createImageView(const VkImage image, const ImageParameters& params) const {
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
    if (VkResult result = vkCreateSampler(_device, &samplerInfo, nullptr, &sampler); result != VK_SUCCESS) {
        throw Error(result);
    }

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
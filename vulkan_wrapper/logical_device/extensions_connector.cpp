#include "extensions_connector.h"

#include <string_view>
#include <vulkan/vulkan.h>

#include "vulkan_wrapper/physical_device/physical_device.h"

namespace {

template <typename T>
void chainExtensionFeature(
    void** next, T& feature, std::string_view extension, const PhysicalDevice& physicalDevice) {
  if (physicalDevice.hasAvailableExtension(extension)) {
    feature.pNext = *next;
    *next = (void*)&feature;
  }
}

}  // namespace

void chainExtensionIndexTypeUint8(
    DeviceFeatures& deviceFeatures, const PhysicalDevice& physicalDevice) {
  deviceFeatures.indexTypeUint8 = VkPhysicalDeviceIndexTypeUint8FeaturesEXT{
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INDEX_TYPE_UINT8_FEATURES_EXT,
    .indexTypeUint8 = VK_TRUE};

  chainExtensionFeature(&deviceFeatures.next, deviceFeatures.indexTypeUint8,
                        VK_EXT_INDEX_TYPE_UINT8_EXTENSION_NAME, physicalDevice);
}

void chainExtensionBufferDeviceAddress(
    DeviceFeatures& deviceFeatures, const PhysicalDevice& physicalDevice) {
  deviceFeatures.bufferDeviceAddress = VkPhysicalDeviceBufferDeviceAddressFeatures{
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
    .bufferDeviceAddress = VK_TRUE};

  chainExtensionFeature(&deviceFeatures.next, deviceFeatures.bufferDeviceAddress,
                        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, physicalDevice);
}

void chainExtensionInheritedViewportScissor(
    DeviceFeatures& deviceFeatures, const PhysicalDevice& physicalDevice) {
  deviceFeatures.inheritedViewportScissor = VkPhysicalDeviceInheritedViewportScissorFeaturesNV{
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INHERITED_VIEWPORT_SCISSOR_FEATURES_NV,
    .inheritedViewportScissor2D = VK_TRUE};

  chainExtensionFeature(&deviceFeatures.next, deviceFeatures.inheritedViewportScissor,
                        VK_NV_INHERITED_VIEWPORT_SCISSOR_EXTENSION_NAME, physicalDevice);
}

void chainExtensionDescriptorIndexing(
    DeviceFeatures& deviceFeatures, const PhysicalDevice& physicalDevice) {
  deviceFeatures.descriptorIndexing = VkPhysicalDeviceDescriptorIndexingFeatures{
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
    .shaderUniformBufferArrayNonUniformIndexing = VK_TRUE,
    .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
    .shaderStorageBufferArrayNonUniformIndexing = VK_TRUE,
    .descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE,
    .descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
    .descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE,
    .descriptorBindingPartiallyBound = VK_TRUE,
    .runtimeDescriptorArray = VK_TRUE};

  chainExtensionFeature(&deviceFeatures.next, deviceFeatures.descriptorIndexing,
                        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, physicalDevice);
}

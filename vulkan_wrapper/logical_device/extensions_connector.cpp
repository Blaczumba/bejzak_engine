#include "extensions_connector.h"

#include <vulkan/vulkan.h>
#include <string_view>

#include "vulkan_wrapper/physical_device/physical_device.h"

namespace {

template<typename T>
void chainExtensionFeature(
    void **next, T &feature, std::string_view extension, const PhysicalDevice& physicalDevice) {
  if (physicalDevice.hasAvailableExtension(extension)) {
    feature.pNext = *next;
    *next = (void *) &feature;
  }
}

} // namesapce


void chainExtensionIndexTypeUint8(void *next, const PhysicalDevice& physicalDevice) {
  VkPhysicalDeviceIndexTypeUint8FeaturesEXT uint8IndexFeatures = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INDEX_TYPE_UINT8_FEATURES_EXT,
      .indexTypeUint8 = VK_TRUE};

  chainExtensionFeature(
      &next, uint8IndexFeatures, VK_EXT_INDEX_TYPE_UINT8_EXTENSION_NAME, physicalDevice);
}

void chainExtensionBufferDeviceAddress(void *next, const PhysicalDevice& physicalDevice) {
  VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
      .bufferDeviceAddress = VK_TRUE};

  chainExtensionFeature(&next, bufferDeviceAddressFeatures,
                        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, physicalDevice);
}

void chainExtensionInheritedViewportScissor(void *next, const PhysicalDevice& physicalDevice) {
  VkPhysicalDeviceInheritedViewportScissorFeaturesNV viewportScissorsFeatures = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INHERITED_VIEWPORT_SCISSOR_FEATURES_NV,
      .inheritedViewportScissor2D = VK_TRUE
  };

  chainExtensionFeature(&next, viewportScissorsFeatures,
                        VK_NV_INHERITED_VIEWPORT_SCISSOR_EXTENSION_NAME, physicalDevice);
}

void chainExtensionDescriptorIndexing(void *next, const PhysicalDevice& physicalDevice) {
  VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
      .shaderUniformBufferArrayNonUniformIndexing = VK_TRUE,
      .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
      .shaderStorageBufferArrayNonUniformIndexing = VK_TRUE,
      .descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE,
      .descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
      .descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE,
      .descriptorBindingPartiallyBound = VK_TRUE,
      .runtimeDescriptorArray = VK_TRUE};

  chainExtensionFeature(
      &next, descriptorIndexingFeatures, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, physicalDevice);
}
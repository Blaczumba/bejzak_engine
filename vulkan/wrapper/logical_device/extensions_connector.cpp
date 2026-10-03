#include "extensions_connector.h"

#include <optional>
#include <string_view>
#include <vulkan/vulkan.h>

#include "vulkan/wrapper/physical_device/physical_device.h"

namespace {

template <typename T>
void chainExtensionFeature(
    void** next, T& feature, const PhysicalDevice& physicalDevice, const char* extension,
    std::unordered_set<const char*>& requestedExtensions) {
  if (extension != nullptr) {
    if (!physicalDevice.hasAvailableExtension(extension)) {
      return;
    }
    requestedExtensions.insert(extension);
  }

  feature.pNext = *next;
  *next = (void*)&feature;
}

template <typename T>
void chainExtensionFeature(void** next, T& feature) {
  feature.pNext = *next;
  *next = (void*)&feature;
}

}  // namespace

ExtensionsConnector::ExtensionsConnector(const PhysicalDevice& physicalDevice) noexcept
  : _physicalDevice(physicalDevice) {}

ExtensionsConnector& ExtensionsConnector::withIndexTypeUint8Extension() {
  _indexTypeUint8 = VkPhysicalDeviceIndexTypeUint8FeaturesEXT{
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INDEX_TYPE_UINT8_FEATURES_EXT,
    .indexTypeUint8 = VK_TRUE};

  chainExtensionFeature(&_next, _indexTypeUint8, _physicalDevice,
                        VK_EXT_INDEX_TYPE_UINT8_EXTENSION_NAME, _requestedDeviceExtensions);
  return *this;
}

ExtensionsConnector& ExtensionsConnector::withBufferDeviceAddressExtension() {
  _bufferDeviceAddress = VkPhysicalDeviceBufferDeviceAddressFeatures{
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
    .bufferDeviceAddress = VK_TRUE};

  chainExtensionFeature(
      &_next, _bufferDeviceAddress, _physicalDevice, nullptr, _requestedDeviceExtensions);
  return *this;
}

ExtensionsConnector& ExtensionsConnector::withInheritedViewportScissorExtension() {
  _inheritedViewportScissor = VkPhysicalDeviceInheritedViewportScissorFeaturesNV{
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INHERITED_VIEWPORT_SCISSOR_FEATURES_NV,
    .inheritedViewportScissor2D = VK_TRUE};

  chainExtensionFeature(
      &_next, _inheritedViewportScissor, _physicalDevice,
      VK_NV_INHERITED_VIEWPORT_SCISSOR_EXTENSION_NAME, _requestedDeviceExtensions);
  return *this;
}

ExtensionsConnector& ExtensionsConnector::withDescriptorIndexingExtension() {
  _descriptorIndexing = VkPhysicalDeviceDescriptorIndexingFeatures{
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
      &_next, _descriptorIndexing, _physicalDevice, nullptr, _requestedDeviceExtensions);
  return *this;
}

ExtensionsConnector& ExtensionsConnector::withMultiviewExtension() {
  _multiview = VkPhysicalDeviceMultiviewFeatures{
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES, .multiview = VK_TRUE};

  chainExtensionFeature(&_next, _multiview, _physicalDevice, nullptr, _requestedDeviceExtensions);
  return *this;
}

ExtensionsConnector& ExtensionsConnector::withStorage8BitExtension() {
  _storage8Bit = VkPhysicalDevice8BitStorageFeatures{
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES,
    .storageBuffer8BitAccess = VK_FALSE,
    .uniformAndStorageBuffer8BitAccess = VK_TRUE,
    .storagePushConstant8 = VK_TRUE};

  chainExtensionFeature(&_next, _storage8Bit, _physicalDevice, nullptr, _requestedDeviceExtensions);
  return *this;
}

ExtensionsConnector& ExtensionsConnector::withStorage16BitExtension() {
  _storage16Bit = VkPhysicalDevice16BitStorageFeatures{
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES,
    .storageBuffer16BitAccess = VK_TRUE,
    .uniformAndStorageBuffer16BitAccess = VK_TRUE,
    .storagePushConstant16 = VK_TRUE};

  chainExtensionFeature(
      &_next, _storage16Bit, _physicalDevice, nullptr, _requestedDeviceExtensions);
  return *this;
}

ExtensionsConnector& ExtensionsConnector::withFragmentShadingRateExtension(
    const VkPhysicalDeviceFragmentShadingRateFeaturesKHR& features) {
  _fragmentShadingRate = features;
  chainExtensionFeature(&_next, _fragmentShadingRate, _physicalDevice,
                        VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME, _requestedDeviceExtensions);
  return *this;
}

ExtensionsConnector& ExtensionsConnector::withFragmentDensityMapExtension(
    const VkPhysicalDeviceFragmentDensityMapFeaturesEXT& features) {
  _fragmentDensityMap = features;
  chainExtensionFeature(&_next, _fragmentDensityMap, _physicalDevice,
                        VK_EXT_FRAGMENT_DENSITY_MAP_EXTENSION_NAME, _requestedDeviceExtensions);
  return *this;
}

ExtensionsConnector& ExtensionsConnector::withSynchronization2() {
  _synchronization2 = VkPhysicalDeviceSynchronization2Features{
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
    .synchronization2 = VK_TRUE};

  chainExtensionFeature(
      &_next, _synchronization2, _physicalDevice, nullptr, _requestedDeviceExtensions);
  return *this;
}

void* ExtensionsConnector::getNext() const {
  return _next;
}

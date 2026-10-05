#include "extensions_connector.h"

#include <vulkan/vulkan.h>

#include "vulkan/wrapper/instance/extensions.h"  // TODO: To be deleted.
#include "vulkan/wrapper/physical_device/physical_device.h"

namespace {

template <typename T>
void chainExtensionFeature(
    void** next, T& feature, const PhysicalDevice& physicalDevice, const char* extension,
    std::unordered_set<const char*>& avalableEequestedDeviceExtensions) {
  if (extension != nullptr) {
    if (!physicalDevice.hasAvailableExtension(extension)) {
      return;
    }
    avalableEequestedDeviceExtensions.insert(extension);
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
  bool isAlreadyChained = _indexTypeUint8.has_value();
  _indexTypeUint8 = VkPhysicalDeviceIndexTypeUint8FeaturesEXT{
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INDEX_TYPE_UINT8_FEATURES_EXT,
    .indexTypeUint8 = VK_TRUE};
  if (!isAlreadyChained) {
    chainExtensionFeature(
        &_next, *_indexTypeUint8, _physicalDevice, VK_EXT_INDEX_TYPE_UINT8_EXTENSION_NAME,
        _availableRequestedDeviceExtensions);
  }
  return *this;
}

ExtensionsConnector& ExtensionsConnector::withBufferDeviceAddressExtension() {
  bool isAlreadyChained = _bufferDeviceAddress.has_value();
  _bufferDeviceAddress = VkPhysicalDeviceBufferDeviceAddressFeatures{
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
    .bufferDeviceAddress = VK_TRUE};

  if (!isAlreadyChained) {
    chainExtensionFeature(&_next, *_bufferDeviceAddress, _physicalDevice, nullptr,
                          _availableRequestedDeviceExtensions);
  }
  return *this;
}

ExtensionsConnector& ExtensionsConnector::withInheritedViewportScissorExtension() {
  bool isAlreadyChained = _inheritedViewportScissor.has_value();
  _inheritedViewportScissor = VkPhysicalDeviceInheritedViewportScissorFeaturesNV{
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INHERITED_VIEWPORT_SCISSOR_FEATURES_NV,
    .inheritedViewportScissor2D = VK_TRUE};

  if (!isAlreadyChained) {
    chainExtensionFeature(
        &_next, *_inheritedViewportScissor, _physicalDevice,
        VK_NV_INHERITED_VIEWPORT_SCISSOR_EXTENSION_NAME, _availableRequestedDeviceExtensions);
  }
  return *this;
}

ExtensionsConnector& ExtensionsConnector::withDescriptorIndexingExtension() {
  bool isAlreadyChained = _descriptorIndexing.has_value();
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

  if (!isAlreadyChained) {
    chainExtensionFeature(&_next, *_descriptorIndexing, _physicalDevice, nullptr,
                          _availableRequestedDeviceExtensions);
  }
  return *this;
}

ExtensionsConnector& ExtensionsConnector::withMultiviewExtension() {
  bool isAlreadyChained = _multiview.has_value();
  _multiview = VkPhysicalDeviceMultiviewFeatures{
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES, .multiview = VK_TRUE};

  if (!isAlreadyChained) {
    chainExtensionFeature(
        &_next, *_multiview, _physicalDevice, nullptr, _availableRequestedDeviceExtensions);
  }
  return *this;
}

ExtensionsConnector& ExtensionsConnector::withStorage8BitExtension() {
  bool isAlreadyChained = _storage8Bit.has_value();
  _storage8Bit = VkPhysicalDevice8BitStorageFeatures{
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES,
    .storageBuffer8BitAccess = VK_FALSE,
    .uniformAndStorageBuffer8BitAccess = VK_TRUE,
    .storagePushConstant8 = VK_TRUE};

  if (!isAlreadyChained) {
    chainExtensionFeature(
        &_next, *_storage8Bit, _physicalDevice, nullptr, _availableRequestedDeviceExtensions);
  }
  return *this;
}

ExtensionsConnector& ExtensionsConnector::withStorage16BitExtension() {
  bool isAlreadyChained = _storage16Bit.has_value();
  _storage16Bit = VkPhysicalDevice16BitStorageFeatures{
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES,
    .storageBuffer16BitAccess = VK_TRUE,
    .uniformAndStorageBuffer16BitAccess = VK_TRUE,
    .storagePushConstant16 = VK_TRUE};

  if (!isAlreadyChained) {
    chainExtensionFeature(
        &_next, *_storage16Bit, _physicalDevice, nullptr, _availableRequestedDeviceExtensions);
  }
  return *this;
}

ExtensionsConnector& ExtensionsConnector::withFragmentShadingRateExtension(
    const VkPhysicalDeviceFragmentShadingRateFeaturesKHR& features) {
  bool isAlreadyChained = _fragmentShadingRate.has_value();
  _fragmentShadingRate = features;
  if (!isAlreadyChained) {
    chainExtensionFeature(
        &_next, *_fragmentShadingRate, _physicalDevice, VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME,
        _availableRequestedDeviceExtensions);
  }
  return *this;
}

ExtensionsConnector& ExtensionsConnector::withFragmentDensityMapExtension(
    const VkPhysicalDeviceFragmentDensityMapFeaturesEXT& features) {
  bool isAlreadyChained = _fragmentDensityMap.has_value();
  _fragmentDensityMap = features;
  if (!isAlreadyChained) {
    chainExtensionFeature(
        &_next, *_fragmentDensityMap, _physicalDevice, VK_EXT_FRAGMENT_DENSITY_MAP_EXTENSION_NAME,
        _availableRequestedDeviceExtensions);
  }
  return *this;
}

ExtensionsConnector& ExtensionsConnector::withFragmentDensityMapOffsetExtension(
    const VkPhysicalDeviceFragmentDensityMapOffsetFeaturesQCOM& features) {
  bool isAlreadyChained = _fragmentDensityMapOffset.has_value();
  _fragmentDensityMapOffset = features;
  if (!isAlreadyChained) {
    chainExtensionFeature(
        &_next, *_fragmentDensityMapOffset, _physicalDevice, VK_QCOM_FRAGMENT_DENSITY_MAP_OFFSET_EXTENSION_NAME,
        _availableRequestedDeviceExtensions);
  }
  return *this;
}

ExtensionsConnector& ExtensionsConnector::withSynchronization2() {
  bool isAlreadyChained = _synchronization2.has_value();
  _synchronization2 = VkPhysicalDeviceSynchronization2Features{
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
    .synchronization2 = VK_TRUE};

  if (!isAlreadyChained) {
    chainExtensionFeature(
        &_next, *_synchronization2, _physicalDevice, nullptr, _availableRequestedDeviceExtensions);
  }
  return *this;
}

VkPhysicalDeviceFeatures2 ExtensionsConnector::getVkPhysicalDeviceFeatures2() const {
  return VkPhysicalDeviceFeatures2{
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
    .pNext = _next,
    .features = VkPhysicalDeviceFeatures{
                                         .geometryShader = VK_TRUE,
                                         .tessellationShader = VK_TRUE,
                                         .sampleRateShading = VK_TRUE,
                                         .depthClamp = VK_TRUE,
                                         .samplerAnisotropy = VK_TRUE,
                                         .shaderStorageImageArrayDynamicIndexing = VK_TRUE,
                                         .shaderInt16 = VK_TRUE}
  };
}

lib::Buffer<const char*> ExtensionsConnector::getAvailableRequestedDeviceExtensions() noexcept {
  for (const char* extension : requestedDeviceExtensions) {
    if (_physicalDevice.hasAvailableExtension(extension)) {
      _availableRequestedDeviceExtensions.insert(extension);
    }
  }
  return lib::Buffer<const char*>(
      _availableRequestedDeviceExtensions.cbegin(), _availableRequestedDeviceExtensions.cend());
}

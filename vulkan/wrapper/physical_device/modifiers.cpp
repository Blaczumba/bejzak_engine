#include "vulkan/wrapper/physical_device/modifiers.h"

#include <algorithm>
#include <bitset>
#include <tuple>
#include <vulkan/vulkan.h>

#include "vulkan/wrapper/physical_device/extensions_connector.h"
#include "vulkan/wrapper/physical_device/physical_device.h"

namespace {

template <typename T>
void chainExtendedField(VkPhysicalDeviceFeatures2& deviceFeatures, T& feature) {
  feature.pNext = deviceFeatures.pNext;
  deviceFeatures.pNext = &feature;
}

VkExtent2D clampExtent(
    VkExtent2D extent, VkExtent2D preferredTexelSize, VkExtent2D minClamp, VkExtent2D maxClamp) {
  VkExtent2D texelSize{std::clamp(preferredTexelSize.width, minClamp.width, maxClamp.width),
                       std::clamp(preferredTexelSize.height, minClamp.height, maxClamp.height)};
  return VkExtent2D{(extent.width + texelSize.width - 1) / texelSize.width,
                    (extent.height + texelSize.height - 1) / texelSize.height};
}

}  // namespace

AttachmentBasedFragmentShadingRateModifier::AttachmentBasedFragmentShadingRateModifier(
    const PhysicalDevice& physicalDevice, SupportedFeature selectedFeature) noexcept
  : _physicalDevice(&physicalDevice), _selectedFeature(selectedFeature) {}

std::tuple<AttachmentBasedFragmentShadingRateModifier, const char*>
AttachmentBasedFragmentShadingRateModifier::create(
    const PhysicalDevice& physicalDevice, ExtensionsConnector& extensionsConnector,
    SupportedFeature preferredFeature) noexcept {
  if (preferredFeature == SupportedFeature::NONE) {
    return {AttachmentBasedFragmentShadingRateModifier(physicalDevice, preferredFeature), ""};
  }
  VkPhysicalDeviceFragmentDensityMapFeaturesEXT fdmFeatures{
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_FEATURES_EXT};
  VkPhysicalDeviceFragmentDensityMapOffsetFeaturesQCOM fdmOffsetQcomFeatures{
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_OFFSET_FEATURES_QCOM};
  VkPhysicalDeviceFragmentShadingRateFeaturesKHR fsrFeatures{
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR};
  VkPhysicalDeviceFeatures2 features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};

  if (physicalDevice.hasAvailableExtension(VK_EXT_FRAGMENT_DENSITY_MAP_EXTENSION_NAME)) {
    chainExtendedField(features, fdmFeatures);
  }
  if (physicalDevice.hasAvailableExtension(VK_QCOM_FRAGMENT_DENSITY_MAP_OFFSET_EXTENSION_NAME)) {
    chainExtendedField(features, fdmOffsetQcomFeatures);
  }
  if (physicalDevice.hasAvailableExtension(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME)) {
    chainExtendedField(features, fsrFeatures);
  }

  vkGetPhysicalDeviceFeatures2(physicalDevice.getVkPhysicalDevice(), &features);

  std::bitset<FEATURES_NUMBER> supportedFeatures;
  supportedFeatures[static_cast<size_t>(SupportedFeature::FRAGMENT_DENSITY_MAP)] =
      fdmFeatures.fragmentDensityMap && fdmFeatures.fragmentDensityMapDynamic
      && fdmFeatures.fragmentDensityMapNonSubsampledImages;
  supportedFeatures[static_cast<size_t>(SupportedFeature::FRAGMENT_DENSITY_MAP_OFFSET)] =
      fdmFeatures.fragmentDensityMap && fdmFeatures.fragmentDensityMapNonSubsampledImages
      && fdmOffsetQcomFeatures.fragmentDensityMapOffset;
  supportedFeatures[static_cast<size_t>(SupportedFeature::FRAGMENT_SHADING_RATE)] =
      fsrFeatures.attachmentFragmentShadingRate;

  SupportedFeature selectedFeature = SupportedFeature::NONE;
  for (size_t i = 0; i < FEATURES_NUMBER; ++i) {
    if (!supportedFeatures[i]) {
      continue;
    }
    selectedFeature = static_cast<SupportedFeature>(i);
    if (selectedFeature == preferredFeature) {
      break;
    }
  }

  fsrFeatures.pNext = fdmFeatures.pNext = fdmOffsetQcomFeatures.pNext = nullptr;
  const char* supportedExtension = nullptr;
  switch (selectedFeature) {
    case SupportedFeature::FRAGMENT_DENSITY_MAP:
      extensionsConnector.withFragmentDensityMapExtension(fdmFeatures);
      supportedExtension = VK_EXT_FRAGMENT_DENSITY_MAP_EXTENSION_NAME;
      break;
    case SupportedFeature::FRAGMENT_DENSITY_MAP_OFFSET:
      extensionsConnector.withFragmentDensityMapExtension(fdmFeatures);
      extensionsConnector.withFragmentDensityMapOffsetExtension(fdmOffsetQcomFeatures);
      supportedExtension = VK_QCOM_FRAGMENT_DENSITY_MAP_OFFSET_EXTENSION_NAME;
      break;
    case SupportedFeature::FRAGMENT_SHADING_RATE:
      extensionsConnector.withFragmentShadingRateExtension(fsrFeatures);
      supportedExtension = VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME;
      break;
  }
  return {AttachmentBasedFragmentShadingRateModifier(physicalDevice, selectedFeature),
          supportedExtension};
}

AttachmentBasedFragmentShadingRateModifier::SupportedFeature
AttachmentBasedFragmentShadingRateModifier::getSelectedFeature() const noexcept {
  return _selectedFeature;
}

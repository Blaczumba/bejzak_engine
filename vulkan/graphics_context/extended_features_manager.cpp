#include "vulkan/graphics_context/extended_features_manager.h"

#include <algorithm>
#include <iterator>

#include "vulkan/wrapper/physical_device/physical_device.h"

namespace vlkn {
namespace {

template <typename T>
void chainExtendedField(void** next, T& feature) {
  feature.pNext = *next;
  *next = (void*)&feature;
}

}  // namespace

FragmentShadingOptimizationImageFeature::FragmentShadingOptimizationImageFeature(
    const PhysicalDevice& physicalDevice, SupportedFeature preferredFeature) {
  VkPhysicalDeviceFragmentDensityMapFeaturesEXT fdmFeatures{
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_FEATURES_EXT};

  VkPhysicalDeviceFragmentShadingRateFeaturesKHR fsrFeatures{
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR};

  VkPhysicalDeviceFeatures2 features2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};

  chainExtendedField(&features2.pNext, fdmFeatures);
  chainExtendedField(&features2.pNext, fsrFeatures);

  vkGetPhysicalDeviceFeatures2(physicalDevice.getVkPhysicalDevice(), &features2);

  bool supportedFeatures[2] = {};
  supportedFeatures[static_cast<uint8_t>(SupportedFeature::FRAGMENT_DENSITY_MAP)] =
      fdmFeatures.fragmentDensityMap;
  supportedFeatures[static_cast<uint8_t>(SupportedFeature::FRAGMENT_SHADING_RATE)] =
      fsrFeatures.attachmentFragmentShadingRate;
  if (supportedFeatures[static_cast<uint8_t>(preferredFeature)]) {
    _feature = preferredFeature;
  } else {
    auto it = std::find(std::cbegin(supportedFeatures), std::cend(supportedFeatures), true);
    _feature =
        it != std::cend(supportedFeatures) ?
            static_cast<SupportedFeature>(std::distance(std::cbegin(supportedFeatures), it)) :
            SupportedFeature::NONE;
  }
}

FragmentShadingOptimizationImageFeature::SupportedFeature
FragmentShadingOptimizationImageFeature::getFeature() const noexcept {
  return _feature;
}

}  // namespace vlkn

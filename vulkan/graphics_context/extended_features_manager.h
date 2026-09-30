#pragma once

#include "vulkan/wrapper/physical_device/physical_device.h"

namespace vlkn {

class FragmentShadingOptimizationImageFeature {
public:
  enum class SupportedFeature : uint8_t {
    FRAGMENT_DENSITY_MAP = 0,
    FRAGMENT_SHADING_RATE,
    NONE,
  };

  FragmentShadingOptimizationImageFeature() = default;

  FragmentShadingOptimizationImageFeature(
      const PhysicalDevice& physicalDevice, SupportedFeature preferredFeature);

  SupportedFeature getFeature() const noexcept;

private:
  SupportedFeature _feature = SupportedFeature::NONE;
};

}  // namespace vlkn

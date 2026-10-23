#pragma once

#include <cstdint>
#include <vulkan/vulkan.h>

#include "vulkan/wrapper/physical_device/extensions_connector.h"
#include "vulkan/wrapper/physical_device/physical_device.h"

class AttachmentBasedFragmentShadingRateModifier {
public:
  enum class SupportedFeature : uint8_t {
    FRAGMENT_DENSITY_MAP = 0,
    FRAGMENT_DENSITY_MAP_OFFSET,
    FRAGMENT_SHADING_RATE,
    NONE  // This must be the last option in the enum.
  };

  AttachmentBasedFragmentShadingRateModifier() noexcept = default;

  ~AttachmentBasedFragmentShadingRateModifier() = default;

  static std::tuple<AttachmentBasedFragmentShadingRateModifier, const char*> create(
      const PhysicalDevice& physicalDevice, ExtensionsConnector& extensionsConnector,
      SupportedFeature preferredFeature = SupportedFeature::NONE) noexcept;

  SupportedFeature getSelectedFeature() const noexcept;

private:
  static constexpr size_t FEATURES_NUMBER = static_cast<size_t>(SupportedFeature::NONE);

  AttachmentBasedFragmentShadingRateModifier(
      const PhysicalDevice& physicalDevice, SupportedFeature selectedFeature) noexcept;

  const PhysicalDevice* _physicalDevice = nullptr;
  SupportedFeature _selectedFeature = SupportedFeature::NONE;
};

#pragma once

#include <cstdint>
#include <optional>
#include <tuple>
#include <utility>
#include <vulkan/vulkan.h>

#include "vulkan/wrapper/builders/dependency_info_builder.h"
#include "vulkan/wrapper/command_buffer/command_buffer.h"
#include "vulkan/wrapper/command_buffer/command_pool.h"
#include "vulkan/wrapper/memory_objects/image.h"
#include "vulkan/wrapper/physical_device/extensions_connector.h"
#include "vulkan/wrapper/physical_device/physical_device.h"
#include "vulkan/wrapper/pipeline/graphics_pipeline_builder.h"
#include "vulkan/wrapper/render_pass/attachment_layout.h"
#include "vulkan/wrapper/render_pass/render_pass.h"

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

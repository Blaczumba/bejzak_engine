#pragma once

#include <cstdint>
#include <optional>
#include <tuple>
#include <vulkan/vulkan.h>

#include "vulkan/wrapper/physical_device/physical_device.h"
#include "vulkan/wrapper/pipeline/graphics_pipeline_builder.h"
#include "vulkan/wrapper/render_pass/attachment_layout.h"
#include "vulkan/wrapper/render_pass/render_pass.h"
#include "vulkan/wrapper/memory_objects/image.h"
#include "vulkan/wrapper/command_buffer/command_buffer.h"

class Modifier {
public:
  virtual ~Modifier() = default;

  virtual void modify(RenderpassBuilder& builder) const {};

  virtual void modify(RenderpassBuilder::Subpass& subpass) const {};

  virtual void modify(GraphicsPipelineBuilder& builder) const {};

  virtual void modify(AttachmentLayout& layout) const {};
};

class AttachmentBasedFragmentShadingRateModifier : public Modifier {
public:
  enum class SupportedFeature : uint8_t {
    FRAGMENT_DENSITY_MAP = 0,
    FRAGMENT_SHADING_RATE,
    NONE  // This must be the last option in the enum.
  };

  struct PhysicalDeviceFeatures {
    std::optional<VkPhysicalDeviceFragmentDensityMapFeaturesEXT> fragmentDensityMapFeatures;
    std::optional<VkPhysicalDeviceFragmentShadingRateFeaturesKHR> fragmentShadingRateFeatures;
  };

  static std::tuple<AttachmentBasedFragmentShadingRateModifier, PhysicalDeviceFeatures> create(
      const PhysicalDevice& physicalDevice,
      SupportedFeature preferredFeature = SupportedFeature::NONE) noexcept;

  void modify(RenderpassBuilder::Subpass& subpass) const override;

  void modify(GraphicsPipelineBuilder& builder) const override;

  void modify(AttachmentLayout& layout) const override;

  std::optional<std::tuple<Image, ImageMetadata>> createFragmentShadingOptimizationImage(
      const LogicalDevice& logicalDevice, const CommandBuffer& commandBuffer, VkExtent2D extent,
      VkExtent2D preferredTexelSize, uint32_t numLayers) const;

private:
  static constexpr size_t FEATURES_NUMBER = static_cast<size_t>(SupportedFeature::NONE);

  AttachmentBasedFragmentShadingRateModifier(
      const PhysicalDevice& physicalDevice, SupportedFeature selectedFeature) noexcept;

  const PhysicalDevice& _physicalDevice;
  const SupportedFeature _selectedFeature;
};

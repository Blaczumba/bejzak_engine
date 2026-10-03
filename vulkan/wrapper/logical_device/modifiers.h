#pragma once

#include <cstdint>
#include <optional>
#include <tuple>
#include <utility>
#include <vulkan/vulkan.h>

#include "vulkan/wrapper/builders/dependency_info_builder.h"
#include "vulkan/wrapper/command_buffer/command_buffer.h"
#include "vulkan/wrapper/logical_device/extensions_connector.h"
#include "vulkan/wrapper/memory_objects/image.h"
#include "vulkan/wrapper/physical_device/physical_device.h"
#include "vulkan/wrapper/pipeline/graphics_pipeline_builder.h"
#include "vulkan/wrapper/render_pass/attachment_layout.h"
#include "vulkan/wrapper/render_pass/render_pass.h"

class Modifier {
public:
  virtual ~Modifier() = default;

  virtual void modify(RenderpassBuilder& builder) const {};

  virtual void modify(RenderpassBuilder::Subpass& subpass) const {};

  virtual void modify(GraphicsPipelineBuilder& builder) const {};

  virtual void modify(AttachmentLayout& layout) const {};
};

class AttachmentBasedFragmentShadingRateModifier {
public:
  enum class SupportedFeature : uint8_t {
    FRAGMENT_DENSITY_MAP = 0,
    FRAGMENT_SHADING_RATE,
    NONE  // This must be the last option in the enum.
  };

  AttachmentBasedFragmentShadingRateModifier() noexcept = default;

  ~AttachmentBasedFragmentShadingRateModifier() = default;

  static std::tuple<AttachmentBasedFragmentShadingRateModifier, const char*> create(
      const PhysicalDevice& physicalDevice, ExtensionsConnector& extensionsConnector,
      SupportedFeature preferredFeature = SupportedFeature::NONE) noexcept;

  void modify(RenderpassBuilder::Subpass& subpass) const;

  void modify(GraphicsPipelineBuilder& builder) const;

  void modify(AttachmentLayout& layout) const;

  SupportedFeature getSelectedFeature() const noexcept;

  std::optional<std::tuple<Image, ImageMetadata>> createFragmentShadingOptimizationImage(
      const LogicalDevice& logicalDevice, VkExtent2D extent, VkExtent2D preferredTexelSize,
      uint32_t numLayers) const;

  void dispatchComputeFragmentShadingOptimizationImage(
      const CommandBuffer& commandBuffer, VkImage image, uint32_t layers, VkPipelineLayout layout,
      std::pair<uint32_t, uint32_t> foveationPoint,
      DependencyInfoBuilder& dependencyInfoBuilder) const;

private:
  static constexpr size_t FEATURES_NUMBER = static_cast<size_t>(SupportedFeature::NONE);

  AttachmentBasedFragmentShadingRateModifier(
      const PhysicalDevice& physicalDevice, SupportedFeature selectedFeature) noexcept;

  const PhysicalDevice* _physicalDevice = nullptr;
  SupportedFeature _selectedFeature = SupportedFeature::NONE;
};

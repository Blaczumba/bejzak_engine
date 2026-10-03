#include "vulkan/wrapper/logical_device/modifiers.h"

#include <algorithm>
#include <bitset>
#include <cmath>
#include <tuple>
#include <utility>
#include <vulkan/vulkan.h>

#include "vulkan/wrapper/builders/dependency_info_builder.h"
#include "vulkan/wrapper/logical_device/extensions_connector.h"
#include "vulkan/wrapper/memory_objects/image.h"
#include "vulkan/wrapper/physical_device/physical_device.h"
#include "vulkan/wrapper/pipeline/graphics_pipeline_builder.h"
#include "vulkan/wrapper/render_pass/attachment_layout.h"
#include "vulkan/wrapper/render_pass/render_pass.h"

namespace {

template <typename T>
void chainExtendedField(VkPhysicalDeviceFeatures2& deviceFeatures, T& feature) {
  feature.pNext = deviceFeatures.pNext;
  deviceFeatures.pNext = &feature;
}

}  // namespace

AttachmentBasedFragmentShadingRateModifier::AttachmentBasedFragmentShadingRateModifier(
    const PhysicalDevice& physicalDevice, SupportedFeature selectedFeature) noexcept
  : _physicalDevice(&physicalDevice), _selectedFeature(selectedFeature) {}

std::tuple<AttachmentBasedFragmentShadingRateModifier, const char*>
AttachmentBasedFragmentShadingRateModifier::create(
    const PhysicalDevice& physicalDevice, ExtensionsConnector& extensionsConnector,
    SupportedFeature preferredFeature) noexcept {
  VkPhysicalDeviceFragmentDensityMapFeaturesEXT fdmFeatures{
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_FEATURES_EXT};
  VkPhysicalDeviceFragmentShadingRateFeaturesKHR fsrFeatures{
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR};
  VkPhysicalDeviceFeatures2 features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};

  if (physicalDevice.hasAvailableExtension(VK_EXT_FRAGMENT_DENSITY_MAP_EXTENSION_NAME)) {
    chainExtendedField(features, fdmFeatures);
  }
  if (physicalDevice.hasAvailableExtension(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME)) {
    chainExtendedField(features, fsrFeatures);
  }

  vkGetPhysicalDeviceFeatures2(physicalDevice.getVkPhysicalDevice(), &features);

  std::bitset<FEATURES_NUMBER> supportedFeatures;
  supportedFeatures[static_cast<size_t>(SupportedFeature::FRAGMENT_DENSITY_MAP)] =
      fdmFeatures.fragmentDensityMap && fdmFeatures.fragmentDensityMapNonSubsampledImages;
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

  const char* supportedExtension = nullptr;
  switch (selectedFeature) {
    case SupportedFeature::FRAGMENT_DENSITY_MAP:
      extensionsConnector.withFragmentDensityMapExtension(fdmFeatures);
      supportedExtension = VK_EXT_FRAGMENT_DENSITY_MAP_EXTENSION_NAME;
      break;
    case SupportedFeature::FRAGMENT_SHADING_RATE:
      extensionsConnector.withFragmentShadingRateExtension(fsrFeatures);
      supportedExtension = VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME;
      break;
  }
  return {AttachmentBasedFragmentShadingRateModifier(physicalDevice, selectedFeature),
          supportedExtension};
}

void AttachmentBasedFragmentShadingRateModifier::modify(RenderpassBuilder::Subpass& subpass) const {
  // TODO: What if _physicalDevice is nullptr?
  if (_selectedFeature == SupportedFeature::FRAGMENT_SHADING_RATE) {
    const VkExtent2D fsrTexelSize = _physicalDevice->getFragmentShadingRateProperties()
                                        .maxFragmentShadingRateAttachmentTexelSize;
    subpass.withShadingRateAttachment(fsrTexelSize.width, fsrTexelSize.height);
  }
}

void AttachmentBasedFragmentShadingRateModifier::modify(GraphicsPipelineBuilder& builder) const {
  if (_selectedFeature == SupportedFeature::FRAGMENT_SHADING_RATE) {
    builder.withFragmentShadingRateStateCreateInfo(
        {1, 1}, VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR,
        VK_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_KHR);
  }
}

void AttachmentBasedFragmentShadingRateModifier::modify(AttachmentLayout& layout) const {
  switch (_selectedFeature) {
    case SupportedFeature::FRAGMENT_DENSITY_MAP:
      layout.addFragmentDensityMapAttachment();
      return;
    case SupportedFeature::FRAGMENT_SHADING_RATE:
      layout.addFragmentShadingRateAttachment();
      return;
  }
}

AttachmentBasedFragmentShadingRateModifier::SupportedFeature
AttachmentBasedFragmentShadingRateModifier::getSelectedFeature() const noexcept {
  return _selectedFeature;
}

std::optional<std::tuple<Image, ImageMetadata>>
AttachmentBasedFragmentShadingRateModifier::createFragmentShadingOptimizationImage(
    const LogicalDevice& logicalDevice, VkExtent2D extent, VkExtent2D preferredTexelSize,
    uint32_t numLayers) const {
  // TODO: What if _physicalDevice is nullptr?
  switch (_selectedFeature) {
    case SupportedFeature::FRAGMENT_DENSITY_MAP:
      {
        const VkPhysicalDeviceFragmentDensityMapPropertiesEXT& fdmProperties =
            _physicalDevice->getFragmentDensityMapProperties();
        const VkExtent2D fdmTexelExtent = VkExtent2D{
          std::clamp(preferredTexelSize.width, fdmProperties.minFragmentDensityTexelSize.width,
                     fdmProperties.maxFragmentDensityTexelSize.width),
          std::clamp(preferredTexelSize.height, fdmProperties.minFragmentDensityTexelSize.height,
                     fdmProperties.maxFragmentDensityTexelSize.height)};
        const VkExtent2D fdmExtent = VkExtent2D{
          static_cast<uint32_t>(std::ceil(extent.width / static_cast<float>(fdmTexelExtent.width))),
          static_cast<uint32_t>(
              std::ceil(extent.height / static_cast<float>(fdmTexelExtent.height)))};
        auto [image, imageMetadata] =
            ImageBuilder()
                .withFormat(VK_FORMAT_R8G8_UNORM)
                .withNumSamples(VK_SAMPLE_COUNT_1_BIT)
                .withExtent(fdmExtent)
                .withLayerCount(numLayers)
                .withAspect(VK_IMAGE_ASPECT_COLOR_BIT)
                .withUsage(VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT | VK_IMAGE_USAGE_STORAGE_BIT)
                .buildImageWithMetadata(logicalDevice);
        return std::tuple{std::move(image), imageMetadata};
      }
    case SupportedFeature::FRAGMENT_SHADING_RATE:
      {
        const VkPhysicalDeviceFragmentShadingRatePropertiesKHR& fsrProperties =
            _physicalDevice->getFragmentShadingRateProperties();
        const VkExtent2D fsrTexelExtent = VkExtent2D{
          std::clamp(preferredTexelSize.width,
                     fsrProperties.minFragmentShadingRateAttachmentTexelSize.width,
                     fsrProperties.maxFragmentShadingRateAttachmentTexelSize.width),
          std::clamp(preferredTexelSize.height,
                     fsrProperties.minFragmentShadingRateAttachmentTexelSize.height,
                     fsrProperties.maxFragmentShadingRateAttachmentTexelSize.height)};
        const VkExtent2D fsrExtent = VkExtent2D{
          static_cast<uint32_t>(std::ceil(extent.width / static_cast<float>(fsrTexelExtent.width))),
          static_cast<uint32_t>(
              std::ceil(extent.height / static_cast<float>(fsrTexelExtent.height)))};
        auto [image, imageMetadata] =
            ImageBuilder()
                .withFormat(VK_FORMAT_R8_UINT)
                .withNumSamples(VK_SAMPLE_COUNT_1_BIT)
                .withExtent(fsrExtent)
                .withLayerCount(numLayers)
                .withAspect(VK_IMAGE_ASPECT_COLOR_BIT)
                .withUsage(VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR
                           | VK_IMAGE_USAGE_STORAGE_BIT)
                .buildImageWithMetadata(logicalDevice);
        return std::tuple{std::move(image), imageMetadata};
      }
  }
  return std::nullopt;
}

void AttachmentBasedFragmentShadingRateModifier::dispatchComputeFragmentShadingOptimizationImage(
    const CommandBuffer& commandBuffer, VkImage image, uint32_t layers, VkPipelineLayout layout,
    std::pair<uint32_t, uint32_t> foveationPoint,
    DependencyInfoBuilder& dependencyInfoBuilder) const {
  if (_selectedFeature == SupportedFeature::NONE) {
    return;
  }
  commandBuffer.pushConstants(
      layout, VK_SHADER_STAGE_COMPUTE_BIT,
      std::span{reinterpret_cast<const std::byte*>(&foveationPoint), sizeof(foveationPoint)});
  commandBuffer.dispatchCompute(16, 16, layers);
  ImageMemoryBarrierBuilder& memoryBarrierBuilder =
      dependencyInfoBuilder.addImageMemoryBarrier()
          .withSrcMasks(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT)
          .withImage(image, VkImageSubresourceRange{
                              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                              .baseMipLevel = 0,
                              .levelCount = 1,
                              .baseArrayLayer = 0,
                              .layerCount = layers,
                            });
  switch (_selectedFeature) {
    case SupportedFeature::FRAGMENT_DENSITY_MAP:
      memoryBarrierBuilder
          .withDstMasks(VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT,
                        VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT)
          .withLayouts(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT);
      break;
    case SupportedFeature::FRAGMENT_SHADING_RATE:
      memoryBarrierBuilder
          .withDstMasks(VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR,
                        VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR)
          .withLayouts(VK_IMAGE_LAYOUT_GENERAL,
                       VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR);
      break;
  }
}

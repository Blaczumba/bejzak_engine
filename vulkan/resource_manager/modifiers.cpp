#include "vulkan/resource_manager/modifiers.h"

#include <algorithm>
#include <bitset>
#include <cmath>
#include <tuple>
#include <limits>
#include <vulkan/vulkan.h>

#include "vulkan/wrapper/physical_device/physical_device.h"
#include "vulkan/wrapper/pipeline/graphics_pipeline_builder.h"
#include "vulkan/wrapper/render_pass/attachment_layout.h"
#include "vulkan/wrapper/render_pass/render_pass.h"
#include "vulkan/wrapper/memory_objects/image.h"

namespace {

template <typename T>
void chainExtendedField(VkPhysicalDeviceFeatures2& deviceFeatures, T& feature) {
  feature.pNext = deviceFeatures.pNext;
  deviceFeatures.pNext = &feature;
}

}  // namespace

AttachmentBasedFragmentShadingRateModifier::AttachmentBasedFragmentShadingRateModifier(
    const PhysicalDevice& physicalDevice, SupportedFeature selectedFeature) noexcept
  : _physicalDevice(physicalDevice), _selectedFeature(selectedFeature) {}

std::tuple<AttachmentBasedFragmentShadingRateModifier,
           AttachmentBasedFragmentShadingRateModifier::PhysicalDeviceFeatures>
AttachmentBasedFragmentShadingRateModifier::create(
    const PhysicalDevice& physicalDevice, SupportedFeature preferredFeature) noexcept {
  VkPhysicalDeviceFragmentDensityMapFeaturesEXT fdmFeatures{
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_FEATURES_EXT};
  VkPhysicalDeviceFragmentShadingRateFeaturesKHR fsrFeatures{
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR};
  VkPhysicalDeviceFeatures2 features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};

  chainExtendedField(features, fdmFeatures);
  chainExtendedField(features, fsrFeatures);

  vkGetPhysicalDeviceFeatures2(physicalDevice.getVkPhysicalDevice(), &features);

  std::bitset<FEATURES_NUMBER> supportedFeatures;
  PhysicalDeviceFeatures physicalDeviceFeatures;

  auto registerFeature = [&]<typename T>(SupportedFeature feature, T vkFeatures,
                                         VkBool32 isSupported, std::optional<T>& out) {
    if (!isSupported) {
      return;
    }
    supportedFeatures[static_cast<size_t>(feature)] = true;
    vkFeatures.pNext = nullptr;
    out = vkFeatures;
  };

  registerFeature(
      SupportedFeature::FRAGMENT_DENSITY_MAP, fdmFeatures, fdmFeatures.fragmentDensityMap,
      physicalDeviceFeatures.fragmentDensityMapFeatures);
  registerFeature(SupportedFeature::FRAGMENT_SHADING_RATE, fsrFeatures,
                  fsrFeatures.attachmentFragmentShadingRate,
                  physicalDeviceFeatures.fragmentShadingRateFeatures);

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

  return {AttachmentBasedFragmentShadingRateModifier(physicalDevice, selectedFeature),
          physicalDeviceFeatures};
}

void AttachmentBasedFragmentShadingRateModifier::modify(RenderpassBuilder::Subpass& subpass) const {
  if (_selectedFeature == SupportedFeature::FRAGMENT_SHADING_RATE) {
    const VkExtent2D fsrTexelSize = _physicalDevice.getFragmentShadingRateProperties()
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

std::optional<std::tuple<Image, ImageMetadata>>
AttachmentBasedFragmentShadingRateModifier::createFragmentShadingOptimizationImage(
    const LogicalDevice& logicalDevice, const CommandBuffer& commandBuffer,
    VkExtent2D extent, VkExtent2D preferredTexelSize, uint32_t numLayers) const {
  switch (_selectedFeature) {
    case SupportedFeature::FRAGMENT_DENSITY_MAP: {
        const VkPhysicalDeviceFragmentDensityMapPropertiesEXT& fdmProperties =
            _physicalDevice.getFragmentDensityMapProperties();
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
                .withUsage(VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT
                           | VK_IMAGE_USAGE_STORAGE_BIT)
                .buildImageWithMetadata(logicalDevice);
        ImageViewBuilder().buildAndAddToImage(image, imageMetadata, 0, 1, 0, numLayers);
        commandBuffer.transitionImageLayout(
            image.getVkImage(), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_GENERAL, 0, imageMetadata.mipLevels, 0,
            imageMetadata.arrayLayers);
        return std::tuple{std::move(image), imageMetadata};
    }
    case SupportedFeature::FRAGMENT_SHADING_RATE:
      {
        const VkPhysicalDeviceFragmentShadingRatePropertiesKHR& fsrProperties =
            _physicalDevice.getFragmentShadingRateProperties();
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
        ImageViewBuilder().buildAndAddToImage(image, imageMetadata, 0, 1, 0, numLayers);
        commandBuffer.transitionImageLayout(
            image.getVkImage(), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_GENERAL, 0, imageMetadata.mipLevels, 0, imageMetadata.arrayLayers);
        return std::tuple{std::move(image), imageMetadata};
      }
  }
  return std::nullopt;
}

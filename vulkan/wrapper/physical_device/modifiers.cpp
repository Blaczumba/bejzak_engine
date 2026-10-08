#include "vulkan/wrapper/physical_device/modifiers.h"

#include <algorithm>
#include <bitset>
#include <cmath>
#include <tuple>
#include <utility>
#include <vulkan/vulkan.h>

#include "vulkan/wrapper/builders/dependency_info_builder.h"
#include "vulkan/wrapper/command_buffer/command_buffer.h"
#include "vulkan/wrapper/command_buffer/command_pool.h"
#include "vulkan/wrapper/command_buffer/single_time_command_buffer.h"
#include "vulkan/wrapper/memory_objects/buffer.h"
#include "vulkan/wrapper/memory_objects/image.h"
#include "vulkan/wrapper/physical_device/extensions_connector.h"
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
      break;
    case SupportedFeature::FRAGMENT_DENSITY_MAP_OFFSET:
      layout.addFragmentDensityMapOffsetAttachment();
      break;
    case SupportedFeature::FRAGMENT_SHADING_RATE:
      layout.addFragmentShadingRateAttachment();
      break;
  }
}

AttachmentBasedFragmentShadingRateModifier::SupportedFeature
AttachmentBasedFragmentShadingRateModifier::getSelectedFeature() const noexcept {
  return _selectedFeature;
}

std::optional<std::tuple<Image, ImageMetadata>>
AttachmentBasedFragmentShadingRateModifier::createFragmentShadingOptimizationImage(
    const LogicalDevice& logicalDevice, const CommandPool& commandPool, VkExtent2D extent,
    VkExtent2D preferredTexelSize, uint32_t numLayers) const {
  // TODO: What if _physicalDevice is nullptr?
  VkFormat format;
  VkImageCreateFlags flags{};
  VkImageUsageFlags usage;
  VkExtent2D optimizationExtent;
  switch (_selectedFeature) {
    case SupportedFeature::FRAGMENT_DENSITY_MAP:
      {
        format = VK_FORMAT_R8G8_UNORM;
        usage = VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT | VK_IMAGE_USAGE_STORAGE_BIT;
        const VkPhysicalDeviceFragmentDensityMapPropertiesEXT& fdmProperties =
            _physicalDevice->getFragmentDensityMapProperties();
        optimizationExtent =
            clampExtent(extent, preferredTexelSize, fdmProperties.minFragmentDensityTexelSize,
                        fdmProperties.maxFragmentDensityTexelSize);
        break;
      }
    case SupportedFeature::FRAGMENT_DENSITY_MAP_OFFSET:
      {
        format = VK_FORMAT_R8G8_UNORM;
        flags = VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM;
        usage = VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        const VkPhysicalDeviceFragmentDensityMapPropertiesEXT& fdmProperties =
            _physicalDevice->getFragmentDensityMapProperties();
        optimizationExtent = VkExtent2D(3 * extent.width/fdmProperties.minFragmentDensityTexelSize.width / 2, 3 * extent.height/fdmProperties.minFragmentDensityTexelSize.height / 2);
//            clampExtent(extent, {2, 2}, fdmProperties.minFragmentDensityTexelSize,
//                        fdmProperties.maxFragmentDensityTexelSize);
        break;
      }
    case SupportedFeature::FRAGMENT_SHADING_RATE:
      {
        format = VK_FORMAT_R8_UINT;
        usage =
            VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR | VK_IMAGE_USAGE_STORAGE_BIT;
        const VkPhysicalDeviceFragmentShadingRatePropertiesKHR& fsrProperties =
            _physicalDevice->getFragmentShadingRateProperties();
        optimizationExtent = clampExtent(
            extent, preferredTexelSize, fsrProperties.minFragmentShadingRateAttachmentTexelSize,
            fsrProperties.maxFragmentShadingRateAttachmentTexelSize);
        SingleTimeCommandBuffer handle(commandPool);
        break;
      }
    default:
      return std::nullopt;
  }
  auto [image, metadata] =
      ImageBuilder()
          .withFormat(format)
          .withNumSamples(VK_SAMPLE_COUNT_1_BIT)
          .withExtent(optimizationExtent)
          .withLayerCount(numLayers)
          .withAspect(VK_IMAGE_ASPECT_COLOR_BIT)
          .withUsage(usage)
          .withFlags(flags)
          .buildImageWithMetadata(logicalDevice);

  VkImageLayout inLayout;
  VkImageLayout outLayout;
  switch (_selectedFeature) {
    case SupportedFeature::FRAGMENT_DENSITY_MAP_OFFSET:
      {
        // TODO: This should be written from config;
        lib::Buffer<std::byte> copyBuffer(
            metadata.imageExtent.width * metadata.imageExtent.height * 2, std::byte{255});
        for (uint32_t i = 0, w = 0, h = 0; i < copyBuffer.size(); i += 2, w++) {
          if (w == metadata.imageExtent.width) {
            w = 0;
            h++;
            if (h == metadata.imageExtent.height) {
              h = 0;
            }
          }
          if (i < copyBuffer.size() / 3) {
            copyBuffer[i] = copyBuffer[i + 1] = std::byte{0};
          }
        }
        auto [buffer, bufferMetadata] =
            BufferBuilder()
                .withSize(copyBuffer.size())
                .withUsage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
                .buildStagingBufferWithMetadata(logicalDevice);
        std::memcpy(bufferMetadata.mappedMemory, copyBuffer.data(), copyBuffer.size());
        lib::Buffer<VkBufferImageCopy> imageCopy(numLayers);
        for (uint32_t layer = 0; layer < imageCopy.size(); layer++) {
          imageCopy[layer] = VkBufferImageCopy{
            .imageSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                 .mipLevel = 0,
                                 .baseArrayLayer = layer,
                                 .layerCount = 1},
            .imageExtent = metadata.imageExtent,
          };
        }
        SingleTimeCommandBuffer handle(commandPool);
        handle.transitionImageLayout(
            image.getVkImage(), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, 1, 0, numLayers);
        handle.copyBufferToImage(buffer.getVkBuffer(), image.getVkImage(), imageCopy);
        inLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        outLayout = VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT;
        break;
      }
    case SupportedFeature::FRAGMENT_DENSITY_MAP:
      {
        inLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        outLayout = VK_IMAGE_LAYOUT_GENERAL;
      }
    case SupportedFeature::FRAGMENT_SHADING_RATE:
      {
        inLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        outLayout = VK_IMAGE_LAYOUT_GENERAL;
      }
  }
  SingleTimeCommandBuffer handle(commandPool);
  handle.transitionImageLayout(
      image.getVkImage(), VK_IMAGE_ASPECT_COLOR_BIT, inLayout, outLayout, 0, 1, 0, numLayers);
  return std::tuple{std::move(image), metadata};
}

void AttachmentBasedFragmentShadingRateModifier::dispatchComputeFragmentShadingOptimizationImage(
    const CommandBuffer& commandBuffer, VkImage image, uint32_t layers, VkPipelineLayout layout,
    std::pair<uint32_t, uint32_t> foveationPoint,
    DependencyInfoBuilder& dependencyInfoBuilder) const {
  if (_selectedFeature != SupportedFeature::FRAGMENT_DENSITY_MAP
      && _selectedFeature != SupportedFeature::FRAGMENT_SHADING_RATE) {
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

#pragma once

#include "graphics_plugin_vulkan.h"

#include "vulkan_wrapper/memory_objects/texture.h"
#include "vulkan_wrapper/resource_manager/asset_manager.h"

namespace {

ErrorOr<Texture> createCubemap(const LogicalDevice &logicalDevice,
                               VkCommandBuffer commandBuffer,
                               VkBuffer stagingBuffer,
                               const ImageDimensions &dimensions,
                               VkFormat format, float samplerAnisotropy) {
  return TextureBuilder()
      .withAspect(VK_IMAGE_ASPECT_COLOR_BIT)
      .withExtent(dimensions.width, dimensions.height)
      .withFormat(format)
      .withMipLevels(dimensions.mipLevels)
      .withUsage(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
      .withLayerCount(6)
      .withMaxAnisotropy(samplerAnisotropy)
      .withMaxLod(static_cast<float>(dimensions.mipLevels))
      .withLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
      .buildImage(logicalDevice, commandBuffer, stagingBuffer,
                  dimensions.copyRegions);
}

} // namespace

namespace xrw {

class TemporalRenderer : public GraphicsPluginVulkan {
 public:
  Status initialize(XrInstance xrInstance, XrSystemId systemId) override {
    RETURN_IF_ERROR((static_cast<GraphicsPluginVulkan&>(*this).initialize(xrInstance, systemId)));
  }

 private:
  AssetManager _assetManager;
  Texture _textureCubemap;

  Status createDescriptorSets() {
    float maxSamplerAnisotropy = _physicalDevice->getMaxSamplerAnisotropy();
    _assetManager.loadImageCubemapAsync(_logicalDevice, TEXTURES_PATH
    "cubemap_yokohama_rgba.ktx");
    {
      SingleTimeCommandBuffer handle(*_singleTimeCommandPool);
      const VkCommandBuffer commandBuffer = handle.getCommandBuffer();
      ASSIGN_OR_RETURN(
          const AssetManager::ImageData &imgData,
          _assetManager.getImageData(TEXTURES_PATH "cubemap_yokohama_rgba.ktx"));
      ASSIGN_OR_RETURN(_textureCubemap,
                       createCubemap(_logicalDevice, commandBuffer,
                                     imgData.stagingBuffer.getVkBuffer(),
                                     imgData.imageDimensions,
                                     VK_FORMAT_R8G8B8A8_UNORM,
                                     maxSamplerAnisotropy));
    }
  }
};

} // xrw
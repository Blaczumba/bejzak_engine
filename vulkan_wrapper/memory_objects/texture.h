#pragma once

#include <span>
#include <vector>
#include <vulkan/vulkan.h>

#include "common/status/status.h"
#include "vulkan_wrapper/memory_allocator/allocation.h"
#include "vulkan_wrapper/memory_allocator/memory_allocator.h"
#include "vulkan_wrapper/memory_objects/image.h"

class LogicalDevice;

struct Texture {
public:
  Texture() = default;

  Texture(Texture&& texture) noexcept;

  Texture& operator=(Texture&& texuture) noexcept;

  ~Texture();

  ErrorOr<VkImageView> addCreateVkImageView(
      uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount);

  void transitionLayout(VkCommandBuffer commandBuffer, VkImageLayout newLayout);

  VkImage getVkImage() const;

  VkImageView getVkImageView(size_t index = 0) const;

  VkSampler getVkSampler() const;

  VkExtent2D getVkExtent2D() const;

  VkExtent3D getVkExtent3D() const;

  VkImageLayout getVkImageLayout() const;

private:
  Texture(const LogicalDevice& logicalDevice, VkImage image, const Allocation allocation,
          const ImageParameters& imageParameters, VkImageLayout layout,
          VkSampler sampler = VK_NULL_HANDLE);

  VkImage _image = VK_NULL_HANDLE;
  std::vector<VkImageView> _views;
  // Create separate Sampler class which is not owned by Texture.
  VkSampler _sampler = VK_NULL_HANDLE;
  Allocation _allocation;
  VkImageLayout _layout;
  ImageParameters _imageParameters;

  const LogicalDevice* _logicalDevice;

  friend class TextureBuilder;
};

class TextureBuilder {
public:
  TextureBuilder& withType(VkImageType type);

  TextureBuilder& withLayout(VkImageLayout layout);

  TextureBuilder& withFormat(VkFormat format);

  TextureBuilder& withExtent(uint32_t width);

  TextureBuilder& withExtent(uint32_t width, uint32_t height);

  TextureBuilder& withExtent(VkExtent2D extent);

  TextureBuilder& withExtent(uint32_t width, uint32_t height, uint32_t depth);

  TextureBuilder& withExtent(VkExtent3D extent);

  TextureBuilder& withAspect(VkImageAspectFlags aspect);

  TextureBuilder& withMipLevels(uint32_t mipLevels);

  TextureBuilder& withNumSamples(VkSampleCountFlagBits numSamples);

  TextureBuilder& withTiling(VkImageTiling tiling);

  TextureBuilder& withUsage(VkImageUsageFlags usage);

  TextureBuilder& withProperties(VkMemoryPropertyFlags properties);

  TextureBuilder& withLayerCount(uint32_t layerCount);

  TextureBuilder& withAdditionalCreateInfoFlags(VkImageCreateFlags flags);

  TextureBuilder& withMagFilter(VkFilter magFilter);

  TextureBuilder& withMinFilter(VkFilter minFilter);

  TextureBuilder& withMipmapMode(VkSamplerMipmapMode mipmapMode);

  TextureBuilder& withAddressModes(
      VkSamplerAddressMode addressModeU, VkSamplerAddressMode addressModeV,
      VkSamplerAddressMode addressModeW);

  TextureBuilder& withMipLodBias(float mipLodBias);

  TextureBuilder& withMaxAnisotropy(float maxAnisotropy);

  TextureBuilder& withCompareOp(VkCompareOp compareOp);

  TextureBuilder& withMinLod(float minLod);

  TextureBuilder& withMaxLod(float maxLod);

  TextureBuilder& withBorderColor(VkBorderColor borderColor);

  TextureBuilder& withUnnormalizedCoordinates(VkBool32 unnormalizedCoordinates);

  ErrorOr<Texture> buildAttachment(
      const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer) const;

  ErrorOr<Texture> buildImage(
      const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, VkBuffer copyBuffer,
      const std::span<const VkBufferImageCopy> copyRegions) const;

  ErrorOr<Texture> buildImageSampler(
      const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer) const;

  ErrorOr<Texture> buildMipmapImage(
      const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, VkBuffer copyBuffer,
      std::span<const VkBufferImageCopy> copyRegions) const;

private:
  ImageParameters _imageParameters;
  SamplerParameters _samplerParameters;
  VkImageLayout _imageLayout = VK_IMAGE_LAYOUT_GENERAL;
};

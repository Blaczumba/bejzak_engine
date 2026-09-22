#pragma once

#include <array>
#include <cstdint>
#include <expected>
#include <initializer_list>
#include <memory>
#include <span>
#include <vector>
#include <vulkan/vulkan.h>

#include "lib/buffer/buffer.h"
#include "vulkan/wrapper/descriptor_set/descriptor_set.h"
#include "vulkan/wrapper/logical_device/logical_device.h"

class DescriptorPool : public std::enable_shared_from_this<const DescriptorPool> {
  DescriptorPool(const LogicalDevice& logicalDevice, VkDescriptorPool descriptorPool,
                 uint32_t maxNumSets, std::span<const VkDescriptorPoolSize> poolSizes) noexcept;

public:
  enum class Error : uint8_t {
    MAX_SETS_REACHED,
    DESCRIPTOR_TYPE_NOT_FOUND,
    INSUFFICIENT_DESCRIPTOR_COUNT
  };

  ~DescriptorPool();

  static std::unique_ptr<DescriptorPool> create(
      const LogicalDevice& logicalDevice, const VkDescriptorPoolCreateInfo& createInfo);

  VkDescriptorPool getVkDescriptorPool() const noexcept;

  std::expected<DescriptorSet, DescriptorPool::Error> createDesriptorSet(
      VkDescriptorSetLayout layout, std::span<const VkDescriptorPoolSize> poolSizes) const;

  template <std::size_t COUNT>
  std::expected<std::array<DescriptorSet, COUNT>, DescriptorPool::Error> createDesriptorSets(
      std::span<const VkDescriptorSetLayout> layouts,
      std::span<const VkDescriptorPoolSize> poolSizes) const;

  std::expected<std::vector<DescriptorSet>, DescriptorPool::Error> createDesriptorSets(
      std::span<const VkDescriptorSetLayout> layout,
      std::span<const VkDescriptorPoolSize> poolSizes) const;

  bool maxSetsReached() const noexcept;

  const LogicalDevice& getLogicalDevice() const;

private:
  std::expected<lib::Buffer<VkDescriptorPoolSize>, DescriptorPool::Error> getUpdatedPoolSizes(
      std::span<const VkDescriptorPoolSize> remainingPoolSizes,
      std::span<const VkDescriptorPoolSize> poolSizes) const;

  VkDescriptorPool _descriptorPool;

  mutable uint32_t _remainingSets;
  mutable lib::Buffer<VkDescriptorPoolSize> _remainingPoolSizes;

  const LogicalDevice& _logicalDevice;
};

template <std::size_t COUNT>
std::expected<std::array<DescriptorSet, COUNT>, DescriptorPool::Error> DescriptorPool::
    createDesriptorSets(std::span<const VkDescriptorSetLayout> layouts,
                        std::span<const VkDescriptorPoolSize> poolSizes) const {
  if (_remainingSets < COUNT) {
    return std::unexpected(DescriptorPool::Error::MAX_SETS_REACHED);
  }

  std::expected<lib::Buffer<VkDescriptorPoolSize>, DescriptorPool::Error> expectedPoolSizes =
      getUpdatedPoolSizes(_remainingPoolSizes, poolSizes);
  if (!expectedPoolSizes.has_value()) {
    return std::unexpected(expectedPoolSizes.error());
  }
  _remainingPoolSizes = std::move(expectedPoolSizes.value());
  _remainingSets -= COUNT;
  return DescriptorSet::create<COUNT>(shared_from_this(), layouts);
}

class DescriptorPoolBuilder {
public:
  DescriptorPoolBuilder&& addPoolSize(VkDescriptorType type, uint32_t descriptorCount) &&;

  DescriptorPoolBuilder&& withPoolSizes(std::span<const VkDescriptorPoolSize> poolSizes) &&;

  DescriptorPoolBuilder&& withPoolSizes(std::initializer_list<VkDescriptorPoolSize> poolSizes) &&;

  DescriptorPoolBuilder&& withPoolSizes(std::vector<VkDescriptorPoolSize>&& poolSizes) && noexcept;

  DescriptorPoolBuilder&& withFlags(VkDescriptorPoolCreateFlags flags) && noexcept;

  std::unique_ptr<DescriptorPool> build(const LogicalDevice& logicalDevice, uint32_t maxNumSets) &&;

private:
  uint32_t _maxNumSets;
  uint32_t _allocatedSets;
  std::vector<VkDescriptorPoolSize> _poolSizes = {
    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1000},
    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100 },
  };
  VkDescriptorPoolCreateFlags _flags;

  void* _pNext = nullptr;
};

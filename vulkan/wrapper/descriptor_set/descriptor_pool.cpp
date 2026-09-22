#include "descriptor_pool.h"

#include <algorithm>
#include <cstdint>
#include <expected>
#include <initializer_list>
#include <memory>
#include <span>

#include "vulkan/wrapper/descriptor_set/descriptor_set.h"
#include "vulkan/wrapper/logical_device/logical_device.h"
#include "vulkan/wrapper/util/check.h"

DescriptorPool::DescriptorPool(
    const LogicalDevice& logicalDevice, VkDescriptorPool descriptorPool, uint32_t maxNumSets,
    std::span<const VkDescriptorPoolSize> poolSizes) noexcept
  : _logicalDevice(logicalDevice), _descriptorPool(descriptorPool), _remainingSets(maxNumSets),
    _remainingPoolSizes(poolSizes) {}

DescriptorPool::~DescriptorPool() {
  _logicalDevice.destroyResource([descriptorPool = _descriptorPool](DestroyerContext context) {
    vkDestroyDescriptorPool(context.device, descriptorPool, context.allocationCallbacks);
  });
}

std::unique_ptr<DescriptorPool> DescriptorPool::create(
    const LogicalDevice& logicalDevice, const VkDescriptorPoolCreateInfo& createInfo) {
  VkDescriptorPool descriptorPool;
  CHECK_VKCMD(
      vkCreateDescriptorPool(logicalDevice.getVkDevice(), &createInfo, nullptr, &descriptorPool),
      "Failed to create VkDescriptorPool.");
  return std::unique_ptr<DescriptorPool>(new DescriptorPool(
      logicalDevice, descriptorPool, createInfo.maxSets,
      std::span<const VkDescriptorPoolSize>(createInfo.pPoolSizes, createInfo.poolSizeCount)));
}

VkDescriptorPool DescriptorPool::getVkDescriptorPool() const noexcept {
  return _descriptorPool;
}

std::expected<DescriptorSet, DescriptorPool::Error> DescriptorPool::createDesriptorSet(
    VkDescriptorSetLayout layout, std::span<const VkDescriptorPoolSize> poolSizes) const {
  if (_remainingSets < 1) {
    return std::unexpected(DescriptorPool::Error::MAX_SETS_REACHED);
  }

  std::expected<lib::Buffer<VkDescriptorPoolSize>, DescriptorPool::Error> expectedPoolSizes =
      getUpdatedPoolSizes(_remainingPoolSizes, poolSizes);
  if (!expectedPoolSizes.has_value()) {
    return std::unexpected(expectedPoolSizes.error());
  }
  _remainingPoolSizes = std::move(expectedPoolSizes.value());
  --_remainingSets;
  return DescriptorSet::create(shared_from_this(), layout);
}

std::expected<std::vector<DescriptorSet>, DescriptorPool::Error> DescriptorPool::
    createDesriptorSets(std::span<const VkDescriptorSetLayout> layouts,
                        std::span<const VkDescriptorPoolSize> poolSizes) const {
  if (_remainingSets < layouts.size()) {
    return std::unexpected(DescriptorPool::Error::MAX_SETS_REACHED);
  }

  std::expected<lib::Buffer<VkDescriptorPoolSize>, DescriptorPool::Error> expectedPoolSizes =
      getUpdatedPoolSizes(_remainingPoolSizes, poolSizes);
  if (!expectedPoolSizes.has_value()) {
    return std::unexpected(expectedPoolSizes.error());
  }
  _remainingPoolSizes = std::move(expectedPoolSizes.value());
  _remainingSets -= layouts.size();
  return DescriptorSet::create(shared_from_this(), layouts);
}

bool DescriptorPool::maxSetsReached() const noexcept {
  return _remainingSets <= 0;
}

const LogicalDevice& DescriptorPool::getLogicalDevice() const {
  return _logicalDevice;
}

DescriptorPoolBuilder&& DescriptorPoolBuilder::addPoolSize(
    VkDescriptorType type, uint32_t descriptorCount) && {
  _poolSizes.emplace_back(type, descriptorCount);
  return std::move(*this);
}

std::expected<lib::Buffer<VkDescriptorPoolSize>, DescriptorPool::Error> DescriptorPool::
    getUpdatedPoolSizes(std::span<const VkDescriptorPoolSize> remainingPoolSizes,
                        std::span<const VkDescriptorPoolSize> poolSizes) const {
  if (_remainingSets <= 0) {
    return std::unexpected(DescriptorPool::Error::MAX_SETS_REACHED);
  }

  // To keep transactionality we need to copy the Buffer and then assign it back.
  lib::Buffer<VkDescriptorPoolSize> tempPoolSizes = remainingPoolSizes;
  for (const auto [type, descriptorCount] : poolSizes) {
    auto it = std::find_if(
        tempPoolSizes.begin(), tempPoolSizes.end(), [type](VkDescriptorPoolSize poolSize) {
          return poolSize.type == type;
        });
    if (it == tempPoolSizes.end()) {
      return std::unexpected(DescriptorPool::Error::DESCRIPTOR_TYPE_NOT_FOUND);
    }

    if (it->descriptorCount < descriptorCount) {
      return std::unexpected(DescriptorPool::Error::INSUFFICIENT_DESCRIPTOR_COUNT);
    }

    it->descriptorCount -= descriptorCount;
  }
  return tempPoolSizes;
}

DescriptorPoolBuilder&& DescriptorPoolBuilder::withPoolSizes(
    std::span<const VkDescriptorPoolSize> poolSizes) && {
  _poolSizes.assign_range(poolSizes);
  return std::move(*this);
}

DescriptorPoolBuilder&& DescriptorPoolBuilder::withPoolSizes(
    std::initializer_list<VkDescriptorPoolSize> poolSizes) && {
  _poolSizes.assign_range(poolSizes);
  return std::move(*this);
}

DescriptorPoolBuilder&& DescriptorPoolBuilder::withPoolSizes(
    std::vector<VkDescriptorPoolSize>&& poolSizes) && noexcept {
  _poolSizes = std::move(poolSizes);
  return std::move(*this);
}

DescriptorPoolBuilder&& DescriptorPoolBuilder::withFlags(
    VkDescriptorPoolCreateFlags flags) && noexcept {
  _flags = flags;
  return std::move(*this);
}

std::unique_ptr<DescriptorPool> DescriptorPoolBuilder::build(
    const LogicalDevice& logicalDevice, uint32_t maxNumSets) && {
  const VkDescriptorPoolCreateInfo createInfo = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .pNext = _pNext,
    .flags = _flags,
    .maxSets = _maxNumSets = maxNumSets,
    .poolSizeCount = static_cast<uint32_t>(_poolSizes.size()),
    .pPoolSizes = !_poolSizes.empty() ? _poolSizes.data() : nullptr};
  return DescriptorPool::create(logicalDevice, createInfo);
}

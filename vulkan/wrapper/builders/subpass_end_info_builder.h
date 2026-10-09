#pragma once

#include <initializer_list>
#include <optional>
#include <span>
#include <vulkan/vulkan.h>

#include "lib/buffer/buffer.h"

class SubpassEndInfoOwningBuilder {
public:
  SubpassEndInfoOwningBuilder() noexcept = default;

  ~SubpassEndInfoOwningBuilder() = default;

  SubpassEndInfoOwningBuilder& withFragmentDensityMapOffsetEndInfo(
      std::initializer_list<VkOffset2D> fragmentDensityOffsets) noexcept;

  SubpassEndInfoOwningBuilder& withFragmentDensityMapOffsetEndInfo(
      std::span<const VkOffset2D> fragmentDensityOffsets) noexcept;

  const VkSubpassEndInfo& build() const noexcept;

private:
  lib::Buffer<VkOffset2D> _fragmentDensityOffsets;
  std::optional<VkSubpassFragmentDensityMapOffsetEndInfoQCOM> _fragmentDensityMapInfo;
  VkSubpassEndInfo _subpassEndInfo{VK_STRUCTURE_TYPE_SUBPASS_END_INFO};
};

class SubpassEndInfoNonOwningBuilder {
public:
  SubpassEndInfoNonOwningBuilder() noexcept = default;

  ~SubpassEndInfoNonOwningBuilder() = default;

  SubpassEndInfoNonOwningBuilder& withFragmentDensityMapOffsetEndInfo(
      std::span<const VkOffset2D> fragmentDensityOffsets) noexcept;

  const VkSubpassEndInfo& build() const noexcept;

private:
  std::optional<VkSubpassFragmentDensityMapOffsetEndInfoQCOM> _fragmentDensityMapInfo;
  VkSubpassEndInfo _subpassEndInfo{VK_STRUCTURE_TYPE_SUBPASS_END_INFO};
};

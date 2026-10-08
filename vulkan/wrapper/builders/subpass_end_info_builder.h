#pragma once

#include <initializer_list>
#include <optional>
#include <vulkan/vulkan.h>

#include "lib/buffer/buffer.h"

class SubpassEndInfoBuilder {
public:
  SubpassEndInfoBuilder() noexcept = default;

  ~SubpassEndInfoBuilder() = default;

  SubpassEndInfoBuilder& withFragmentDensityMapOffsetEndInfo(
      std::initializer_list<VkOffset2D> fragmentDensityOffsets) noexcept;

  VkSubpassEndInfo build() const noexcept;

private:
  lib::Buffer<VkOffset2D> _fragmentDensityOffsets;
  std::optional<VkSubpassFragmentDensityMapOffsetEndInfoQCOM> _fragmentDensityMapInfo;
  void* _pNext = nullptr;
};

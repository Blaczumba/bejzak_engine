#pragma once

#include <vulkan/vulkan.h>
#include <initializer_list>
#include <optional>

class SubpassEndInfoBuilder {
 public:
  SubpassEndInfoBuilder() noexcept = default;

  ~SubpassEndInfoBuilder() = default;

  SubpassEndInfoBuilder& withFragmentDensityMapOffsetEndInfo(std::initializer_list<VkOffset2D> fragmentDensityOffsets) noexcept;

  VkSubpassEndInfo build() const noexcept;

 private:
  std::optional<VkSubpassFragmentDensityMapOffsetEndInfoQCOM> _fragmentDensityMapInfo;
  void* _pNext = nullptr;
};
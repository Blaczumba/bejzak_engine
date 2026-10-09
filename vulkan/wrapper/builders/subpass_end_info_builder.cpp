#include "vulkan/wrapper/builders/subpass_end_info_builder.h"

#include <initializer_list>

namespace {

template <typename Feature>
void chain(const void** pNext, Feature& feature) {
  feature.pNext = *pNext;
  *pNext = &feature;
}

}  // namespace

SubpassEndInfoOwningBuilder& SubpassEndInfoOwningBuilder::withFragmentDensityMapOffsetEndInfo(
    std::initializer_list<VkOffset2D> fragmentDensityOffsets) noexcept {
  _fragmentDensityOffsets = lib::Buffer<VkOffset2D>(fragmentDensityOffsets);
  _fragmentDensityMapInfo = VkSubpassFragmentDensityMapOffsetEndInfoQCOM{
    .sType = VK_STRUCTURE_TYPE_SUBPASS_FRAGMENT_DENSITY_MAP_OFFSET_END_INFO_QCOM,
    .fragmentDensityOffsetCount = static_cast<uint32_t>(_fragmentDensityOffsets.size()),
    .pFragmentDensityOffsets = _fragmentDensityOffsets.data()};
  chain(&_subpassEndInfo.pNext, _fragmentDensityMapInfo);
  return *this;
}

SubpassEndInfoOwningBuilder& SubpassEndInfoOwningBuilder::withFragmentDensityMapOffsetEndInfo(
    std::span<const VkOffset2D> fragmentDensityOffsets) noexcept {
  _fragmentDensityOffsets = lib::Buffer<VkOffset2D>(fragmentDensityOffsets);
  _fragmentDensityMapInfo = VkSubpassFragmentDensityMapOffsetEndInfoQCOM{
    .sType = VK_STRUCTURE_TYPE_SUBPASS_FRAGMENT_DENSITY_MAP_OFFSET_END_INFO_QCOM,
    .fragmentDensityOffsetCount = static_cast<uint32_t>(_fragmentDensityOffsets.size()),
    .pFragmentDensityOffsets = _fragmentDensityOffsets.data()};
  chain(&_subpassEndInfo.pNext, _fragmentDensityMapInfo);
  return *this;
}

const VkSubpassEndInfo& SubpassEndInfoOwningBuilder::build() const noexcept {
  return _subpassEndInfo;
}

SubpassEndInfoNonOwningBuilder& SubpassEndInfoNonOwningBuilder::withFragmentDensityMapOffsetEndInfo(
    std::span<const VkOffset2D> fragmentDensityOffsets) noexcept {
  _fragmentDensityMapInfo = VkSubpassFragmentDensityMapOffsetEndInfoQCOM{
    .sType = VK_STRUCTURE_TYPE_SUBPASS_FRAGMENT_DENSITY_MAP_OFFSET_END_INFO_QCOM,
    .fragmentDensityOffsetCount = static_cast<uint32_t>(fragmentDensityOffsets.size()),
    .pFragmentDensityOffsets = fragmentDensityOffsets.data()};
  chain(&_subpassEndInfo.pNext, _fragmentDensityMapInfo);
  return *this;
}

const VkSubpassEndInfo& SubpassEndInfoNonOwningBuilder::build() const noexcept {
  return _subpassEndInfo;
}

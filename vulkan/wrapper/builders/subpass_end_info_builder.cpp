#include "vulkan/wrapper/builders/subpass_end_info_builder.h"

#include <initializer_list>

namespace {

template <typename Feature>
void chain(void** pNext, Feature& feature) {
  feature.pNext = *pNext;
  *pNext = &feature;
}

}  // namespace

SubpassEndInfoBuilder& SubpassEndInfoBuilder::withFragmentDensityMapOffsetEndInfo(
    std::initializer_list<VkOffset2D> fragmentDensityOffsets) noexcept {
  bool isChained = _fragmentDensityMapInfo.has_value();
  _fragmentDensityOffsets = lib::Buffer<VkOffset2D>(fragmentDensityOffsets);
  _fragmentDensityMapInfo = VkSubpassFragmentDensityMapOffsetEndInfoQCOM{
    .sType = VK_STRUCTURE_TYPE_SUBPASS_FRAGMENT_DENSITY_MAP_OFFSET_END_INFO_QCOM,
    .fragmentDensityOffsetCount = static_cast<uint32_t>(_fragmentDensityOffsets.size()),
    .pFragmentDensityOffsets = _fragmentDensityOffsets.data()};
  if (!isChained) {
    chain(&_pNext, *_fragmentDensityMapInfo);
  }
  return *this;
}

VkSubpassEndInfo SubpassEndInfoBuilder::build() const noexcept {
  return VkSubpassEndInfo{.sType = VK_STRUCTURE_TYPE_SUBPASS_END_INFO, .pNext = _pNext};
}

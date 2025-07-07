#pragma once

#include "physical_device/physical_device.h"
#include "status/status.h"

#include <vulkan/vulkan.h>

#include <span>
#include <stdexcept>
#include <string>
#include <vector>

class PushConstants {
	std::vector<VkPushConstantRange> _ranges;
	uint32_t cumulatedOffset;

	uint32_t _maxSize;
public:
	PushConstants(const PhysicalDevice& physicalDevice) : cumulatedOffset(0) {
		const auto& limits = physicalDevice.getPropertyManager().getPhysicalDeviceLimits();
		_maxSize = limits.maxPushConstantsSize;
	}
	PushConstants() = default;

	template<typename StructObject>
	Status addPushConstant(VkShaderStageFlags shaderStages);

	std::span<const VkPushConstantRange> getVkPushConstantRange() const {
		return _ranges;
	}

	uint32_t getOffset(uint32_t index) const {
		return _ranges[index].offset;
	}

	uint32_t getSize(uint32_t index) const {
		return _ranges[index].size;
	}

	VkShaderStageFlags getVkShaderStageFlags(uint32_t index) const {
		return _ranges[index].stageFlags;
	}
};


template<typename StructObject>
Status PushConstants::addPushConstant(VkShaderStageFlags shaderStages) {
	static constexpr uint32_t size = sizeof(StructObject);

	if (size + cumulatedOffset > _maxSize) {
		return Error(EngineError::RESOURCE_EXHAUSTED);
	}

	_ranges.emplace_back(shaderStages, cumulatedOffset, size);
	cumulatedOffset += size;

	return StatusOk();
}
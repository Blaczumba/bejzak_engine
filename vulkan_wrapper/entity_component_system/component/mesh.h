#pragma once

#include "vulkan_wrapper/descriptor_set/bindless_descriptor_set_writer.h"
#include "vulkan_wrapper/entity_component_system/entity/entity.h"
#include "vulkan_wrapper/memory_objects/buffer.h"
#include "vulkan_wrapper/primitives/geometry.h"

#include <memory>

class MeshComponent {
	static constexpr ComponentType componentID = 2;

public:
	Buffer vertexBuffer;
	Buffer indexBuffer;
	Buffer vertexBufferPrimitive;
	AABB aabb;
	VkIndexType indexType;

	static constexpr std::enable_if_t<componentID < MAX_COMPONENTS, ComponentType> getComponentID() { return componentID; }
};

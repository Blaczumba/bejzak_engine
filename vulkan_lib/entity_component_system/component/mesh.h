#pragma once

#include "vulkan_lib/descriptor_set/bindless_descriptor_set_writer.h"
#include "vulkan_lib/entity_component_system/entity/entity.h"
#include "vulkan_lib/memory_objects/buffer.h"
#include "vulkan_lib/primitives/geometry.h"

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

#pragma once

#include "common/entity_component_system/entity/entity.h"
#include "common/util/geometry.h"
#include "vulkan_wrapper/memory_objects/buffer.h"

#include <memory>

class Buffer;

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

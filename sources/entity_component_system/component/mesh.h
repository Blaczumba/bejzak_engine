#pragma once

#include "descriptor_set/bindless_descriptor_set_writer.h"
#include "entity_component_system/entity/entity.h"
#include "memory_objects/buffer.h"
#include "primitives/geometry.h"

#include <memory>

class MeshComponent {
	static constexpr ComponentType componentID = 2;

public:
	Buffer vertexBuffer;
	Buffer indexBuffer;
	Buffer vertexBufferPrimitive;
	AABB aabb;
	VkIndexType indexType;
	TextureHandle diffuse;
	TextureHandle normal;
	TextureHandle metallicRoughness;

	static constexpr std::enable_if_t<componentID < MAX_COMPONENTS, ComponentType> getComponentID() { return componentID; }
};

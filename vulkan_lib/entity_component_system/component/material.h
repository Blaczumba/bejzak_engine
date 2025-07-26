#pragma once

#include "vulkan_lib/entity_component_system/entity/entity.h"
#include "vulkan_lib/descriptor_set/bindless_descriptor_set_writer.h"

class MaterialComponent {
	static constexpr ComponentType componentID = 3;
public:
	TextureHandle diffuse;
	TextureHandle normal;
	TextureHandle metallicRoughness;

	static constexpr std::enable_if_t<componentID < MAX_COMPONENTS, ComponentType> getComponentID() { return componentID; }
};

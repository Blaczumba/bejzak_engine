#pragma once

#include "vulkan_lib/entity_component_system/entity/entity.h"

#include <memory>

#include <glm/glm.hpp>

class TransformComponent {
	static constexpr ComponentType componentID = 4;

public:
	glm::mat4 model;

	static constexpr std::enable_if_t<componentID < MAX_COMPONENTS, ComponentType> getComponentID() { return componentID; }
};

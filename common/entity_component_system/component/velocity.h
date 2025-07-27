#pragma once

#include "common/entity_component_system/entity/entity.h"

#include <memory>

#include <glm/glm.hpp>

class VelocityComponent {
	static constexpr ComponentType componentID = 5;

public:
	float dx, dy;

	static constexpr std::enable_if_t < componentID < MAX_COMPONENTS, ComponentType> getComponentID() { return componentID; }
};

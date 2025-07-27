#pragma once

#include "common/entity_component_system/entity/entity.h"

#include <memory>

#include <glm/glm.hpp>

class PositionComponent {
	static constexpr ComponentType componentID = 6;

public:
	float x, y;

	static constexpr std::enable_if_t < componentID < MAX_COMPONENTS, ComponentType> getComponentID() { return componentID; }
};

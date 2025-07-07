#pragma once

#include "entity_component_system/entity/entity.h"

#include <memory>

class MaterialComponent {
	static constexpr ComponentType componentID = 3;
public:

	static constexpr std::enable_if_t<componentID < MAX_COMPONENTS, ComponentType> getComponentID() { return componentID; }
};

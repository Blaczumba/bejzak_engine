#pragma once

#include "entity_component_system/entity/entity.h"

#include <array>
#include <optional>
#include <unordered_map>
#include <set>
#include <vector>

// #include <boost/container/flat_set.hpp>

class ComponentPool {
public:
	virtual void destroyEntity(Entity entity) = 0;
	virtual ~ComponentPool() = default;
};

template<typename Component>
class ComponentPoolImpl : public ComponentPool {
	std::vector<std::pair<Entity, Component>> _components;
	std::array<Entity, MAX_ENTITIES> _entities; // TODO Try unordered flat map

public:
	~ComponentPoolImpl() override = default;

	void addComponent(Entity entity, Component&& component) {
		_entities[entity] = _components.size();
		_components.emplace_back(entity, std::move(component));
	}

	void destroyEntity(Entity entity) override {
		Entity lastEntity = _components.back().first;
		_components[_entities[entity]] = std::move(_components.back());
		_entities[lastEntity] = _entities[entity];
		_components.pop_back();
	}

	Component& getComponent(Entity entity) {
		return _components[_entities[entity]].second;
	}

	std::vector<std::pair<Entity, Component>>& getComponents() {
		return _components;
	}
};

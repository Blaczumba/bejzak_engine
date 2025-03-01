#pragma once

#include "entity_component_system/component/component_pool.h"
#include "entity_component_system/entity/entity_manager.h"

#include <array>
#include <bitset>
#include <functional>
#include <memory>
#include <optional>
#include <tuple>
#include <unordered_map>

class Registry {
	EntityManager entityManager;
	std::array<std::unique_ptr<ComponentPool>, MAX_COMPONENTS> _componentsData;
	std::array<Signature, MAX_ENTITIES> _signatures;

public:
	Entity createEntity() {
		return entityManager.createEntity();
	}

	void destroyEntity(Entity entity) {
		Signature i = 1;
		for (ComponentType j = 0; j < MAX_COMPONENTS; i <<= 2, ++j) {
			if ((_signatures[entity] & i) != 0) {
				_componentsData[j]->destroyEntity(entity);
			}
		}
		_signatures[entity].reset();
		entityManager.destroyEntity(entity);
	}

	template<typename Component>
	void addComponent(Entity entity, Component&& component) {
		if (!_componentsData[Component::getComponentID()]) {
			_componentsData[Component::getComponentID()] = std::make_unique<ComponentPoolImpl<Component>>();
		}
		static_cast<ComponentPoolImpl<Component>*>(_componentsData[Component::getComponentID()].get())->addComponent(entity, std::move(component));
		_signatures[entity].set(Component::getComponentID());
	}

	template<typename Component>
	Component& getComponent(Entity entity) {
		if (!_componentsData[Component::getComponentID()]) {
			_componentsData[Component::getComponentID()] = std::make_unique<ComponentPoolImpl<Component>>();
		}
		_signatures[entity].set(Component::getComponentID());
		return static_cast<ComponentPoolImpl<Component>*>(_componentsData[Component::getComponentID()].get())->getComponent(entity);
	}

	template<typename... Components>
	std::tuple<Components&...> getComponents(Entity entity) {
		return std::tie(static_cast<ComponentPoolImpl<Components>*>(_componentsData[Components::getComponentID()].get())->getComponent(entity)...);
	}

	template<typename... Components>
	void updateComponents(std::function<void(Components&...)> callback) {
		Signature signature = getSignature<Components...>();
		// Ensure the components exist in the registry
		std::tuple<ComponentPoolImpl<Components>&...> pools = {
			*(static_cast<ComponentPoolImpl<Components>*>(_componentsData[Components::getComponentID()].get()))...
		};

		// Iterate over all entities
		for (Entity entity = 0; entity < MAX_ENTITIES; ++entity) { // TODO: get entities from system probably.
			// Check if the entity matches the signature of these components
			if ((_signatures[entity] & signature) == signature) {
				// Get references to the components for the entity
				callback(std::get<ComponentPoolImpl<Components>&>(pools).getComponent(entity)...);
			}
		}
	}

	template<typename... Components>
	void updateComponents(std::function<void(Components&...)> callback, const std::vector<Entity>& entities) {
		const Signature signature = getSignature<Components...>();

		auto components = std::tuple<std::vector<Components>&...>(
			static_cast<ComponentPoolImpl<Components>*>(_componentsData[Components::getComponentID()].get())->getComponents()...
		);
		
		for (Entity entity : entities) {
			if ((_signatures[entity] & signature) == signature) {
				callback(std::get<std::vector<Components>&>(components)[entity]...);
			}
		}
	}
};

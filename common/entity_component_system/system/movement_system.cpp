#include "movement_system.h"

#include "common/entity_component_system/component/position.h"
#include "common/entity_component_system/component/velocity.h"

#include <chrono>
#include <iostream>
#include <tuple>

MovementSystem::MovementSystem(Registry* reg) : registry(reg) {}

void MovementSystem::update(float deltaTime) {
//    registry->updateComponents<PositionComponent, VelocityComponent>(
//        [deltaTime](PositionComponent& pos, VelocityComponent& vel) {
//            pos.x += vel.dx * deltaTime;
//            pos.y += vel.dy * deltaTime;
//
//            //std::cout << "Entity " << entity << " moved to ("
//            //    << pos.x << ", " << pos.y << ")\n";
//        }
//    );
}
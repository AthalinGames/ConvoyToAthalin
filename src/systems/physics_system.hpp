#pragma once

#include "common.hpp"
#include "ecs/tiny_ecs.hpp"
#include "ecs/game_components.hpp"
#include "ecs/tiny_ecs_registry.hpp"

// A simple physics system that moves rigid bodies and checks for collision
class PhysicsSystem
{
public:
    void step(float elapsed_ms_raw, float game_speed);

    static vec2 calculate_enemy_position(Enemy& enemy, Entity enemy_entity, const Map& current_map, float seconds, bool update_enemy);

    PhysicsSystem()
    {
    }
};

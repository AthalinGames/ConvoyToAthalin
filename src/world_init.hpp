#pragma once

#include "common.hpp"
#include "ecs/tiny_ecs.hpp"
#include "systems/render_system.hpp"

// These are hard coded to the dimensions of the entity texture
constexpr float ARCHER_BB_WIDTH = 0.4f * 100.f;
constexpr float ARCHER_BB_HEIGHT = 0.4f * 100.f;
constexpr float SLIME_WIDTH = 0.1f * (float)window_width_px;
constexpr float SLIME_HEIGHT = 0.1f * (float)window_width_px;
constexpr float MAP_WIDTH = (float)window_width_px;
constexpr float MAP_HEIGHT = (float)window_height_px;

// the player
Entity createArcher(RenderSystem* renderer, vec2 pos);
// the enemy unit
Entity createEnemy(RenderSystem* renderer, vec2 pos);
// the combat map
Entity createMap(RenderSystem* renderer, vec2 pos, const std::vector<vec2>& checkpoints);
// a red line for debugging purposes
Entity createLine(vec2 position, vec2 size);

#pragma once

#include "common.hpp"
#include "ecs/tiny_ecs.hpp"
#include "systems/render_system.hpp"

// These are hard coded to the dimensions of the entity texture
const float ARCHER_BB_WIDTH = 0.4f * 300.f;
const float ARCHER_BB_HEIGHT = 0.4f * 300.f;

// the player
Entity createArcher(RenderSystem* renderer, vec2 pos);
// a red line for debugging purposes
Entity createLine(vec2 position, vec2 size);

#pragma once

#include "common.hpp"
#include "ecs/tiny_ecs.hpp"
#include "systems/render_system.hpp"

// These are hard coded to the dimensions of the entity texture
constexpr float ARCHER_BB_WIDTH = 0.1f * static_cast<float>(window_height_px);
constexpr float ARCHER_BB_HEIGHT = 0.1f * static_cast<float>(window_height_px);
constexpr float ARROW_BB_WIDTH = 0.05f * static_cast<float>(window_height_px);
constexpr float ARROW_BB_HEIGHT = 0.05f * static_cast<float>(window_height_px);
constexpr float CARD_HEIGHT = 0.2f * static_cast<float>(window_height_px);
constexpr float CARD_WIDTH = 0.2f * static_cast<float>(window_height_px);
constexpr float CARD_AXIS_HEIGHT = window_height_px * 0.9; // axis on which cards are placed
constexpr float CARD_AXIS_WIDTH = window_width_px;
constexpr float MAP_WIDTH = static_cast<float>(window_width_px);
constexpr float MAP_HEIGHT = static_cast<float>(window_height_px);
constexpr float SLIME_WIDTH = 0.15f * static_cast<float>(window_height_px);
constexpr float SLIME_HEIGHT = 0.15f * static_cast<float>(window_height_px);


// the player
Entity createArcher(RenderSystem* renderer, vec2 pos);
// the arrows
Entity createArrow(RenderSystem* renderer, vec2 pos, float velocity, vec2 dir);
// the enemy unit
Entity createEnemy(RenderSystem* renderer, vec2 pos);
// the cards
Entity createCard(RenderSystem* renderer);
// the combat map
Entity createMap(RenderSystem* renderer, vec2 pos, const std::vector<vec2>& checkpoints);
// a red line for debugging purposes
Entity createLine(vec2 position, vec2 size);

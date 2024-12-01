#pragma once

#include "common.hpp"
#include "ecs/tiny_ecs.hpp"
#include "systems/render_system.hpp"

// These are hard coded to the dimensions of the entity texture
constexpr float ARCHER_BB_WIDTH = 0.1f * window_height_px;
constexpr float ARCHER_BB_HEIGHT = 0.1f * window_height_px;
constexpr float ARROW_BB_WIDTH = 0.05f * window_height_px;
constexpr float ARROW_BB_HEIGHT = 0.05f * window_height_px;
constexpr float CARD_HEIGHT = 0.2f * window_height_px;
constexpr float CARD_WIDTH = 0.2f * window_height_px;
constexpr float BACKGROUND_WIDTH = window_width_px;
constexpr float BACKGROUND_HEIGHT = window_height_px;
constexpr float SLIME_WIDTH = 0.15f * window_height_px;
constexpr float SLIME_HEIGHT = 0.15f * window_height_px;
constexpr float LINE_WIDTH = 0.005f * window_height_px;
constexpr float FIGHT_LOCATION_WIDTH = 0.05f * window_height_px;
constexpr float FIGHT_LOCATION_HEIGHT = 0.05f * window_height_px;
constexpr float OVERVIEW_ICON_WIDTH = 0.1f * window_height_px;
constexpr float OVERVIEW_ICON_HEIGHT = 0.1f * window_height_px;
constexpr float START_ICON_LOC_X = 0.15f * window_width_px;
constexpr float START_ICON_LOC_Y = 0.8f * window_height_px;
constexpr float GOAL_ICON_LOC_X = 0.85f * window_width_px;
constexpr float GOAL_ICON_LOC_Y = 0.15f * window_height_px;
constexpr float CARD_AXIS_HEIGHT = window_height_px * 0.9; // axis on which cards are placed
constexpr float CARD_AXIS_WIDTH = window_width_px;
constexpr float MAP_WIDTH = static_cast<float>(window_width_px);
constexpr float MAP_HEIGHT = static_cast<float>(window_height_px);

// These are hard coded for each render layer
constexpr float Z_BACKGROUND = 1;
constexpr float Z_MIDDLE = 0;
constexpr float Z_FOREGROUND = -1;

// the player
Entity createPlayer();
// archer tower
Entity createArcher(RenderSystem* renderer, vec2 pos);
// the arrows
Entity createArrow(RenderSystem* renderer, vec2 pos, float velocity, vec2 dir);
// the enemy unit
Entity createEnemy(RenderSystem* renderer, vec2 pos);
// the cards
void realignCards();
Entity createCard(RenderSystem* renderer);
// the combat map
Entity createMap(RenderSystem* renderer, const std::vector<vec2>& checkpoints);
// the overview map
Entity createOverviewMap(RenderSystem* renderer);
// location on the overview map
Entity createFightLocation(RenderSystem* renderer, vec2 pos);
// line between locations on the overview map
Entity createOverviewLine(RenderSystem* renderer, vec2 firstPos, vec2 secondPos);
// a red line for debugging purposes
Entity createLine(vec2 position, vec2 size);

Entity createGoalIcon(RenderSystem* renderer);

Entity createStartIcon(RenderSystem* renderer);

Entity createOverviewSelection(RenderSystem* renderer, vec2 pos);

Entity createText(RenderSystem* renderer, vec2 pos, float scale, const std::string &&text);

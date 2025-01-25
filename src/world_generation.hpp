#pragma once

#include "common.hpp"
#include "world_init.hpp"

// Overview params
constexpr uint8_t grid_width = 5;
constexpr uint8_t grid_height = 8;
constexpr uint8_t path_count = 4;

constexpr std::array overview_locations{
    vec2{0.4f * window_width_px, START_ICON_LOC_Y},
    vec2{GOAL_ICON_LOC_X, 0.4f * window_height_px},
    vec2{START_ICON_LOC_X, 0.55f * window_height_px},
    vec2{0.65f * window_width_px, GOAL_ICON_LOC_Y}
};

typedef std::array<uint, grid_height> overview_path;
typedef std::array<overview_path, path_count> overview_paths;

typedef std::array<std::array<bool, grid_width>, grid_height> overview_grid_visited;

typedef std::array<std::array<Entity, grid_width>, grid_height> overview_grid_entities;

std::pair<overview_paths, overview_grid_visited> generate_overview_paths(std::default_random_engine &rng);
overview_grid_entities create_fight_locations(RenderSystem *renderer, std::default_random_engine &rng, const overview_grid_visited &visited);
std::pair<Entity, Entity> build_overview_graph(RenderSystem *renderer, const overview_paths &paths, const overview_grid_entities &locations);

std::vector<vec2> generateMapCheckpoints(std::default_random_engine rng, uint path_length);


#include "world_generation.hpp"
#include <array>

#include "ecs/tiny_ecs_registry.hpp"

constexpr bool is_left_ok(const overview_grid_visited &visited, const uint next_height, const uint current_pos) {
    if (current_pos < 1) {
        return false;
    }
    const bool is_straight_visited = visited[next_height][current_pos];
    const bool is_left_visited = visited[next_height-1][current_pos-1];
    return !(is_straight_visited && is_left_visited);
}

constexpr bool is_right_ok(const overview_grid_visited &visited, const uint next_height, const uint current_pos) {
    if (current_pos >= grid_width - 1) {
        return false;
    }
    const bool is_straight_visited = visited[next_height][current_pos];
    const bool is_right_visited = visited[next_height-1][current_pos+1];
    return !(is_straight_visited && is_right_visited);
}

std::pair<overview_paths, overview_grid_visited> generate_overview_paths(std::default_random_engine &rng) {
    auto overview_path_start_dist = std::uniform_int_distribution<uint>(0, grid_width - 1);
    enum class Direction {
        Left = -1,
        Straight = 0,
        Right = 1,
    };

    overview_grid_visited visited{};
    overview_paths overview_paths{};

    for (uint path_no = 0; path_no < overview_paths.size(); ++path_no) {
        uint next_pos = overview_path_start_dist(rng);
        overview_paths[path_no][0] = next_pos;
        visited[0][next_pos] = true;

        for (uint height = 1; height < grid_height; ++height) {
            std::vector<Direction> possible_directions{};
            possible_directions.push_back(Direction::Straight);
            if (is_left_ok(visited, height, next_pos)) {
                possible_directions.push_back(Direction::Left);
            }
            if (is_right_ok(visited, height, next_pos)) {
                possible_directions.push_back(Direction::Right);
            }
            std::ranges::shuffle(possible_directions, rng);
            const auto selected_direction = possible_directions.at(0);
            next_pos += static_cast<int>(selected_direction);
            assert(next_pos < grid_width);
            overview_paths[path_no][height] = next_pos;
            visited[height][next_pos] = true;
        }
    }
    return {overview_paths, visited};
}

overview_grid_entities create_fight_locations(RenderSystem *renderer, std::default_random_engine &rng, const overview_grid_visited &visited) {
    std::uniform_real_distribution<float> dist(0, 1);
    overview_grid_entities locations{};

    for (uint height = 0; height < grid_height; ++height) {
        for (uint width = 0; width < grid_width; ++width) {
            if (!visited[height][width]) {
                continue;
            }
            constexpr float grid_offset_y = (1.0f / grid_width) / 2;
            constexpr float grid_offset_x = (1.0f / grid_height) / 2;
            const auto y_percentage = static_cast<float>(width) / grid_width + grid_offset_y;
            const auto x_percentage = static_cast<float>(height) / grid_height + grid_offset_x;
            const auto lerp_x_1 = overview_locations[1] * x_percentage + overview_locations[0] * (1 - x_percentage);
            const auto lerp_x_2 = overview_locations[3] * x_percentage + overview_locations[2] * (1 - x_percentage);
            auto location_pos = lerp_x_1 * y_percentage + lerp_x_2 * (1 - y_percentage);
            location_pos += (vec2(dist(rng), dist(rng)) - 0.5f) * 30.f;
            locations[height][width] = createFightLocation(renderer, location_pos);
        }
    }

    return locations;
}

Entity build_overview_graph(RenderSystem *renderer, const overview_paths &paths, const overview_grid_entities &locations) {
    const Entity starting_location = createStartIcon(renderer);
    const Entity goal_location = createGoalIcon(renderer);
    OverviewMapLocation& start_loc_props = registry.overviewMapLocations.get(starting_location);
    OverviewMapLocation& end_loc_props = registry.overviewMapLocations.get(goal_location);

    for (uint path_no = 0; path_no < path_count; ++path_no) {
        uint width_location = paths[path_no][0];
        Entity last_loc_entity = locations[0][width_location];
        OverviewMapLocation& last_loc_props = registry.overviewMapLocations.get(last_loc_entity);
        // Make starting positions selectable
        last_loc_props.selectable = true;
        // Setup Graph for Starting Location
        last_loc_props.previous_locations.push_back(starting_location);
        start_loc_props.next_locations.push_back(last_loc_entity);
        vec2 last_loc_pos = registry.stationaries.get(last_loc_entity).position;
        createOverviewLine(renderer, vec2{START_ICON_LOC_X, START_ICON_LOC_Y}, last_loc_pos);
        // Setup Graph for connections between fights
        for (uint height = 0; height < grid_height; ++height) {
            // retrieve data for new location
            width_location = paths[path_no][height];
            Entity current_loc_entity = locations[height][width_location];
            OverviewMapLocation& current_loc_props = registry.overviewMapLocations.get(current_loc_entity);
            const vec2 current_loc_pos = registry.stationaries.get(current_loc_entity).position;
            // Setup Graph
            current_loc_props.previous_locations.push_back(last_loc_entity);
            last_loc_props.next_locations.push_back(current_loc_entity);
            createOverviewLine(renderer, last_loc_pos, current_loc_pos);
            // Setup for next iteration
            last_loc_pos = current_loc_pos;
            last_loc_entity = current_loc_entity;
        }
        // Setup Graph for Goal Location
        end_loc_props.next_locations.push_back(last_loc_entity);
        last_loc_props.previous_locations.push_back(goal_location);
        createOverviewLine(renderer, last_loc_pos, vec2{GOAL_ICON_LOC_X, GOAL_ICON_LOC_Y});
    }

    return starting_location;
}

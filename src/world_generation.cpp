
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

vec2 circle_section_lerp(const vec2 origin, const float radius, const float start_angle, const float end_angle, const float percentage) {
	const float angle = start_angle * percentage + end_angle * (1 - percentage);
	const vec2 circle_pos = {radius * cos(angle), radius * sin(angle)};
	return circle_pos + origin;
}

overview_grid_entities create_fight_locations(RenderSystem *renderer, std::default_random_engine &rng, const overview_grid_visited &visited) {
    std::uniform_real_distribution<float> dist(0, 1);
    overview_grid_entities locations{};

    for (uint height = 0; height < grid_height; ++height) {
        for (uint width = 0; width < grid_width; ++width) {
            if (!visited[height][width]) {
                continue;
            }
            constexpr float grid_offset_width = (1.0f / grid_width) / 2;
            constexpr float grid_offset_height = (1.0f / grid_height) / 2;
            const auto width_percentage = static_cast<float>(width) / grid_width + grid_offset_width;
            const auto height_percentage = static_cast<float>(height) / grid_height + grid_offset_height;
        	constexpr vec2 start_pos = {START_ICON_LOC_X, START_ICON_LOC_Y};
        	constexpr vec2 goal_pos = {GOAL_ICON_LOC_X, GOAL_ICON_LOC_Y};
        	const vec2 selected_pos = height_percentage < 0.5f ? start_pos : goal_pos;
        	const float dist_percentage = height_percentage < 0.5f ? height_percentage : 1 - height_percentage;
        	const float start_angle = height_percentage < 0.5f ? -M_PI_2 * (1 - dist_percentage) : M_PI;
        	const float end_angle = height_percentage < 0.5f ? 0 : M_PI_2 * (1 + dist_percentage);
        	const float radius = length(goal_pos - start_pos) * dist_percentage;
        	auto location_pos = circle_section_lerp(selected_pos, radius + 0.05 * window_height_px, start_angle, end_angle, width_percentage);
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
        OverviewMapLocation& first_loc_props = registry.overviewMapLocations.get(last_loc_entity);
        // Make starting positions selectable
        first_loc_props.selectable = true;
        // Setup Graph for Starting Location
        first_loc_props.previous_locations.push_back(starting_location);
        start_loc_props.next_locations.push_back(last_loc_entity);
        vec2 last_loc_pos = registry.stationaries.get(last_loc_entity).position;
        createOverviewLine(renderer, vec2{START_ICON_LOC_X, START_ICON_LOC_Y}, last_loc_pos);
        // Setup Graph for connections between fights
        for (uint height = 0; height < grid_height; ++height) {
            // retrieve data for new location
            width_location = paths[path_no][height];
            Entity current_loc_entity = locations[height][width_location];
            OverviewMapLocation& current_loc_props = registry.overviewMapLocations.get(current_loc_entity);
        	OverviewMapLocation& last_loc_props = registry.overviewMapLocations.get(last_loc_entity);
            const vec2 current_loc_pos = registry.stationaries.get(current_loc_entity).position;
            // Setup Graph
            current_loc_props.previous_locations.push_back(last_loc_entity);
            last_loc_props.next_locations.push_back(current_loc_entity);
            createOverviewLine(renderer, last_loc_pos, current_loc_pos);
        	if (height == grid_height - 1) {
        		current_loc_props.next_locations.push_back(goal_location);
        	}
            // Setup for next iteration
            last_loc_pos = current_loc_pos;
            last_loc_entity = current_loc_entity;
        }
        // Setup Graph for Goal Location
        end_loc_props.previous_locations.push_back(last_loc_entity);
        createOverviewLine(renderer, last_loc_pos, vec2{GOAL_ICON_LOC_X, GOAL_ICON_LOC_Y});
    }

    return starting_location;
}

int mod(int k, const int n) {
	return ((k %= n) < 0) ? k + n : k;
}

std::vector<vec2> generateMapCheckpoints(std::default_random_engine rng, const uint path_length) {
	enum class DirectionChange {
		LEFT = 0, RIGHT, STRAIGHT
	};
	enum class Direction {
		UP = 0, RIGHT, DOWN, LEFT
	};
	std::discrete_distribution<int> section_length_dist({0, 0, 5, 5, 1, 1});
	std::uniform_int_distribution<uint> x_coord_dist(1, MAP_COUNT_X - 3);
	std::uniform_int_distribution<uint> y_coord_dist(2, MAP_COUNT_Y - 4); // Keep bottom space empty for cards
	std::uniform_int_distribution<uint> direction_dist(0, 2);

	std::vector<vec2> map_checkpoints{};
	uint current_length = 0;

	// first checkpoint
	auto current_dir_change = static_cast<DirectionChange>(direction_dist(rng));
	Direction current_dir;
	vec2 current_checkpoint;
	switch (current_dir_change) {
		case DirectionChange::LEFT: {
			current_checkpoint = {0, y_coord_dist(rng)};
			current_dir = Direction::RIGHT;
			current_dir_change = DirectionChange::STRAIGHT;
			break;
		}
		case DirectionChange::RIGHT: {
			current_checkpoint = {MAP_COUNT_X - 1, y_coord_dist(rng)};
			current_dir = Direction::LEFT;
			current_dir_change = DirectionChange::STRAIGHT;
			break;
		}
		case DirectionChange::STRAIGHT: {
			current_checkpoint = {x_coord_dist(rng), 0};
			current_dir = Direction::DOWN;
			break;
		}
	}
	map_checkpoints.push_back(current_checkpoint);
	uint iteration_count = 0;
	bool restart = false;
	while (current_length < path_length) {
		++iteration_count;
		if (iteration_count > 1000) {
			restart = true;
			break;
		}
		const int section_length = section_length_dist(rng);
		vec2 new_checkpoint;

		Direction new_dir;
		switch (current_dir_change) {
			case DirectionChange::LEFT: {
				int id = static_cast<int>(current_dir);
				int new_id = mod(id - 1, 4);
				new_dir = static_cast<Direction>(new_id);
				break;
			}
			case DirectionChange::RIGHT: {
				int id = static_cast<int>(current_dir);
				int new_id = mod(id + 1, 4);
				new_dir = static_cast<Direction>(new_id);
				break;
			}
			case DirectionChange::STRAIGHT: {
				new_dir = current_dir;
				break;
			}
		}

		switch (new_dir) {
			case Direction::LEFT: {
				new_checkpoint = current_checkpoint + vec2{-section_length, 0};
				break;
			}
			case Direction::RIGHT: {
				new_checkpoint = current_checkpoint + vec2{section_length, 0};
				break;
			}
			case Direction::DOWN: {
				new_checkpoint = current_checkpoint + vec2{0, section_length};
				break;
			}
			case Direction::UP: {
				new_checkpoint = current_checkpoint + vec2{0, -section_length};
				break;
			}
		}
		current_dir_change = static_cast<DirectionChange>(direction_dist(rng));

		// check if new point is valid
		// check if point is within bounds
		if (new_checkpoint.x < 1 || new_checkpoint.x > MAP_COUNT_X - 3) {
			// checkpoint is too far right or left
			continue; // this just retries until it finds a fitting solution
		}
		if (new_checkpoint.y < 1 || new_checkpoint.y > MAP_COUNT_Y - 4) {
			// checkpoint is too high or too low
			continue;
		}
		// check if point is either on the existing line or has a one tile gap
		vec2 last_checkpoint = map_checkpoints.front();
		bool regenerate = false;
		for (uint i = 1; i < map_checkpoints.size(); ++i) {
			const vec2 next_checkpoint = map_checkpoints.at(i);
			const float manhattan_dist_section = abs(next_checkpoint.x - last_checkpoint.x) + abs(
				                                     next_checkpoint.y - last_checkpoint.y);
			const float manhattan_dist_last = abs(last_checkpoint.x - new_checkpoint.x) + abs(
				                                  last_checkpoint.y - new_checkpoint.y);
			const float manhattan_dist_next = abs(next_checkpoint.x - new_checkpoint.x) + abs(
				                                  next_checkpoint.y - new_checkpoint.y);

			if (manhattan_dist_last > manhattan_dist_section || manhattan_dist_next > manhattan_dist_section) {
				printf("Checking checkpoints\n");
				const float small_dist = min(manhattan_dist_last, manhattan_dist_next);
				if (small_dist < 2) {
					printf("Checkpoint too close to another checkpoint %f,\t%f\t%d\n", new_checkpoint.x,
					       new_checkpoint.y, section_length);
					regenerate = true;
					break; // the section hit too close to a checkpoint
				}
			} else {
				printf("Checking sections\n");
				if (next_checkpoint.x - last_checkpoint.x == 0) {
					// vertical section
					if (abs(last_checkpoint.x - new_checkpoint.x) < 2) {
						printf("Checkpoint too close to vertical section %f,\t%f\t%d\n", new_checkpoint.x,
						       new_checkpoint.y, section_length);
						regenerate = true;
						break;
					}
				} else if (next_checkpoint.y - last_checkpoint.y == 0) {
					// horizontal section
					if (abs(last_checkpoint.y - new_checkpoint.y) < 2) {
						printf("Checkpoint too close to horizontal section %f,\t%f\t%d\n", new_checkpoint.x,
						       new_checkpoint.y, section_length);
						regenerate = true;
						break;
					}
				}
			}

			printf("(%f, %f) -> (%f, %f) (%f, %f)\n", new_checkpoint.x, new_checkpoint.y, last_checkpoint.x,
			       last_checkpoint.y, next_checkpoint.x, next_checkpoint.y);
			printf("         | CP Dist: %f\n", manhattan_dist_section);
			printf("         | LS Dist: %f\n", manhattan_dist_last);
			printf("         | LN Dist: %f\n", manhattan_dist_next);
			printf("         | VT Dist: %f\n", abs(last_checkpoint.x - new_checkpoint.x));
			printf("         | HZ DIST: %f\n", abs(last_checkpoint.y - new_checkpoint.y));

			last_checkpoint = next_checkpoint;
		}
		if (regenerate) {
			continue;
		}

		current_length += section_length;
		map_checkpoints.push_back(new_checkpoint);
		current_checkpoint = new_checkpoint;
		current_dir = new_dir;
	}
	if (restart) {
		// the current generation was probably impossible, thus we restart the generation
		return generateMapCheckpoints(rng, path_length);
	}
	// check that no corner of the final checkpoint is hit
	for (uint i = 0; i < map_checkpoints.size() - 1; ++i) {
		float dist = length(map_checkpoints.at(i) - map_checkpoints.back());
		if (dist < 2) {
			restart = true;
			break;
		}
	}
	if (restart) {
		// regenerate path to fix issue
		return generateMapCheckpoints(rng, path_length);
	}

	return map_checkpoints;
}

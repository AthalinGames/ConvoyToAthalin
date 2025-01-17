// Header
#include "world_system.hpp"
#include "world_init.hpp"

// stlib
#include <cassert>
#include <imgui.h>
#include <sstream>

#include "physics_system.hpp"
#include "ecs/game_components.hpp"

// Game configuration
/* TODO: Replace with our game config
const size_t MAX_TURTLES = 15;
const size_t MAX_FISH = 5;
const size_t TURTLE_DELAY_MS = 2000 * 3;
const size_t FISH_DELAY_MS = 5000 * 3;
*/

// Create the fish world
WorldSystem::WorldSystem()
	: points(0) {
	// Seeding rng with random device
	rng = std::default_random_engine(std::random_device()());
	current_td_system.reset(new TDSystem());
}

WorldSystem::~WorldSystem() {
	// Destroy music components
	// TODO: Replace with our sounds
	if (background_music != nullptr)
		Mix_FreeMusic(background_music);
	if (salmon_dead_sound != nullptr)
		Mix_FreeChunk(salmon_dead_sound);
	if (salmon_eat_sound != nullptr)
		Mix_FreeChunk(salmon_eat_sound);
	Mix_CloseAudio();
	//*/

	// cleanup entities before registry is cleared
	current_td_system.reset();

	// Destroy all created components
	registry.clear_all_components();

	// Close the window
	glfwDestroyWindow(window);
}

// Debugging
namespace {
	void glfw_err_cb(int error, const char *desc) {
		fprintf(stderr, "%d: %s", error, desc);
	}
}

// World initialization
// Note, this has a lot of OpenGL specific things, could be moved to the renderer
GLFWwindow* WorldSystem::create_window(const bool windowed) {
	///////////////////////////////////////
	// Initialize GLFW
	glfwSetErrorCallback(glfw_err_cb);
	if (!glfwInit()) {
		fprintf(stderr, "Failed to initialize GLFW");
		return nullptr;
	}

	//-------------------------------------------------------------------------
	// If you are on Linux or Windows, you can change these 2 numbers to 4 and 3 and
	// enable the glDebugMessageCallback to have OpenGL catch your mistakes for you.
	// GLFW / OGL Initialization
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#if __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	glfwWindowHint(GLFW_RESIZABLE, 0);

	// Create the main window (for rendering, keyboard, and mouse input)
	GLFWmonitor* monitor;
	if (windowed) {
		monitor = nullptr;
	} else {
		monitor = glfwGetPrimaryMonitor();
	}
	window = glfwCreateWindow(window_width_px, window_height_px, "Convoy to Athalin", monitor, nullptr);
	if (window == nullptr) {
		fprintf(stderr, "Failed to glfwCreateWindow");
		return nullptr;
	}

	// Setting callbacks to member functions (that's why the redirect is needed)
	// Input is handled using GLFW, for more info see
	// http://www.glfw.org/docs/latest/input_guide.html
	glfwSetWindowUserPointer(window, this);
	auto key_redirect = [](GLFWwindow *wnd, int _0, int _1, int _2, int _3) {
		static_cast<WorldSystem *>(glfwGetWindowUserPointer(wnd))->on_key(_0, _1, _2, _3);
	};
	auto cursor_pos_redirect = [](GLFWwindow *wnd, double _0, double _1) {
		static_cast<WorldSystem *>(glfwGetWindowUserPointer(wnd))->on_mouse_move({_0, _1});
	};
	auto mouse_key_redirect = [](GLFWwindow *wnd, int key, int action, int mods) {
		static_cast<WorldSystem *>(glfwGetWindowUserPointer(wnd))->on_mouse_button(key, action, mods);
	};
	glfwSetKeyCallback(window, key_redirect);
	glfwSetCursorPosCallback(window, cursor_pos_redirect);
	glfwSetMouseButtonCallback(window, mouse_key_redirect);

	//////////////////////////////////////
	// Loading music and sounds with SDL
	if (SDL_Init(SDL_INIT_AUDIO) < 0) {
		fprintf(stderr, "Failed to initialize SDL Audio");
		return nullptr;
	}
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1) {
		fprintf(stderr, "Failed to open audio device");
		return nullptr;
	}

	// TODO: Replace with our sounds
	background_music = Mix_LoadMUS(audio_path("music.wav").c_str());
	salmon_dead_sound = Mix_LoadWAV(audio_path("salmon_dead.wav").c_str());
	salmon_eat_sound = Mix_LoadWAV(audio_path("salmon_eat.wav").c_str());

	if (background_music == nullptr || salmon_dead_sound == nullptr || salmon_eat_sound == nullptr) {
		fprintf(stderr, "Failed to load sounds\n %s\n %s\n %s\n make sure the data directory is present",
			audio_path("music.wav").c_str(),
			audio_path("salmon_dead.wav").c_str(),
			audio_path("salmon_eat.wav").c_str());
		return nullptr;
	}
	//*/

	return window;
}

void WorldSystem::init(RenderSystem* renderer) {
	this->renderer = renderer;
	// Playing background music indefinitely
	Mix_PlayMusic(background_music, -1); // TODO: Replace with our bgm
	fprintf(stderr, "Loaded music\n");

	// Set all states to default
    restart_game();
}

// Update our game world
bool WorldSystem::step(const float elapsed_ms) {
	assert(registry.screenStates.components.size() <= 1);
	ScreenState &screen = registry.screenStates.components[0];

	float min_timer_ms = 4000.f;
	for (const Entity entity: registry.deathTimers.entities) {
		// progress timer
		DeathTimer &timer = registry.deathTimers.get(entity);
		timer.timer_ms -= elapsed_ms;
		if (timer.timer_ms < min_timer_ms) {
			min_timer_ms = timer.timer_ms;
		}

		// restart the game once the death timer expired
		if (timer.timer_ms < 0) {
			registry.deathTimers.remove(entity);
			screen.screen_darken_factor = 0;
			restart_game();
			return true;
		}
	}
	// reduce window brightness if any of the present salmons is dying
	screen.screen_darken_factor = 1 - min_timer_ms / 4000;
	// Updating window title with points
	std::stringstream title_ss;
	title_ss << "Points: " << points;
	glfwSetWindowTitle(window, title_ss.str().c_str());

	// Remove debug info from the last step
	while (!registry.debugComponents.entities.empty())
	    registry.remove_all_components_of(registry.debugComponents.entities.back());

	// Removing out of screen entities
	auto& motion_container = registry.motions;

	// Remove entities that leave the screen on the left side
	// Iterate backwards to be able to remove without unterfering with the next object to visit
	// (the containers exchange the last element with the current)
	for (int i = static_cast<int>(motion_container.components.size())-1; i>=0; --i) {
	    Motion& motion = motion_container.components[i];
		if (motion.position.x + abs(motion.scale.x) < 0.f) {
			if(!registry.players.has(motion_container.entities[i])) // don't remove the player
				registry.remove_all_components_of(motion_container.entities[i]);
		}
	}

	// Render Health and Food display
	ImGui::Begin("Health");
	ImGui::SetWindowPos({window_width_px * 0.1, window_height_px * 0.03});
	ImGui::SetWindowSize({window_width_px * 0.06, 65});
	for (const auto & player : registry.players.components) {
		ImGui::Text("HP: %d\nFood: %d", player.health, player.food);
	}
	ImGui::End();

	// If td system is running, run also its step
	if (!current_td_system->is_over()) {
		current_td_system->step(elapsed_ms);
	} else if (td_fight_launched) {
		// Check if Player is already dead
		if (registry.deathTimers.size() > 0) {
			return true;
		}

		// TD Fight should be finished
		current_td_system.reset(new TDSystem());
		// check if Player is dead
		for (const auto & player : registry.players.components) {
			if (player.health <= 0) {
				const Entity gameOver = createGameOver(renderer);
				registry.deathTimers.emplace(gameOver);
				return true;
			}
		}

        Player& current_player = registry.players.get(player);
        current_player.won_battles++;
		// Setup Overview-Map for next selection
		auto &current_map_pos_props = registry.overviewMapLocations.get(current_map_pos);
		current_map_pos_props.active = false;
		for (const Entity next_location : current_map_pos_props.next_locations) {
			auto &location_props = registry.overviewMapLocations.get(next_location);
			location_props.active = false;
			location_props.selectable = false;
		}
		auto &next_map_pos_props = registry.overviewMapLocations.get(next_map_pos);
		for (const Entity next_location : next_map_pos_props.next_locations) {
			auto &location_props = registry.overviewMapLocations.get(next_location);
			location_props.selectable = true;
		}
		current_map_pos = next_map_pos;

		registry.invisibles.remove(tutorial_hint);

		// Finally get back to normal steps
		td_fight_launched = false;
	}

	return true;
}

// Reset the world state to its initial state
void WorldSystem::restart_game() {
	// Debugging for memory/component leaks
	registry.list_all_components();
	printf("Restarting\n");

	// Reset the game speed
	current_speed = 1.f;

	// Remove all entities that we created
	// All that have a motion, we could also iterate over all fish, turtles, ... but that would be more cumbersome
	while (!registry.motions.entities.empty()) {
		registry.remove_all_components_of(registry.motions.entities.back());
	}

    //Remove all stationaries
    while (!registry.stationaries.entities.empty()) {
	    registry.remove_all_components_of(registry.stationaries.entities.back());
    }

	//Remove player state
	while (!registry.players.entities.empty()) {
		registry.remove_all_components_of(registry.players.entities.back());
	}

	//Remove all items
	while (!registry.items.entities.empty()) {
		printf("registry.items.entities.size(): %lu\n", registry.items.entities.size());
		registry.remove_all_components_of(registry.items.entities.back());
	}

	// Debugging for memory/component leaks
	registry.list_all_components();

	ScreenState &screen = registry.screenStates.components[0];
	screen.screen_darken_factor = 0;

	td_fight_launched = false;
	// Setup initial Player Hand (Start with one archer)
    player = createPlayer(renderer);
	createItem(TowerType::ARCHER);
    createItem(TowerType::KNIGHT);
    createItem(ConsumableType::BOMB);

    registry.players.get(player).placement_marker = createPlacementMarker(renderer);

	current_td_system.reset(new TDSystem());
	overview_map = createOverviewMap(renderer);
	// Generate possible paths
	// TODO fix path crossing
	std::array<std::array<bool, grid_width>, grid_height> visited{};
	std::array<std::array<uint8_t, grid_height>, path_count> paths{};
	for (int path_no = 0; path_no < path_count; ++path_no) {
		auto next_pos = static_cast<uint8_t>(overview_path_start_dist(rng));
		paths[path_no][0] = next_pos;
		visited[0][next_pos] = true;
		for (int height = 1; height < grid_height; ++height) {
			std::vector<uint8_t> next_positions{};
			next_positions.push_back(next_pos);
			if (next_pos > 0 && !(visited[height - 1][next_pos - 1] && visited[height][next_pos])) {
				next_positions.push_back(next_pos - 1);
			} else if (next_pos < grid_width - 1 && !(visited[height - 1][next_pos + 1] && visited[height][next_pos])) {
				next_positions.push_back(next_pos + 1);
			}
			std::shuffle(next_positions.begin(), next_positions.end(), rng);
			const auto width_location = next_positions.back();
			paths[path_no][height] = width_location;
			visited[height][width_location] = true;
		}
	}

	// Render locations
	std::array<std::array<vec2, grid_width>, grid_height> location_position;
	std::array<std::array<Entity, grid_width>, grid_height> location_entities;
	for (int height = 0; height < grid_height; ++height) {
		for (int width = 0; width < grid_width; ++width) {
			if (visited[height][width]) {
				constexpr float grid_offset_y = (1.0f / grid_width) / 2;
				constexpr float grid_offset_x = (1.0f / grid_height) / 2;
				const auto y_percentage = static_cast<float>(width) / grid_width + grid_offset_y;
				const auto x_percentage = static_cast<float>(height) / grid_height + grid_offset_x;
				const auto lerp_x_1 = overview_locations[1] * x_percentage + overview_locations[0] * (1 - x_percentage);
				const auto lerp_x_2 = overview_locations[3] * x_percentage + overview_locations[2] * (1 - x_percentage);
				auto location_pos = lerp_x_1 * y_percentage + lerp_x_2 * (1 - y_percentage);
				location_pos.x += (uniform_dist(rng) - 0.5f) * 30;
				location_pos.y += (uniform_dist(rng) - 0.5f) * 30;
				location_position[height][width] = location_pos;
				const auto location_entity = createFightLocation(renderer, location_pos);
				location_entities[height][width] = location_entity;
			}
		}
	}
	// Add Start and Goal
	current_map_pos = createStartIcon(renderer);
	auto &start_loc_props = registry.overviewMapLocations.get(current_map_pos);
	auto &end_loc_props = registry.overviewMapLocations.get(createGoalIcon(renderer));

	// Render path connections and connect internal graph
	for (int path_no = 0; path_no < path_count; ++path_no) {
		auto width_location = paths[path_no][0];
		Entity last_loc_entity = location_entities[0][width_location];
		auto &loc_props = registry.overviewMapLocations.get(last_loc_entity);
		// Make starting positions selectable
		loc_props.selectable = true;
		loc_props.previous_locations.push_back(current_map_pos);
		start_loc_props.next_locations.push_back(last_loc_entity);
		auto last_pos = location_position[0][width_location];
		createOverviewLine(renderer, vec2{START_ICON_LOC_X, START_ICON_LOC_Y}, last_pos);
		for (int height = 1; height < grid_height; ++height) {
			width_location = paths[path_no][height];
			Entity current_loc_entity = location_entities[height][width_location];
			auto &curr_loc_props = registry.overviewMapLocations.get(current_loc_entity);
			auto &last_loc_props = registry.overviewMapLocations.get(last_loc_entity);
			// Setup location linking
			curr_loc_props.previous_locations.push_back(last_loc_entity);
			last_loc_props.next_locations.push_back(current_loc_entity);
			// Render connection between locations
			const auto current_pos = location_position[height][width_location];
			createOverviewLine(renderer, last_pos, current_pos);
			// Fetch next
			last_pos = current_pos;
			last_loc_entity = current_loc_entity;
		}
		end_loc_props.previous_locations.push_back(last_loc_entity);
		createOverviewLine(renderer, last_pos, vec2{GOAL_ICON_LOC_X, GOAL_ICON_LOC_Y});
	}

	tutorial_hint = createText(renderer, {5, window_height_px - 5}, {10, 10}, "Hold 'T' to show the tutorial", FontType::SQUARE);
}

// Compute collisions between entities
void WorldSystem::handle_collisions() {
	// Loop over all collisions detected by the physics system
	const auto& collisionsRegistry = registry.collisions;
	for (uint i = 0; i < collisionsRegistry.components.size(); i++) {
		// The entity and its collider
		const Entity entity = collisionsRegistry.entities[i];
		const Entity entity_other = collisionsRegistry.components[i].other_entity;

		// If td fight is running handle its collisions
		if (!current_td_system->is_over()) {
			current_td_system->handle_collision(entity, entity_other);
		}
	}

	// Remove all collisions from this simulation step
	registry.collisions.clear();
}

void WorldSystem::handle_post_collision_actions() {
	// If td fight is running handle its post collision actions
	if (!current_td_system->is_over()) {
		current_td_system->handle_aiming();
	}
}

// Should the game be over ?
bool WorldSystem::is_over() const {
	return static_cast<bool>(glfwWindowShouldClose(window));
}

// On key callback
void WorldSystem::on_key(const int key, int, const int action, const int mod) {
	//TODO: handle keyboard shortcuts
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		registry.players.clear();
		current_td_system.reset(); // without this cleanup we would get an error
		exit(0);
	}

	// If td fight is running handle the proper key-presses
	if (!current_td_system->is_over()) {
		current_td_system->on_key(key, 0, action, mod);
		return;
	}

	switch (key) {
		// restart game key
		case GLFW_KEY_R: {
			if (action == GLFW_RELEASE) {
				int w, h;
				glfwGetWindowSize(window, &w, &h);

				restart_game();
			}
			break;
		}
		case GLFW_KEY_T: {
			if (action == GLFW_PRESS) {
				tutorial_text = createText(renderer, tutorial_pos, {8, 20}, tutorial_string.data(), FontType::SQUARE);
			} else if (action == GLFW_RELEASE) {
				registry.remove_all_components_of(tutorial_text);
			}
			break;
		}
		// debugging
		case GLFW_KEY_D: {
			if (action == GLFW_RELEASE)
				debugging.in_debug_mode = false;
			else
				debugging.in_debug_mode = true;
			break;
		}
		// Control the current speed with `<` `>`
		case GLFW_KEY_COMMA: {
			if (action == GLFW_RELEASE && (mod & GLFW_MOD_SHIFT)) {
				current_speed -= 0.1f;
				printf("Current speed = %f\n", current_speed);
			}
			break;
		}
		case GLFW_KEY_PERIOD: {
			if (action == GLFW_RELEASE && (mod & GLFW_MOD_SHIFT)) {
				current_speed += 0.1f;
				printf("Current speed = %f\n", current_speed);
			}
			break;
		}
		default: {}
	}

	// Enforce that the speed cannot be less than 0
	current_speed = fmax(0.f, current_speed);
}

void WorldSystem::on_mouse_move(const vec2 pos) {
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A1: HANDLE SALMON ROTATION HERE
	// xpos and ypos are relative to the top-left of the window, the salmon's
	// default facing direction is (1, 0)
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//TODO: handle drag and drop tower placement

	// If td fight is running handle the proper mouse movements
	if (!current_td_system->is_over()) {
		current_td_system->on_mouse_move(pos, window);
	} else {
		auto &overview_map_reg = registry.overviewMapLocations;

		registry.clickables.clear();
		for (const auto entity : overview_map_reg.entities) {
			auto &loc_props = registry.overviewMapLocations.get(entity);
			if (loc_props.selectable) {
				const auto map_pos = registry.stationaries.get(entity);
				const vec2 dp = map_pos.position - pos;
				const float dist_squared = dot(dp, dp);
				vec2 bounding_box = {abs(map_pos.scale.x), abs(map_pos.scale.y)};
				bounding_box *= 0.3f;
				const float element_r_squared = dot(bounding_box, bounding_box);
				// TODO fix selection flickering
				if (dist_squared < element_r_squared) {
					if (registry.invisibles.has(loc_props.overview_selection)) {
						registry.invisibles.remove(loc_props.overview_selection);
					}
					registry.clickables.emplace(entity);
				} else if (!registry.invisibles.has(loc_props.overview_selection)) {
					registry.invisibles.emplace(loc_props.overview_selection);
				}
			}
		}
	}
}

void WorldSystem::on_mouse_button(const int button, const int action, const int mods) {
	if (!current_td_system->is_over()) {
		current_td_system->on_mouse_button(button, action, mods, window);
	} else {
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
			auto &clickables = registry.clickables;
			for (const auto entity : clickables.entities) {
				if (registry.overviewMapLocations.has(entity)) {
					current_td_system.reset( new TDSystem(rng()));
					current_td_system->init(renderer, player);
					registry.invisibles.emplace(tutorial_hint);
					td_fight_launched = true;
					next_map_pos = entity;
				}
			}
		}
	}
}
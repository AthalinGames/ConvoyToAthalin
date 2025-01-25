// Header
#include "world_system.hpp"
#include "world_init.hpp"

// stlib
#include <cassert>
#include <imgui.h>
#include <sstream>

#include "physics_system.hpp"
#include "world_generation.hpp"
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
	current_shop_system.reset();

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

	for (const Entity entity : registry.statusTextTimers.entities) {
		constexpr float status_time_length = 2000.f;
		StatusTextTimer &timer = registry.statusTextTimers.get(entity);
		timer.timer_ms -= elapsed_ms;
		if (timer.timer_ms < 0) {
			registry.remove_all_components_of(entity);
			continue;
		}

		auto &position = registry.stationaries.get(entity);
		auto &color = registry.colors.get(entity);
		color.a = timer.timer_ms / status_time_length;
		position.position.y += elapsed_ms * (100.f/window_height_px);
        switch (timer.type) {
            case StatusType::HEALTH:
                position.position.x = TILE_WIDTH / 4;
                break;
            case StatusType::FOOD:
                position.position.x = TILE_WIDTH * 2 - TILE_WIDTH / 8;
                break;
        	case StatusType::COINS:
        		position.position.x = TILE_WIDTH * 5.2;
        	    break;
        }


	}
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
	//ImGui::Begin("Health");
	//ImGui::SetWindowPos({window_width_px * 0.1, window_height_px * 0.03});
	//ImGui::SetWindowSize({window_width_px * 0.06, 65});
	//for (const auto & player : registry.players.components) {
	//	ImGui::Text("HP: %d\nFood: %d", player.health, player.food);
	//}
	//ImGui::End();

	// If td system is running, run also its step
	if (!current_td_system->is_over()) {
		current_td_system->step(elapsed_ms);
	} else if (!current_shop_system->is_over()) {
		current_shop_system->step(elapsed_ms);
	} else if (td_fight_launched | shop_launched) {
		if (td_fight_launched) {
			// Do fight cleanup
			// Check if Player is already dead
			if (registry.deathTimers.size() > 0) {
				return true;
			}
			// TD Fight should be finished
			current_td_system.reset(new TDSystem());
			// check if Player is dead
			for (const auto & player : registry.players.components) {
				if (player.getHealth() <= 0) {
					const Entity gameOver = createGameOver(renderer);
					registry.deathTimers.emplace(gameOver);
					return true;
				}
			}

			Player& current_player = registry.players.get(player);
			current_player.won_battles++;

			td_fight_launched = false;
		} else if (shop_launched) {
			// Do shop cleanup
			current_shop_system.reset(new ShopSystem());
			shop_launched = false;
		}

		// Setup Overview-Map for next selection
		auto &current_map_pos_props = registry.overviewMapLocations.get(current_map_pos);
		current_map_pos_props.active = false;
		for (const Entity next_location : current_map_pos_props.next_locations) {
			auto &location_props = registry.overviewMapLocations.get(next_location);
			location_props.active = false;
			location_props.selectable = false;
		}
		auto &next_map_pos_props = registry.overviewMapLocations.get(next_map_pos);
		registry.invisibles.remove(next_map_pos_props.overview_selection);
		for (const Entity next_location : next_map_pos_props.next_locations) {
			auto &location_props = registry.overviewMapLocations.get(next_location);
			location_props.selectable = true;
		}
		next_map_pos_props.selectable = false;
		current_map_pos = next_map_pos;

		registry.invisibles.remove(tutorial_hint);

		// Finally get back to overview-map steps
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
    createItem(ConsumableType::SPIKES);
    createItem(ConsumableType::BARRIER);
    createItem(ConsumableType::HEALTH_POTION);

    registry.players.get(player).placement_marker = createPlacementMarker(renderer);

	current_td_system.reset(new TDSystem());
	current_shop_system.reset(new ShopSystem());
	overview_map = createOverviewMap(renderer);
	// Generate possible paths
	// TODO fix path crossing
	const auto [paths, visited] = generate_overview_paths(rng);

	// Render locations
	const overview_grid_entities location_entities = create_fight_locations(renderer, rng, visited);

	// Add Start and Goal
	current_map_pos = build_overview_graph(renderer, paths, location_entities);

	tutorial_hint = createText(renderer, {5, window_height_px - 5}, {10, 10}, "Hold 'T' to show the tutorial", FontType::SQUARE);
}

// Compute collisions between entities
void WorldSystem::handle_collisions() const {
	// Loop over all collisions detected by the physics system
	auto& collisionsRegistry = registry.collisions;
	//for (uint i = 0; i < collisionsRegistry.components.size(); i++) {
    size_t collision_count = collisionsRegistry.size();
    while (!collisionsRegistry.components.empty()) {
		// The entity and its collider
		const Entity entity = collisionsRegistry.entities.back();
		const Entity entity_other = collisionsRegistry.components.back().other_entity;

		// If td fight is running handle its collisions
		if (!current_td_system->is_over()) {
			current_td_system->handle_collision(entity, entity_other);
		}
        if (collision_count == collisionsRegistry.size()) {
            collisionsRegistry.pop_back();
            //collisionsRegistry.components.pop_back();
            //collisionsRegistry.entities.pop_back();
        } else {
            collision_count = collisionsRegistry.size();
        }
	}

	// Remove all collisions from this simulation step
	registry.collisions.clear();
}

void WorldSystem::handle_post_collision_actions() const {
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
	if (!current_shop_system->is_over()) {
		current_shop_system->on_key(key, 0, action, mod);
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
	} else if (!current_shop_system->is_over()) {
		current_shop_system->on_mouse_move(pos);
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
	} else if (!current_shop_system->is_over()) {
		current_shop_system->on_mouse_button(button, action, mods);
	} else {
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
			auto &clickables = registry.clickables;
			for (const auto entity : clickables.entities) {
				if (registry.overviewMapLocations.has(entity)) {
					const OverviewMapLocation &loc_props = registry.overviewMapLocations.get(entity);
					if (loc_props.type == LocationType::FIGHT) {
						current_td_system.reset( new TDSystem(rng()));
						current_td_system->init(renderer, player);
						td_fight_launched = true;
					} else {
						current_shop_system.reset(new ShopSystem(rng()));
						current_shop_system->init(renderer, player, loc_props.type);
						shop_launched = true;
					}
					registry.invisibles.emplace(tutorial_hint);
					next_map_pos = entity;
				}
			}
		}
	}
}
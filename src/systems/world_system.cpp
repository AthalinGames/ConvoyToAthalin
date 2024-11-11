// Header
#include "world_system.hpp"
#include "world_init.hpp"

// stlib
#include <cassert>
#include <sstream>

#include "physics_system.hpp"

// Game configuration
/* TODO: Replace with our game config
const size_t MAX_TURTLES = 15;
const size_t MAX_FISH = 5;
const size_t TURTLE_DELAY_MS = 2000 * 3;
const size_t FISH_DELAY_MS = 5000 * 3;
*/

// Create the fish world
WorldSystem::WorldSystem()
	: points(0)
	, next_turtle_spawn(0.f) //TODO: Replace with our spawn time variables
	, next_fish_spawn(0.f) {
	// Seeding rng with random device
	rng = std::default_random_engine(std::random_device()());
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
GLFWwindow* WorldSystem::create_window() {
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
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#if __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	glfwWindowHint(GLFW_RESIZABLE, 0);

	// Create the main window (for rendering, keyboard, and mouse input)
	window = glfwCreateWindow(window_width_px, window_height_px, "Convoy to Athalin", nullptr, nullptr);
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
	glfwSetKeyCallback(window, key_redirect);
	glfwSetCursorPosCallback(window, cursor_pos_redirect);

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

void WorldSystem::init(RenderSystem* renderer_arg) {
	this->renderer = renderer_arg;
	// Playing background music indefinitely
	Mix_PlayMusic(background_music, -1); // TODO: Replace with our bgm
	fprintf(stderr, "Loaded music\n");

	// Set all states to default
    restart_game();
}

// Update our game world
bool WorldSystem::step(float elapsed_ms_since_last_update) {
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

	// Spawning new turtles
	/* TODO: Spawn our own enemies
	next_turtle_spawn -= elapsed_ms_since_last_update * current_speed;
	if (registry.hardShells.components.size() <= MAX_TURTLES && next_turtle_spawn < 0.f) {
		// Reset timer
		next_turtle_spawn = (TURTLE_DELAY_MS / 2) + uniform_dist(rng) * (TURTLE_DELAY_MS / 2);
		// Create turtle
		Entity entity = createTurtle(renderer, {0,0});
		// Setting random initial position and constant velocity
		Motion& motion = registry.motions.get(entity);
		motion.position =
			vec2(window_width_px -200.f,
				 50.f + uniform_dist(rng) * (window_height_px - 100.f));
		motion.velocity = vec2(-100.f, 0.f);
	}


	// Spawning new fish

	next_fish_spawn -= elapsed_ms_since_last_update * current_speed;
	if (registry.softShells.components.size() <= MAX_FISH && next_fish_spawn < 0.f) {
		// !!!  TODO A1: Create new fish with createFish({0,0}), as for the Turtles above
	}
	*/

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A2: HANDLE PEBBLE SPAWN HERE
	// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 2
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	// Processing the salmon state TODO: Process our player state instead?
	assert(registry.screenStates.components.size() <= 1);
    ScreenState &screen = registry.screenStates.components[0];

    float min_timer_ms = 3000.f;
	for (Entity entity : registry.deathTimers.entities) {
		// progress timer
		DeathTimer& timer = registry.deathTimers.get(entity);
		timer.timer_ms -= elapsed_ms_since_last_update;
		if(timer.timer_ms < min_timer_ms){
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
	screen.screen_darken_factor = 1 - min_timer_ms / 3000;

	for (const auto tower_entity : registry.shotTimers.entities) {
		auto& shot_timer = registry.shotTimers.get(tower_entity);
		shot_timer.time -= elapsed_ms_since_last_update;

		if (shot_timer.time < 0) {
			registry.shotTimers.remove(tower_entity);
		}
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
	while (registry.motions.entities.size() > 0)
	    registry.remove_all_components_of(registry.motions.entities.back());

    //Remove all stationaries
    while (registry.stationaries.entities.size() > 0)
        registry.remove_all_components_of(registry.stationaries.entities.back());

	towers.clear();
    maps.clear();

	// Debugging for memory/component leaks
	registry.list_all_components();

	// Create a new salmon
	/* TODO: create player base?
	player_salmon = createSalmon(renderer, { 100, 200 });
	registry.colors.insert(player_salmon, {1, 0.8f, 0.8f});
	*/
    const auto debug_map = createMap(renderer,
                                                {window_width_px/2, window_height_px/2},
                                                {vec2(0, 180), vec2(510, 180), vec2(510, 450), vec2(930, 450), vec2(1050, 330)}); //TODO percentage relative to window size
    maps.push_back(debug_map);
    Map& map = registry.maps.get(debug_map);
    map.active = true;

	const auto debug_archer = createArcher(renderer, {400, 300});
	towers.push_back(debug_archer);

    const auto debug_enemy = createEnemy(renderer, {0, 100});
    enemies.push_back(debug_enemy);
    Enemy& enemy = registry.enemies.get(debug_enemy);
    enemy.speed = 100.f;

    registry.list_all_components();

	// !! TODO A2: Enable static pebbles on the ground, for reference
	// Create pebbles on the floor, use this for reference
	/*
	for (uint i = 0; i < 20; i++) {
		int w, h;
		glfwGetWindowSize(window, &w, &h);
		float radius = 30 * (uniform_dist(rng) + 0.3f); // range 0.3 .. 1.3
		Entity pebble = createPebble({ uniform_dist(rng) * w, h - uniform_dist(rng) * 20 },
			         { radius, radius });
		float brightness = uniform_dist(rng) * 0.5 + 0.5;
		registry.colors.insert(pebble, { brightness, brightness, brightness});
	}
	*/
}

// Compute collisions between entities
void WorldSystem::handle_collisions() {
	// Loop over all collisions detected by the physics system
	auto& collisionsRegistry = registry.collisions;
	for (uint i = 0; i < collisionsRegistry.components.size(); i++) {
		// The entity and its collider
		Entity entity = collisionsRegistry.entities[i];
		Entity entity_other = collisionsRegistry.components[i].other_entity;

		// For now, we are only interested in collisions that involve the salmon
		if (registry.players.has(entity)) {
			//Player& player = registry.players.get(entity);

			// Checking Player - HardShell collisions
			/* TODO: check instead enemy collision with player base?
			if (registry.hardShells.has(entity_other)) {
				// initiate death unless already dying
				if (!registry.deathTimers.has(entity)) {
					// Scream, reset timer, and make the salmon sink
					registry.deathTimers.emplace(entity);
					Mix_PlayChannel(-1, salmon_dead_sound, 0);

					// !!! TODO A1: change the salmon orientation and color on death
				}
			}
			// Checking Player - SoftShell collisions
			// TODO: later check collision of enemy and player projectile
			else if (registry.softShells.has(entity_other)) {
				if (!registry.deathTimers.has(entity)) {
					// chew, count points, and set the LightUp timer
					registry.remove_all_components_of(entity_other);
					Mix_PlayChannel(-1, salmon_eat_sound, 0);
					++points;

					// !!! TODO A1: create a new struct called LightUp in components.hpp and add an instance to the salmon entity by modifying the ECS registry
				}
			}
			*/
		} else if (registry.towers.has(entity) && registry.enemies.has(entity_other)) {
			auto& tower = registry.towers.get(entity);
			if (registry.aimingAts.has(entity)) {
				auto& aiming = registry.aimingAts.get(entity);
				auto& aimedEnemy = registry.enemies.get(aiming.aimed_entity);
				auto& otherEnemy = registry.enemies.get(entity_other);
				switch (tower.priority) {
					case FIRST:
						if (aimedEnemy.enemy_progress < otherEnemy.enemy_progress) {
							aiming.aimed_entity = entity_other;
						}
						break;
					case LAST:
						if (aimedEnemy.enemy_progress > otherEnemy.enemy_progress) {
							aiming.aimed_entity = entity_other;
						}
						break;
				}
			} else {
				auto& aimingAt = registry.aimingAts.emplace(entity);
				aimingAt.aimed_entity = entity_other;
			}
		}
	}

	// Remove all collisions from this simulation step
	registry.collisions.clear();
}

void WorldSystem::handle_aiming() {
	auto& aimingRegistry = registry.aimingAts;
	for (uint i = 0; i < aimingRegistry.entities.size(); i++) {
		const Entity tower_entity = aimingRegistry.entities[i];
		const Entity enemy_entity = aimingRegistry.components[i].aimed_entity;
		auto& tower_motion = registry.motions.get(tower_entity);
		auto& enemy_motion = registry.motions.get(enemy_entity);
		const auto d_p = tower_motion.position - enemy_motion.position;
		const auto angle = atan2(d_p.y, d_p.x);
		tower_motion.angle = angle;

		if (!registry.shotTimers.has(tower_entity)) {
			auto& archer = registry.archers.get(tower_entity);
			registry.shotTimers.emplace(tower_entity);
			if (registry.archers.has(tower_entity)) {
				createArrow(renderer, tower_motion.position, archer.arrow_speed, d_p);
			}
		}
	}
	// Clear aiming for next iteration
	registry.aimingAts.clear();
}

// Should the game be over ?
bool WorldSystem::is_over() const {
	return bool(glfwWindowShouldClose(window));
}

// On key callback
void WorldSystem::on_key(int key, int, int action, int mod) {
	//TODO: handle keyboard shortcuts

	// Resetting game
	if (action == GLFW_RELEASE && key == GLFW_KEY_R) {
		int w, h;
		glfwGetWindowSize(window, &w, &h);

        restart_game();
	}

	// Debugging
	if (key == GLFW_KEY_D) {
		if (action == GLFW_RELEASE)
			debugging.in_debug_mode = false;
		else
			debugging.in_debug_mode = true;
	}

	// Control the current speed with `<` `>`
	if (action == GLFW_RELEASE && (mod & GLFW_MOD_SHIFT) && key == GLFW_KEY_COMMA) {
		current_speed -= 0.1f;
		printf("Current speed = %f\n", current_speed);
	}
	if (action == GLFW_RELEASE && (mod & GLFW_MOD_SHIFT) && key == GLFW_KEY_PERIOD) {
		current_speed += 0.1f;
		printf("Current speed = %f\n", current_speed);
	}
	current_speed = fmax(0.f, current_speed);
}

void WorldSystem::on_mouse_move(vec2 mouse_position) {
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A1: HANDLE SALMON ROTATION HERE
	// xpos and ypos are relative to the top-left of the window, the salmon's
	// default facing direction is (1, 0)
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//TODO: handle drag and drop tower placement

	(vec2)mouse_position; // dummy to avoid compiler warning
}

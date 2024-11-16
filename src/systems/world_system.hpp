#pragma once

// internal
#include "common.hpp"

// stlib
#include <vector>
#include <random>

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_mixer.h>

#include "render_system.hpp"
#include "td_system.hpp"

// Container for all our entities and game logic. Individual rendering / update is
// deferred to the relative update() methods
class WorldSystem
{
public:
    WorldSystem();

    // Creates a window
    GLFWwindow* create_window();

    // starts the game
    void init(RenderSystem* renderer);

    // Releases all associated resources
    ~WorldSystem();

    // Steps the game ahead by ms milliseconds
    bool step(float elapsed_ms);

    // Check for collisions
    void handle_collisions();

    // Aim all towers at their current enemy
    void handle_post_collision_actions();

    // Should the game be over ?
    bool is_over()const;
private:
    // Input callback functions
    void on_key(int key, int, int action, int mod);
    void on_mouse_move(vec2 pos);

    // restart level
    void restart_game();

    // OpenGL window handle
    GLFWwindow* window;

    // Number of fish eaten by the salmon, displayed in the window title
    unsigned int points;

    // Game state
    RenderSystem* renderer;
    float current_speed;

    Entity overview_map;

    // Overview params
    static constexpr uint8_t grid_width = 5;
    static constexpr uint8_t grid_height = 8;
    static constexpr uint8_t path_count = 4;

    static constexpr std::array<vec2, 4> overview_locations{
        vec2{0.4f * window_width_px, START_ICON_LOC_Y},
        vec2{GOAL_ICON_LOC_X, 0.4f * window_height_px},
        vec2{START_ICON_LOC_X, 0.55f * window_height_px},
        vec2{0.65f * window_width_px, GOAL_ICON_LOC_Y}
    };

    // TDSystem handle;
    TDSystem current_td_system;

    // music references
    Mix_Music* background_music;
    Mix_Chunk* salmon_dead_sound;
    Mix_Chunk* salmon_eat_sound;

    // C++ random number generator
    std::default_random_engine rng;
    std::uniform_real_distribution<float> uniform_dist; // number between 0..1
    std::uniform_int_distribution<uint8_t> overview_path_start_dist = std::uniform_int_distribution<uint8_t>(0, grid_width - 1);
};

#pragma once

// internal
#include "common.hpp"

// stdlib
#include <random>

#include "render_system.hpp"
#include "ecs/tiny_ecs_registry.hpp"
#include "world_init.hpp"

class TDSystem {
public:
    explicit TDSystem(unsigned int seed);
    TDSystem();

    // starts the TDSystem
    void init(RenderSystem* renderer, Entity player);

    // Releases all associated resources
    ~TDSystem();

    // Steps the td fight ahead by ms milliseconds
    bool step(float elapsed_ms);

    // Check for collisions
    void handle_collision(Entity first, Entity second);

    // Aim all towers at their current enemy
    void handle_aiming();

    // Should the td fight be over?
    bool is_over() const;

    // Input callback functions
    void on_key(int key, int, int action, int mods);
    void on_mouse_move(vec2 pos, GLFWwindow* window);
    void on_mouse_button(int button, int action, int mods, GLFWwindow* window);
private:

    std::vector<Entity> generate_combat(int difficulty);
    // restart TD fight
    void restart_td_fight();

    bool running = true;

    // Game state
    RenderSystem* renderer;
    Entity player;
    float current_speed;
    bool dragging; // shows if mouse is currently dragging something
    Entity dragged_entity;
    std::vector<Entity> towers;
    std::vector<Entity> cards;
    std::set<Entity> enemies;
    Entity map;

    // C++ random number generator
    std::default_random_engine rng;
    std::uniform_real_distribution<float> uniform_dist;
};

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

    void cleanup_ecs();

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

    // restart TD fight
    std::vector<Entity> generate_combat(int difficulty);
    Entity generate_map(int difficulty) const;
    void restart_td_fight();

    enum class GamePhase {
        SETUP,
        RUNNING,
        FIGHT_DONE,
        CHOOSE_REWARD,
        ENDED
    };
    GamePhase current_phase = GamePhase::SETUP;

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
    // Post game state
    std::vector<Entity> new_cards;

    // Entities that should get deleted when TD-Fight ends
    std::vector<Entity> cleanup_entities;

    // Tutorial entity;
    Entity tutorial_text;
    Entity tutorial_background;
    static constexpr std::string_view tutorial_string = R"(
This is the tower defence screen, here you do the actual fighting against enemies.
On the bottom there are some cards that represent your towers, to place a tower
you can drag and drop a card onto the desired location.
(Think about the range of the tower, that is currently not displayed)
To start the round you need to press 'c'.
WARNING: After the round started you cannot place any towers
You defend the camping fire and the enemies spawn at the opposite side of the path
)";
    static constexpr auto tutorial_pos = vec2(100, 100);

    // C++ random number generator
    std::default_random_engine rng;
    std::uniform_real_distribution<float> uniform_dist = std::uniform_real_distribution<float>(0.0, 1.0);
};

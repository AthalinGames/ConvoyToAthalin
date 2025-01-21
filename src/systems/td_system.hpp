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

    // Handle an enemy dying
    void handle_enemy_death(Entity enemy_entity, Enemy &enemy);

    // Handle a consumable being destroyed
    void handle_consumable_destruction(Entity consumable_entity);

    // Check for collisions
    void handle_collision(Entity first, Entity second);

    // Aim all towers at their current enemy
    void handle_aiming() const;

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
    Entity placement_marker;
    float current_speed;
    bool dragging; // shows if mouse is currently dragging something
    Entity dragged_entity;
    std::vector<Entity> towers;
    std::set<Entity> consumables;
    std::vector<Entity> cards;
    std::set<Entity> enemies;
    Entity map;
    Entity start_button;
    // Post game state
    std::vector<Entity> new_cards;

    // Entities that should get deleted when TD-Fight ends
    std::vector<Entity> cleanup_entities;

    // Tutorial entity;
    Entity tutorial_text;
    Entity tutorial_background;
    static constexpr std::string_view tutorial_string = R"(
This is the tower defence screen, here you do the actual fighting against enemies.
On the bottom there are some cards that represent your towers (grey) and consumables (yellow),
to place a tower you can drag and drop a card with left-clock onto the desired location.
If you decide against placing a tower you are currently dragging you can either press right-click or
let go left-click at the bottom of the screen.
Placing a tower consumes 5 food (visible in the upper-left corner below HP).
If you don't have enough food the tower cannot be placed.
To start the round you need to press 'c'.
You defend the campfire and the enemies spawn at the opposite side of the path.
When the combat ends you gain 5 food and additionally 10 food for each tower card (grey),
that was not placed.
Then you can select 1 of 3 cards to add to your collection of available cards.
)";
    static constexpr auto tutorial_pos = vec2(100, 100);

    // C++ random number generator
    std::default_random_engine rng;
    std::uniform_real_distribution<float> uniform_dist = std::uniform_real_distribution<float>(0.0, 1.0);
};

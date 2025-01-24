#pragma once
#include <random>

#include "render_system.hpp"
#include "ecs/game_components.hpp"

class ShopSystem {
public:
    explicit ShopSystem(unsigned int seed);
    ShopSystem();

    // starts the ShopSystem
    void init(RenderSystem* renderSystem, Entity player, LocationType location);
    void restartShop();

    // Releases all associated resources
    ~ShopSystem();

    // Steps the shop ahead by ms milliseconds
    bool step(float elapsed_ms) const;

    void cleanup_ecs();

    // Is the Shop System finished?
    bool is_over() const;

    // Input callback functions
    void on_key(int key, int, int action, int mods);
    void on_mouse_move(vec2 pos, GLFWwindow* window);
    void on_mouse_button(int button, int action, int mods, GLFWwindow* window);
private:
    bool over;

    uint card_count = 4;

    RenderSystem* renderSystem = nullptr;
    Entity player;
    LocationType location;

    std::vector<Entity> new_cards;

    std::vector<Entity> cleanup_entities;

    std::default_random_engine rng;
};


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

    void check_if_buyable(const Player &player_stats);

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
    void on_mouse_move(const vec2 pos);
    void on_mouse_button(int button, int action, int mods);
private:
    bool over;

    uint card_count = 4;

    RenderSystem* renderSystem = nullptr;
    Entity player;
    LocationType location;

    Entity button;

    // Tutorial entities
    Entity tutorial_text;
    Entity tutorial_button;
    static constexpr std::string_view tutorial_string = R"(
This is the Shop.
Here you can buy up to four Consumables or Towers, depending on the shop-type.
The price of each card is listed under each one, if you cannot afford a card,
it is greyed out.
The required coins are gathered by killing enemies.
)";
    static constexpr auto tutorial_pos = vec2(100, 100);

    std::vector<Entity> new_cards;

    std::vector<Entity> cleanup_entities;

    std::default_random_engine rng;
};


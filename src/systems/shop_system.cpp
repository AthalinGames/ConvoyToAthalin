#include "shop_system.hpp"

#include "ecs/tiny_ecs_registry.hpp"
#include "common.hpp"
#include "world_init.hpp"

ShopSystem::ShopSystem(const unsigned int seed): location() {
    rng = std::default_random_engine(seed);
    over = false;
}

ShopSystem::ShopSystem(): location() {
    over = true;
}

void ShopSystem::cleanup_ecs() {
    for (const Entity cleanup_entity : cleanup_entities) {
        registry.remove_all_components_of(cleanup_entity);
    }
    for (const Entity new_card : new_cards) {
        registry.remove_all_components_of(new_card);
    }
}

ShopSystem::~ShopSystem() {
    cleanup_ecs();
}

void ShopSystem::init(RenderSystem *renderSystem, const Entity player, const LocationType location) {
    this->renderSystem = renderSystem;
    this->player = player;
    this->location = location;

    restartShop();
}

void ShopSystem::check_if_buyable(const Player &player_stats) {
    for (const Entity new_card : new_cards) {
        const Item& item = registry.items.get(new_card);
        Card& card = registry.cards.get(new_card);
        if (item.gold_cost > player_stats.getCoins()) {
            card.selectable = false;
            registry.colors.insert(new_card, {0.5f, 0.5f, 0.5f, 1.0f});
        }
    }
}

void ShopSystem::restartShop() {
    registry.list_all_components();
    printf("Restarting Shop...\n");

    cleanup_ecs();

    // Debugging for memory/component leaks
    registry.list_all_components();

    if (location == LocationType::MERCHANT) {
        cleanup_entities.push_back(createMerchantBackground());
    } else if (location == LocationType::GARRISON) {
        cleanup_entities.push_back(createMerchantBackground()); // TODO add Background for second shop
    } else {
        assert(false && "Invalid location");
    }

    // Generate Cards
    for (uint i = 0; i < card_count; ++i) {
        Entity card;
        if (location == LocationType::MERCHANT) {
            card = createRandomItem(rng, ConsumableType::CONSUMABLE_TYPE_COUNT);
        } else {
            card = createRandomItem(rng, TowerType::TOWER_TYPE_COUNT);
        }
        createCardFromItem(renderSystem, card);
        new_cards.push_back(card);
    }
    // Layout new Cards
    const float x_offset = window_width_px / (card_count + 3);
    for (uint i = 0; i < card_count; ++i) {
        const Entity card = new_cards[i];
        Stationary& card_pos = registry.stationaries.get(card);
        registry.renderForeground.get(card).z_position = Z_MIDDLE;
        const vec2 pos = {x_offset * (i + 2), window_height_px / 2};
        card_pos.position = pos;
        const Item& item = registry.items.get(card);
        cleanup_entities.push_back(
            createText(renderSystem, pos + vec2(0, CARD_HEIGHT/1.5), vec2(CARD_WIDTH/20, CARD_WIDTH/10),
            std::to_string(item.gold_cost) + " $",
            FontType::SLIM));
    }
    // Render Shop Text
    constexpr auto font_scale = vec2(CARD_WIDTH/10, CARD_WIDTH/5);
    const std::string shop_text =
        location == LocationType::GARRISON ? "Hire Troops from the Garrison" : "Buy Items from the Merchant";
    cleanup_entities.push_back(createText(
        renderSystem, {window_width_px / 2 - font_scale.x * (shop_text.length() / 2), window_height_px / 3}, font_scale,
        shop_text,
        FontType::SLIM
    ));
    // Determine if Cards can be bought
    const Player& player_stats = registry.players.get(player);
    check_if_buyable(player_stats);

    button = createButton(renderSystem, {window_width_px / 2, 2.1 * (window_height_px / 3)}, {CARD_WIDTH * 0.8, CARD_WIDTH * 0.25}, "Continue");
    cleanup_entities.push_back(button);
}

bool ShopSystem::step(float elapsed_ms) const {
    return true;
}

bool ShopSystem::is_over() const {
    return over;
}

void ShopSystem::on_key(const int key, const int, const int action, const int mods) {
    switch (key) {
        case GLFW_KEY_C:
        case GLFW_KEY_R: {
            if (action == GLFW_PRESS) {
                over = true;
            }
            break;
        }
        default: {}
    }
}

bool check_mouse_collision(const vec2 pos, const Stationary& elem_pos) {
    const vec2 dp = elem_pos.position - pos;
    const float dist_squared = dot(dp, dp);
    vec2 bounding_box = {abs(elem_pos.position.x), abs(elem_pos.position.y)};
    bounding_box *= 0.1f;
    const float card_squared = dot(bounding_box, bounding_box);
    return dist_squared < card_squared;
}

void ShopSystem::updateCardStats(const vec2 pos, const Entity card_entity, const Entity player) {
    auto &stats_window = registry.cardStatsWindows.get(registry.players.get(player).cardStatsWindow);
    registry.invisibles.remove(stats_window.text_entity);
    registry.invisibles.remove(stats_window.background_entity);
    auto &text_stationary = registry.stationaries.get(stats_window.text_entity);
    auto &bg_stationary = registry.stationaries.get(stats_window.background_entity);

    std::vector<std::string> stats_text;
    std::string next_line;
    stats_window.text_rows = 0;
    stats_window.text_cols = 0;
    if (registry.towers.has(card_entity)) {
        auto tower = registry.towers.get(card_entity);
        stats_text.push_back("Cost: " + std::to_string(tower.food_cost) + " Food\n");
        stats_text.push_back("Gain: " + std::to_string(tower.food_gain) + " Food\n");
        stats_text.push_back("Range: " + std::to_string(static_cast<int>(tower.range / TILE_WIDTH * 100)) + "\n");
        if (registry.archers.has(card_entity)) {
            stats_text.push_back("Damage: " + std::to_string(registry.archers.get(card_entity).damage) + "\n");
        } else if (registry.knights.has(card_entity)) {
            stats_text.push_back("Damage: " + std::to_string(registry.knights.get(card_entity).damage) + "\n");
        }
        stats_window.text_rows = 4;
    } else if (registry.consumables.has(card_entity)) {
        auto consumable = registry.consumables.get(card_entity);
        if (registry.healthPotions.has(card_entity)) {
            stats_text.push_back("Restores: " + std::to_string(registry.healthPotions.get(card_entity).health) + " Health\n");
            stats_window.text_rows = 1;
        } else if (registry.bombs.has(card_entity)) {
            auto bomb = registry.bombs.get(card_entity);
            stats_text.push_back("Range: " + std::to_string(static_cast<int>(consumable.range / TILE_WIDTH * 100)) + "\n");
            stats_text.push_back("Damage: " + std::to_string(bomb.damage) + "\n");
            stats_window.text_rows = 2;
        } else if (registry.spikes.has(card_entity)) {
            stats_text.push_back("Damage: " + std::to_string(registry.spikes.get(card_entity).damage) + "\n");
            stats_window.text_rows = 1;
        } else if (registry.barriers.has(card_entity)) {
            stats_text.push_back("Health: " + std::to_string(registry.barriers.get(card_entity).health) + "\n");
            stats_window.text_rows = 1;
        }
    }
    std::string final_text;
    for (auto &line : stats_text) {
        final_text += line;
        stats_window.text_cols = max(stats_window.text_cols, line.size() + 1);
    }
    updateText(stats_window.text_entity, final_text);
    bg_stationary.scale = text_stationary.scale * vec2(stats_window.text_cols, stats_window.text_rows) + text_stationary.scale;
    text_stationary.position = vec2(max(0.f + text_stationary.scale.x, pos.x - bg_stationary.scale.x + text_stationary.scale.x),
                                    max(0.f + text_stationary.scale.y, pos.y - bg_stationary.scale.y + text_stationary.scale.y));

    bg_stationary.position = text_stationary.position;
    bg_stationary.position += bg_stationary.scale / 2.f - text_stationary.scale;
}

void ShopSystem::on_mouse_move(const vec2 pos) {
    registry.clickables.clear();

    bool card_selected = false;
    for (const Entity card : new_cards) {
        const auto& card_props = registry.cards.get(card);
        if (!card_props.selectable) {
            continue;
        }
        auto& card_pos = registry.stationaries.get(card);
        if (check_mouse_collision(pos, card_pos)) {
            card_pos.scale = vec2(1.2 * CARD_WIDTH, 1.2 * CARD_HEIGHT);
            updateCardStats(pos, card, player);
            card_selected = true;
            registry.clickables.emplace(card);
        } else {
            card_pos.scale = vec2(CARD_WIDTH, CARD_HEIGHT);
        }
    }
    if(!card_selected) {
        auto &stats_window = registry.cardStatsWindows.get(registry.players.get(player).cardStatsWindow);
        if (!registry.invisibles.has(stats_window.text_entity)) {
            registry.invisibles.emplace(stats_window.text_entity);
        }
        if (!registry.invisibles.has(stats_window.background_entity)) {
            registry.invisibles.emplace(stats_window.background_entity);
        }
    } else {
        auto &stats_window = registry.cardStatsWindows.get(registry.players.get(player).cardStatsWindow);
        registry.invisibles.remove(stats_window.text_entity);
        registry.invisibles.remove(stats_window.background_entity);
    }
    if (check_mouse_collision(pos, registry.stationaries.get(button))) {
        registry.clickables.emplace(button);
    }
}

void ShopSystem::on_mouse_button(const int button, const int action, int) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        for (const Entity entity : registry.clickables.entities) {
            if (entity == this->button) {
                registry.buttons.get(entity).click(true);
                continue;
            }
            returnCardToItem(entity);
            const Item& item = registry.items.get(entity);
            Player& player_stats = registry.players.get(player);
            player_stats.updateCoins(player_stats.getCoins() - item.gold_cost);
            std::erase(new_cards, entity);
            auto &stats_window = registry.cardStatsWindows.get(registry.players.get(player).cardStatsWindow);
            if (!registry.invisibles.has(stats_window.text_entity)) {
                registry.invisibles.emplace(stats_window.text_entity);
            }
            if (!registry.invisibles.has(stats_window.background_entity)) {
                registry.invisibles.emplace(stats_window.background_entity);
            }
            check_if_buyable(player_stats);
        }
    } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        if (registry.clickables.has(this->button)) {
            auto &stats_window = registry.cardStatsWindows.get(registry.players.get(player).cardStatsWindow);
            if (!registry.invisibles.has(stats_window.text_entity)) {
                registry.invisibles.emplace(stats_window.text_entity);
            }
            if (!registry.invisibles.has(stats_window.background_entity)) {
                registry.invisibles.emplace(stats_window.background_entity);
            }
            over = true;
        }
        registry.buttons.get(this->button).click(false);
    }
}



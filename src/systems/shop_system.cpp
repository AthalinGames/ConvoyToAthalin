#include "shop_system.hpp"

#include "world_init.hpp"
#include "ecs/tiny_ecs_registry.hpp"

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
    for (const Entity new_card : new_cards) {
        const Item& item = registry.items.get(new_card);
        Card& card = registry.cards.get(new_card);
        if (item.gold_cost > player_stats.getCoins()) {
            card.selectable = false;
            registry.colors.insert(new_card, {0.5f, 0.5f, 0.5f, 1.0f});
        }
    }
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

void ShopSystem::on_mouse_move(vec2 pos, GLFWwindow *window) {

}

void ShopSystem::on_mouse_button(int button, int action, int mods, GLFWwindow *window) {

}



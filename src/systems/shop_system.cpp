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

void ShopSystem::on_mouse_move(const vec2 pos) {
    registry.clickables.clear();

    for (const Entity card : new_cards) {
        const auto& card_props = registry.cards.get(card);
        if (!card_props.selectable) {
            continue;
        }
        auto& card_pos = registry.stationaries.get(card);
        if (check_mouse_collision(pos, card_pos)) {
            card_pos.scale = vec2(1.2 * CARD_WIDTH, 1.2 * CARD_HEIGHT);
            registry.clickables.emplace(card);
        } else {
            card_pos.scale = vec2(CARD_WIDTH, CARD_HEIGHT);
        }
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
            check_if_buyable(player_stats);
        }
    } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        if (registry.clickables.has(this->button)) {
            over = true;
        }
        registry.buttons.get(this->button).click(false);
    }
}



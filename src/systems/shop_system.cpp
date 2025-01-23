#include "shop_system.hpp"

#include "ecs/tiny_ecs_registry.hpp"

ShopSystem::ShopSystem(const unsigned int seed, LocationType location) {
    rng = std::default_random_engine(seed);
    over = false;
}

ShopSystem::ShopSystem() {
    over = true;
}

void ShopSystem::cleanup_ecs() {
    for (const Entity cleanup_entity : cleanup_entities) {
        registry.remove_all_components_of(cleanup_entity);
    }
}

ShopSystem::~ShopSystem() {
    cleanup_ecs();
}

void ShopSystem::init(RenderSystem *renderSystem, const Entity player) {
    this->renderSystem = renderSystem;
    this->player = player;

    restartShop();
}

void ShopSystem::restartShop() {
    registry.list_all_components();
    printf("Restarting Shop...\n");

    cleanup_ecs();

    // Debugging for memory/component leaks
    registry.list_all_components();


}

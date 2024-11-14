//
// Created by joris on 14.11.24.
//

#include "td_system.hpp"

TDSystem::TDSystem(const unsigned int seed) {
    rng = std::default_random_engine(seed);
}

TDSystem::TDSystem() {
    running = false;
}


TDSystem::~TDSystem() {

    // Destroy all components related to TD Fights
    for (const auto card : cards) {
        registry.remove_all_components_of(card);
    }
    for (const auto enemy : enemies) {
        registry.remove_all_components_of(enemy);
    }
    for (const auto tower : towers) {
        registry.remove_all_components_of(tower);
    }
    registry.remove_all_components_of(map);
}

void TDSystem::init(RenderSystem *renderer) {
    this->renderer = renderer;

    // Set all states to default
    restart_td_fight();
}

bool TDSystem::step(float elapsed_ms) {
    assert(registry.screenStates.components.size() <= 1);
    ScreenState &screen = registry.screenStates.components[0];

    float min_timer_ms = 3000.f;
    for (Entity entity : registry.deathTimers.entities) {
        // progress timer
        DeathTimer& timer = registry.deathTimers.get(entity);
        timer.timer_ms -= elapsed_ms;
        if(timer.timer_ms < min_timer_ms){
            min_timer_ms = timer.timer_ms;
        }

        // restart the game once the death timer expired
        if (timer.timer_ms < 0) {
            registry.deathTimers.remove(entity);
            screen.screen_darken_factor = 0;
            // TODO do actual handling for that
            restart_td_fight();
            return true;
        }
    }
    // reduce window brightness if any of the present salmons is dying
    screen.screen_darken_factor = 1 - min_timer_ms / 3000;

    for (const auto tower_entity : registry.shotTimers.entities) {
        auto& shot_timer = registry.shotTimers.get(tower_entity);
        shot_timer.time -= elapsed_ms;

        if (shot_timer.time < 0) {
            registry.shotTimers.remove(tower_entity);
        }
    }
    
    return true;
}

void TDSystem::restart_td_fight() {
    // Debugging for memory/component leaks
    registry.list_all_components();
    printf("Restarting TD fight\n");
    
    // Remove all components related to a td fight
    for (const auto card : cards) {
        registry.remove_all_components_of(card);
    }
    for (const auto enemy : enemies) {
        registry.remove_all_components_of(enemy);
    }
    for (const auto tower : towers) {
        registry.remove_all_components_of(tower);
    }
    registry.remove_all_components_of(map);

    towers.clear();
    enemies.clear();
    cards.clear();

    // Debugging for memory/component leaks
    registry.list_all_components();

    const auto debug_map = createMap(renderer,
                                                {window_width_px/2, window_height_px/2},
                                                {vec2(0, 180), vec2(510, 180), vec2(510, 450), vec2(930, 450), vec2(1050, 330)}); //TODO percentage relative to window size
    map = debug_map;
    Map& map = registry.maps.get(debug_map);
    map.active = true;
    printf("Created active map\n");
    printf("Mapcount: %lu\n", registry.maps.size());

    const auto debug_archer = createArcher(renderer, {400, 300});
    towers.push_back(debug_archer);

    const auto debug_enemy = createEnemy(renderer, {0, 100});
    enemies.push_back(debug_enemy);
    Enemy& enemy = registry.enemies.get(debug_enemy);
    enemy.speed = 100.f;

    const auto debug_card = createCard(renderer);
    cards.push_back(debug_card);

    const auto debug_card2 = createCard(renderer);
    cards.push_back(debug_card2);

    for (int i = 0; i < 6; ++i) {
        const auto debug_cards = createCard(renderer);
        cards.push_back(debug_cards);
    }

    registry.list_all_components();
}

void TDSystem::handle_collision(const Entity first, const Entity second) {
    if(registry.enemies.has(second) && registry.arrows.has(first)){
        auto& enemy = registry.enemies.get(second);
        auto& arrow = registry.arrows.get(first);
        enemy.health -= arrow.damage;
        printf("enemy hit");
        if (enemy.health <= 0) {
            enemy.alive = false;
            // clear tower aiming
            auto& aimingRegistry = registry.aimingAts;
            for(const Entity& aiming : aimingRegistry.entities) {
                if (aimingRegistry.get(aiming).aimed_entity == second) {
                    aimingRegistry.remove(aiming);
                }
            }
            registry.remove_all_components_of(second); //TODO slime seems to be called somewhere while it does not longer exist
        }
    } else if (registry.towers.has(first) && registry.enemies.has(second)) {
        auto& tower = registry.towers.get(first);
        if (registry.aimingAts.has(first)) {
            auto& aiming = registry.aimingAts.get(first);
            auto& aimedEnemy = registry.enemies.get(aiming.aimed_entity);
            auto& otherEnemy = registry.enemies.get(second);
            switch (tower.priority) {
                case FIRST:
                    if (aimedEnemy.enemy_progress < otherEnemy.enemy_progress) {
                        aiming.aimed_entity = second;
                    }
                break;
                case LAST:
                    if (aimedEnemy.enemy_progress > otherEnemy.enemy_progress) {
                        aiming.aimed_entity = second;
                    }
                break;
            }
        } else {
            auto& aimingAt = registry.aimingAts.emplace(first);
            aimingAt.aimed_entity = second;
        }
    }
}

void TDSystem::handle_aiming() {
    auto& aimingRegistry = registry.aimingAts;
    for (uint i = 0; i < aimingRegistry.entities.size(); i++) {
        const Entity tower_entity = aimingRegistry.entities[i];
        const Entity enemy_entity = aimingRegistry.components[i].aimed_entity;
        auto& tower_motion = registry.motions.get(tower_entity);
        auto& enemy_motion = registry.motions.get(enemy_entity);
        const auto d_p = tower_motion.position - enemy_motion.position;
        const auto angle = atan2(d_p.y, d_p.x);
        tower_motion.angle = angle;

        if (!registry.shotTimers.has(tower_entity)) {
            auto& archer = registry.archers.get(tower_entity);
            registry.shotTimers.emplace(tower_entity);
            if (registry.archers.has(tower_entity)) {
                createArrow(renderer, tower_motion.position, archer.arrow_speed, d_p);
            }
        }
    }
    // Clear aiming for next iteration
    registry.aimingAts.clear();
}

bool TDSystem::is_over() const {
    return !running;
}

void TDSystem::on_key(const int key, int, const int action, const int mods) {
    // TODO fight specific key handling
    (int) key, action, mods; // dummy to avoid compiler warning
}

void TDSystem::on_mouse_move(const vec2 pos) {
    // TODO fight specific mouse handling
    (vec2) pos; // dummy to avoid compiler warning
}



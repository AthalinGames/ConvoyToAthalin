//
// Created by joris on 14.11.24.
//

#include "td_system.hpp"

#include "physics_system.hpp"
#include "world_generation.hpp"
#include "world_system.hpp"

TDSystem::TDSystem(const unsigned int seed) : dragging(false) {
    rng = std::default_random_engine(seed);
}

TDSystem::TDSystem() {
    current_phase = GamePhase::ENDED;
}

void TDSystem::cleanup_ecs() {
    // Remove all components related to a td fight that are still on map
    for (const auto &visualization : registry.hitboxVisualizations.entities) {
        auto hitbox_visualization = registry.hitboxVisualizations.get(visualization);
        for (const auto &poly_lines: hitbox_visualization.lines) {
            for (const auto &line_entity: poly_lines) {
                registry.remove_all_components_of(line_entity);
            }
        }
    }
    registry.hitboxVisualizations.clear();
    for (const auto visualization : registry.pathVisualizations.entities) {
        registry.pathVisualizations.remove(visualization);
    }
    for (const auto visualization : registry.towerVisualizations.entities) {
        registry.towerVisualizations.remove(visualization);
    }
    for (const auto card: cards) {
        if (registry.cards.has(card)){//TODO: if you stop dragging just when combat ends, Entity can lose card component before being erased from cards vector
            returnCardToItem(card);
        }
        registry.colors.remove(card);
    }
    for (const auto enemy: enemies) {
        registry.remove_all_components_of(enemy);
    }
    for (const auto tower : towers) {
        returnTowerToItem(tower);
    }
    for (const auto consumable: consumables) {
        registry.remove_all_components_of(consumable);
    }
    for (const auto card: new_cards) {
        registry.remove_all_components_of(card);
    }
    for (const auto entity: cleanup_entities) {
        registry.remove_all_components_of(entity);
    }

    registry.maps.remove(map);

    towers.clear();
    consumables.clear();
    enemies.clear();
    cards.clear();
    new_cards.clear();

    registry.shotTimers.clear();
    registry.bombTimers.clear();
}

TDSystem::~TDSystem() {
    cleanup_ecs();
}

void TDSystem::init(RenderSystem *renderer, const Entity player) {
    this->renderer = renderer;
    this->player = player;
    placement_marker = registry.players.get(player).placement_marker;

    // Set all states to default
    restart_td_fight();
}

bool TDSystem::step(const float elapsed_ms) {
    Map &td_map = registry.maps.get(map);

    switch (current_phase) {
        case GamePhase::SETUP: {
            break;
        }
        case GamePhase::RUNNING: {
            td_map.combat_time += elapsed_ms;
            for (const auto hit_entity : registry.hitTimers.entities) {
                auto &timer = registry.hitTimers.get(hit_entity);
                timer.timer_ms -= elapsed_ms;
                if (timer.timer_ms < 0) {
                    registry.hitTimers.remove(hit_entity);
                    registry.colors.remove(hit_entity);
                    auto &enemy = registry.enemies.get(hit_entity);
                    if (enemy.getHealth() <= 0) {
                        handle_enemy_death(hit_entity, enemy);
                    }
                    continue;
                }

                auto &color = registry.colors.get(hit_entity);
                const float factor = exp(-0.02f * timer.timer_ms);
                color.r = factor;
                color.g = factor;
                color.b = factor;
            }
            for (const auto tower_entity: registry.shotTimers.entities) {
                auto &shot_timer = registry.shotTimers.get(tower_entity);
                shot_timer.time -= elapsed_ms;

                if (registry.archers.has(tower_entity)) {
                    const auto &bow_entity = registry.archers.get(tower_entity).bow;
                    RenderRequest &render_request = registry.renderGameLayer.get(bow_entity);
                    if (shot_timer.time < 0) {
                        //change bow to empty
                        render_request.atlas_ids = {static_cast<unsigned int>(BOW_SPRITE::SHOOT)};
                    } else if (shot_timer.time < 150.) {
                        //change bow to drawn
                        render_request.atlas_ids = {static_cast<unsigned int>(BOW_SPRITE::DRAW)};
                    } else if (shot_timer.time < 500.) {
                        //change bow to loaded
                        render_request.atlas_ids = {static_cast<unsigned int>(BOW_SPRITE::LOAD)};
                    }
                } else if (registry.knights.has(tower_entity)) {
                    const auto &knight = registry.knights.get(tower_entity);
                    //const auto &knight_motion = registry.motions.get(tower_entity);
                    const auto &sword_entity = knight.sword;
                    auto &sword = registry.swords.get(sword_entity);
                    auto &sword_motion = registry.motions.get(sword_entity);
                    auto &tower_motion = registry.motions.get(tower_entity);
                    RenderRequest &render_request = registry.renderGameLayer.get(sword_entity);
                    if (shot_timer.time <= knight.swing_time && shot_timer.time >= 0) {
                        if (sword_motion.use_direction_sprite) {
                            sword_motion.use_direction_sprite = false;
                        }
                        render_request.atlas_ids = {static_cast<unsigned int>(SWORD_SPRITE::SWING)};
                        sword.has_collision = true;
                        const float pos_angle = tower_motion.angle - 2 * M_PI * shot_timer.time / knight.swing_time - M_PI_2;
                        sword_motion.angle = pos_angle - M_PI_2 - M_PI_2 / 2;
                        const float radius = registry.towers.get(tower_entity).range - 0.5f * TOWER_WIDTH;
                        sword_motion.position = tower_motion.position - radius * vec2(cos(pos_angle), sin(pos_angle)) + vec2(0, TOWER_HEIGHT * 0.25);
                    }
                    else {
                        if (!sword_motion.use_direction_sprite) {
                            sword_motion.use_direction_sprite = true;
                        }
                        sword.hit_entities.clear();
                        sword.has_collision = false;
                        sword_motion.angle = tower_motion.angle;
                        sword_motion.position = tower_motion.position;
                    }
                    //keep angle within [-pi, pi]
                    while (sword_motion.angle < -M_PI) {
                        sword_motion.angle += 2 * M_PI;
                    }
                    while (sword_motion.angle > M_PI) {
                        sword_motion.angle -= 2 * M_PI;
                    }

                    Transform tf;
                    tf.translate(sword_motion.position);
                    tf.rotate(sword_motion.angle);
                    tf.scale(sword_motion.scale);
                    auto &hitbox_visualization = registry.hitboxVisualizations.get(sword_entity);
                    for (uint i = 0; i < hitbox_visualization.lines[hitbox_visualization.active_poly].size(); i++) {
                        auto &stationary = registry.stationaries.get(hitbox_visualization.lines[hitbox_visualization.active_poly][i]);
                        stationary.position = tf * hitbox_visualization.offsets[hitbox_visualization.active_poly][i];
                        stationary.angle = sword_motion.angle + hitbox_visualization.angles[hitbox_visualization.active_poly][i];
                        while (stationary.angle > M_PI) {
                            stationary.angle -= 2 * M_PI;
                        }
                    }
                }

                if (shot_timer.time < 0) {
                    registry.shotTimers.remove(tower_entity);
                }
            }
            for (const auto bomb_entity: registry.bombTimers.entities) {
                auto &bomb = registry.bombs.get(bomb_entity);
                auto &bomb_timer = registry.bombTimers.get(bomb_entity);
                bomb_timer.time -= elapsed_ms;
                RenderRequest &render_request = registry.renderGameLayer.get(bomb_entity);
                if (bomb_timer.time > bomb_timer.explosion_time) {
                    const float burn_time_remaining = bomb_timer.time - bomb_timer.explosion_time;
                    if (burn_time_remaining > bomb_timer.burn_time * 0.66) {
                        render_request.atlas_ids = {static_cast<unsigned int>(BOMB_SPRITE::BOMB1)};
                    } else if (burn_time_remaining > bomb_timer.burn_time * 0.33) {
                        render_request.atlas_ids = {static_cast<unsigned int>(BOMB_SPRITE::BOMB2)};
                    } else if (burn_time_remaining > 0.0f) {
                        render_request.atlas_ids = {static_cast<unsigned int>(BOMB_SPRITE::BOMB3)};
                    }
                } else {
                    if(!bomb.exploding) {
                        bomb.exploding = true;
                    }
                    if (bomb_timer.time > bomb_timer.explosion_time * 0.6f) {
                        render_request.atlas_ids = {static_cast<unsigned int>(BOMB_SPRITE::EXPLOSION1)};
                    } else if (bomb_timer.time > bomb_timer.explosion_time * 0.3f) {
                        render_request.atlas_ids = {static_cast<unsigned int>(BOMB_SPRITE::EXPLOSION2)};
                    } else if (bomb_timer.time > bomb_timer.explosion_time * 0.0f) {
                        render_request.atlas_ids = {static_cast<unsigned int>(BOMB_SPRITE::EXPLOSION3)};
                    }
                }
                if (bomb_timer.time <= 0) {
                    handle_consumable_destruction(bomb_entity);
                }
            }
            for (const auto barrier_entity: registry.barrierTimers.entities) {
                auto &barrier = registry.barriers.get(barrier_entity);
                auto &barrier_timer = registry.barrierTimers.get(barrier_entity);

                barrier_timer.time -= elapsed_ms;
                if (barrier_timer.time <= 0) {
                    if (barrier.health <= 0) {
                        handle_consumable_destruction(barrier_entity);
                        continue;
                    }
                    //TODO: if barrier not at hp zero yet, just remove timer component, so barrier does not break completely
                    barrier.health--;
                    RenderRequest &render_request = registry.renderGameLayer.get(barrier_entity);
                    if (barrier.health == 1) {
                        render_request.atlas_ids = {static_cast<unsigned int>(BARRIER_SPRITE::BARRIER_DAMAGED)};
                        registry.barrierTimers.remove(barrier_entity);
                    } else if (barrier.health <= 0) {
                        barrier_timer.time += barrier_timer.hold_time;
                        render_request.atlas_ids = {static_cast<unsigned int>(BARRIER_SPRITE::BARRIER_BROKEN)};
                        for (const auto slowed_entity : registry.sloweds.entities) {
                            auto slowed = registry.sloweds.get(slowed_entity);
                            if (slowed.origin == barrier_entity) {
                                registry.sloweds.remove(slowed_entity);
                            }
                        }
                    } else {
                        registry.barrierTimers.remove(barrier_entity);
                    }
                }
            }

            for (const auto enemy_entity : registry.enemyWalkTimers.entities) {
                auto &walk_timer = registry.enemyWalkTimers.get(enemy_entity);
                walk_timer.time -= elapsed_ms;
                while (walk_timer.time <= 0) {
                    walk_timer.time += walk_timer.start_time;
                }
                //RenderRequest &render_request = registry.renderGameLayer.get(enemy_entity);
                //auto const enemy = registry.enemies.get(enemy_entity);
                //if (registry.slimes.has(enemy_entity)){ // || registry.slimesBig.has(enemy_entity)) {
                //    render_request.atlas_ids = //TODO: move this to render_system
                //}
            }
            if (!td_map.enemies.empty()) {
                Enemy &next_enemy = registry.enemies.get(td_map.enemies[0]);
                if (td_map.combat_time > next_enemy.spawn_time) {
                    next_enemy.spawned = true;
                    auto path_vector = td_map.checkpoints[0] - td_map.checkpoints[1];
                    const float angle = atan2(path_vector.y, path_vector.x);
                    Motion& enemy_motion = registry.motions.get(td_map.enemies[0]);
                    enemy_motion.angle = angle;
                    registry.invisibles.remove(td_map.enemies[0]);
                    auto &walk_timer = registry.enemyWalkTimers.emplace(td_map.enemies[0]);
                    if (registry.slimesBig.has(td_map.enemies[0])) {
                        walk_timer.start_time *= 1.7f;
                    }
                    walk_timer.time = walk_timer.start_time * 100.f / next_enemy.speed; // scale original time of walkTimer to enemy speed
                    td_map.enemies.erase(td_map.enemies.begin());
                }
            }
            // Check if enemy completed Path
            for (std::size_t i = 0; i < registry.enemies.size(); ++i) {
                auto &enemy = registry.enemies.components[i];
                const Entity enemy_entity = registry.enemies.entities[i];
                if (enemy.enemy_progress >= 1.0f && enemy.alive) {
                    for (Player &player: registry.players.components) {
                        player.updateHealth(player.getHealth() - enemy.damage);
                    }
                    // Delete damaging entity
                    handle_enemy_death(enemy_entity, enemy);
                }
            }

            // Check if round is won
            if (registry.enemies.components.empty() && registry.maps.get(map).enemies.empty()) {
                current_phase = GamePhase::FIGHT_DONE;
            }

            // Check if player still has health
            for (std::size_t i = 0; i < registry.players.size(); ++i) {
                const auto &player = registry.players.components[i];
                if (player.getHealth() < 1) {
                    current_phase = GamePhase::ENDED;
                }
            }

            break;
        }
        case GamePhase::FIGHT_DONE: {
            current_phase = GamePhase::CHOOSE_REWARD;

            // reset dragged card
            if (dragging) {
                dragging = false;
                registry.cards.get(dragged_entity).dragged = false;
                registry.invisibles.remove(dragged_entity);
                if (!registry.invisibles.has(placement_marker)) {
                    registry.invisibles.emplace(placement_marker);
                }
                realignCards();
            }

            // Add gathered food
            auto& current_player = registry.players.get(player);
            int food_gain = current_player.baseFoodGain;
            for (const Entity card : cards) {
                if (registry.towers.has(card)) {
                    const auto& tower = registry.towers.get(card);
                    food_gain += tower.food_gain;
                }
            }
            current_player.updateFood(food_gain + current_player.getFood());
            // Setup next screen
            const Entity square = createBlackSquare(renderer, {window_width_px / 2, window_height_px / 2},
                                                        {window_width_px, window_height_px}, 0.5f);
            cleanup_entities.push_back(square);
            const Entity text = createText(renderer, {window_width_px * 0.35, window_height_px * 0.2}, {20, 20},  "Choose a new Card:", FontType::SQUARE);
            cleanup_entities.push_back(text);
            // Create random Items
            // TODO think about amount of items
            const Entity random_item0 = createRandomItem(rng);
            createCardFromItem(renderer, random_item0);
            new_cards.push_back(random_item0);
            const Entity random_item1 = createRandomItem(rng);
            createCardFromItem(renderer, random_item1);
            new_cards.push_back(random_item1);
            const Entity random_item2 = createRandomItem(rng);
            createCardFromItem(renderer, random_item2);

            Stationary& stationary0 = registry.stationaries.get(random_item0);
            stationary0.position = { 1 * (window_width_px / 3), window_height_px / 2};
            Stationary& stationary1 = registry.stationaries.get(random_item1);
            stationary1.position = {1 * (window_width_px / 2), window_height_px / 2};
            Stationary& stationary2 = registry.stationaries.get(random_item2);
            stationary2.position = {2 * (window_width_px / 3), window_height_px / 2};
            new_cards.push_back(random_item2);
            for (const auto card : cards) {
                registry.colors.remove(card);
                returnCardToItem(card);
            }
            cards.clear();
            break;
        }
        case GamePhase::CHOOSE_REWARD: {
            break;
        }
        case GamePhase::ENDED: {
            break;
        }
    }
    return true;
}

std::vector<Entity> TDSystem::generate_combat(int difficulty) {
    //TODO: maybe remove entry from pool when selected, to increase variation

    //{enemy_type, amount, interval, speed}
    std::vector<vec4> combat_pool = {vec4(EnemyType::SLIME, 1, 1000., 120.),
                                     vec4(EnemyType::SLIME, 2, 1000., 120.),
                                     vec4(EnemyType::SLIME, 3, 1000., 120.),
                                     vec4(EnemyType::SLIME, 4, 1000., 120.),
                                     vec4(EnemyType::SLIME, 1, 500., 120.),
                                     vec4(EnemyType::SLIME, 2, 500., 120.),
                                     vec4(EnemyType::SLIME, 3, 500., 120.),
                                     vec4(EnemyType::SLIME, 4, 500., 120.),
                                     vec4(EnemyType::SLIME_BIG, 1, 1000., 100.),
                                     vec4(EnemyType::SLIME_BIG, 2, 1000., 100.),
                                     vec4(EnemyType::SLIME_BIG, 1, 500., 100.),
                                     vec4(EnemyType::SLIME_BIG, 2, 500., 100.),};
    std::vector<Entity> enemy_list = {};
    uint wave_amount = 1 + int(std::floor(difficulty/2)); //TODO: better curve maybe some kind of sigmoid
    std::uniform_int_distribution<int> uniform_int_dist(0, combat_pool.size()-1);

    if (difficulty == 0) {
        const auto debug_enemy = createEnemy(renderer, {0, 100}, EnemyType::SLIME);
        const auto debug_enemy2 = createEnemy(renderer, {0, 100}, EnemyType::SLIME_BIG);
        const auto spawned_enemy1 = createEnemy(renderer, {0, 100}, EnemyType::SLIME);
        const auto spawned_enemy2 = createEnemy(renderer, {0, 100}, EnemyType::SLIME);
        // when calling createEnemy later the enemy component values break

        enemies.emplace(debug_enemy);
        auto &enemy = registry.enemies.get(debug_enemy);
        enemy.speed = 100.f;

        enemies.emplace(debug_enemy2);
        Enemy &enemy2 = registry.enemies.get(debug_enemy2);
        enemy2.speed = 100.f;
        enemy2.spawn_time = 1000;
        enemy2.addHealth(120);
        enemy2.setPlayerDamage(5);


        enemies.emplace(spawned_enemy1);
        Enemy &spawn1 = registry.enemies.get(spawned_enemy1);
        spawn1.speed = 100.f;
        spawn1.addHealth(100);
        enemy2.spawns_enemies.push_back(spawned_enemy1);

        enemies.emplace(spawned_enemy2);
        Enemy &spawn2 = registry.enemies.get(spawned_enemy2);
        spawn2.speed = 120.f;
        spawn2.addHealth(100);
        enemy2.spawns_enemies.push_back(spawned_enemy2);

        return {debug_enemy, debug_enemy2};
    }
    if (difficulty >= 1) {
        float spawn_time = 0;
        for (uint i = 0; i <= wave_amount; ++i) {
            vec4 wave = combat_pool[uniform_int_dist(rng)];
            spawn_time += wave[2];
            for (int j = 0; j < wave[1]; ++j) {
                const Entity new_enemy = createEnemy(renderer, {0, 100}, static_cast<EnemyType>(wave[0]));
                std::vector<Entity> spawns_enemies = {};
                if (static_cast<EnemyType>(wave[0]) == EnemyType::SLIME_BIG) {
                    registry.enemies.get(new_enemy).addHealth(static_cast<int>(std::floor(difficulty/2) * 20));
                    registry.enemies.get(new_enemy).setPlayerDamage(5);
                    const auto spawned_enemy1 = createEnemy(renderer, {0, 100}, EnemyType::SLIME);
                    const auto spawned_enemy2 = createEnemy(renderer, {0, 100}, EnemyType::SLIME);
                    enemies.emplace(spawned_enemy1);
                    Enemy &spawn1 = registry.enemies.get(spawned_enemy1);
                    spawn1.addHealth(100 + static_cast<int>(std::floor(difficulty/2) * 50));
                    spawn1.speed = 100.f;
                    spawns_enemies.push_back(spawned_enemy1);
                    enemies.emplace(spawned_enemy2);
                    Enemy &spawn2 = registry.enemies.get(spawned_enemy2);
                    spawn2.addHealth(100 + static_cast<int>(std::floor(difficulty/2) * 50));
                    spawn2.speed = 120.f;
                    spawns_enemies.push_back(spawned_enemy2);
                }
                enemies.emplace(new_enemy);
                Enemy& enemy = registry.enemies.get(new_enemy);
                enemy.addHealth(100 + static_cast<int>(std::floor(difficulty/2) * 50));
                enemy.speed = wave[3] + wave[3] * (difficulty / 10);
                enemy.spawn_time = spawn_time;
                for (auto spawn_entity : spawns_enemies) {
                    enemy.spawns_enemies.push_back(spawn_entity);
                }

                enemy_list.push_back(new_enemy);

                spawn_time += wave[2];
            }
        }
    } else {
        for (int i = 0; i < difficulty+1; ++i) {
            const Entity new_enemy = createEnemy(renderer, {0, 100}, EnemyType::SLIME);
            enemies.emplace(new_enemy);
            Enemy& enemy = registry.enemies.get(new_enemy);
            enemy.addHealth(static_cast<int>(std::floor(difficulty/2) * 50));
            enemy.speed = 100.f + 5. * difficulty + i * 5;
            enemy.spawn_time = i * 1000.;
            enemy_list.push_back(new_enemy);
        }
        //return enemy_list;
    }

    //TODO: add animation timer for every different enemy speed in enemies to player entity
    //std::set<float> enemy_speeds;
    //auto &current_player = registry.players.get(player);
    //for (const auto enemy_entity : enemies) {
    //    const auto enemy = registry.enemies.get(enemy_entity);
    //    if (!current_player.animation_timers.contains(enemy.speed)) {
    //        const Entity timer_entity = Entity();
    //        auto timer = registry.enemyWalkTimers.emplace(timer_entity);
    //        timer.time = 1000.f * 100.f / enemy.speed;
    //        registry.players.get(player).animation_timers.emplace(enemy.speed, timer_entity);
    //    }
    //}

    return enemy_list;
}

Entity TDSystem::generate_map(const int difficulty) const {
    //TODO: maps accessible for different difficulty levels some maps more likely for difficulty to be in pool,
    // maybe do normal distr curve around difficulty?
    const auto difficulty_to_path_length = std::vector({20, 18, 16, 14, 12, 10, 8, 6});

    const auto generated_path = generateMapCheckpoints(rng, difficulty_to_path_length.at(difficulty));
    const Entity new_map = createMap(renderer, generated_path, rng, uniform_dist);
    createPathVisualization(renderer, registry.maps.get(new_map).checkpoints);
    return  new_map;
}

void TDSystem::restart_td_fight() {
    // Debugging for memory/component leaks
    registry.list_all_components();
    printf("Restarting TD fight\n");

    cleanup_ecs();

    // Debugging for memory/component leaks
    registry.list_all_components();

    current_phase = GamePhase::SETUP;

    Player& current_player = registry.players.get(player);
    int difficulty = current_player.won_battles; //TODO: actual difficulty calculation here later

    const Entity debug_map = generate_map(difficulty);
    map = debug_map;
    cleanup_entities.push_back(map);
    Map &current_map = registry.maps.get(debug_map);
    current_map.active = true;



    current_map.enemies = generate_combat(difficulty);

    printf("Created active map\n");
    printf("Mapcount: %lu\n", registry.maps.size());

    for (const Entity itemEntity: registry.items.entities) {
        createCardFromItem(renderer, itemEntity);
        cards.push_back(itemEntity);
        if (registry.towers.has(itemEntity)) {
            Tower& tower = registry.towers.get(itemEntity);
            if (tower.food_cost > current_player.getFood()) {
                registry.cards.get(itemEntity).selectable = false;
                registry.colors.insert(itemEntity, {0.5f, 0.5f, 0.5f, 1.f});
            }
        }
    }

    const Entity text = createText(renderer, {8, window_height_px - 10}, {16, 20}, "Hold 'T' to show the Tutorial", FontType::SQUARE);
    cleanup_entities.push_back(text);

    const Entity card_background = createBlackSquare(renderer, {window_width_px / 2, CARD_AXIS_HEIGHT}, {window_width_px, CARD_HEIGHT}, 0.75);
    cleanup_entities.push_back(card_background);

    const Entity button = createRoundStartButton(renderer, {TILE_WIDTH * (MAP_COUNT_X - 1) - TILE_WIDTH / 4, TILE_HEIGHT / 4});
    cleanup_entities.push_back(button);
    start_button = button;

    registry.list_all_components();
}

void TDSystem::handle_enemy_death(const Entity enemy_entity, Enemy &enemy) {
    assert(registry.enemies.has(enemy_entity) && "Entity not an enemy");
    enemy.alive = false;
    const auto enemy_motion = registry.motions.get(enemy_entity);
    for (const auto spawn_entity : enemy.spawns_enemies) {
        auto &spawn_enemy = registry.enemies.get(spawn_entity);
        auto &spawn_motion = registry.motions.get(spawn_entity);
        if (registry.slimes.has(spawn_entity)) {
            spawn_motion.position = enemy_motion.position;
            spawn_motion.angle = enemy_motion.angle;
            spawn_enemy.enemy_progress = enemy.enemy_progress;
            spawn_enemy.next_checkpoint = enemy.next_checkpoint;
            spawn_enemy.section_progress = enemy.section_progress;
            spawn_enemy.spawned = true;
            registry.invisibles.remove(spawn_entity);
            auto &walk_timer = registry.enemyWalkTimers.emplace(spawn_entity);
            walk_timer.time = walk_timer.start_time * 100.f / spawn_enemy.speed; // scale original time of walkTimer to enemy speed
        }
    }
    // clear tower aiming
    auto &aimingRegistry = registry.aimingAts;
    for (const Entity &aiming: aimingRegistry.entities) {
        if (aimingRegistry.get(aiming).aimed_entity == enemy_entity) {
            aimingRegistry.remove(aiming);
        }
    }
    // clear hitbox visualizations
    auto &hitbox_visualization = registry.hitboxVisualizations.get(enemy_entity);
    for (const auto &poly_lines : hitbox_visualization.lines) {
        for (const auto &line_entity : poly_lines) {
            registry.remove_all_components_of(line_entity);
        }
    }
    registry.remove_all_components_of(enemy_entity);
    enemies.erase(enemy_entity);
}

void TDSystem::handle_consumable_destruction(Entity consumable_entity) {
    if (registry.barriers.has(consumable_entity) || registry.spikes.has(consumable_entity)) {
        // clear hitbox visualizations
        auto &hitbox_visualization = registry.hitboxVisualizations.get(consumable_entity);
        for (const auto &poly_lines: hitbox_visualization.lines) {
            for (const auto &line_entity: poly_lines) {
                registry.remove_all_components_of(line_entity);
            }
        }
    }
    registry.remove_all_components_of(consumable_entity);
    consumables.erase(consumable_entity);
}

void TDSystem::handle_collision(const Entity first, const Entity second) {
    if (registry.enemies.has(second) && registry.arrows.has(first)) {
        auto &enemy = registry.enemies.get(second);
        auto &arrow = registry.arrows.get(first);
        if (arrow.hit_entities.contains(second)) {
            // Arrow has already hit that enemy
            return;
        }
        enemy.addDamage(arrow.damage);
        arrow.hit_entities.emplace(second);
        if (arrow.max_hitcount <= arrow.hit_entities.size()) {
            // clear hitbox visualizations
            auto &hitbox_visualization = registry.hitboxVisualizations.get(first);
            for (const auto &poly_lines: hitbox_visualization.lines) {
                for (const auto &line_entity: poly_lines) {
                    registry.remove_all_components_of(line_entity);
                }
            }
            registry.remove_all_components_of(first);
        }
    } else if (registry.enemies.has(second) && registry.swords.has(first)) {
        auto &sword = registry.swords.get(first);
        if (!sword.has_collision) {
            //Sword is currently on cooldown
            return;
        }
        if (sword.hit_entities.contains(second)) {
            // Sword has already hit that enemy
            return;
        }
        auto &enemy = registry.enemies.get(second);
        enemy.addDamage(sword.damage);
        sword.hit_entities.emplace(second);
    } else if (registry.towers.has(first) && registry.enemies.has(second)) {
        auto &tower = registry.towers.get(first);
        if (registry.aimingAts.has(first)) {
            auto &aiming = registry.aimingAts.get(first);
            auto &aimedEnemy = registry.enemies.get(aiming.aimed_entity);
            auto &otherEnemy = registry.enemies.get(second);
            switch (tower.priority) {
                case EnemyPriority::FIRST:
                    if (aimedEnemy.enemy_progress < otherEnemy.enemy_progress) {
                        aiming.aimed_entity = second;
                    }
                    break;
                case EnemyPriority::LAST:
                    if (aimedEnemy.enemy_progress > otherEnemy.enemy_progress) {
                        aiming.aimed_entity = second;
                    }
                    break;
            }
        } else {
            auto &aimingAt = registry.aimingAts.emplace(first);
            aimingAt.aimed_entity = second;
        }
    } else if (registry.bombs.has(first) && registry.enemies.has(second)) {
        auto &bomb = registry.bombs.get(first);
        if (bomb.exploding) {
            if (bomb.hit_entities.contains(second)){
                return;
            }
            auto &enemy = registry.enemies.get(second);
            enemy.addDamage(bomb.damage);
            bomb.hit_entities.emplace(second);
        }
    } else if (registry.spikes.has(first) && registry.enemies.has(second)) {
        auto &spike = registry.spikes.get(first);
        auto &enemy = registry.enemies.get(second);

        if (spike.hit_entities.contains(second)) {
            // spike has already hit that enemy
            return;
        }
        enemy.addDamage(spike.damage);
        spike.hit_entities.emplace(second);
        // delete spike if the amount of enemies has been reached
        if (spike.max_hitcount <= spike.hit_entities.size()) {
            handle_consumable_destruction(first);
        }
    } else if (registry.barriers.has(first) && registry.enemies.has(second)) {
        auto &barrier = registry.barriers.get(first);
        //TODO: emplace timer again if not there
        if (!registry.sloweds.has(second) && barrier.health > 0) {
            auto &slowed = registry.sloweds.emplace(second);
            slowed.origin = first;
        }
        if (!registry.barrierTimers.has(first)) {
            registry.barrierTimers.emplace(first);
        }
    }
}

void TDSystem::handle_aiming() const {
    if (current_phase != GamePhase::RUNNING)
        return;
    const auto &aimingRegistry = registry.aimingAts;
    for (uint i = 0; i < aimingRegistry.entities.size(); i++) {
        const Entity tower_entity = aimingRegistry.entities[i];
        const Entity enemy_entity = aimingRegistry.components[i].aimed_entity;
        if (registry.enemies.get(enemy_entity).spawned) {
            auto &tower_motion = registry.motions.get(tower_entity);
            auto &enemy_motion = registry.motions.get(enemy_entity);
            auto d_p = tower_motion.position - enemy_motion.position;
            TowerType towerType;
            if (registry.archers.has(tower_entity)) {
                towerType = TowerType::ARCHER;
            } else if (registry.knights.has(tower_entity)) {
                towerType = TowerType::KNIGHT;
            } else {
                towerType = TowerType::TOWER_TYPE_COUNT;
                assert(false && "Tower type not found");
            }

            if (towerType == TowerType::ARCHER) {
                Enemy &enemy = registry.enemies.get(enemy_entity);
                const Archer &archer = registry.archers.get(tower_entity);
                const Map &map = registry.maps.get(this->map);
                const float arrow_fly_time = length(d_p) / archer.arrow_speed;
                // estimation as the enemy also moves during this time TODO: think about considering enemy speed
                d_p = tower_motion.position -
                      PhysicsSystem::calculate_enemy_position(enemy, enemy_entity, map, arrow_fly_time, false);
            }
            const float angle = atan2(d_p.y, d_p.x);
            tower_motion.angle = angle;
            if (towerType == TowerType::ARCHER) {
                auto &bow = registry.archers.get(tower_entity).bow;
                auto &bow_motion = registry.motions.get(bow);
                bow_motion.angle = angle - M_PI_2 - M_PI_2 / 2; //+ (2 * M_PI) - (M_PI_2/2);
                //keep angle within [-pi, pi]
                while (bow_motion.angle < -M_PI) {
                    bow_motion.angle += 2 * M_PI;
                }
                while (bow_motion.angle > M_PI) {
                    bow_motion.angle -= 2 * M_PI;
                }
            }

            if (!registry.shotTimers.has(tower_entity)) {
                if (towerType == TowerType::ARCHER) {
                    const auto &archer = registry.archers.get(tower_entity);
                    registry.shotTimers.emplace(tower_entity);
                    createArrow(renderer, tower_motion.position, archer.arrow_speed, d_p);
                } else if (towerType == TowerType::KNIGHT) {
                    const auto &knight = registry.knights.get(tower_entity);
                    ShotTimer &swing_timer = registry.shotTimers.emplace(tower_entity);
                    swing_timer.time = knight.cooldown;
                }
            }
        }
    }
    // Clear aiming for next iteration
    registry.aimingAts.clear();
}

bool TDSystem::is_over() const {
    return current_phase == GamePhase::ENDED;
}

void TDSystem::on_key(const int key, int, const int action, const int mods) {
    switch (key) {
        case GLFW_KEY_C: {
            Map &map_component = registry.maps.get(map);
            if (action == GLFW_RELEASE && current_phase == GamePhase::SETUP) {
                realignCards();
                current_phase = GamePhase::RUNNING;
                map_component.combat_time = 0.f;
            }
            break;
        }
        case GLFW_KEY_T: {
            if (action == GLFW_PRESS) {
                tutorial_background = createBlackSquare(renderer, tutorial_pos + vec2{495, 130}, {1020, 255}, 0.75f);
                tutorial_text = createText(renderer, tutorial_pos, {10, 20}, tutorial_string.data(), FontType::SQUARE);
                cleanup_entities.push_back(tutorial_text);
                cleanup_entities.push_back(tutorial_background);
            } else if (action == GLFW_RELEASE) {
                registry.remove_all_components_of(tutorial_text);
                registry.remove_all_components_of(tutorial_background);
                std::erase(cleanup_entities, tutorial_text);
                std::erase(cleanup_entities, tutorial_background);
            }
            break;
        }
        case GLFW_KEY_M:
            if (action == GLFW_RELEASE) {
                for (auto &enemy_entity : enemies) {
                    if (registry.enemies.get(enemy_entity).spawned) {
                        auto &hitbox_visualization = registry.hitboxVisualizations.get(enemy_entity);
                        for (auto &line_entity: hitbox_visualization.lines[hitbox_visualization.active_poly]) {
                            if (!registry.invisibles.has(line_entity)) {
                                registry.invisibles.emplace(line_entity);
                            }
                        }
                    }

                }
                for (auto &consumable_entity : consumables) {
                    if (registry.barriers.has(consumable_entity) || registry.spikes.has(consumable_entity)) {
                        if (registry.consumables.get(consumable_entity).placed) {
                            auto &hitbox_visualization = registry.hitboxVisualizations.get(consumable_entity);
                            for (auto &line_entity: hitbox_visualization.lines[hitbox_visualization.active_poly]) {
                                if (!registry.invisibles.has(line_entity)) {
                                    registry.invisibles.emplace(line_entity);
                                }
                            }
                        }
                    }
                }
                for (auto &sword_entity : registry.swords.entities) {
                    auto &hitbox_visualization = registry.hitboxVisualizations.get(sword_entity);
                    for (auto &line_entity: hitbox_visualization.lines[hitbox_visualization.active_poly]) {
                        if (!registry.invisibles.has(line_entity)) {
                            registry.invisibles.emplace(line_entity);
                        }
                    }
                }
                for (auto &arrow_entity : registry.arrows.entities) {
                    auto &hitbox_visualization = registry.hitboxVisualizations.get(arrow_entity);
                    for (auto &line_entity: hitbox_visualization.lines[hitbox_visualization.active_poly]) {
                        if (!registry.invisibles.has(line_entity)) {
                            registry.invisibles.emplace(line_entity);
                        }
                    }
                }
                for (auto tile_entity : registry.pathVisualizations.entities) {
                    if (!registry.invisibles.has(tile_entity)) {
                        registry.invisibles.emplace(tile_entity);
                    }
                }
                for (auto tile_entity : registry.towerVisualizations.entities) {
                    if (!registry.invisibles.has(tile_entity)) {
                        registry.invisibles.emplace(tile_entity);
                    }
                }
            } else { // GLFW_PRESS and on windows also GLFW_REPEAT
                for (auto &enemy_entity : enemies) {
                    if (registry.enemies.get(enemy_entity).spawned) {
                        auto &hitbox_visualization = registry.hitboxVisualizations.get(enemy_entity);
                        for (auto &line_entity: hitbox_visualization.lines[hitbox_visualization.active_poly]) {
                            registry.invisibles.remove(line_entity);
                        }
                    }

                }
                for (auto &consumable_entity : consumables) {
                    if (registry.barriers.has(consumable_entity) || registry.spikes.has(consumable_entity)) {
                        if (registry.consumables.get(consumable_entity).placed) {
                            auto &hitbox_visualization = registry.hitboxVisualizations.get(consumable_entity);
                            for (auto &line_entity: hitbox_visualization.lines[hitbox_visualization.active_poly]) {
                                registry.invisibles.remove(line_entity);
                            }
                        }
                    }
                }
                for (auto &sword_entity : registry.swords.entities) {
                    auto &hitbox_visualization = registry.hitboxVisualizations.get(sword_entity);
                    if (registry.swords.get(sword_entity).has_collision) {
                        for (auto &line_entity: hitbox_visualization.lines[hitbox_visualization.active_poly]) {
                            registry.invisibles.remove(line_entity);
                        }
                    } else {
                        for (auto &line_entity: hitbox_visualization.lines[hitbox_visualization.active_poly]) {
                            if (!registry.invisibles.has(line_entity)) {
                                registry.invisibles.emplace(line_entity);
                            }
                        }
                    }
                }
                for (auto &arrow_entity : registry.arrows.entities) {
                    auto &hitbox_visualization = registry.hitboxVisualizations.get(arrow_entity);
                    for (auto &line_entity: hitbox_visualization.lines[hitbox_visualization.active_poly]) {
                        registry.invisibles.remove(line_entity);
                    }
                }
                for (auto tile_entity : registry.pathVisualizations.entities) {
                    registry.invisibles.remove(tile_entity);
                }
                for (auto tile_entity : registry.towerVisualizations.entities) {
                    registry.invisibles.remove(tile_entity);
                }
            }
            break;
        case GLFW_KEY_R: {
            if (action == GLFW_PRESS) {
                current_phase = GamePhase::ENDED;
            }
            break;
        }
        default: {
        }
    }
}

void TDSystem::on_mouse_move(const vec2 pos, GLFWwindow *window) {
    // TODO fight specific mouse handling

    if (current_phase == GamePhase::SETUP) {
        registry.clickables.clear();
        const auto& card_pos = registry.stationaries.get(start_button);
        const vec2 dp = card_pos.position - pos;
        const float dist_squared = dot(dp, dp);
        vec2 bounding_box = {abs(card_pos.scale.x), abs(card_pos.scale.y)};
        bounding_box *= 0.3f;
        const float start_squared = dot(bounding_box, bounding_box);
        if (dist_squared < start_squared) {
            registry.clickables.emplace(start_button);
        }
    }

    if (current_phase == GamePhase::SETUP || current_phase == GamePhase::RUNNING) {
        if (dragging) {
            if (registry.cards.has(dragged_entity)) {
                registry.stationaries.get(dragged_entity).position = pos;
                registry.stationaries.get(placement_marker).position = pos;
                //registry.stationaries.get(dragged_entity).scale = vec2(0.5 * CARD_WIDTH, 0.5 * CARD_HEIGHT);
            }
        } else if (pos[1] > (CARD_AXIS_HEIGHT - CARD_HEIGHT / 2) && pos[1] < (CARD_AXIS_HEIGHT + CARD_HEIGHT / 2) &&
                   pos[0] < CARD_AXIS_WIDTH) {
            auto &cardRegistry = registry.cards;
            auto card_count = cardRegistry.entities.size();
            float card_offset = CARD_AXIS_WIDTH / (static_cast<float>(card_count) + 1);
            float x_pos_percent =
                    (pos[0] - card_offset / 2) / CARD_AXIS_WIDTH; // offset selection area to middle of card
            // generate 1 more selection area since card positions are aligned with card_count+1/CARD_AXIS_WIDTH
            auto selected_card_id = static_cast<unsigned int>(std::floor(x_pos_percent * (card_count + 1)));
            //if(selected_card_id<card_count) {
            for (std::size_t i = 0; i < card_count; ++i) {
                Entity card_entity = cardRegistry.entities[i];
                if (i == selected_card_id && registry.cards.get(card_entity).selectable) {
                    registry.stationaries.get(card_entity).scale = vec2(1.3 * CARD_WIDTH, 1.3 * CARD_HEIGHT);
                    cardRegistry.components[i].selected = true;
                } else {
                    registry.stationaries.get(card_entity).scale = vec2(CARD_WIDTH, CARD_HEIGHT);
                    cardRegistry.components[i].selected = false;
                }
            }
        } else { // unselect all cards
            auto &cardRegistry = registry.cards;
            auto card_count = cardRegistry.entities.size();
            for (std::size_t i = 0; i < card_count; ++i) {
                Entity card_entity = cardRegistry.entities[i];
                registry.stationaries.get(card_entity).scale = vec2(CARD_WIDTH, CARD_HEIGHT);
                cardRegistry.components[i].selected = false;
            }
        }
    } else if (current_phase == GamePhase::CHOOSE_REWARD) {
        registry.clickables.clear();

        for (const Entity card : new_cards) {
            auto& card_pos = registry.stationaries.get(card);
            const vec2 dp = card_pos.position - pos;
            const float dist_squared = dot(dp, dp);
            vec2 bounding_box = {abs(card_pos.scale.x), abs(card_pos.scale.y)};
            bounding_box *= 0.3f;
            const float card_squared = dot(bounding_box, bounding_box);
            if (dist_squared < card_squared) {
                card_pos.scale = vec2(1.3 * CARD_WIDTH, 1.3 * CARD_HEIGHT);
                registry.clickables.emplace(card);
            } else {
                card_pos.scale = vec2(CARD_WIDTH, CARD_HEIGHT);
            }
        }
    }
}

void TDSystem::on_mouse_button(int button, int action, int mods, GLFWwindow *window) {
    printf("mouse button\n");

    if (current_phase == GamePhase::SETUP) {
        if (registry.clickables.has(start_button) && action == GLFW_PRESS) {
            printf("Button Clicked!\n");
            registry.renderForeground.get(start_button).atlas_ids = {static_cast<uint>(BUTTONS::START_DOWN)};
        } else if (action == GLFW_RELEASE) {
            auto& buttonRenderRequest = registry.renderForeground.get(start_button);
            if (buttonRenderRequest.atlas_ids.at(0) == static_cast<uint>(BUTTONS::START_DOWN)) {
                buttonRenderRequest.atlas_ids = {static_cast<uint>(BUTTONS::START_UP)};
            }
            if (registry.clickables.has(start_button)) {
                current_phase = GamePhase::RUNNING;
                registry.colors.insert(start_button, {0.5f, 0.5f, 0.5f, 1.0f});
            }
        }
    }

    if (current_phase == GamePhase::SETUP || current_phase == GamePhase::RUNNING) {
        if (dragging) { //TODO: combat ending when card is being dragged destroys a lot
            //double mouse_x;
            //double mouse_y;
            //glfwGetCursorPos(window, &mouse_x, &mouse_y);

            if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
                dragging = false;
                registry.cards.get(dragged_entity).dragged = false;
                registry.invisibles.remove(dragged_entity);
                if (!registry.invisibles.has(placement_marker)) {
                    registry.invisibles.emplace(placement_marker);
                }
                realignCards();
            }
            //if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) { //TODO move this to on_mouse_move
            //    if (registry.cards.has(dragged_entity)) {
            //        registry.stationaries.get(dragged_entity).position = vec2(mouse_x, mouse_y);
            //        registry.stationaries.get(dragged_entity).scale = vec2(200.f, 200.f);
            //    }
            //} else
            if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
                //TODO range of placed tower seems really small, maybe only on collision with tower
                dragging = false;
                auto &maps = registry.maps;
                const auto &mapProperties = maps.get(map);
                vec2 card_pos = registry.stationaries.get(dragged_entity).position;
                registry.invisibles.remove(dragged_entity);
                if (!registry.invisibles.has(placement_marker)) {
                    registry.invisibles.emplace(placement_marker);
                }

                // block placement on other towers
                bool place_occupied = false;
                constexpr float tower_blocked_radius = TILE_HEIGHT;
                //abs(distance(vec2(0, 0), vec2(TOWER_WIDTH, TOWER_HEIGHT)));
                for (std::size_t tower_index = 0; tower_index < registry.towers.size(); ++tower_index) {
                    if (!registry.towers.components[tower_index].placed) {
                        continue;
                    }
                    const Entity tower_entity = registry.towers.entities[tower_index];
                    printf("dist to tower %f", distance(registry.motions.get(tower_entity).position, card_pos));
                    if (abs(distance(registry.motions.get(tower_entity).position, card_pos)) < tower_blocked_radius) {
                        place_occupied = true;
                    }
                }

                // block placement on enemy path
                float path_blocked_radius = TOWER_WIDTH / 1.5f;//window_height_px * 0.05;
                if (abs(distance(mapProperties.checkpoints[0], card_pos)) < path_blocked_radius) {
                    place_occupied = true;
                }
                for (std::size_t i = 1; i < mapProperties.checkpoints.size(); i++) {
                    vec2 prev_checkpoint = mapProperties.checkpoints[i - 1];
                    vec2 curr_checkpoint = mapProperties.checkpoints[i];
                    if (abs(distance(mapProperties.checkpoints[i], card_pos)) < path_blocked_radius) {
                        place_occupied = true;
                        break;
                    }
                    vec2 vec_prev = prev_checkpoint - card_pos;
                    vec2 vec_curr = curr_checkpoint - card_pos;
                    vec2 vec_prod = vec_prev * vec_curr;
                    if (!(vec_prod[0] > 0 && vec_prod[1] > 0)) {
                        // cursor between prev and curr checkpoint
                        float d = abs((curr_checkpoint[1] - prev_checkpoint[1]) * card_pos[0]
                                      - (curr_checkpoint[0] - prev_checkpoint[0]) * card_pos[1]
                                      + curr_checkpoint[0] * prev_checkpoint[1]
                                      - curr_checkpoint[1] * prev_checkpoint[0]) /
                                  distance(prev_checkpoint, curr_checkpoint);
                        if (d < path_blocked_radius) {
                            place_occupied = true;
                        }
                    }
                    //printf("%f %f, %f %f, %f %f\n", a[0], a[1], b[0], b[1], ab[0], ab[1]);
                    //printf("%f %f\n", distance(prev_checkpoint, vec2 (mouse_x, mouse_y)), distance(curr_checkpoint, vec2 (mouse_x, mouse_y)));
                }
                if ((card_pos[1] > (CARD_AXIS_HEIGHT - CARD_HEIGHT / 2) &&
                     card_pos[1] < (CARD_AXIS_HEIGHT + CARD_HEIGHT / 2) &&
                     card_pos[0] < CARD_AXIS_WIDTH)) {
                    registry.cards.get(dragged_entity).dragged = false;
                } else if (place_occupied) {
                    if (registry.consumables.has(dragged_entity)) {
                        // successfully place card as bomb
                        std::erase(cards, dragged_entity); // C++20 is nice
                        createConsumableFromCard(renderer, dragged_entity, player, consumables);
                        //auto consumable_motion = registry.motions.get(dragged_entity);
                    } else {
                        registry.cards.get(dragged_entity).dragged = false;
                    }
                } else {
                    // successfully place card as tower or bomb
                    std::erase(cards, dragged_entity); // C++20 is nice
                    if (registry.towers.has(dragged_entity)) {
                        createTowerFromCard(renderer, dragged_entity);
                        auto &dragged_tower = registry.towers.get(dragged_entity);
                        //auto tower_motion = registry.motions.get(dragged_entity);
                        towers.emplace_back(dragged_entity);
                        dragged_tower.placed = true;
                        // subtract food cost
                        auto &current_player = registry.players.get(player);
                        current_player.updateFood(current_player.getFood() - dragged_tower.food_cost);
                        // recalculate cards that can be placed
                        for (const Entity card: cards) {
                            if (registry.towers.has(card)) {
                                auto &tower = registry.towers.get(card);
                                auto &card_comp = registry.cards.get(card);
                                if (card_comp.selectable && tower.food_cost > current_player.getFood()) {
                                    card_comp.selectable = false;
                                    registry.colors.insert(card, {0.5f, 0.5f, 0.5f, 1.f});
                                }
                            }
                        }
                    } else if (registry.consumables.has(dragged_entity)) {
                        createConsumableFromCard(renderer, dragged_entity, player, consumables);
                        //auto consumable_motion = registry.motions.get(dragged_entity);
                    }
                }
                realignCards();
            }
        } else {
            //TODO: move start of dragging here
            if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
                for (uint i = 0; i < registry.cards.entities.size(); i++) {
                    if (registry.cards.components[i].selected) {
                        registry.cards.components[i].dragged = true;
                        dragged_entity = registry.cards.entities[i];
                        dragging = true;
                        printf("z:%f\n", registry.renderForeground.get(dragged_entity).z_position);
                        if (!registry.healthPotions.has(dragged_entity)) {
                            registry.invisibles.emplace(dragged_entity);
                            registry.stationaries.get(placement_marker).position = registry.stationaries.get(
                                    dragged_entity).position;
                            float marker_scale = 2 * TOWER_WIDTH;
                            if (registry.towers.has(dragged_entity)) {
                                marker_scale = 2 * registry.towers.get(dragged_entity).range;
                            } else if (registry.consumables.has(dragged_entity)) {
                                marker_scale = 2 * registry.consumables.get(dragged_entity).range;
                            }
                            registry.stationaries.get(placement_marker).scale = vec2(marker_scale, marker_scale);
                            registry.invisibles.remove(placement_marker);
                        }
                    }
                }
            }
        }
    } else if (current_phase == GamePhase::CHOOSE_REWARD) {
        for (const Entity entity : registry.clickables.entities) {
            cards.push_back(entity);
            std::erase(new_cards, entity);
            current_phase = GamePhase::ENDED;
        }
    }
}

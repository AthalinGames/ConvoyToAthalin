//
// Created by joris on 14.11.24.
//

#include "td_system.hpp"

#include "physics_system.hpp"
#include "world_system.hpp"

TDSystem::TDSystem(const unsigned int seed) : dragging(false) {
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
    for (const auto weapon : registry.weapons.entities) {
        registry.remove_all_components_of(weapon);
    }
    registry.remove_all_components_of(map);
}

void TDSystem::init(RenderSystem *renderer, Entity player) {
    this->renderer = renderer;
    this->player = player;

    // Set all states to default
    restart_td_fight();
}

bool TDSystem::step(float elapsed_ms) {
    assert(registry.screenStates.components.size() <= 1);
    ScreenState &screen = registry.screenStates.components[0];
    Map& td_map = registry.maps.get(map);

    float min_timer_ms = 4000.f;
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
            running = false;
            return true;
        }
    }
    // reduce window brightness if any of the present salmons is dying
    screen.screen_darken_factor = 1 - min_timer_ms / 4000;

    if(td_map.combat_started) {
        td_map.combat_time += elapsed_ms;
        for (const auto tower_entity: registry.shotTimers.entities) {
            auto &shot_timer = registry.shotTimers.get(tower_entity);
            shot_timer.time -= elapsed_ms;

            if (shot_timer.time < 0) {
                registry.shotTimers.remove(tower_entity);

            }
            if (registry.archers.has(tower_entity)) {
                const auto &bow_entity = registry.archers.get(tower_entity).bow;
                RenderRequest& render_request = registry.renderRequests.get(bow_entity);
                if (RenderRequestSingle *single_request = std::get_if<RenderRequestSingle>(&render_request)) {
                    if (shot_timer.time < 0) {
                        //change bow to empty
                        single_request->used_texture = TEXTURE_ASSET_ID::BOW3;
                    } else if (shot_timer.time < 150.) {
                        //change bow to drawn
                        single_request->used_texture = TEXTURE_ASSET_ID::BOW2;
                    } else if (shot_timer.time < 500.) {
                        //change bow to loaded
                        single_request->used_texture = TEXTURE_ASSET_ID::BOW1;
                    }
                }
            }

        }
        if(!td_map.enemies.empty()){
            Enemy& next_enemy = registry.enemies.get(td_map.enemies[0]);
            if(td_map.combat_time > next_enemy.spawn_time) {
                next_enemy.spawned = true;
                registry.invisibles.remove(td_map.enemies[0]);
                td_map.enemies.erase(td_map.enemies.begin());
            }
        }
    }

    // Check if enemy completed Path
    for (std::size_t i = 0; i < registry.enemies.size(); ++i) {
        const auto& enemy = registry.enemies.components[i];
        const Entity enemy_entity = registry.enemies.entities[i];
        if (enemy.enemy_progress >= 1.0f && enemy.alive) {
            for (Player &player : registry.players.components) {
                player.health -= enemy.damage;
            }
            // Delete damaging entity
            registry.remove_all_components_of(enemy_entity);
            enemies.erase(enemy_entity);
        }
    }
    // Check if player still has health
    for (std::size_t i = 0; i < registry.players.size(); ++i) {
        const auto& player = registry.players.components[i];
        const Entity player_entity = registry.players.entities[i];
        if (player.health <= 0 && !registry.deathTimers.has(player_entity)) {
            createGameOver(renderer);
            registry.deathTimers.emplace(player_entity);
        }
    }
    
    return true;
}

std::vector<Entity> TDSystem::generate_combat(int difficulty) {
    if (difficulty == 0) {
        const auto debug_enemy = createEnemy(renderer, {0, 100});
        enemies.emplace(debug_enemy);
        Enemy &enemy = registry.enemies.get(debug_enemy);
        enemy.speed = 100.f;

        const auto debug_enemy2 = createEnemy(renderer, {0, 100});
        enemies.emplace(debug_enemy2);
        Enemy &enemy2 = registry.enemies.get(debug_enemy2);
        enemy2.speed = 100.f;
        enemy2.spawn_time = 1000;

        return {debug_enemy, debug_enemy2};
    } else {
        std::vector<Entity> enemy_list = {};
        for (int i = 0; i < difficulty+1; ++i) {
            const Entity new_enemy = createEnemy(renderer, {0, 100});
            enemies.emplace(new_enemy);
            Enemy& enemy = registry.enemies.get(new_enemy);
            enemy.health += static_cast<int>(std::floor(difficulty/2) * 50);
            enemy.speed = 100.f + 5. * difficulty + i * 5;
            enemy.spawn_time = i * 1000.;
            enemy_list.push_back(new_enemy);
        }
        return enemy_list;
    }
}

std::vector<vec2> TDSystem::grid_to_coordinates(std::vector<vec2> grid_coords) {
    std::vector<vec2> map_coordinates = {};
    const int grid_width = 16;
    const int grid_height = 9;
    const float width_step = window_width_px/grid_width;
    const float height_step = window_height_px/grid_height;
    const float width_step_center = width_step/2;
    const float height_step_center = height_step/2;
    for (int i = 0; i < grid_coords.size(); ++i) {
        vec2 grid_coord = grid_coords[i];
        float x = grid_coord.x * width_step + width_step_center;
        float y = grid_coord.y * height_step + height_step_center;
        map_coordinates.emplace_back(x, y);
    }
    return map_coordinates;
}

Entity TDSystem::generate_map(int difficulty) {
    //TODO: maps accesible for different difficulty levels some maps more likely for difficulty to be in pool,
    // maybe do normal distr curve around difficulty?

    Entity new_map;
    if (difficulty == 0) {
        std::vector<vec2> path_coords = grid_to_coordinates({
            vec2(0, 2), vec2(7, 2), vec2(7, 5), vec2(13, 5)
        });
        new_map = createMap(renderer, path_coords, TEXTURE_ASSET_ID::MAP);
    }
    else if (difficulty == 1) {
        std::vector<vec2> path_coords = grid_to_coordinates({
            vec2(0, 6), vec2(7, 6), vec2(7, 3), vec2(12, 3), vec2(12, 5)
        });
        new_map = createMap(renderer, path_coords, TEXTURE_ASSET_ID::MAP2);
    }
    else if (difficulty == 2) {
        std::vector<vec2> path_coords = grid_to_coordinates({
            vec2(0, 6), vec2(2, 6), vec2(2, 0), vec2(14, 0), vec2(14, 6), vec2(6, 6), vec2(6, 2), vec2(11, 2), vec2(11, 4)});
        new_map = createMap(renderer, path_coords, TEXTURE_ASSET_ID::MAP3);
    }
    else if (difficulty == 3) {
        std::vector<vec2> path_coords = grid_to_coordinates({
            vec2(0, 3), vec2(13, 3), vec2(13, 0), vec2(8, 0), vec2(8, 6)
        });
        new_map = createMap(renderer, path_coords,TEXTURE_ASSET_ID::MAP4);
    }
    else if (difficulty == 4) {
        std::vector<vec2> path_coords = grid_to_coordinates({
            vec2(0, 5), vec2(2, 5), vec2(2, 1), vec2(6, 1), vec2(6, 6), vec2(9, 6), vec2(9, 3), vec2(12, 3)
        });
        new_map = createMap(renderer, path_coords, TEXTURE_ASSET_ID::MAP5);
    }
    else if (difficulty == 5) {
        std::vector<vec2> path_coords = grid_to_coordinates({
            vec2(15, 6), vec2(11, 6), vec2(11, 4), vec2(8, 4), vec2(8, 2), vec2(5, 2)
        });
        new_map = createMap(renderer, path_coords, TEXTURE_ASSET_ID::MAP6);
    }
    else {
        new_map = createMap(renderer,{
            vec2(0, 180),vec2(550, 180), vec2(550, 440), vec2(970, 440)}, //TODO percentage relative to window size
            TEXTURE_ASSET_ID::MAP);
    }
    return  new_map;
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
    for (const auto weapon : registry.weapons.entities) {
        registry.remove_all_components_of(weapon);
    }
    registry.remove_all_components_of(map);

    towers.clear();
    enemies.clear();
    cards.clear();

    // Debugging for memory/component leaks
    registry.list_all_components();

    Player& current_player = registry.players.get(player);
    int difficulty = current_player.won_battles; //TODO: actual difficulty calculation here later

    const Entity debug_map = generate_map(difficulty);
    map = debug_map;
    Map& current_map = registry.maps.get(debug_map);
    current_map.active = true;



    current_map.enemies = generate_combat(difficulty);

    printf("Created active map\n");
    printf("Mapcount: %lu\n", registry.maps.size());

    const Entity debug_card = createCard(renderer);
    cards.push_back(debug_card);

    const Entity debug_card2 = createCard(renderer);
    cards.push_back(debug_card2);

    for (int i = 0; i < 6; ++i) {
        const Entity debug_cards = createCard(renderer);
        cards.push_back(debug_cards);
    }

    createText(renderer, {8, window_height_px - 10}, {16, 20}, "Hold 'T' to show the Tutorial");

    registry.list_all_components();
}

void TDSystem::handle_collision(const Entity first, const Entity second) {
    if(registry.enemies.has(second) && registry.arrows.has(first)){
        auto& enemy = registry.enemies.get(second);
        auto& arrow = registry.arrows.get(first);
        enemy.health -= arrow.damage;
        if (arrow.hit_entities.count(second)) {
            // Arrow has already hit that enemy
            return;
        }
        arrow.hit_entities.emplace(second);
        if (enemy.health <= 0) {
            enemy.alive = false;
            // clear tower aiming
            auto& aimingRegistry = registry.aimingAts;
            for(const Entity& aiming : aimingRegistry.entities) {
                if (aimingRegistry.get(aiming).aimed_entity == second) {
                    aimingRegistry.remove(aiming);
                }
            }
            registry.remove_all_components_of(second);
        }
        // delete arrow if the amount of enemies has been reached
        if (arrow.max_hitcount <= arrow.hit_entities.size()) {
            registry.remove_all_components_of(first);
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
    const auto& aimingRegistry = registry.aimingAts;
    for (uint i = 0; i < aimingRegistry.entities.size(); i++) {
        const Entity tower_entity = aimingRegistry.entities[i];
        const Entity enemy_entity = aimingRegistry.components[i].aimed_entity;
        if(registry.enemies.get(enemy_entity).spawned) {
            auto &tower_motion = registry.motions.get(tower_entity);
            auto &enemy_motion = registry.motions.get(enemy_entity);
            auto d_p = tower_motion.position - enemy_motion.position;
            if (registry.archers.has(tower_entity)) {
                Enemy& enemy = registry.enemies.get(enemy_entity);
                const Archer& archer = registry.archers.get(tower_entity);
                const Map& map = registry.maps.get(this->map);
                const float arrow_fly_time = length(d_p) / archer.arrow_speed; // estimation as the enemy also moves during this time TODO: think about considering enemy speed
                d_p = tower_motion.position - PhysicsSystem::calculate_enemy_position(enemy, map, arrow_fly_time, false);
            }
            const float angle = atan2(d_p.y, d_p.x);
            tower_motion.angle = angle;
            if (registry.archers.has(tower_entity)) {
                auto &bow_motion = registry.motions.get(registry.archers.get(tower_entity).bow);
                bow_motion.angle = angle - M_PI_2 - M_PI_2/2; //+ (2 * M_PI) - (M_PI_2/2);
                if (bow_motion.angle < M_PI) { //keep angle within [-pi, pi]
                    bow_motion.angle += 2*M_PI;
                }
            }

            if (!registry.shotTimers.has(tower_entity)) {
                const auto &archer = registry.archers.get(tower_entity);
                registry.shotTimers.emplace(tower_entity);
                if (registry.archers.has(tower_entity)) {
                    createArrow(renderer, tower_motion.position, archer.arrow_speed, d_p);
                }
            }
        }
    }
    // Clear aiming for next iteration
    registry.aimingAts.clear();
}

bool TDSystem::is_over() const {
    return !running || (registry.enemies.components.empty() && registry.maps.get(map).enemies.empty());
}

void TDSystem::on_key(const int key, int, const int action, const int mods) {
    switch (key) {
        case GLFW_KEY_C: {
            Map& map_component = registry.maps.get(map);
            if (action == GLFW_RELEASE && !map_component.combat_started) {
                realignCards();
                map_component.combat_started = true;
                map_component.combat_time = 0.f;
            }
            break;
        }
        case GLFW_KEY_T: {
            if (action == GLFW_PRESS) {
                tutorial_text = createText(renderer, tutorial_pos, {10, 20}, tutorial_string.data());
            } else if (action == GLFW_RELEASE) {
                registry.remove_all_components_of(tutorial_text);
            }
            break;
        }
        default: {}
    }
}

void TDSystem::on_mouse_move(const vec2 pos, GLFWwindow* window) {
    // TODO fight specific mouse handling

    if (!registry.maps.get(map).combat_started) {
        if (dragging) {
            if (registry.cards.has(dragged_entity)) {
                registry.stationaries.get(dragged_entity).position = pos;
                registry.stationaries.get(dragged_entity).scale = vec2(0.5 * CARD_WIDTH, 0.5 * CARD_HEIGHT);
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
                if (i == selected_card_id) {
                    registry.stationaries.get(card_entity).scale = vec2(1.3 * CARD_WIDTH, 1.3 * CARD_HEIGHT);
                    cardRegistry.components[i].selected = true;
                } else {
                    registry.stationaries.get(card_entity).scale = vec2(CARD_WIDTH, CARD_HEIGHT);
                    cardRegistry.components[i].selected = false;
                }
            }
        }
    }
}

void TDSystem::on_mouse_button(int button, int action, int mods, GLFWwindow* window) {
    printf("mouse button\n");
    if (!registry.maps.get(map).combat_started) {
        if (dragging) {
            //double mouse_x;
            //double mouse_y;
            //glfwGetCursorPos(window, &mouse_x, &mouse_y);

            if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
                dragging = false;
                registry.cards.get(dragged_entity).dragged = false;
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
                auto maps = registry.maps;
                const auto &mapProperties = maps.get(map);
                vec2 card_pos = registry.stationaries.get(dragged_entity).position;

                // block placement on other towers
                bool place_occupied = false;
                float tower_blocked_radius = ARCHER_BB_HEIGHT;//abs(distance(vec2(0, 0), vec2(ARCHER_BB_WIDTH, ARCHER_BB_HEIGHT)));
                for (Entity &tower_entity: registry.towers.entities) {
                    if (abs(distance(registry.motions.get(tower_entity).position, card_pos)) < tower_blocked_radius) {
                        place_occupied = true;
                    }
                }

                // block placement on enemy path
                float path_blocked_radius = window_height_px * 0.05;
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
                    if (!(vec_prod[0] > 0 && vec_prod[1] > 0)) { // cursor between prev and curr checkpoint
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
                if (place_occupied || (card_pos[1] > (CARD_AXIS_HEIGHT - CARD_HEIGHT / 2) &&
                                       card_pos[1] < (CARD_AXIS_HEIGHT + CARD_HEIGHT / 2) &&
                                       card_pos[0] < CARD_AXIS_WIDTH)) {
                    registry.cards.get(dragged_entity).dragged = false;
                } else { // successfully place card as tower
                    registry.renderRequests.remove(dragged_entity, true);
                    registry.cards.remove(dragged_entity, true);
                    registry.remove_all_components_of(dragged_entity);
                    for (uint i = 0; i < cards.size(); i++) { // delete old card entity
                        if (cards[i] == dragged_entity) {
                            cards.erase(cards.cbegin() + i);
                        }
                    }
                    dragged_entity = createArcher(renderer,
                                                  card_pos); // TODO: for some reason the z position does still not work and only 1st placed archer is below card after next archer placed
                    towers.emplace_back(dragged_entity);
                    //registry.renderRequests.insert(dragged_entity, {
                    //        Z_MIDDLE,
                    //        TEXTURE_ASSET_ID::ARCHER,
                    //        EFFECT_ASSET_ID::TEXTURED,
                    //        GEOMETRY_BUFFER_ID::SPRITE,
                    //});
                    //auto &motion = registry.motions.emplace(dragged_entity);
                    //motion.position = card_pos;
                    //motion.angle = M_PI / 2;
                    //motion.velocity = vec2(0, 0);
                    //motion.scale = vec2({-ARCHER_BB_WIDTH, ARCHER_BB_HEIGHT});
                    //auto &tower = registry.towers.emplace(dragged_entity);
                    //tower.range = 50;
                    printf("rage: %f, z: %f\n", registry.towers.get(dragged_entity).range,
                           std::get<RenderRequestSingle>(registry.renderRequests.get(dragged_entity)).z_position);
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
                        printf("z:%f\n", std::get<RenderRequestSingle>(registry.renderRequests.get(dragged_entity)).z_position);
                    }
                }
            }
        }
    }
}


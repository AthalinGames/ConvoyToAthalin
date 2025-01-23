#include "world_init.hpp"
#include "ecs/tiny_ecs_registry.hpp"

Entity createPlayer(RenderSystem *renderer) {
	const auto entity = Entity();

	Player &player = registry.players.emplace(entity);

	player.status_bar_cleanup_func = [&] {
		for (const auto status_bar_entity: player.status_bar_entities) {
			registry.remove_all_components_of(status_bar_entity);
		}
	};

	const auto status_background = createBlackSquare(renderer, {window_width_px / 2, TILE_HEIGHT / 4},
	                                                 {window_width_px, TILE_HEIGHT / 2}, 0.75);
	player.status_bar_entities.push_back(status_background);

	constexpr vec2 text_scale = {TILE_WIDTH / 6, TILE_HEIGHT / 3};

	const auto hp_text = createText(renderer, {TILE_WIDTH / 4, TILE_HEIGHT / 4}, text_scale,
	                                "Hp:", FontType::SLIM);
	player.status_bar_entities.push_back(hp_text);

	const auto hp_number = createText(renderer, {TILE_WIDTH, TILE_HEIGHT / 4}, text_scale,
	                                  std::to_string(player.getHealth()), FontType::SLIM);
	player.status_bar_entities.push_back(hp_number);
	player.health_update_callback = [hp_number, &renderer](const int new_hp, const int old_hp) {
		updateText(hp_number, std::to_string(new_hp));
        std::string status_sign;
        if (new_hp - old_hp >= 0) {
            status_sign = "+";
        }
		createStatusText(renderer, status_sign + std::to_string(new_hp - old_hp) + " HP", new_hp > old_hp, StatusType::HEALTH);
	};

	const auto food_text = createText(renderer, {TILE_WIDTH * 2 - TILE_WIDTH / 8, TILE_HEIGHT / 4}, text_scale, "Food:",
	                                  FontType::SLIM);
	player.status_bar_entities.push_back(food_text);

	const auto food_number = createText(renderer, {TILE_WIDTH * 3, TILE_HEIGHT / 4}, text_scale,
	                                    std::to_string(player.getFood()), FontType::SLIM);
	player.status_bar_entities.push_back(food_number);
	player.food_update_callback = [food_number, &renderer](const int new_food, const int old_food) {
		updateText(food_number, std::to_string(new_food));
        std::string status_sign;
        if (new_food - old_food >= 0) {
            status_sign = "+";
        }
		createStatusText(renderer, status_sign + std::to_string(new_food - old_food) + " Food", new_food > old_food, StatusType::FOOD);
	};

	return entity;
}

Entity createItem(const ItemType item) {
	const auto entity = Entity();

	registry.items.emplace(entity);

	std::visit(overloaded{
		[entity] (const TowerType towerType) {
            auto &tower = registry.towers.emplace(entity);
			switch (towerType) {
				case TowerType::ARCHER: {
					registry.archers.emplace(entity);
                    tower.range = 3.5*TOWER_WIDTH; //TODO: save range somewhere in components? when trying #include "world_init.hpp" in game_components compilation fails
					break;
				}
                case TowerType::KNIGHT: {
                    registry.knights.emplace(entity);
                    tower.range = 1.4 * TOWER_WIDTH;
                    break;
                }
				case TowerType::TOWER_TYPE_COUNT: {
					assert(false && "Invalid Tower type");
				}
			}
		},
		[entity] (const ConsumableType consumableType) {
            auto &consumable = registry.consumables.emplace(entity);
			switch (consumableType) {
                case ConsumableType::BOMB: {
                    registry.bombs.emplace(entity);
                    consumable.range = TOWER_WIDTH;
                    break;
                }
                case ConsumableType::SPIKES: {
                    registry.spikes.emplace(entity);
                    consumable.range = 0.25f * TOWER_WIDTH;
                    break;
                }
                case ConsumableType::BARRIER: {
                    registry.barriers.emplace(entity);
                    consumable.range = 0.25f * TOWER_WIDTH;
                    break;
                }
                case ConsumableType::HEALTH_POTION: {
                    registry.healthPotions.emplace(entity);
                    break;
                }
				case ConsumableType::CONSUMABLE_TYPE_COUNT: {
					assert(false && "Invalid Consumable type");
				}
			}
		}
	}, item);

	return entity;
}

Entity createRandomItem(std::default_random_engine &rng) {
	std::uniform_int_distribution<unsigned int> distribution(0, item_type_count - 1);
	const unsigned int item_id = distribution(rng);
	if (item_id < tower_type_count) {
		return createItem(static_cast<TowerType>(item_id));
	} else if (item_id < consumable_type_count + tower_type_count) {
		return createItem(static_cast<ConsumableType>(item_id - tower_type_count));
	} else {
		assert(false && "Invalid item type");
		return Entity();
	}
}

void createHitboxVisualization(RenderSystem *renderer, const Entity entity, const std::vector<std::vector<vec2>> polys) {
    auto &visualization = registry.hitboxVisualizations.emplace(entity);
    auto &motion = registry.motions.get(entity);

    Transform tf;
    tf.translate(motion.position);
    if (!motion.use_direction_sprite) {
        tf.rotate(motion.angle);
    }
    tf.scale(motion.scale);

    for (const std::vector<vec2> &poly : polys) {
        std::vector<Entity> lines;
        std::vector<vec2> offsets;
        std::vector<float> angles;

        for (int i = 0; i < poly.size() - 1; ++i) {
            vec2 vert_curr = poly[i];
            vec2 vert_next = poly[i + 1];
            vec2 middle = 0.5f * (vert_next + vert_curr);
            vec2 dp = vert_curr - vert_next;
            float angle = atan2(dp.y, dp.x) + M_PI_2;
            const auto line_entity = Entity();
            auto &stationary = registry.stationaries.emplace(line_entity);

            //stationary.angle = angle;
            stationary.angle = motion.angle + angle;
            while (stationary.angle > M_PI) {
                stationary.angle -= 2 * M_PI;
            }

            stationary.scale = vec2(TOWER_WIDTH / 8, motion.scale.y * distance(vert_curr, vert_next));

            stationary.position = tf * middle;

            registry.renderForeground.insert(line_entity, {
                    {Stationary{}},
                    {0},
                    Z_BACKGROUND / 2,
                    TEXTURE_ASSET_ID::HITBOX_LINE,
                    EFFECT_ASSET_ID::TEXTURED,
                    GEOMETRY_BUFFER_ID::SPRITE,
            });
            lines.push_back(line_entity);
            offsets.push_back(middle);
            angles.push_back(angle);
            registry.invisibles.emplace(line_entity);
        }

        //TODO: if line between last and first vertex is done in loop, for some reason, mouse call gets triggered before start_button has been generated
        vec2 vert_curr = poly.back();
        vec2 vert_next = poly.front();
        vec2 middle = 0.5f * (vert_next + vert_curr);
        vec2 dp = vert_curr - vert_next;
        float angle = atan2(dp.y, dp.x) + M_PI_2;
        const auto line_entity = Entity();
        auto &stationary = registry.stationaries.emplace(line_entity);

        //stationary.angle = angle;
        stationary.angle = motion.angle + angle;
        while (stationary.angle > M_PI) {
            stationary.angle -= 2 * M_PI;
        }

        stationary.scale = vec2(TOWER_WIDTH / 8, registry.motions.get(entity).scale.y * distance(vert_curr, vert_next));// (TOWER_WIDTH/2, distance(vert_curr, vert_next));
        stationary.position = tf * middle;

        registry.renderForeground.insert(line_entity, {
                {Stationary{}},
                {0},
                Z_BACKGROUND / 2,
                TEXTURE_ASSET_ID::HITBOX_LINE,
                EFFECT_ASSET_ID::TEXTURED,
                GEOMETRY_BUFFER_ID::SPRITE,
        });
        lines.push_back(line_entity);
        offsets.push_back(middle);
        angles.push_back(angle);
        registry.invisibles.emplace(line_entity);

        visualization.lines.push_back(lines);
        visualization.offsets.push_back(offsets);
        visualization.angles.push_back(angles);
    }
}

void createArcherFromCard(RenderSystem *renderer, const Entity card, Motion &motion) {
	const auto bow = Entity();

	motion.angle = -M_PI_2;
	motion.use_direction_sprite = true;
	motion.scale = vec2({TOWER_WIDTH, TOWER_HEIGHT});
	vec2 motion_pos = motion.position;

	RenderRequest request = registry.renderForeground.get(card);
	request.atlas_ids = {static_cast<unsigned int>(DIRECTION_SPRITE::DOWN)};
	request.z_position = Z_MIDDLE;
	request.used_texture = TEXTURE_ASSET_ID::ARCHER;
	request.used_effect = EFFECT_ASSET_ID::TEXTURED_ATLAS;
	registry.renderForeground.remove(card);
	registry.renderGameLayer.emplace(card, request);

	registry.bows.emplace(bow);
	registry.weapons.emplace(bow);
	registry.archers.get(card).bow = bow;

	Mesh &bow_mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(bow, &bow_mesh);

	Motion &bow_motion = registry.motions.emplace(bow);
	bow_motion.position = motion_pos;
	//TODO: for some reason motion.position changes to some weird uninitialized from here until we are back in on_mouse_button
	bow_motion.angle = M_PI_2;
	bow_motion.velocity = vec2(0, 0);
	bow_motion.scale = vec2({TOWER_WIDTH, TOWER_HEIGHT});
	printf("%f|%f\n", motion.position.x, motion.position.y);
	registry.renderGameLayer.insert(bow, {
		                                {Stationary{}},
		                                {static_cast<unsigned int>(BOW_SPRITE::LOAD)},
		                                Z_FOREGROUND,
		                                TEXTURE_ASSET_ID::BOW,
		                                EFFECT_ASSET_ID::TEXTURED_ATLAS,
		                                GEOMETRY_BUFFER_ID::SPRITE,
	                                });
}

void createKnightFromCard(RenderSystem *renderer, const Entity card, Motion &motion) {
	const auto sword = Entity();

	motion.angle = -M_PI_2;
	motion.use_direction_sprite = true;
	motion.scale = vec2({TOWER_WIDTH, TOWER_HEIGHT});
	vec2 motion_pos = motion.position;

	RenderRequest request = registry.renderForeground.get(card);
	request.atlas_ids = {static_cast<unsigned int>(DIRECTION_SPRITE::DOWN)};
	request.z_position = Z_MIDDLE;
	request.used_texture = TEXTURE_ASSET_ID::KNIGHT;
	request.used_effect = EFFECT_ASSET_ID::TEXTURED_ATLAS;
	registry.renderForeground.remove(card);
	registry.renderGameLayer.emplace(card, request);

	registry.swords.emplace(sword);
	registry.weapons.emplace(sword);
	registry.knights.get(card).sword = sword;

	Mesh &sword_mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(sword, &sword_mesh);

	Motion &sword_motion = registry.motions.emplace(sword);
	sword_motion.position = motion_pos;
	//TODO: for some reason motion.position changes to some weird uninitialized from here until we are back in on_mouse_button
	sword_motion.angle = -M_PI_2;
	sword_motion.use_direction_sprite = true;
	sword_motion.velocity = vec2(0, 0);
	sword_motion.scale = vec2({TOWER_WIDTH, TOWER_HEIGHT});
	printf("%f|%f\n", motion.position.x, motion.position.y);
	auto sword_request = registry.renderGameLayer.insert(sword, {
                                                                        {Stationary{}},
                                                                        {static_cast<unsigned int>(DIRECTION_SPRITE::DOWN)},
                                                                        Z_BACKGROUND,
                                                                        TEXTURE_ASSET_ID::SWORD,
                                                                        EFFECT_ASSET_ID::TEXTURED_ATLAS,
                                                                        GEOMETRY_BUFFER_ID::SPRITE,
                                                                    });
    createHitboxVisualization(renderer, sword, {getCollisionMeshOfTexture(sword_request.used_texture)});
}

void createTowerFromCard(RenderSystem *renderer, const Entity card) {
	assert(registry.cards.has(card));
	registry.cards.remove(card, true);
	const Stationary &card_pos = registry.stationaries.get(card);
	Motion &tower_motion = registry.motions.emplace(card);
	tower_motion.position = card_pos.position;
	tower_motion.velocity = vec2(0, 0);
	registry.stationaries.remove(card);

	if (registry.archers.has(card)) {
		printf("%f|%f\n", tower_motion.position.x, tower_motion.position.y);
		createArcherFromCard(renderer, card, tower_motion);
		printf("%f|%f\n", tower_motion.position.x, tower_motion.position.y);
	} else if (registry.knights.has(card)) {
		createKnightFromCard(renderer, card, tower_motion);
	} else {
		assert(false && "Invalid Tower type for tower creation");
	}
}

void createBombFromCard(RenderSystem *renderer, const Entity card, Motion &motion) {

    motion.scale = vec2({2 * TOWER_WIDTH, 2 * TOWER_HEIGHT});
    //vec2 motion_pos = motion.position;

	RenderRequest request = registry.renderForeground.get(card);
	request.atlas_ids = {static_cast<unsigned int>(BOMB_SPRITE::BOMB0)};
	request.z_position = Z_FOREGROUND;
	request.used_texture = TEXTURE_ASSET_ID::BOMB;
	request.used_effect = EFFECT_ASSET_ID::TEXTURED_ATLAS;
	registry.renderForeground.remove(card);
	registry.renderGameLayer.emplace(card, request);

	registry.bombTimers.emplace(card);
}

void createSpikesFromCard(RenderSystem *renderer, const Entity card, Motion& motion, std::set<Entity> &placed_consumables) {

    motion.angle = 0;
    motion.scale = vec2({TOWER_WIDTH, TOWER_HEIGHT});
    vec2 motion_pos = motion.position;
    float angle = motion.angle;
    vec2 scale = motion.scale;


    // convert card entity to left spike on map
    RenderRequest request = registry.renderForeground.get(card);
    request.atlas_ids = {static_cast<unsigned int>(SPIKES_SPRITE::SPIKE_LEFT)};
    request.z_position = Z_FOREGROUND;
    request.used_texture = TEXTURE_ASSET_ID::SPIKES;
    request.used_effect = EFFECT_ASSET_ID::TEXTURED_ATLAS;
    registry.renderForeground.remove(card);
    registry.renderGameLayer.emplace(card, request);

    createHitboxVisualization(renderer, card, {getCollisionMeshOfTexture(request.used_texture, request.atlas_ids[0])});

    //TODO: create new entities for rest of spikes
    const auto spike_middle_entity = Entity();
    auto &spike_middle_motion = registry.motions.emplace(spike_middle_entity);
    printf("angle: %f\n", motion.angle);
    spike_middle_motion.angle = angle;
    spike_middle_motion.position = motion_pos;
    spike_middle_motion.scale = scale;

    request.atlas_ids = {static_cast<unsigned int>(SPIKES_SPRITE::SPIKE_MIDDLE)};
    registry.renderGameLayer.emplace(spike_middle_entity, request);

    createHitboxVisualization(renderer, spike_middle_entity, {getCollisionMeshOfTexture(request.used_texture, request.atlas_ids[0])});

    registry.consumables.emplace(spike_middle_entity).placed = true;
    placed_consumables.emplace(spike_middle_entity);
    registry.spikes.emplace(spike_middle_entity);

    const auto spike_right_entity = Entity();
    auto &spike_right_motion = registry.motions.emplace(spike_right_entity);
    printf("angle: %f\n", motion.angle);
    spike_right_motion.angle = angle;
    spike_right_motion.position = motion_pos;
    spike_right_motion.scale = scale;

    request.atlas_ids = {static_cast<unsigned int>(SPIKES_SPRITE::SPIKE_RIGHT)};
    registry.renderGameLayer.emplace(spike_right_entity, request);

    registry.consumables.emplace(spike_right_entity).placed = true;
    placed_consumables.emplace(spike_right_entity);
    registry.spikes.emplace(spike_right_entity);

    createHitboxVisualization(renderer, spike_right_entity, {getCollisionMeshOfTexture(request.used_texture, request.atlas_ids[0])});
}

void createBarrierFromCard(RenderSystem *renderer, const Entity card, Motion &motion) {

    motion.scale = vec2({TOWER_WIDTH, TOWER_HEIGHT});
    //vec2 motion_pos = motion.position;

    RenderRequest request = registry.renderForeground.get(card);
    request.atlas_ids = {static_cast<unsigned int>(BARRIER_SPRITE::BARRIER_START)};
    request.z_position = Z_FOREGROUND;
    request.used_texture = TEXTURE_ASSET_ID::BARRIER;
    request.used_effect = EFFECT_ASSET_ID::TEXTURED_ATLAS;
    registry.renderForeground.remove(card);
    registry.renderGameLayer.emplace(card, request);
    createHitboxVisualization(renderer, card, {getCollisionMeshOfTexture(request.used_texture)});
}

void createConsumableFromCard(RenderSystem *renderer, const Entity card, const Entity player_entity, std::set<Entity> &placed_consumables) {
    //TODO: return created entities for case, that multiple were created or take consumable vector as input var
    assert(registry.cards.has(card));
    registry.cards.remove(card, true);
    const Stationary& card_pos = registry.stationaries.get(card);
    Motion& consumable_motion = registry.motions.emplace(card);
    consumable_motion.position = card_pos.position;
    consumable_motion.velocity = vec2(0, 0);
    registry.stationaries.remove(card);
    registry.consumables.get(card).placed = true;

    if (registry.bombs.has(card)) {
        createBombFromCard(renderer, card, consumable_motion);
        placed_consumables.emplace(card);
    } else if (registry.spikes.has(card)) {
        createSpikesFromCard(renderer, card, consumable_motion, placed_consumables);
        placed_consumables.emplace(card);
    } else if (registry.barriers.has(card)) {
        createBarrierFromCard(renderer, card, consumable_motion);
        placed_consumables.emplace(card);
    } else if (registry.healthPotions.has(card)) {
        auto &player = registry.players.get(player_entity);
        player.updateHealth(min(player.getHealth() + registry.healthPotions.get(card).health, player.maxHealth));
        registry.remove_all_components_of(card);
    }
    else {
        assert(false && "Invalid Consumable type for consumable creation");
    }

}

void returnArcherToItem(const Entity tower) {
	const Entity bow = registry.archers.get(tower).bow;
	registry.remove_all_components_of(bow);
}

void returnKnightToItem(const Entity tower) {
	const Entity sword = registry.knights.get(tower).sword;
	registry.remove_all_components_of(sword);
}

void returnTowerToItem(const Entity tower) {
	assert(registry.items.has(tower));
	registry.motions.remove(tower);
	registry.meshPtrs.remove(tower);
	registry.renderGameLayer.remove(tower);

	if (registry.items.has(tower)) {
		registry.towers.get(tower).placed = false;
		if (registry.archers.has(tower)) {
			returnArcherToItem(tower);
		} else if (registry.knights.has(tower)) {
			returnKnightToItem(tower);
		}
	} else {
		assert(false && "Invalid Tower type for returning to Item");
	}
}

Entity createArrow(RenderSystem *renderer, const vec2 pos, const float velocity, const vec2 dir) {
	const auto entity = Entity();

	Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Motion &motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = static_cast<float>(atan2(dir.y, dir.x)) + (M_PI_2 / 2) + M_PI;
	motion.velocity = -velocity * normalize(dir);
	motion.scale = vec2({TOWER_WIDTH, TOWER_HEIGHT});

	registry.arrows.emplace(entity);

	auto request = registry.renderGameLayer.insert(entity, {
		                                {Stationary{}},
		                                {static_cast<unsigned int>(BOW_SPRITE::ARROW)},
		                                Z_MIDDLE,
		                                TEXTURE_ASSET_ID::BOW,
		                                EFFECT_ASSET_ID::TEXTURED_ATLAS,
		                                GEOMETRY_BUFFER_ID::SPRITE,
	                                });

    createHitboxVisualization(renderer, entity, {getCollisionMeshOfTexture(request.used_texture)});

	return entity;
}

Entity createEnemy(RenderSystem *renderer, const vec2 pos, const EnemyType enemyType) {
	const auto entity = Entity();

	Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Motion &motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.0f;
	motion.velocity = vec2(0, 0);
	motion.scale = vec2({SLIME_WIDTH, SLIME_HEIGHT});
	motion.use_direction_sprite = true;

	registry.enemies.emplace(entity, [entity] {
		if (registry.hitTimers.has(entity)) {
			registry.hitTimers.remove(entity);
		} else {
			registry.colors.emplace(entity, 1.f, 1.f, 1.f, 1.f);
		}
		registry.hitTimers.emplace(entity);
	});

	switch (enemyType) {
		case EnemyType::SLIME:
			registry.slimes.emplace(entity);
			registry.renderGameLayer.insert(entity, {
				                                {Stationary{}},
				                                {static_cast<unsigned int>(DIRECTION_SPRITE::DOWN)},
				                                Z_MIDDLE,
				                                TEXTURE_ASSET_ID::SLIME,
				                                EFFECT_ASSET_ID::TEXTURED_ATLAS,
				                                GEOMETRY_BUFFER_ID::SPRITE,
			                                });
			break;
		case EnemyType::SLIME_BIG:
			motion.scale = vec2({2 * SLIME_WIDTH, 2 * SLIME_HEIGHT});
			registry.slimesBig.emplace(entity);
			registry.renderGameLayer.insert(entity, {
				                                {Stationary{}},
				                                {static_cast<unsigned int>(DIRECTION_SPRITE::DOWN)},
				                                Z_MIDDLE,
				                                TEXTURE_ASSET_ID::SLIME_BIG,
				                                EFFECT_ASSET_ID::TEXTURED_ATLAS,
				                                GEOMETRY_BUFFER_ID::SPRITE,
			                                });
			break;
		case EnemyType::ENEMY_TYPE_COUNT:
			assert(false && "Invalid Enemy type");
	}
	registry.invisibles.emplace(entity);

    //TODO: for each collision poly
    std::vector<std::vector<vec2>> polys;
    for (int i = 0;
         i < static_cast<unsigned int>(DIRECTION_SPRITE::COUNT) * static_cast<unsigned int>(SLIME_WALK_FRAME::COUNT);
         i += static_cast<unsigned int>(SLIME_WALK_FRAME::COUNT)) {
        polys.push_back(getCollisionMeshOfTexture(registry.renderGameLayer.get(entity).used_texture, i));
    }
    createHitboxVisualization(renderer, entity, polys);

	return entity;
}

void realignCards() {
	auto &current_cards = registry.cards.entities;
	if (!current_cards.empty()) {
		float card_offset = CARD_AXIS_WIDTH / (static_cast<float>(current_cards.size()) + 1);
		auto &first_card = registry.stationaries.get(registry.cards.entities[0]);
		first_card.position = vec2(card_offset,
		                           CARD_AXIS_HEIGHT);
		first_card.scale = vec2(CARD_WIDTH, CARD_HEIGHT);
		for (uint i = 1; i < current_cards.size(); i++) {
			Entity &current_card = current_cards[i];
			Entity &prev_card = current_cards[i - 1];
			auto &stationary = registry.stationaries.get(current_card);
			auto &prev_stationary = registry.stationaries.get(prev_card);
			stationary.position = vec2(prev_stationary.position[0] + card_offset,
			                           CARD_AXIS_HEIGHT);
			stationary.scale = vec2(CARD_WIDTH, CARD_HEIGHT);
		}
	}
}

void createCardFromItem(RenderSystem *renderer, const Entity item) {
	Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(item, &mesh);

	registry.cards.emplace(item);

	Stationary &card_texture = registry.stationaries.emplace(item);
	card_texture.scale = vec2({CARD_WIDTH, CARD_HEIGHT});

	realignCards();
	//TODO: do this with switch, like createItem
	if (registry.archers.has(item)) {
		registry.renderForeground.insert(item, {
			                                 {Stationary{}},
			                                 {0},
			                                 Z_FOREGROUND,
			                                 TEXTURE_ASSET_ID::ARCHER_CARD,
			                                 EFFECT_ASSET_ID::TEXTURED,
			                                 GEOMETRY_BUFFER_ID::SPRITE,
		                                 });
	} else if (registry.knights.has(item)) {
		registry.renderForeground.insert(item, {
			                                 {Stationary{}},
			                                 {0},
			                                 Z_FOREGROUND,
			                                 TEXTURE_ASSET_ID::KNIGHT_CARD,
			                                 EFFECT_ASSET_ID::TEXTURED,
			                                 GEOMETRY_BUFFER_ID::SPRITE,
		                                 });
	} else if (registry.bombs.has(item)) {
		registry.renderForeground.insert(item, {
			                                 {Stationary{}},
			                                 {0},
			                                 Z_FOREGROUND,
			                                 TEXTURE_ASSET_ID::BOMB_CARD,
			                                 EFFECT_ASSET_ID::TEXTURED,
			                                 GEOMETRY_BUFFER_ID::SPRITE,
		                                 });
    } else if (registry.spikes.has(item)) {
        registry.renderForeground.insert(item, {
                                             {Stationary{}},
                                             {0},
                                             Z_FOREGROUND,
                                             TEXTURE_ASSET_ID::SPIKES_CARD,
                                             EFFECT_ASSET_ID::TEXTURED,
                                             GEOMETRY_BUFFER_ID::SPRITE,
                                          });
    } else if (registry.barriers.has(item)) {
        registry.renderForeground.insert(item, {
                                             {Stationary{}},
                                             {0},
                                             Z_FOREGROUND,
                                             TEXTURE_ASSET_ID::BARRIER_CARD,
                                             EFFECT_ASSET_ID::TEXTURED,
                                             GEOMETRY_BUFFER_ID::SPRITE,
                                             });
    } else if (registry.healthPotions.has(item)) {
        registry.renderForeground.insert(item, {
                                             {Stationary{}},
                                             {0},
                                             Z_FOREGROUND,
                                             TEXTURE_ASSET_ID::HEALTH_POTION_CARD,
                                             EFFECT_ASSET_ID::TEXTURED,
                                             GEOMETRY_BUFFER_ID::SPRITE,
                                             });
    }
}

void returnCardToItem(const Entity card) {
	if (!registry.cards.has(card)) {
		printf("entity is not a card");
	}
	assert(registry.cards.has(card));

	registry.meshPtrs.remove(card);
	registry.cards.remove(card);
	registry.stationaries.remove(card);
	registry.renderForeground.remove(card);
}

TD_MAP_ATLAS_TEXTURES tileAdjacentToPath(const vec2 tile_pos, const std::vector<vec2> &path,
                                         const TD_MAP_ATLAS_TEXTURES initial_tile) {
	vec2 checkpoint = path[0];
	bool top_left = false, top_right = false, bottom_left = false, bottom_right = false;
	if (tile_pos.y == 0) {
		bottom_left = true;
		bottom_right = true;
	}
	// TODO fix path coming from the top
	if (checkpoint == tile_pos) {
		top_right = true;
	} else if (checkpoint + vec2{0, 1} == tile_pos) {
		bottom_right = true;
	}
	for (uint i = 1; i < path.size(); ++i) {
		const vec2 second_checkpoint = path[i];
		// check sides
		if (checkpoint.y == second_checkpoint.y) {
			// this section is horizontal
			if ((checkpoint.x <= tile_pos.x && second_checkpoint.x >= tile_pos.x) ||
			    (checkpoint.x >= tile_pos.x && second_checkpoint.x <= tile_pos.x)) {
				// left side of the square is on the line
				if (checkpoint.y == tile_pos.y) {
					top_left = true;
				} else if (checkpoint.y + 1 == tile_pos.y) {
					bottom_left = true;
				}
			}
			if ((checkpoint.x + 1 <= tile_pos.x && second_checkpoint.x + 1 >= tile_pos.x) ||
			    (checkpoint.x + 1 >= tile_pos.x && second_checkpoint.x + 1 <= tile_pos.x)) {
				// right side of the square is on the line
				if (checkpoint.y == tile_pos.y) {
					top_right = true;
				} else if (checkpoint.y + 1 == tile_pos.y) {
					bottom_right = true;
				}
			}
		} else if (checkpoint.x == second_checkpoint.x) {
			// this section is vertical
			if ((checkpoint.y <= tile_pos.y && second_checkpoint.y >= tile_pos.y) ||
			    (checkpoint.y >= tile_pos.y && second_checkpoint.y <= tile_pos.y)) {
				// top side of the square is on the line
				if (checkpoint.x == tile_pos.x) {
					top_left = true;
				} else if (checkpoint.x + 1 == tile_pos.x) {
					//printf("%f, %f\n%f, %f; %f, %f\n", tile_pos.x, tile_pos.y, checkpoint.x, checkpoint.y, second_checkpoint.x, second_checkpoint.y);
					top_right = true;
				}
			}
			if ((checkpoint.y + 1 <= tile_pos.y && second_checkpoint.y + 1 >= tile_pos.y) ||
			    (checkpoint.y + 1 >= tile_pos.y && second_checkpoint.y + 1 <= tile_pos.y)) {
				// bottom side of the square is on the line
				if (checkpoint.x == tile_pos.x) {
					bottom_left = true;
				} else if (checkpoint.x + 1 == tile_pos.x) {
					//printf("%f, %f\n%f, %f; %f, %f\n", tile_pos.x, tile_pos.y, checkpoint.x, checkpoint.y, second_checkpoint.x, second_checkpoint.y);
					bottom_right = true;
				}
			}
		}
		checkpoint = second_checkpoint;
	}
	// Corners
	if (top_left && !top_right && !bottom_left && !bottom_right) {
		return TD_MAP_ATLAS_TEXTURES::DIRT_GRASS_CORNER_BOTTOM_RIGHT;
	}
	if (!top_left && top_right && !bottom_left && !bottom_right) {
		return TD_MAP_ATLAS_TEXTURES::DIRT_GRASS_CORNER_BOTTOM_LEFT;
	}
	if (!top_left && !top_right && bottom_left && !bottom_right) {
		return TD_MAP_ATLAS_TEXTURES::DIRT_GRASS_CORNER_TOP_RIGHT;
	}
	if (!top_left && !top_right && !bottom_left && bottom_right) {
		return TD_MAP_ATLAS_TEXTURES::DIRT_GRASS_CORNER_TOP_LEFT;
	}
	// Sides
	if (top_left && top_right && !bottom_left && !bottom_right) {
		return TD_MAP_ATLAS_TEXTURES::DIRT_GRASS_BOTTOM;
	}
	if (top_left && !top_right && bottom_left && !bottom_right) {
		return TD_MAP_ATLAS_TEXTURES::DIRT_GRASS_RIGHT;
	}
	if (!top_left && top_right && !bottom_left && bottom_right) {
		return TD_MAP_ATLAS_TEXTURES::DIRT_GRASS_LEFT;
	}
	if (!top_left && !top_right && bottom_left && bottom_right) {
		return TD_MAP_ATLAS_TEXTURES::DIRT_GRASS_TOP;
	}
	// Curves
	if (top_left && top_right && bottom_left && !bottom_right) {
		return TD_MAP_ATLAS_TEXTURES::DIRT_GRASS_CORNER_TOP_LEFT_INVERTED;
	}
	if (top_left && top_right && !bottom_left && bottom_right) {
		return TD_MAP_ATLAS_TEXTURES::DIRT_GRASS_CORNER_TOP_RIGHT_INVERTED;
	}
	if (top_left && !top_right && bottom_left && bottom_right) {
		return TD_MAP_ATLAS_TEXTURES::DIRT_GRASS_CORNER_BOTTOM_LEFT_INVERTED;
	}
	if (!top_left && top_right && bottom_left && bottom_right) {
		return TD_MAP_ATLAS_TEXTURES::DIRT_GRASS_CORNER_BOTTOM_RIGHT_INVERTED;
	}
	// Double-corners
	if (top_left && !top_right && !bottom_left && bottom_right) {
		return TD_MAP_ATLAS_TEXTURES::DIRT_GRASS_CORNER_DOUBLE;
	}
	if (!top_left && top_right && bottom_left && !bottom_right) {
		return TD_MAP_ATLAS_TEXTURES::DIRT_GRASS_CORNER_DOUBLE_MIRRORED;
	}
	// Full
	if (top_left && top_right && bottom_left && bottom_right) {
		return TD_MAP_ATLAS_TEXTURES::DIRT;
	}
	return initial_tile;
}

std::vector<vec2> grid_to_coordinates(const std::vector<vec2> &grid_coords) {
	std::vector<vec2> map_coordinates = {};
	constexpr float width_step = window_width_px / (MAP_COUNT_X - 1);
	constexpr float height_step = window_height_px / (MAP_COUNT_Y - 1);
	constexpr float width_step_center = width_step / 2;
	constexpr float height_step_center = height_step / 2;
	for (const auto grid_coord: grid_coords) {
		float x = grid_coord.x * width_step + width_step_center;
		float y = grid_coord.y * height_step + height_step_center;
		map_coordinates.emplace_back(x, y);
	}
	return map_coordinates;
}

Entity createMap(RenderSystem *renderer, const std::vector<vec2> &checkpoints,
                 std::default_random_engine rng, std::uniform_real_distribution<float> dist) {
	//is & for checkpoint necessary? yes, we don't need to copy that value
	const auto entity = Entity();

	Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Stationary &map_texture = registry.stationaries.emplace(entity);
	//map_texture.position = vec2({window_width_px/2, window_height_px/2});
	//map_texture.scale = vec2({BACKGROUND_WIDTH, BACKGROUND_HEIGHT});
	map_texture.position = vec2(0, 0);

	Map &map_attributes = registry.maps.emplace(entity);
	const std::vector<vec2> map_coordinates = grid_to_coordinates(checkpoints);
	map_attributes.checkpoints = map_coordinates;

	//calculate path length
	float path_length = 0;
	if (map_coordinates.size() > 1) {
		path_length += abs(distance(map_coordinates[0], map_coordinates[1]));
		map_attributes.section_lengths.push_back(path_length);
		for (uint i = 2; i < map_coordinates.size(); ++i) {
			const float section_length = abs(distance(map_coordinates[i - 1], map_coordinates[i]));
			map_attributes.section_lengths.push_back(section_length);
			path_length += section_length;
		}
	}
	map_attributes.path_length = path_length;

	// build map

	map_texture.scale = vec2(TILE_WIDTH, TILE_HEIGHT);

	std::vector<Stationary> stationaries(MAP_COUNT_X * MAP_COUNT_Y);
	std::vector<uint> atlas_ids(MAP_COUNT_X * MAP_COUNT_Y);
	for (uint x = 0; x < MAP_COUNT_X; x++) {
		for (uint y = 0; y < MAP_COUNT_Y; y++) {
			const uint index = y + x * MAP_COUNT_Y;
			const float selection = dist(rng);
			stationaries[index] = {
				.position = {
					x * TILE_WIDTH,
					y * TILE_HEIGHT,
				},
				.use_direction_sprite = false,
				.scale = {1, 1}
			};
			TD_MAP_ATLAS_TEXTURES selected_tile;
			if (selection < 0.4) {
				selected_tile = TD_MAP_ATLAS_TEXTURES::GRASS;
			} else if (selection < 0.8) {
				selected_tile = TD_MAP_ATLAS_TEXTURES::GRASS_2;
			} else if (selection < 0.85) {
				selected_tile = TD_MAP_ATLAS_TEXTURES::GRASS_FLOWER;
			} else if (selection < 0.9) {
				selected_tile = TD_MAP_ATLAS_TEXTURES::GRASS_FLOWER_2;
			} else if (selection < 0.95) {
				selected_tile = TD_MAP_ATLAS_TEXTURES::GRASS_STICK;
			} else {
				selected_tile = TD_MAP_ATLAS_TEXTURES::GRASS_STONE;
			}
			atlas_ids[index] = static_cast<uint>(tileAdjacentToPath({x, y}, checkpoints, selected_tile));
		}
	}

	registry.renderBackground.insert(entity, {
		                                 std::move(stationaries),
		                                 std::move(atlas_ids),
		                                 Z_MIDDLE,
		                                 TEXTURE_ASSET_ID::TD_MAP_ATLAS,
		                                 EFFECT_ASSET_ID::TEXTURED_ATLAS,
		                                 GEOMETRY_BUFFER_ID::SPRITE,
	                                 });

	const auto map_decoration = Entity();
	map_attributes.map_decoration = map_decoration;
	map_attributes.destruction_lambda = [map_decoration] {
		registry.remove_all_components_of(map_decoration);
	};

	Stationary &deco_pos = registry.stationaries.emplace(map_decoration);
	deco_pos.position = map_coordinates.back() - vec2(0, TILE_HEIGHT * 0.3);
	deco_pos.use_direction_sprite = false;
	deco_pos.scale = vec2(1.4 * TILE_WIDTH, 1.4 * TILE_HEIGHT);

	registry.renderBackground.insert(map_decoration, {
		                                 {
			                                 Stationary{},
			                                 {
				                                 0.8f * vec2{TILE_WIDTH, -TILE_HEIGHT},
				                                 0,
				                                 false,
				                                 {1, 1}
			                                 },
			                                 {
				                                 0.8f * vec2{TILE_WIDTH, TILE_HEIGHT},
				                                 0,
				                                 false,
				                                 {1, 1}
			                                 },
			                                 {
				                                 0.8f * vec2{-TILE_WIDTH, -TILE_HEIGHT},
				                                 0,
				                                 false,
				                                 {1, 1}
			                                 }
		                                 },
		                                 {
			                                 static_cast<uint>(TD_MAP_DECORATION_TEXTURES::CAMPFIRE2),
			                                 static_cast<uint>(TD_MAP_DECORATION_TEXTURES::TENT),
			                                 static_cast<uint>(TD_MAP_DECORATION_TEXTURES::TENT2),
			                                 static_cast<uint>(TD_MAP_DECORATION_TEXTURES::WAGON),
		                                 },
		                                 Z_FOREGROUND,
		                                 TEXTURE_ASSET_ID::TD_MAP_DECORATION_ATLAS,
		                                 EFFECT_ASSET_ID::TEXTURED_ATLAS,
		                                 GEOMETRY_BUFFER_ID::SPRITE,
	                                 });

	return entity;
}

Entity createPlacementMarker(RenderSystem *renderer) {
	const auto entity = Entity();
	registry.placementMarkers.emplace(entity);
	Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Stationary &placement_marker = registry.stationaries.emplace(entity);
	placement_marker.scale = vec2({2 * TOWER_WIDTH, 2 * TOWER_HEIGHT});

	registry.renderForeground.insert(entity, {
		                                 {Stationary{}},
		                                 {0},
		                                 Z_FOREGROUND,
		                                 TEXTURE_ASSET_ID::PLACEMENT_MARKER,
		                                 EFFECT_ASSET_ID::TEXTURED,
		                                 GEOMETRY_BUFFER_ID::SPRITE,
	                                 });

	registry.invisibles.emplace(entity);

	return entity;
}

Entity createGameOver(RenderSystem *renderer) {
	const auto entity = Entity();

	Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Stationary &gameover_texture = registry.stationaries.emplace(entity);
	gameover_texture.scale = vec2({BACKGROUND_WIDTH, BACKGROUND_HEIGHT});
	gameover_texture.position = vec2({window_width_px / 2, window_height_px / 2});

	registry.renderForeground.insert(entity, {
		                                 {Stationary{}},
		                                 {0},
		                                 Z_FOREGROUND,
		                                 TEXTURE_ASSET_ID::GAME_OVER,
		                                 EFFECT_ASSET_ID::TEXTURED,
		                                 GEOMETRY_BUFFER_ID::SPRITE,
	                                 });

	return entity;
}


Entity createOverviewMap(RenderSystem *renderer) {
	const auto entity = Entity();

	Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Stationary &overview_texture = registry.stationaries.emplace(entity);
	overview_texture.position = vec2({window_width_px / 2, window_height_px / 2});
	overview_texture.scale = vec2({BACKGROUND_WIDTH, BACKGROUND_HEIGHT});

	registry.renderBackground.insert(entity, {
		                                 {Stationary{}},
		                                 {},
		                                 Z_BACKGROUND,
		                                 TEXTURE_ASSET_ID::OVERVIEW_MAP,
		                                 EFFECT_ASSET_ID::TEXTURED,
		                                 GEOMETRY_BUFFER_ID::SPRITE,
	                                 });

	return entity;
}

Entity createFightLocation(RenderSystem *renderer, const vec2 pos) {
	const auto entity = Entity();

	Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Stationary &fight_location_pos = registry.stationaries.emplace(entity);
	fight_location_pos.position = pos;
	fight_location_pos.scale = vec2({0.6 * OVERVIEW_ICON_WIDTH, 0.6 * OVERVIEW_ICON_HEIGHT});

	auto &properties = registry.overviewMapLocations.emplace(entity);
	properties.active = true;
	properties.overview_selection = createOverviewSelection(renderer, pos);
	properties.type = LocationType::FIGHT;

	registry.renderBackground.insert(entity, {
		                                 {Stationary{}},
		                                 {static_cast<unsigned int>(OVERVIEW_ICON_TEXTURES::FIGHT)},
		                                 Z_BACKGROUND / 2,
		                                 TEXTURE_ASSET_ID::OVERVIEW_ICONS_ATLAS,
		                                 EFFECT_ASSET_ID::TEXTURED_ATLAS,
		                                 GEOMETRY_BUFFER_ID::SPRITE,
	                                 });

	return entity;
}

Entity createGarrisonLocation(RenderSystem *renderer, const vec2 pos) {
	const auto entity = Entity();

	Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Stationary &garrison_location_pos = registry.stationaries.emplace(entity);
	garrison_location_pos.position = pos;
	garrison_location_pos.scale = vec2({OVERVIEW_ICON_WIDTH, OVERVIEW_ICON_HEIGHT});

	auto &properties = registry.overviewMapLocations.emplace(entity);
	properties.active = true;
	properties.overview_selection = createOverviewSelection(renderer, pos);
	properties.type = LocationType::GARRISON;

	registry.renderBackground.insert(entity, {
		{Stationary{}},
		{static_cast<unsigned int>(OVERVIEW_ICON_TEXTURES::GARRISON)},
		Z_BACKGROUND / 2,
		TEXTURE_ASSET_ID::OVERVIEW_ICONS_ATLAS,
		EFFECT_ASSET_ID::TEXTURED_ATLAS,
		GEOMETRY_BUFFER_ID::SPRITE,
	});

	return entity;
}

Entity createShopLocation(RenderSystem *renderer, const vec2 pos) {
	const auto entity = Entity();

	Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Stationary &garrison_location_pos = registry.stationaries.emplace(entity);
	garrison_location_pos.position = pos;
	garrison_location_pos.scale = vec2({OVERVIEW_ICON_WIDTH, OVERVIEW_ICON_HEIGHT});

	auto &properties = registry.overviewMapLocations.emplace(entity);
	properties.active = true;
	properties.overview_selection = createOverviewSelection(renderer, pos);
	properties.type = LocationType::MERCHANT;

	registry.renderBackground.insert(entity, {
		{Stationary{}},
		{static_cast<unsigned int>(OVERVIEW_ICON_TEXTURES::MERCHANT)},
		Z_BACKGROUND / 2,
		TEXTURE_ASSET_ID::OVERVIEW_ICONS_ATLAS,
		EFFECT_ASSET_ID::TEXTURED_ATLAS,
		GEOMETRY_BUFFER_ID::SPRITE,
	});

	return entity;
}



Entity createStartIcon(RenderSystem *renderer) {
	const auto entity = Entity();

	Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Stationary &start_icon = registry.stationaries.emplace(entity);
	start_icon.position = vec2(START_ICON_LOC_X, START_ICON_LOC_Y);
	start_icon.scale = vec2({OVERVIEW_ICON_WIDTH, OVERVIEW_ICON_HEIGHT});

	registry.overviewMapLocations.emplace(entity);

	registry.renderBackground.insert(entity, {
		                                 {Stationary{}},
		                                 {static_cast<unsigned int>(OVERVIEW_ICON_TEXTURES::START)},
		                                 Z_BACKGROUND / 2,
		                                 TEXTURE_ASSET_ID::OVERVIEW_ICONS_ATLAS,
		                                 EFFECT_ASSET_ID::TEXTURED_ATLAS,
		                                 GEOMETRY_BUFFER_ID::SPRITE,
	                                 });

	return entity;
}

Entity createGoalIcon(RenderSystem *renderer) {
	const auto entity = Entity();

	Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Stationary &goal_icon = registry.stationaries.emplace(entity);
	goal_icon.position = vec2(GOAL_ICON_LOC_X, GOAL_ICON_LOC_Y);
	goal_icon.scale = vec2({OVERVIEW_ICON_WIDTH, OVERVIEW_ICON_HEIGHT});

	registry.overviewMapLocations.emplace(entity);

	registry.renderBackground.insert(entity, {
		                                 {Stationary{}},
		                                 {static_cast<unsigned int>(OVERVIEW_ICON_TEXTURES::END)},
		                                 Z_BACKGROUND / 2,
		                                 TEXTURE_ASSET_ID::OVERVIEW_ICONS_ATLAS,
		                                 EFFECT_ASSET_ID::TEXTURED_ATLAS,
		                                 GEOMETRY_BUFFER_ID::SPRITE,
	                                 });

	return entity;
}


Entity createOverviewLine(RenderSystem *renderer, const vec2 firstPos, const vec2 secondPos) {
	const auto entity = Entity();

	Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Stationary &line_texture = registry.stationaries.emplace(entity);
	const auto dp = firstPos - secondPos;
	line_texture.position = 0.5f * (firstPos + secondPos);
	line_texture.angle = atan2(dp.y, dp.x) + M_PI_2;
	line_texture.scale = vec2({LINE_WIDTH, 0.5f * length(dp)});

	registry.renderBackground.insert(entity, {
		                                 {Stationary{}},
		                                 {0},
		                                 Z_BACKGROUND / 2,
		                                 TEXTURE_ASSET_ID::BLACK_PIXEL,
		                                 EFFECT_ASSET_ID::TEXTURED,
		                                 GEOMETRY_BUFFER_ID::SPRITE,
	                                 });

	return entity;
}

Entity createOverviewSelection(RenderSystem *renderer, const vec2 pos) {
	const auto entity = Entity();

	Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Stationary &selection = registry.stationaries.emplace(entity);
	selection.position = pos;
	selection.scale = vec2({OVERVIEW_ICON_WIDTH, OVERVIEW_ICON_HEIGHT});

	registry.invisibles.insert(entity, {});

	registry.renderBackground.insert(entity, {
		                                 {Stationary{}},
		                                 {static_cast<unsigned int>(OVERVIEW_ICON_TEXTURES::SELECTION)},
		                                 Z_BACKGROUND - Z_BACKGROUND / 4,
		                                 TEXTURE_ASSET_ID::OVERVIEW_ICONS_ATLAS,
		                                 EFFECT_ASSET_ID::TEXTURED_ATLAS,
		                                 GEOMETRY_BUFFER_ID::SPRITE,
	                                 });

	return entity;
}

Entity createRoundStartButton(RenderSystem *renderer, const vec2 pos) {
	const auto entity = Entity();

	Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Stationary &button = registry.stationaries.emplace(entity);
	button.position = pos;
	button.scale = vec2({TILE_WIDTH / 2, TILE_HEIGHT / 2});

	registry.renderForeground.insert(entity, {
		                                 {Stationary{}},
		                                 {static_cast<uint>(BUTTONS::START_UP)},
		                                 Z_FOREGROUND,
		                                 TEXTURE_ASSET_ID::BUTTONS,
		                                 EFFECT_ASSET_ID::TEXTURED_ATLAS,
		                                 GEOMETRY_BUFFER_ID::SPRITE,
	                                 });

	return entity;
}

Entity createLine(const vec2 position, const vec2 size) {
	const auto entity = Entity();

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	registry.renderForeground.insert(
		entity, {
			{Stationary{}},
			{0},
			Z_FOREGROUND,
			TEXTURE_ASSET_ID::TEXTURE_COUNT,
			EFFECT_ASSET_ID::PEBBLE,
			GEOMETRY_BUFFER_ID::DEBUG_LINE
		});

	// Create motion
	Motion &motion = registry.motions.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = {0.f, 0.f};
	motion.position = position;
	motion.scale = size;

	registry.debugComponents.emplace(entity);
	return entity;
}

Entity createText(RenderSystem *renderer, const vec2 pos, const vec2 scale, const std::string &text,
                  const FontType font) {
	const auto entity = Entity();

	Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	registry.renderForeground.insert(entity, createTextRenderRequest(text, scale, font));

	registry.texts.insert(entity, {text, scale, font});

	// Create stationary
	Stationary &stationary = registry.stationaries.emplace(entity);
	stationary.position = pos;
	stationary.scale = scale;

	return entity;
}

void updateText(const Entity text_entity, const std::string &new_text) {
	Text &curr_text = registry.texts.get(text_entity);
	if (curr_text.text == new_text) {
		return;
	}
	curr_text.text = new_text;
	registry.renderForeground.remove(text_entity);
	registry.renderForeground.insert(text_entity, createTextRenderRequest(new_text, curr_text.scale, curr_text.font));
}

Entity createStatusText(RenderSystem *renderer, const std::string &text, const bool positive, const StatusType type) {
    //vec2 pos = ;
    //switch (type) {
    //    case StatusType::HEALTH:
    //        pos.x = TILE_WIDTH / 4;
    //    case StatusType::FOOD:
    //        pos.x = TILE_WIDTH * 2 - TILE_WIDTH / 8;
    //}
	const auto entity = createText(renderer, {TILE_WIDTH, TILE_HEIGHT}, {TILE_WIDTH/6, TILE_HEIGHT/3}, text, FontType::SLIM);

	auto &color = registry.colors.emplace(entity);
	color.a = 1.f;
	if (positive) {
		color.g = 1.f;
	} else {
		color.r = 1.f;
	}

	auto &timer = registry.statusTextTimers.emplace(entity);
    timer.type = type;

	return entity;
}

Entity createBlackSquare(RenderSystem *renderer, const vec2 pos, const vec2 size, const float alpha) {
	const auto entity = Entity();

	Mesh &mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Stationary &position = registry.stationaries.emplace(entity);
	position.position = pos;
	position.scale = size;

	vec4 &color = registry.colors.emplace(entity);
	color.r = 1.f;
	color.g = 1.f;
	color.b = 1.f;
	color.a = alpha;

	registry.renderForeground.insert(entity, {
		                                 {Stationary{}},
		                                 {0},
		                                 Z_BACKGROUND,
		                                 TEXTURE_ASSET_ID::BLACK_PIXEL,
		                                 EFFECT_ASSET_ID::TEXTURED,
		                                 GEOMETRY_BUFFER_ID::SPRITE,
	                                 });

	return entity;
}

RenderRequest createTextRenderRequest(const std::string &text, const vec2 scale, const FontType font) {
	RenderRequest request{};
	request.used_texture = static_cast<TEXTURE_ASSET_ID>(font);
	request.used_effect = EFFECT_ASSET_ID::TEXTURED_ATLAS;
	request.used_geometry = GEOMETRY_BUFFER_ID::SPRITE;
	request.z_position = Z_FOREGROUND;
	vec2 current_pos = {};
	for (const char character: text) {
		switch (character) {
			case '\r': {
				continue;
			}
			case '\n': {
				current_pos.y += scale.y;
				current_pos.x = 0;
				continue;
			}
			case ' ': {
				current_pos.x += scale.x;
				continue;
			}
			default: {
				if (character < 0x20 || character > 0x7e) {
					printf("The character '%c' will not be rendered\n", character);
					continue;
				}
				Stationary &pos = request.offset_positions.emplace_back();
				// All chars below 0x20 do not have a representation (thus those chars are ignored on the atlas)
				request.atlas_ids.push_back(static_cast<unsigned int>(character) - 0x20);

				pos.position = current_pos;
				pos.scale = scale;

				current_pos.x += scale.x;
				if (font == FontType::SLIM) {
					current_pos.x += scale.x * 0.1;
				}
			}
		}
	}
	return request;
}

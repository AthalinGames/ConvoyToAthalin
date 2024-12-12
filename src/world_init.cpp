#include "world_init.hpp"
#include "ecs/tiny_ecs_registry.hpp"

Entity createPlayer() {
	const auto entity = Entity();

	registry.players.emplace(entity);

	return entity;
}

Entity createItem(const ItemType item) {
	const auto entity = Entity();

	registry.items.emplace(entity);

	std::visit(overloaded{
		[entity] (const TowerType tower) {
			registry.towers.emplace(entity);
			switch (tower) {
				case TowerType::ARCHER: {
					registry.archers.emplace(entity);
					break;
				}
				case TowerType::TOWER_TYPE_COUNT: {
					assert(false && "Invalid Tower type");
				}
			}
		},
		[] (const ConsumableType consumable) {
			switch (consumable) {
				//TODO
				case ConsumableType::CONSUMABLE_TYPE_COUNT: {
					assert(false && "Invalid Consumable type");
				}
			}
		}
	}, item);

	return entity;
}

Entity createRandomItem(std::default_random_engine& rng) {
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


void createArcherFromCard(RenderSystem* renderer, const Entity card, Motion& motion) {
	const auto bow = Entity();

	motion.angle = M_PI;
	motion.use_direction_sprite = true;
	motion.scale = vec2({ARCHER_BB_WIDTH, ARCHER_BB_HEIGHT});
    vec2 motion_pos = motion.position;

	RenderRequest& request = registry.renderRequests.get(card);
	request.z_position = Z_MIDDLE;
	request.used_texture = TEXTURE_ASSET_ID::ARCHER;

	registry.bows.emplace(bow);
	registry.weapons.emplace(bow);
	registry.archers.get(card).bow = bow;

	Mesh& bow_mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(bow, &bow_mesh);

	Motion& bow_motion = registry.motions.emplace(bow);
	bow_motion.position = motion_pos; //TODO: for some reason motion.position changes to some weird uninitialized from here until we are back in on_mouse_button
	bow_motion.angle = M_PI/2;
	bow_motion.velocity = vec2(0, 0);
	bow_motion.scale = vec2({ARCHER_BB_WIDTH, ARCHER_BB_HEIGHT});
    printf("%f|%f\n", motion.position.x, motion.position.y);
	registry.renderRequests.insert(bow, {
		{Stationary{}},
		{0},
		Z_MIDDLE,
		TEXTURE_ASSET_ID::BOW1,
		EFFECT_ASSET_ID::TEXTURED,
		GEOMETRY_BUFFER_ID::SPRITE,
	});
}

void createTowerFromCard(RenderSystem* renderer, const Entity card) {
	assert(registry.cards.has(card));
	registry.cards.remove(card);
	const Stationary& card_pos = registry.stationaries.get(card);
	Motion& tower_motion = registry.motions.emplace(card);
	tower_motion.position = card_pos.position;
	tower_motion.velocity = vec2(0, 0);
    registry.stationaries.remove(card);

	if (registry.archers.has(card)) {
        printf("%f|%f\n", tower_motion.position.x, tower_motion.position.y);
		createArcherFromCard(renderer, card, tower_motion);
        printf("%f|%f\n", tower_motion.position.x, tower_motion.position.y);
	} else {
		assert(false && "Invalid Tower type for tower creation");
	}
}

void returnArcherToItem(const Entity tower) {
	const Entity bow = registry.archers.get(tower).bow;
	registry.remove_all_components_of(bow);
}

void returnTowerToItem(const Entity tower) {
	assert(registry.items.has(tower));
	registry.motions.remove(tower);
	registry.meshPtrs.remove(tower);
	registry.renderRequests.remove(tower);

	if (registry.items.has(tower)) {
		returnArcherToItem(tower);
	} else {
		assert(false && "Invalid Tower type for returning to Item");
	}
}


Entity createArrow(RenderSystem *renderer, const vec2 pos, const float velocity, const vec2 dir) {
	const auto entity = Entity();

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Motion& motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = static_cast<float>(atan2(dir.y, dir.x)) + (M_PI_2/2) + M_PI;
	motion.velocity = -velocity * normalize(dir);
	motion.scale = vec2({ARROW_BB_WIDTH, ARROW_BB_HEIGHT});

    registry.arrows.emplace(entity);

	registry.renderRequests.insert(entity, {
		{Stationary{}},
		{0},
		Z_MIDDLE,
		TEXTURE_ASSET_ID::ARROW,
		EFFECT_ASSET_ID::TEXTURED,
		GEOMETRY_BUFFER_ID::SPRITE,
	});

	return entity;
}

Entity createEnemy(RenderSystem *renderer, const vec2 pos) {
	const Entity entity = Entity();

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Motion& motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.0f;
	motion.velocity = vec2(0, 0);
	motion.scale = vec2({SLIME_WIDTH, SLIME_HEIGHT});
    motion.use_direction_sprite = true;

    registry.enemies.emplace(entity);
    registry.slimes.emplace(entity);
    registry.invisibles.emplace(entity);

	registry.renderRequests.insert(entity, {
		{Stationary{}},
		{},
		Z_MIDDLE,
		TEXTURE_ASSET_ID::SLIME,
		EFFECT_ASSET_ID::TEXTURED,
		GEOMETRY_BUFFER_ID::SPRITE,
	});

	return entity;
}

void realignCards(){
    auto& current_cards = registry.cards.entities;
    if(!current_cards.empty()) {
        float card_offset = CARD_AXIS_WIDTH/(static_cast<float>(current_cards.size())+1);
        auto& first_card = registry.stationaries.get(registry.cards.entities[0]);
        first_card.position = vec2(card_offset,
                                   CARD_AXIS_HEIGHT);
        first_card.scale = vec2(CARD_WIDTH, CARD_HEIGHT);
        for (uint i = 1; i < current_cards.size(); i++) {
            Entity& current_card = current_cards[i];
            Entity& prev_card = current_cards[i-1];
            auto& stationary = registry.stationaries.get(current_card);
            auto& prev_stationary = registry.stationaries.get(prev_card);
            stationary.position = vec2(prev_stationary.position[0]+card_offset,
                                       CARD_AXIS_HEIGHT);
            stationary.scale = vec2(CARD_WIDTH, CARD_HEIGHT);
        }
    }

}

void createCardFromItem(RenderSystem *renderer, const Entity item) {

    Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
    registry.meshPtrs.emplace(item, &mesh);

    registry.cards.emplace(item);

    Stationary& card_texture = registry.stationaries.emplace(item);
    card_texture.scale = vec2({CARD_WIDTH, CARD_HEIGHT});

    realignCards();

    registry.renderRequests.insert(item, {
    	{Stationary{}},
    	{0},
    	Z_FOREGROUND,
        TEXTURE_ASSET_ID::ARCHER_CARD,
    	EFFECT_ASSET_ID::TEXTURED,
    	GEOMETRY_BUFFER_ID::SPRITE,
    });
}

void returnCardToItem(const Entity card) {
	assert(registry.cards.has(card));

	registry.meshPtrs.remove(card);
	registry.cards.remove(card);
	registry.stationaries.remove(card);
	registry.renderRequests.remove(card);
}


Entity createMap(RenderSystem *renderer, const std::vector<vec2>& checkpoints, TEXTURE_ASSET_ID map_sprite) { //is & for checkpoint necessary?
	const auto entity = Entity();

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Stationary& map_texture = registry.stationaries.emplace(entity);
	map_texture.position = vec2({window_width_px/2, window_height_px/2});
	map_texture.scale = vec2({BACKGROUND_WIDTH, BACKGROUND_HEIGHT});

	Map& map_attributes = registry.maps.emplace(entity);
	map_attributes.checkpoints = checkpoints;//{vec2(0, 100), vec2(300, 100), vec2(300, 400)};

	//calculate path length
	float path_length = 0;
	if (checkpoints.size() > 1) {
		path_length += abs(distance(checkpoints[0], checkpoints[1]));
        map_attributes.section_lengths.push_back(path_length);
		for (uint i = 2; i < checkpoints.size(); ++i) {
            const float section_lenght = abs(distance(checkpoints[i-1], checkpoints[i]));
            map_attributes.section_lengths.push_back(section_lenght);
			path_length += section_lenght;
		}
	}
	map_attributes.path_length = path_length;

    registry.renderRequests.insert(entity, {
    	{Stationary{}},
    	{0},
    	Z_BACKGROUND,
    	map_sprite,
    	EFFECT_ASSET_ID::TEXTURED,
    	GEOMETRY_BUFFER_ID::SPRITE,
    });

    return entity;
}

Entity createGameOver(RenderSystem *renderer) {
	const auto entity = Entity();

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Stationary& gameover_texture = registry.stationaries.emplace(entity);
	gameover_texture.scale = vec2({BACKGROUND_WIDTH, BACKGROUND_HEIGHT});
	gameover_texture.position = vec2({window_width_px/2, window_height_px/2});

	registry.renderRequests.insert(entity, {
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

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Stationary& overview_texture = registry.stationaries.emplace(entity);
	overview_texture.position = vec2({window_width_px/2, window_height_px/2});
	overview_texture.scale = vec2({BACKGROUND_WIDTH, BACKGROUND_HEIGHT});

	registry.renderRequests.insert(entity, {
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

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Stationary& fight_location_pos = registry.stationaries.emplace(entity);
	fight_location_pos.position = pos;
	fight_location_pos.scale = vec2({0.6 * OVERVIEW_ICON_WIDTH, 0.6 * OVERVIEW_ICON_HEIGHT});

	auto &properties = registry.overviewMapLocations.emplace(entity);
	properties.active = true;
	properties.overview_selection = createOverviewSelection(renderer, pos);

	registry.renderRequests.insert(entity, {
		{Stationary{}},
		{static_cast<unsigned int>(OVERVIEW_ICON_TEXTURES::FIGHT)},
		Z_MIDDLE,
		TEXTURE_ASSET_ID::OVERVIEW_ICONS_ATLAS,
		EFFECT_ASSET_ID::TEXTURED_ATLAS,
		GEOMETRY_BUFFER_ID::SPRITE,
	});

	return entity;
}

Entity createStartIcon(RenderSystem *renderer) {
	const auto entity = Entity();

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Stationary& start_icon = registry.stationaries.emplace(entity);
	start_icon.position = vec2(START_ICON_LOC_X, START_ICON_LOC_Y);
	start_icon.scale = vec2({OVERVIEW_ICON_WIDTH, OVERVIEW_ICON_HEIGHT});

	registry.overviewMapLocations.emplace(entity);

	registry.renderRequests.insert(entity, {
		{Stationary{}},
		{static_cast<unsigned int>(OVERVIEW_ICON_TEXTURES::START)},
		Z_MIDDLE,
		TEXTURE_ASSET_ID::OVERVIEW_ICONS_ATLAS,
		EFFECT_ASSET_ID::TEXTURED_ATLAS,
		GEOMETRY_BUFFER_ID::SPRITE,
	});

	return entity;
}

Entity createGoalIcon(RenderSystem *renderer) {
	const auto entity = Entity();

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Stationary& goal_icon = registry.stationaries.emplace(entity);
	goal_icon.position = vec2(GOAL_ICON_LOC_X, GOAL_ICON_LOC_Y);
	goal_icon.scale = vec2({OVERVIEW_ICON_WIDTH, OVERVIEW_ICON_HEIGHT});

	registry.overviewMapLocations.emplace(entity);

	registry.renderRequests.insert(entity, {
		{Stationary{}},
		{static_cast<unsigned int>(OVERVIEW_ICON_TEXTURES::END)},
		Z_MIDDLE,
		TEXTURE_ASSET_ID::OVERVIEW_ICONS_ATLAS,
		EFFECT_ASSET_ID::TEXTURED_ATLAS,
		GEOMETRY_BUFFER_ID::SPRITE,
	});

	return entity;
}


Entity createOverviewLine(RenderSystem *renderer, const vec2 firstPos, const vec2 secondPos) {
	const auto entity = Entity();

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Stationary& line_texture = registry.stationaries.emplace(entity);
	const auto dp = firstPos - secondPos;
	line_texture.position =  0.5f * (firstPos + secondPos);
	line_texture.angle = atan2(dp.y, dp.x) + M_PI_2;
	line_texture.scale = vec2({LINE_WIDTH, 0.5f * length(dp)});

	registry.renderRequests.insert(entity, {
		{Stationary{}},
		{0},
		Z_MIDDLE,
		TEXTURE_ASSET_ID::BLACK_PIXEL,
		EFFECT_ASSET_ID::TEXTURED,
		GEOMETRY_BUFFER_ID::SPRITE,
	});

	return entity;
}

Entity createOverviewSelection(RenderSystem *renderer, const vec2 pos) {
	const auto entity = Entity();

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Stationary& selection = registry.stationaries.emplace(entity);
	selection.position = pos;
	selection.scale = vec2({OVERVIEW_ICON_WIDTH, OVERVIEW_ICON_HEIGHT});

	registry.invisibles.insert(entity, {});

	registry.renderRequests.insert(entity, {
		{Stationary{}},
		{static_cast<unsigned int>(OVERVIEW_ICON_TEXTURES::SELECTION)},
		Z_MIDDLE,
		TEXTURE_ASSET_ID::OVERVIEW_ICONS_ATLAS,
		EFFECT_ASSET_ID::TEXTURED_ATLAS,
		GEOMETRY_BUFFER_ID::SPRITE,
	});

	return entity;
}

Entity createLine(const vec2 position, const vec2 size) {
	const auto entity = Entity();

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	registry.renderRequests.insert(
		entity, {
			{Stationary{}},
			{0},
			Z_FOREGROUND,
			TEXTURE_ASSET_ID::TEXTURE_COUNT,
			EFFECT_ASSET_ID::PEBBLE,
			GEOMETRY_BUFFER_ID::DEBUG_LINE
		});

	// Create motion
	Motion& motion = registry.motions.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.position = position;
	motion.scale = size;

	registry.debugComponents.emplace(entity);
	return entity;
}

Entity createText(RenderSystem* renderer, const vec2 pos, const vec2 scale, const std::string &text) {
	const auto entity = Entity();

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	registry.renderRequests.insert(entity, createTextRenderRequest(text, scale));

	// Create stationary
	Stationary& stationary = registry.stationaries.emplace(entity);
	stationary.position = pos;
	stationary.scale = scale;

	registry.texts.emplace(entity);

	return entity;
}

Entity createBlackSquare(RenderSystem *renderer, const vec2 pos, const vec2 size, const float alpha) {
	const auto entity = Entity();

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Stationary& position = registry.stationaries.emplace(entity);
	position.position = pos;
	position.scale = size;

	vec4& color = registry.colors.emplace(entity);
	color.r = 1.f;
	color.g = 1.f;
	color.b = 1.f;
	color.a = alpha;

	registry.renderRequests.insert(entity, {
		{Stationary{}},
		{0},
		Z_FOREGROUND,
		TEXTURE_ASSET_ID::BLACK_PIXEL,
		EFFECT_ASSET_ID::TEXTURED,
		GEOMETRY_BUFFER_ID::SPRITE,
	});

	return entity;
}


RenderRequest createTextRenderRequest(const std::string& text, const vec2 scale) {
	RenderRequest request{};
	request.used_texture = TEXTURE_ASSET_ID::ASCII_CHAR_ATLAS;
	request.used_effect = EFFECT_ASSET_ID::TEXTURED_ATLAS;
	request.used_geometry = GEOMETRY_BUFFER_ID::SPRITE;
	vec2 current_pos = {};
	for (const char character : text) {
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
				Stationary& pos = request.offset_positions.emplace_back();
				// All chars below 0x20 do not have a representation (thus those chars are ignored on the atlas)
				request.atlas_ids.push_back(static_cast<unsigned int>(character) - 0x20);

				pos.position = current_pos;
				pos.scale = scale;

				current_pos.x += scale.x;
			}
		}
	}
	return request;
}

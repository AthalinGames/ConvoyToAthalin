#include "world_init.hpp"
#include "ecs/tiny_ecs_registry.hpp"

Entity createPlayer() {
	const auto entity = Entity();

	registry.players.emplace(entity);

	return entity;
}

Entity createArcher(RenderSystem *renderer, const vec2 pos) {
	const auto entity = Entity();

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Motion& motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = M_PI;
    motion.use_direction_sprite = true;
	motion.velocity = vec2(0, 0);
	motion.scale = vec2({ARCHER_BB_WIDTH, ARCHER_BB_HEIGHT});

	auto& tower = registry.towers.emplace(entity);
	tower.range = 50;
	registry.archers.emplace(entity);

	registry.renderRequests.insert(entity, RenderRequestSingle{
		Z_MIDDLE,
		0,
		TEXTURE_ASSET_ID::ARCHER,
		EFFECT_ASSET_ID::TEXTURED,
		GEOMETRY_BUFFER_ID::SPRITE,
	});

    const auto bow = Entity();
    registry.bows.emplace(bow);
    registry.weapons.emplace(bow);
    registry.archers.get(entity).bow = bow;

    Mesh& bow_mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
    registry.meshPtrs.emplace(bow, &bow_mesh);

    Motion& bow_motion = registry.motions.emplace(bow);
    bow_motion.position = pos;
    bow_motion.angle = M_PI/2;
    bow_motion.velocity = vec2(0, 0);
    bow_motion.scale = vec2({ARCHER_BB_WIDTH, ARCHER_BB_HEIGHT});

    registry.renderRequests.insert(bow, RenderRequestSingle{
            Z_MIDDLE,
            0,
            TEXTURE_ASSET_ID::BOW1,
            EFFECT_ASSET_ID::TEXTURED,
            GEOMETRY_BUFFER_ID::SPRITE,
    });

	return entity;
}

Entity createArrow(RenderSystem *renderer, const vec2 pos, float velocity, vec2 dir) {
	const auto entity = Entity();

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Motion& motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = static_cast<float>(atan2(dir.y, dir.x)) + (M_PI_2/2) + M_PI;
	motion.velocity = -velocity * normalize(dir);
	motion.scale = vec2({ARROW_BB_WIDTH, ARROW_BB_HEIGHT});

    registry.arrows.emplace(entity);

	registry.renderRequests.insert(entity, RenderRequestSingle{
		Z_MIDDLE,
		0,
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

    registry.enemies.emplace(entity);
    registry.slimes.emplace(entity);
    registry.invisibles.emplace(entity);

	registry.renderRequests.insert(entity, RenderRequestSingle{
		Z_MIDDLE,
		0,
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

Entity createCard(RenderSystem *renderer) {
    const auto entity = Entity();

    Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
    registry.meshPtrs.emplace(entity, &mesh);

    registry.cards.emplace(entity);

    registry.archers.emplace(entity);

    Stationary& card_texture = registry.stationaries.emplace(entity);
    card_texture.scale = vec2({CARD_WIDTH, CARD_HEIGHT});

    realignCards();

    registry.renderRequests.insert(entity, RenderRequestSingle{
    	Z_FOREGROUND,
    	0,
        TEXTURE_ASSET_ID::ARCHER_CARD,
    	EFFECT_ASSET_ID::TEXTURED,
    	GEOMETRY_BUFFER_ID::SPRITE,
    });

    return entity;
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

    registry.renderRequests.insert(entity, RenderRequestSingle{
    	Z_BACKGROUND,
    	0,
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

	registry.renderRequests.insert(entity, RenderRequestSingle{
	Z_FOREGROUND,
		0,
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

	registry.renderRequests.insert(entity, RenderRequestSingle{
		Z_BACKGROUND,
		0,
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

	registry.renderRequests.insert(entity, RenderRequestSingle{
		Z_MIDDLE,
		static_cast<unsigned int>(OVERVIEW_ICON_TEXTURES::FIGHT),
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

	registry.renderRequests.insert(entity, RenderRequestSingle{
		Z_MIDDLE,
		static_cast<unsigned int>(OVERVIEW_ICON_TEXTURES::START),
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

	registry.renderRequests.insert(entity, RenderRequestSingle{
		Z_MIDDLE,
		static_cast<unsigned int>(OVERVIEW_ICON_TEXTURES::END),
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

	registry.renderRequests.insert(entity, RenderRequestSingle{
		Z_MIDDLE,
		0,
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

	registry.renderRequests.insert(entity, RenderRequestSingle{
		Z_MIDDLE,
		static_cast<unsigned int>(OVERVIEW_ICON_TEXTURES::SELECTION),
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
		entity,RenderRequestSingle{
			Z_FOREGROUND,
			0,
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

	if (text.length() > 1) {
		registry.renderRequests.insert(entity, createTextRenderRequest(text, scale));
	} else {
		registry.renderRequests.insert(entity, RenderRequestSingle{
			Z_FOREGROUND,
			static_cast<unsigned int>(text.at(0)) - 0x20,
			TEXTURE_ASSET_ID::ASCII_CHAR_ATLAS,
			EFFECT_ASSET_ID::TEXTURED_ATLAS,
			GEOMETRY_BUFFER_ID::SPRITE,
		});
	}

	// Create stationary
	Stationary& stationary = registry.stationaries.emplace(entity);
	stationary.position = pos;
	stationary.scale = scale;

	registry.texts.emplace(entity);

	return entity;
}

RenderRequestMulti createTextRenderRequest(const std::string& text, const vec2 scale) {
	RenderRequestMulti request{};
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
				auto& [requestSingle, pos] = request.requests.emplace_back();
				requestSingle.used_effect = EFFECT_ASSET_ID::TEXTURED_ATLAS;
				requestSingle.used_geometry = GEOMETRY_BUFFER_ID::SPRITE;
				requestSingle.used_texture = TEXTURE_ASSET_ID::ASCII_CHAR_ATLAS;
				requestSingle.used_texture_atlas_texture_id = static_cast<unsigned int>(character) - 0x20; // all chars after 0x20 are rendered

				pos.position = current_pos;
				pos.scale = scale;

				current_pos.x += scale.x;
			}
		}
	}
	return request;
}

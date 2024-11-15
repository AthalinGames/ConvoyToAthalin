#include "world_init.hpp"
#include "ecs/tiny_ecs_registry.hpp"

// TODO: create player base

Entity createArcher(RenderSystem *renderer, const vec2 pos) {
	const auto entity = Entity();

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Motion& motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = M_PI/2;
	motion.velocity = vec2(0, 0);
	motion.scale = vec2({-ARCHER_BB_WIDTH, ARCHER_BB_HEIGHT});

	auto& tower = registry.towers.emplace(entity);
	tower.range = 50;
	registry.archers.emplace(entity);

	registry.renderRequests.insert(entity, {
		TEXTURE_ASSET_ID::ARCHER,
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
	motion.angle = static_cast<float>(atan2(dir.y, dir.x));
	motion.velocity = -velocity * normalize(dir);
	motion.scale = vec2({-ARROW_BB_WIDTH, ARROW_BB_HEIGHT});

    Arrow& arrow = registry.arrows.emplace(entity);


	registry.renderRequests.insert(entity, {
		TEXTURE_ASSET_ID::ARROW,
		EFFECT_ASSET_ID::TEXTURED,
		GEOMETRY_BUFFER_ID::SPRITE,
	});

	return entity;
}

Entity createEnemy(RenderSystem *renderer, const vec2 pos) {
	const auto entity = Entity();

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Motion& motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.0f;
	motion.velocity = vec2(0, 0);
	motion.scale = vec2({-SLIME_WIDTH, SLIME_HEIGHT});

    auto& enemy = registry.enemies.emplace(entity);
    auto& slime = registry.slimes.emplace(entity);

	registry.renderRequests.insert(entity, {
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
        for (uint i = 1; i < current_cards.size(); i++) {
            Entity& current_card = current_cards[i];
            Entity& prev_card = current_cards[i-1];
            auto& stationary = registry.stationaries.get(current_card);
            auto& prev_stationary = registry.stationaries.get(prev_card);
            stationary.position = vec2(prev_stationary.position[0]+card_offset,
                                       CARD_AXIS_HEIGHT);
        }
    }

}

Entity createCard(RenderSystem *renderer) {
    const auto entity = Entity();

    Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
    registry.meshPtrs.emplace(entity, &mesh);

    auto& card = registry.cards.emplace(entity);

    registry.archers.emplace(entity);

    Stationary& card_texture = registry.stationaries.emplace(entity);
    card_texture.scale = vec2({CARD_WIDTH, CARD_HEIGHT});

    realignCards();

    registry.renderRequests.insert(entity, {
            TEXTURE_ASSET_ID::ARCHER_CARD,
            EFFECT_ASSET_ID::TEXTURED,
            GEOMETRY_BUFFER_ID::SPRITE,
    });

    return entity;
}

Entity createMap(RenderSystem *renderer, const vec2 pos, const std::vector<vec2>& checkpoints) { //is & for checkpoint necessary?
	const auto entity = Entity();

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Stationary& map_texture = registry.stationaries.emplace(entity);
	map_texture.position = pos;
	map_texture.scale = vec2({MAP_WIDTH, MAP_HEIGHT});

	Map& map_attributes = registry.maps.emplace(entity);
	map_attributes.checkpoints = checkpoints;//{vec2(0, 100), vec2(300, 100), vec2(300, 400)};

	//calculate path length
	float path_length = 0;
	if (checkpoints.size() > 1) {
		path_length += abs(distance(checkpoints[0], checkpoints[1]));
		for (uint i = 2; i < checkpoints.size(); ++i) {
			path_length += abs(distance(checkpoints[i-1], checkpoints[i]));
		}
	}
	map_attributes.path_length = path_length;

    registry.renderRequests.insert(entity, {
            TEXTURE_ASSET_ID::MAP,
            EFFECT_ASSET_ID::TEXTURED,
            GEOMETRY_BUFFER_ID::SPRITE,
    });

    return entity;
}

/*
Entity createFish(RenderSystem* renderer, vec2 position)
{
	// Reserve en entity
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object
	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// Initialize the position, scale, and physics components
	auto& motion = registry.motions.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = { -50.f, 0.f };
	motion.position = position;

	// Setting initial values, scale is negative to make it face the opposite way
	motion.scale = vec2({ -FISH_BB_WIDTH, FISH_BB_HEIGHT });

	// Create an (empty) Fish component to be able to refer to all fish
	registry.softShells.emplace(entity);
	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::FISH,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE });

	return entity;
}
*/

/* TODO: create enemy
Entity createTurtle(RenderSystem* renderer, vec2 position)
{
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// Initialize the motion
	auto& motion = registry.motions.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = { -100.f, 0.f };
	motion.position = position;

	// Setting initial values, scale is negative to make it face the opposite way
	motion.scale = vec2({ -TURTLE_BB_WIDTH, TURTLE_BB_HEIGHT });

	// Create and (empty) Turtle component to be able to refer to all turtles
	registry.hardShells.emplace(entity);
	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::TURTLE,
		 EFFECT_ASSET_ID::TEXTURED,
		 GEOMETRY_BUFFER_ID::SPRITE });

	return entity;
}
*/

Entity createLine(vec2 position, vec2 scale)
{
	Entity entity = Entity();

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::TEXTURE_COUNT,
		 EFFECT_ASSET_ID::PEBBLE,
		 GEOMETRY_BUFFER_ID::DEBUG_LINE });

	// Create motion
	Motion& motion = registry.motions.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.position = position;
	motion.scale = scale;

	registry.debugComponents.emplace(entity);
	return entity;
}

/*
Entity createPebble(vec2 pos, vec2 size)
{
	auto entity = Entity();

	// Setting initial motion values
	Motion& motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = size;

	// Create and (empty) Salmon component to be able to refer to all turtles
	registry.hardShells.emplace(entity);
	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::TEXTURE_COUNT, // TEXTURE_COUNT indicates that no txture is needed
			EFFECT_ASSET_ID::PEBBLE,
			GEOMETRY_BUFFER_ID::PEBBLE });

	return entity;
}
*/
#pragma once
#include <map>

#include "common.hpp"
#include <vector>
#include <unordered_map>
#include "../ext/stb_image/stb_image.h"

// Player component
struct Player {

};

// Enemy components
struct Enemy {
    int health = 1;
	float enemy_progress = 0.0f;
    float speed = 0.f;
    uint next_checkpoint = 1; // checkpoint 0 is start position
    float section_progress = 0.f;
    bool alive = true;
};

struct Slime {

};

// Tower components
enum EnemyPriority {
	LAST,
	FIRST
};

struct Tower {
	float range = 0.0f;
	EnemyPriority priority = FIRST;
	bool is_aiming = false;
};

// given to card representation of tower while off field
struct Card{
    bool selected = false;
    bool dragged = false; //TODO: follow mouse pointer when true
    // TODO: add int parameter for position from left to right on screen?
};

struct TowerAimingAt {
	Entity aimed_entity;
};

struct ShotTimer {
	float time = 1000.0f;
};

// Archer
struct Archer {
	float arrow_speed = 1000.0f;
};

// Arrow
struct Arrow {
    int damage = 1;
};

// All data relevant to the shape and motion of entities
struct Motion {
	vec2 position = { 0.f, 0.f };
	float angle = 0.f;
	vec2 velocity = { 0.f, 0.f };
	vec2 scale = { 10.f, 10.f };
};

// Stucture to store collision information
struct Collision
{
	// Note, the first object is stored in the ECS container.entities
	Entity other_entity; // the second object involved in the collision
	explicit Collision(const Entity& other_entity) { this->other_entity = other_entity; };
};

// Defines attributes of stationary entity
struct Stationary
{
    vec2 position = { 0.f, 0.f };
    float angle = 0.f;
    vec2 scale = { 100.f, 100.f };
};

struct Map
{
    std::vector<vec2> checkpoints = {vec2(0.f,0.f)};
    float path_length = 0;
    unsigned int enemies_remaining = 0;
    bool combat_started = false;
    bool active = false;
};

struct OverviewMapLocation {
	std::vector<Entity> next_locations{};
	std::vector<Entity> previous_locations{};
	Entity overview_selection;
	bool selectable = false;
	bool active = false;
};

struct Invisible {
};

struct Clickable {
};

// Data structure for toggling debug mode
struct Debug {
	bool in_debug_mode = false;
	bool in_freeze_mode = false;
};
extern Debug debugging;

// Sets the brightness of the screen
struct ScreenState
{
	float screen_darken_factor = -1;
};

// A struct to refer to debugging graphics in the ECS
struct DebugComponent
{
	// Note, an empty struct has size 1
};

// A timer that will be associated to dying salmon
struct DeathTimer
{
	float timer_ms = 3000.f;
};

// Single Vertex Buffer element for non-textured meshes (coloured.vs.glsl & salmon.vs.glsl)
struct ColoredVertex
{
	vec3 position;
	vec3 color;
};

// Single Vertex Buffer element for textured sprites (textured.vs.glsl)
struct TexturedVertex
{
	vec3 position;
	vec2 texcoord;
};

// Mesh datastructure for storing vertex and index buffers
struct Mesh
{
	static bool loadFromOBJFile(std::string obj_path, std::vector<ColoredVertex>& out_vertices, std::vector<uint16_t>& out_vertex_indices, vec2& out_size);
	vec2 original_size = {1,1};
	std::vector<ColoredVertex> vertices;
	std::vector<uint16_t> vertex_indices;
};

/**
 * The following enumerators represent global identifiers refering to graphic
 * assets. For example TEXTURE_ASSET_ID are the identifiers of each texture
 * currently supported by the system.
 *
 * So, instead of referring to a game asset directly, the game logic just
 * uses these enumerators and the RenderRequest struct to inform the renderer
 * how to structure the next draw command.
 *
 * There are 2 reasons for this:
 *
 * First, game assets such as textures and meshes are large and should not be
 * copied around as this wastes memory and runtime. Thus separating the data
 * from its representation makes the system faster.
 *
 * Second, it is good practice to decouple the game logic from the render logic.
 * Imagine, for example, changing from OpenGL to Vulkan, if the game logic
 * depends on OpenGL semantics it will be much harder to do the switch than if
 * the renderer encapsulates all asset data and the game logic is agnostic to it.
 *
 * The final value in each enumeration is both a way to keep track of how many
 * enums there are, and as a default value to represent uninitialized fields.
 */

enum class TEXTURE_ASSET_ID {
	FISH = 0,
	TURTLE,
	ARCHER,
    ARCHER_CARD,
	ARROW,
    SLIME,
    MAP,
	OVERVIEW_MAP,
	BLACK_PIXEL,
	FIGHT_ICON,
	START_ICON,
	GOAL_ICON,
	MAP_SELECTION,
	TEXTURE_COUNT
};

constexpr const char* TextureAssetIDToString(const TEXTURE_ASSET_ID id) {
	switch (id) {
		case TEXTURE_ASSET_ID::FISH: return "fish.png";
		case TEXTURE_ASSET_ID::TURTLE: return "turtle.png";
		case TEXTURE_ASSET_ID::ARCHER: return "archer.png";
        case TEXTURE_ASSET_ID::ARCHER_CARD: return "archerCard.png";
		case TEXTURE_ASSET_ID::ARROW: return "arrow.png";
        case TEXTURE_ASSET_ID::SLIME: return "Slime.png";
        case TEXTURE_ASSET_ID::MAP: return "tdmap.png";
		case TEXTURE_ASSET_ID::OVERVIEW_MAP: return "overview_map.png";
		case TEXTURE_ASSET_ID::BLACK_PIXEL: return "blackPixel.png";
		case TEXTURE_ASSET_ID::FIGHT_ICON: return "fightIcon.png";
		case TEXTURE_ASSET_ID::START_ICON: return "start_icon.png";
		case TEXTURE_ASSET_ID::GOAL_ICON: return "goal_icon.png";
		case TEXTURE_ASSET_ID::MAP_SELECTION: return "map_selection.png";
		default: {
			fprintf(stderr, "Invalid TEXTURE_ASSET_ID: %d", static_cast<int>(id));
			assert(false);
            return "";
		};
	}
}

constexpr int texture_count = static_cast<int>(TEXTURE_ASSET_ID::TEXTURE_COUNT);

enum class EFFECT_ASSET_ID {
	COLOURED = 0,
	PEBBLE,
	SALMON,
	TEXTURED,
	WATER, // TODO GROUND,
	EFFECT_COUNT
};

constexpr const char* EffectAssetIDToString(const EFFECT_ASSET_ID id) {
	switch (id) {
		case EFFECT_ASSET_ID::COLOURED: return "coloured";
		case EFFECT_ASSET_ID::PEBBLE: return "pebble";
		case EFFECT_ASSET_ID::SALMON: return "salmon";
		case EFFECT_ASSET_ID::TEXTURED: return "textured";
		case EFFECT_ASSET_ID::WATER: return "water";
		default: {
			fprintf(stderr, "Invalid EFFECT_ASSET_ID: %d", static_cast<int>(id));
			assert(false);
            return "";
		}
	}
}

constexpr int effect_count = static_cast<int>(EFFECT_ASSET_ID::EFFECT_COUNT);

enum class GEOMETRY_BUFFER_ID {
	SALMON = 0,
	SPRITE,
	PEBBLE,
	DEBUG_LINE,
	SCREEN_TRIANGLE,
	GEOMETRY_COUNT
};

constexpr int geometry_count = static_cast<int>(GEOMETRY_BUFFER_ID::GEOMETRY_COUNT);

struct RenderRequest {
	float z_position;
	TEXTURE_ASSET_ID used_texture = TEXTURE_ASSET_ID::TEXTURE_COUNT;
	EFFECT_ASSET_ID used_effect = EFFECT_ASSET_ID::EFFECT_COUNT;
	GEOMETRY_BUFFER_ID used_geometry = GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;
};


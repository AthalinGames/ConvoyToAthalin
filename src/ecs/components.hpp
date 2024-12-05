#pragma once
#include <map>

#include "common.hpp"
#include <vector>
#include <unordered_map>
#include <variant>

#include "../ext/stb_image/stb_image.h"

// Player component
struct Player {
	int health = 100;
	int maxHealth = 100;
	int coins = 0;
	std::vector<Entity> owned_cards;
    int won_battles = 0;
};

// Enemy components
struct Enemy {
    int health = 100;
	float enemy_progress = 0.0f;
    float speed = 0.f;
    float spawn_time = 0.f; // spawn time after combat started(in ms)
    bool spawned = false;
    uint next_checkpoint = 1; // checkpoint 0 is start position
    float section_progress = 0.f;
    bool alive = true;
	int damage = 100;
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
    int damage = 50;
	std::size_t max_hitcount = 1;
	std::set<Entity> hit_entities{};
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
	explicit Collision(const Entity other_entity) : other_entity(other_entity) {};
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
    std::vector<Entity> enemies = {};
    bool combat_started = false;
    float combat_time = 0.f; // time passed since combat started (in ms)
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

// Text line that is rendered at a specific position and scale
struct Text {
	std::string text;
	vec2 position;
	float size;
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
	GAME_OVER,
	OVERVIEW_ICONS_ATLAS,
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
		case TEXTURE_ASSET_ID::GAME_OVER: return "game_over.png";
		case TEXTURE_ASSET_ID::OVERVIEW_ICONS_ATLAS: return "overviewIconsAtlas.png";
		default: {
			fprintf(stderr, "Invalid TEXTURE_ASSET_ID: %d", static_cast<int>(id));
			assert(false);
            return "";
		};
	}
}

constexpr int texture_count = static_cast<int>(TEXTURE_ASSET_ID::TEXTURE_COUNT);

struct AtlasTexture {
	// position of 0,0 of the texture
	vec2 tex_pos;
	// size of texture in x and y direction
	vec2 tex_size;
};

const std::set texture_atlases = {
	TEXTURE_ASSET_ID::OVERVIEW_ICONS_ATLAS
};

enum class OVERVIEW_ICON_TEXTURES {
	START = 0,
	SELECTION,
	END,
	FIGHT,
	COUNT
};

inline void initTextureAtlasTextures(const TEXTURE_ASSET_ID atlas_id, std::map<TEXTURE_ASSET_ID, std::vector<AtlasTexture>>& atlasLookup) {
	// here the texture positions are defined
	switch (atlas_id) {
		case TEXTURE_ASSET_ID::OVERVIEW_ICONS_ATLAS: {
			constexpr unsigned int cols = 3, rows = 3, maxCount = cols * rows;
			constexpr float tex_width = 1.f / cols, tex_height = 1.f / rows;
			constexpr unsigned int definedCount = static_cast<unsigned int>(OVERVIEW_ICON_TEXTURES::COUNT);
			assert(definedCount <= maxCount); // Atlas cannot have more defined textures than available space
			atlasLookup.emplace(atlas_id, std::vector<AtlasTexture>{});
			for (unsigned int i = 0; i < definedCount; i++) {
				const float x_start = (i % cols) * tex_width;
				const float y_start = (i / cols) * tex_height;
				AtlasTexture& texDef = atlasLookup.at(atlas_id).emplace_back();
				texDef.tex_pos = vec2(x_start, y_start);
				texDef.tex_size = vec2(tex_width, tex_height);
			}
			break;
		}
		default:
			assert(false && "Texture atlas has no textures defined");
	}
}

enum class EFFECT_ASSET_ID {
	COLOURED = 0,
	PEBBLE,
	SALMON,
	TEXTURED,
	TEXTURED_ATLAS,
	WATER, // TODO GROUND,
	EFFECT_COUNT
};

constexpr const char* EffectAssetIDToString(const EFFECT_ASSET_ID id) {
	switch (id) {
		case EFFECT_ASSET_ID::COLOURED: return "coloured";
		case EFFECT_ASSET_ID::PEBBLE: return "pebble";
		case EFFECT_ASSET_ID::SALMON: return "salmon";
		case EFFECT_ASSET_ID::TEXTURED: return "textured";
		case EFFECT_ASSET_ID::TEXTURED_ATLAS: return "texturedAtlas";
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

struct RenderRequestSingle {
	float z_position;
	unsigned int used_texture_atlas_texture_id;
	TEXTURE_ASSET_ID used_texture = TEXTURE_ASSET_ID::TEXTURE_COUNT;
	EFFECT_ASSET_ID used_effect = EFFECT_ASSET_ID::EFFECT_COUNT;
	GEOMETRY_BUFFER_ID used_geometry = GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;
};

struct RenderRequestMulti {
	std::vector<std::pair<RenderRequestSingle, Stationary>> requests;
};

using RenderRequest = std::variant<RenderRequestSingle, RenderRequestMulti>;

// Collision Definitions
// Positions are defined by percentages of texture positions (top left is 0, 0 and bottom right is 1, 1)
// For proper positions this grid must be shifted, so that 0, 0 is in the center
// Final Transformations are done when calculating the collision
// Remember that even if a texture-atlas is used, only the percentages for the single texture should be used
constexpr vec2 grid_shift{0.5, 0.5};

const std::vector basic_bounding_box {
	vec2{0, 0} - grid_shift,
	vec2{1, 0} - grid_shift,
	vec2{1, 1} - grid_shift,
	vec2{0, 1} - grid_shift
};

const std::vector slime_collision_poly {
	vec2{0.22, 0} - grid_shift,
	vec2{0.78, 0} - grid_shift,
	vec2{0.91, 0.88} - grid_shift,
	vec2{0.88, 0.59} - grid_shift,
	vec2{0.75, 0.47} - grid_shift,
	vec2{0.59, 0.41} - grid_shift,
	vec2{0.41, 0.41} - grid_shift,
	vec2{0.25, 0.47} - grid_shift,
	vec2{0.13, 0.59} - grid_shift,
	vec2{0.09, 0.88} - grid_shift
};

const std::vector arrow_collision_poly {
	vec2{0.09, 0.52} - grid_shift,
	vec2{0.92, 0.52} - grid_shift,
	vec2{0.92, 0.40} - grid_shift,
	vec2{0.09, 0.40} - grid_shift
};

inline const std::vector<vec2>& getCollisionMeshOfTexture(const TEXTURE_ASSET_ID id) {
	switch (id) {
		case TEXTURE_ASSET_ID::SLIME:
			return slime_collision_poly;
		case TEXTURE_ASSET_ID::ARROW:
			return arrow_collision_poly;
		default: // This is just the bounding box of the texture
			return basic_bounding_box;
	}
}


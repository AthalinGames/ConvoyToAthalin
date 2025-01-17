#pragma once
#include <map>

#include "common.hpp"
#include <vector>
#include <unordered_map>
#include <variant>

#include "../ext/stb_image/stb_image.h"

#include "game_components.hpp"

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

// Walk animation timers
struct EnemyWalkTimer{ // timer for Slime and SlimeBig
    float time = 0.f;
    float start_time = 700.f;
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
    KNIGHT,
    BOMB,
    SPIKES,
    ARCHER_CARD,
    KNIGHT_CARD,
    BOMB_CARD,
    SPIKES_CARD,
    BOW,
    SWORD,
    SLIME,
    SLIME_BIG,
	OVERVIEW_MAP,
	BLACK_PIXEL,
    PLACEMENT_MARKER,
	GAME_OVER,
	TD_MAP_DECORATION_ATLAS,
    TD_MAP_ATLAS,
	OVERVIEW_ICONS_ATLAS,
	ASCII_CHAR_ATLAS,
	SLIM_ASCII_CHAR_ATLAS,
	BUTTONS,
	TEXTURE_COUNT
};

constexpr const char* TextureAssetIDToString(const TEXTURE_ASSET_ID id) {
	switch (id) {
		case TEXTURE_ASSET_ID::FISH: return "fish.png";
		case TEXTURE_ASSET_ID::TURTLE: return "turtle.png";
		case TEXTURE_ASSET_ID::ARCHER: return "archer.png";
        case TEXTURE_ASSET_ID::KNIGHT: return "knight.png";
        case TEXTURE_ASSET_ID::BOMB: return "items/bomb.png";
        case TEXTURE_ASSET_ID::SPIKES: return "items/spikes.png";
        case TEXTURE_ASSET_ID::ARCHER_CARD: return "archerCard.png";
        case TEXTURE_ASSET_ID::KNIGHT_CARD: return "knightCard.png";
        case TEXTURE_ASSET_ID::BOMB_CARD: return "bombCard.png";
        case TEXTURE_ASSET_ID::SPIKES_CARD: return "spikesCard.png";
        case TEXTURE_ASSET_ID::BOW: return "bow_and_arrow.png";
        case TEXTURE_ASSET_ID::SWORD: return "sword.png";
        case TEXTURE_ASSET_ID::SLIME: return "Slime.png";
        case TEXTURE_ASSET_ID::SLIME_BIG: return "slime_big.png";
		case TEXTURE_ASSET_ID::OVERVIEW_MAP: return "overview_map.png";
		case TEXTURE_ASSET_ID::BLACK_PIXEL: return "blackPixel.png";
        case TEXTURE_ASSET_ID::PLACEMENT_MARKER: return "placement_marker.png";
		case TEXTURE_ASSET_ID::GAME_OVER: return "game_over.png";
		case TEXTURE_ASSET_ID::TD_MAP_DECORATION_ATLAS: return "TDMapDecorationAtlas.png";
		case TEXTURE_ASSET_ID::TD_MAP_ATLAS: return "TDMapAtlas.png";
		case TEXTURE_ASSET_ID::OVERVIEW_ICONS_ATLAS: return "overviewIconsAtlas.png";
		case TEXTURE_ASSET_ID::ASCII_CHAR_ATLAS: return "charmap-oldschool_preview.png"; // Source: https://opengameart.org/content/ascii-bitmap-font-oldschool
		case TEXTURE_ASSET_ID::SLIM_ASCII_CHAR_ATLAS: return "font.png";
		case TEXTURE_ASSET_ID::BUTTONS: return "buttons.png";
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
	TEXTURE_ASSET_ID::OVERVIEW_ICONS_ATLAS,
	TEXTURE_ASSET_ID::ASCII_CHAR_ATLAS,
    TEXTURE_ASSET_ID::ARCHER,
    TEXTURE_ASSET_ID::BOW,
    TEXTURE_ASSET_ID::KNIGHT,
    TEXTURE_ASSET_ID::SWORD,
    TEXTURE_ASSET_ID::BOMB,
    TEXTURE_ASSET_ID::SPIKES,
    TEXTURE_ASSET_ID::SLIME,
    TEXTURE_ASSET_ID::SLIME_BIG,
	TEXTURE_ASSET_ID::TD_MAP_DECORATION_ATLAS,
	TEXTURE_ASSET_ID::TD_MAP_ATLAS,
	TEXTURE_ASSET_ID::SLIM_ASCII_CHAR_ATLAS,
	TEXTURE_ASSET_ID::BUTTONS,
};

enum class FontType {
	SQUARE = static_cast<int>(TEXTURE_ASSET_ID::ASCII_CHAR_ATLAS),
	SLIM = static_cast<int>(TEXTURE_ASSET_ID::SLIM_ASCII_CHAR_ATLAS),
};

// Component that is needed for text updates
struct Text {
	std::string text;
	vec2 scale;
	FontType font;
};

enum class OVERVIEW_ICON_TEXTURES {
	START = 0,
	SELECTION,
	END,
	FIGHT,
	COUNT
};

enum class TD_MAP_ATLAS_TEXTURES {
	DIRT = 0,
	DIRT_GRASS_BOTTOM,
	DIRT_GRASS_CORNER_BOTTOM_RIGHT,
	DIRT_GRASS_CORNER_BOTTOM_RIGHT_INVERTED,
	DIRT_GRASS_CORNER_DOUBLE,
	DIRT_GRASS_CORNER_TOP_RIGHT,
	DIRT_GRASS_CORNER_TOP_RIGHT_INVERTED,
	DIRT_GRASS_LEFT,
	DIRT_GRASS_RIGHT,
	DIRT_GRASS_TOP,
	GRASS,
	GRASS_FLOWER,
	GRASS_FLOWER_2,
	GRASS_STICK,
	GRASS_STONE,
	GRASS_2,
	DIRT_GRASS_CORNER_BOTTOM_LEFT,
	DIRT_GRASS_CORNER_BOTTOM_LEFT_INVERTED,
	DIRT_GRASS_CORNER_TOP_LEFT,
	DIRT_GRASS_CORNER_TOP_LEFT_INVERTED,
	DIRT_GRASS_CORNER_DOUBLE_MIRRORED,
};

enum class TD_MAP_DECORATION_TEXTURES {
	CAMPFIRE,
	CAMPFIRE2,
	TENT,
	TENT2,
	WAGON,
	COUNT
};

enum class DIRECTION_SPRITE {
    DOWN = 0,
    LEFT,
    UP,
    RIGHT,
    COUNT
};

enum class SWORD_SPRITE {
    HOLD = static_cast<unsigned int>(DIRECTION_SPRITE::COUNT),
    SWING,
    COUNT
};

enum class BOW_SPRITE {
    LOAD = 0,
    DRAW,
    SHOOT,
    ARROW,
    COUNT
};

enum class BOMB_SPRITE {
    BOMB0 = 0,
    BOMB1,
    BOMB2,
    BOMB3,
    EXPLOSION1,
    EXPLOSION2,
    EXPLOSION3,
    COUNT,
};

enum class SPIKES_SPRITE {
    SPIKE_LEFT = 0,
    SPIKE_MIDDLE,
    SPIKE_RIGHT,
    COUNT,
};

enum class SLIME_WALK_FRAME {
    FRAME0 = 0,
    FRAME1,
    FRAME2,
    COUNT,
};

enum class BUTTONS {
	START_UP = 0,
	START_DOWN,
};

inline void initTextureAtlasTextures(const TEXTURE_ASSET_ID atlas_id, std::map<TEXTURE_ASSET_ID, std::vector<AtlasTexture>>& atlasLookup) {
	// here the texture positions are defined
	switch (atlas_id) {
		case TEXTURE_ASSET_ID::OVERVIEW_ICONS_ATLAS: {
			constexpr unsigned int cols = 3, rows = 3, maxCount = cols * rows;
			constexpr float tex_width = 1.f / cols, tex_height = 1.f / rows;
			constexpr auto definedCount = static_cast<unsigned int>(OVERVIEW_ICON_TEXTURES::COUNT);
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
		// TODO think about a collective font system
		case TEXTURE_ASSET_ID::SLIM_ASCII_CHAR_ATLAS: {
			// TODO remember that the char offset is 0x20, so to get the correct char you need to subtract 0x20
			constexpr unsigned int cols = 18, rows = 6, maxCount = cols * rows;
			constexpr float tex_width = 1.f / cols, tex_height = 1.f / rows;
			atlasLookup.emplace(atlas_id, std::vector<AtlasTexture>{});
			for (unsigned int i = 0; i < maxCount; i++) {
				const float x_start = (i % cols) * tex_width;
				const float y_start = (i / cols) * tex_height;
				AtlasTexture& texDef = atlasLookup.at(atlas_id).emplace_back();
				texDef.tex_pos = vec2(x_start, y_start);
				texDef.tex_size = vec2(tex_width, tex_height);
			}
			break;
		}
		case TEXTURE_ASSET_ID::ASCII_CHAR_ATLAS: {
			// TODO remember that the char offset is 0x20, so to get the correct char you need to subtract 0x20
			constexpr unsigned int cols = 18, rows = 7, maxCount = cols * rows;
			constexpr float tex_width = 1.f / cols, tex_height = 1.f / rows;
			atlasLookup.emplace(atlas_id, std::vector<AtlasTexture>{});
			for (unsigned int i = 0; i < maxCount; i++) {
				const float x_start = (i % cols) * tex_width;
				const float y_start = (i / cols) * tex_height;
				AtlasTexture& texDef = atlasLookup.at(atlas_id).emplace_back();
				texDef.tex_pos = vec2(x_start, y_start);
				texDef.tex_size = vec2(tex_width, tex_height);
			}
			break;
		}
		case TEXTURE_ASSET_ID::TD_MAP_ATLAS: {
			constexpr unsigned int cols = 4, rows = 4, maxCount = cols * rows;
			constexpr float tex_width = 1.f / cols, tex_height = 1.f / rows;
			atlasLookup.emplace(atlas_id, std::vector<AtlasTexture>{});
			// insert actual texture positions
			for (unsigned int i = 0; i < maxCount; i++) {
				const float x_start = (i % cols) * tex_width;
				const float y_start = (i / cols) * tex_height;
				AtlasTexture& texDef = atlasLookup.at(atlas_id).emplace_back();
				texDef.tex_pos = vec2(x_start, y_start);
				texDef.tex_size = vec2(tex_width, tex_height);
			}
			// insert mirrored texture positions
			AtlasTexture& texDef = atlasLookup.at(atlas_id).emplace_back();
			texDef.tex_pos = vec2(3 * tex_width, 0);
			texDef.tex_size = vec2(-tex_width, tex_height);
			AtlasTexture& texDef2 = atlasLookup.at(atlas_id).emplace_back();
			texDef2.tex_pos = vec2(4 * tex_width, 0);
			texDef2.tex_size = vec2(-tex_width, tex_height);
			AtlasTexture& texDef3 = atlasLookup.at(atlas_id).emplace_back();
			texDef3.tex_pos = vec2(2 * tex_width, tex_height);
			texDef3.tex_size = vec2(-tex_width, tex_height);
			AtlasTexture& texDef4 = atlasLookup.at(atlas_id).emplace_back();
			texDef4.tex_pos = vec2(3 * tex_width, tex_height);
			texDef4.tex_size = vec2(-tex_width, tex_height);
			AtlasTexture& texDef5 = atlasLookup.at(atlas_id).emplace_back();
			texDef5.tex_pos = vec2(tex_width, tex_height);
			texDef5.tex_size = vec2(-tex_width, tex_height);
			break;
		}
		case TEXTURE_ASSET_ID::TD_MAP_DECORATION_ATLAS: {
			constexpr unsigned int cols = 3, rows = 3, maxCount = cols * rows;
			constexpr float tex_width = 1.f / cols, tex_height = 1.f / rows;
			constexpr auto definedCount = static_cast<unsigned int>(TD_MAP_DECORATION_TEXTURES::COUNT);
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
        case TEXTURE_ASSET_ID::ARCHER: {
            constexpr unsigned int cols = 2, rows = 2, maxCount = cols * rows;
            constexpr float tex_width = 1.f / cols, tex_height = 1.f / rows;
            atlasLookup.emplace(atlas_id, std::vector<AtlasTexture>{});
            for (unsigned int i = 0; i < maxCount; i++) {
                const float x_start = (i % cols) * tex_width;
                const float y_start = (i / cols) * tex_height;
                AtlasTexture& texDef = atlasLookup.at(atlas_id).emplace_back();
                texDef.tex_pos = vec2(x_start, y_start);
                texDef.tex_size = vec2(tex_width, tex_height);
            }
            break;
        }
        case TEXTURE_ASSET_ID::BOW: {
            constexpr unsigned int cols = 2, rows = 2, maxCount = cols * rows;
            constexpr float tex_width = 1.f / cols, tex_height = 1.f / rows;
            atlasLookup.emplace(atlas_id, std::vector<AtlasTexture>{});
            for (unsigned int i = 0; i < maxCount; i++) {
                const float x_start = (i % cols) * tex_width;
                const float y_start = (i / cols) * tex_height;
                AtlasTexture& texDef = atlasLookup.at(atlas_id).emplace_back();
                texDef.tex_pos = vec2(x_start, y_start);
                texDef.tex_size = vec2(tex_width, tex_height);
            }
            break;
        }
        case TEXTURE_ASSET_ID::KNIGHT: {
            constexpr unsigned int cols = 2, rows = 2, maxCount = cols * rows;
            constexpr float tex_width = 1.f / cols, tex_height = 1.f / rows;
            atlasLookup.emplace(atlas_id, std::vector<AtlasTexture>{});
            for (unsigned int i = 0; i < maxCount; i++) {
                const float x_start = (i % cols) * tex_width;
                const float y_start = (i / cols) * tex_height;
                AtlasTexture& texDef = atlasLookup.at(atlas_id).emplace_back();
                texDef.tex_pos = vec2(x_start, y_start);
                texDef.tex_size = vec2(tex_width, tex_height);
            }
            break;
        }
        case TEXTURE_ASSET_ID::SWORD: {
            constexpr unsigned int cols = 2, rows = 3, maxCount = cols * rows;
            constexpr float tex_width = 1.f / cols, tex_height = 1.f / rows;
            atlasLookup.emplace(atlas_id, std::vector<AtlasTexture>{});
            for (unsigned int i = 0; i < maxCount; i++) {
                const float x_start = (i % cols) * tex_width;
                const float y_start = (i / cols) * tex_height;
                AtlasTexture& texDef = atlasLookup.at(atlas_id).emplace_back();
                texDef.tex_pos = vec2(x_start, y_start);
                texDef.tex_size = vec2(tex_width, tex_height);
            }
            break;
        }
        case TEXTURE_ASSET_ID::BOMB: {
            constexpr unsigned int cols = 4, rows = 2, maxCount = cols * rows;
            constexpr float tex_width = 1.f / cols, tex_height = 1.f / rows;
            atlasLookup.emplace(atlas_id, std::vector<AtlasTexture>{});
            for (unsigned int i = 0; i < maxCount; i++) {
                const float x_start = (i % cols) * tex_width;
                const float y_start = (i / cols) * tex_height;
                AtlasTexture& texDef = atlasLookup.at(atlas_id).emplace_back();
                texDef.tex_pos = vec2(x_start, y_start);
                texDef.tex_size = vec2(tex_width, tex_height);
            }
            break;
        }
        case TEXTURE_ASSET_ID::SPIKES: {
            constexpr unsigned int cols = 3, rows = 1, maxCount = cols * rows;
            constexpr float tex_width = 1.f / cols, tex_height = 1.f / rows;
            atlasLookup.emplace(atlas_id, std::vector<AtlasTexture>{});
            for (unsigned int i = 0; i < maxCount; i++) {
                const float x_start = (i % cols) * tex_width;
                const float y_start = (i / cols) * tex_height;
                AtlasTexture& texDef = atlasLookup.at(atlas_id).emplace_back();
                texDef.tex_pos = vec2(x_start, y_start);
                texDef.tex_size = vec2(tex_width, tex_height);
            }
            break;
        }
        case TEXTURE_ASSET_ID::SLIME: {
            constexpr unsigned int cols = 3, rows = 4, maxCount = cols * rows;
            constexpr float tex_width = 1.f / cols, tex_height = 1.f / rows;
            atlasLookup.emplace(atlas_id, std::vector<AtlasTexture>{});
            for (unsigned int i = 0; i < maxCount; i++) {
                const float x_start = (i % cols) * tex_width;
                const float y_start = (i / cols) * tex_height;
                AtlasTexture& texDef = atlasLookup.at(atlas_id).emplace_back();
                texDef.tex_pos = vec2(x_start, y_start);
                texDef.tex_size = vec2(tex_width, tex_height);
            }
            break;
        }
        case TEXTURE_ASSET_ID::SLIME_BIG: {
            constexpr unsigned int cols = 3, rows = 4, maxCount = cols * rows;
            constexpr float tex_width = 1.f / cols, tex_height = 1.f / rows;
            atlasLookup.emplace(atlas_id, std::vector<AtlasTexture>{});
            for (unsigned int i = 0; i < maxCount; i++) {
                const float x_start = (i % cols) * tex_width;
                const float y_start = (i / cols) * tex_height;
                AtlasTexture& texDef = atlasLookup.at(atlas_id).emplace_back();
                texDef.tex_pos = vec2(x_start, y_start);
                texDef.tex_size = vec2(tex_width, tex_height);
            }
            break;
        }
		case TEXTURE_ASSET_ID::BUTTONS: {
			constexpr unsigned int cols = 2, rows = 2, maxCount = cols * rows;
            constexpr float tex_width = 1.f / cols, tex_height = 1.f / rows;
            atlasLookup.emplace(atlas_id, std::vector<AtlasTexture>{});
            for (unsigned int i = 0; i < maxCount; i++) {
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
	WATER,
    //CHARACTER_SPRITE,
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
			fprintf(stderr, "Invalid EFFECT_ASSET_ID: %d\n", static_cast<int>(id));
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
	std::vector<Stationary> offset_positions;
	std::vector<unsigned int> atlas_ids;
	float z_position;
	TEXTURE_ASSET_ID used_texture = TEXTURE_ASSET_ID::TEXTURE_COUNT;
	EFFECT_ASSET_ID used_effect = EFFECT_ASSET_ID::EFFECT_COUNT;
	GEOMETRY_BUFFER_ID used_geometry = GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;
};


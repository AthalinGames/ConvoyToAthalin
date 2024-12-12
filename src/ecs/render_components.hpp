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
    ARCHER_L,
    ARCHER_U,
    ARCHER_R,
    ARCHER_D,
    ARCHER_CARD,
    BOW1,
    BOW2,
    BOW3,
	ARROW,
    SLIME,
    SLIME_L,
    SLIME_U,
    SLIME_R,
    SLIME_D,
    MAP,
    MAP2,
    MAP3,
    MAP4,
    MAP5,
    MAP6,
    MAP7,
	OVERVIEW_MAP,
	BLACK_PIXEL,
	GAME_OVER,
	OVERVIEW_ICONS_ATLAS,
	ASCII_CHAR_ATLAS,
	TEXTURE_COUNT
};

constexpr const char* TextureAssetIDToString(const TEXTURE_ASSET_ID id) {
	switch (id) {
		case TEXTURE_ASSET_ID::FISH: return "fish.png";
		case TEXTURE_ASSET_ID::TURTLE: return "turtle.png";
		case TEXTURE_ASSET_ID::ARCHER: return "archer.png";
        case TEXTURE_ASSET_ID::ARCHER_L: return "archer_perspective2.png";
        case TEXTURE_ASSET_ID::ARCHER_U: return "archer_perspective1.png";
        case TEXTURE_ASSET_ID::ARCHER_R: return "archer_perspective4.png";
        case TEXTURE_ASSET_ID::ARCHER_D: return "archer_perspective3.png";
        case TEXTURE_ASSET_ID::ARCHER_CARD: return "archerCard.png";
        case TEXTURE_ASSET_ID::BOW1: return "bow_and_arrow1.png";
        case TEXTURE_ASSET_ID::BOW2: return "bow_and_arrow2.png";
        case TEXTURE_ASSET_ID::BOW3: return "bow_and_arrow3.png";
		case TEXTURE_ASSET_ID::ARROW: return "arrow.png";
        case TEXTURE_ASSET_ID::SLIME: return "Slime.png";
        case TEXTURE_ASSET_ID::SLIME_L: return "Slime4.png";
        case TEXTURE_ASSET_ID::SLIME_U: return "Slime1.png";
        case TEXTURE_ASSET_ID::SLIME_R: return "Slime2.png";
        case TEXTURE_ASSET_ID::SLIME_D: return "Slime3.png";
        case TEXTURE_ASSET_ID::MAP: return "tdmap_tiled.png";
        case TEXTURE_ASSET_ID::MAP2: return "tdmap_tiled2.png";
        case TEXTURE_ASSET_ID::MAP3: return "tdmap_tiled3.png";
        case TEXTURE_ASSET_ID::MAP4: return "tdmap_tiled4.png";
        case TEXTURE_ASSET_ID::MAP5: return "tdmap_tiled5.png";
        case TEXTURE_ASSET_ID::MAP6: return "tdmap_tiled6.png";
        case TEXTURE_ASSET_ID::MAP7: return "tdmap_tiled7.png";
		case TEXTURE_ASSET_ID::OVERVIEW_MAP: return "overview_map.png";
		case TEXTURE_ASSET_ID::BLACK_PIXEL: return "blackPixel.png";
		case TEXTURE_ASSET_ID::GAME_OVER: return "game_over.png";
		case TEXTURE_ASSET_ID::OVERVIEW_ICONS_ATLAS: return "overviewIconsAtlas.png";
		case TEXTURE_ASSET_ID::ASCII_CHAR_ATLAS: return "charmap-oldschool_preview.png"; // Source: https://opengameart.org/content/ascii-bitmap-font-oldschool
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


#pragma once

#include <array>
#include <utility>

//#include <ft2build.h>
//#include FT_FREETYPE_H

#include "common.hpp"
#include "ecs/components.hpp"
#include "ecs/tiny_ecs.hpp"

struct Character {
	GLuint textureID;
	ivec2 size;
	ivec2 bearing;
	//FT_Pos advance;
};

// System responsible for setting up OpenGL and for rendering all the
// visual entities in the game
class RenderSystem {
	/**
	 * The following arrays store the assets the game will use. They are loaded
	 * at initialization and are assumed to not be modified by the render loop.
	 *
	 * Whenever possible, add to these lists instead of creating dynamic state
	 * it is easier to debug and faster to execute for the computer.
	 */
	std::array<GLuint, texture_count> texture_gl_handles;
	std::array<ivec2, texture_count> texture_dimensions;

	// Make sure these paths remain in sync with the associated enumerators.
	// Associated id with .obj path
	const std::vector < std::pair<GEOMETRY_BUFFER_ID, std::string>> mesh_paths =
	{
		  std::pair(GEOMETRY_BUFFER_ID::SALMON, mesh_path("salmon.obj"))
		  // specify meshes of other assets here
	};

	// Make sure these paths remain in sync with the associated enumerators.
	const std::array<std::string, texture_count> texture_paths = [] {
		std::array<std::string, texture_count> textures{};
		for (int i = 0; i < texture_count; i++) { // iterate over textures to create paths
			textures[i] = textures_path(TextureAssetIDToString(static_cast<TEXTURE_ASSET_ID> (i)));
		}
		return textures;
	} ();

	// Setup TextureAtlas lookup table
	const std::map<TEXTURE_ASSET_ID, std::vector<AtlasTexture>> atlasLookup = [] {
		std::map<TEXTURE_ASSET_ID, std::vector<AtlasTexture>> lookup{};
		for (const auto & texture_atlas : texture_atlases) {
			initTextureAtlasTextures(texture_atlas, lookup);
		}
		return lookup;
	} ();

	std::array<GLuint, effect_count> effects;
	// Make sure these paths remain in sync with the associated enumerators.
	const std::array<std::string, effect_count> effect_paths = [] {
		std::array<std::string, effect_count> effects{};
		for (int i = 0; i < effect_count; i++) { // iterate over shaders to create paths
			effects[i] = shader_path(EffectAssetIDToString(static_cast<EFFECT_ASSET_ID> (i)));
		}
		return effects;
	} ();

	std::array<GLuint, geometry_count> vertex_buffers;
	std::array<GLuint, geometry_count> index_buffers;
	std::array<Mesh, geometry_count> meshes;

	std::map<char, Character> Characters;

public:
	// Initialize the window
	bool init(GLFWwindow* window);

	template <class T>
	void bindVBOandIBO(GEOMETRY_BUFFER_ID gid, std::vector<T> vertices, std::vector<uint16_t> indices, GLenum drawType=GL_STATIC_DRAW);

	void initializeGlTextures();

	void initializeGlEffects();

	void initializeGlMeshes();
	Mesh& getMesh(GEOMETRY_BUFFER_ID id) { return meshes[static_cast<int>(id)]; };

	void initializeGlGeometryBuffers();
	// Initialize the screen texture used as intermediate render target
	// The draw loop first renders to this texture, then it is used for the water
	// shader
	bool initScreenTexture();

	void initializeImGui();

//	void initializeFont();

	// Destroy resources associated to one or all entities created by the system
	~RenderSystem();

	// Draw all entities
	void draw();

	constexpr static mat3 createProjectionMatrix();

private:
	// Internal drawing functions for each entity type

	void doTexturedRender(GLuint program, const RenderRequestSingle& render_request) const;

	void drawTexturedMesh(
		Entity entity,
		mat3& projection,
		Transform& transform,
		const RenderRequestSingle& render_request) const;

	void drawToScreen();

	// Window handle
	GLFWwindow* window;

	// Screen texture handles
	GLuint frame_buffer;
	GLuint off_screen_render_buffer_color;
	GLuint off_screen_render_buffer_depth;

	Entity screen_state_entity;
};

bool loadEffectFromFile(
	const std::string& vs_path, const std::string& fs_path, GLuint& out_program);

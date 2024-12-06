// internal
#include "render_system.hpp"
#include "world_init.hpp"
#include <SDL.h>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "ecs/tiny_ecs_registry.hpp"

// applies rotation to transform or selects fitting directional sprite depending on use_direction_sprite
void RenderSystem::applyTextureRotation(RenderRequestSingle& render_request, //TODO maybe take just RenderRequest and check which one
                                        Transform& transform,
                                        const Entity entity,
                                        const float angle,
                                        const bool use_direction_sprite) {

    // if bow_and_arrow rotate, if character sprite choose view direction sprite
    if (!use_direction_sprite) {
        transform.rotate(angle);
        if (registry.bows.has(entity)) {
            if (angle < 0) {
                render_request.z_position = Z_BACKGROUND;
            } else {
                render_request.z_position = Z_FOREGROUND;
            }
        }
    } else { // decide cardinal directions by angle in pi/4
        if (registry.archers.has(entity)) {
            float angle_by_pi = angle / M_PI;
            //printf("angle %f\n", angle);
            float temp_whole;
            //angle_by_pi = std::modf(angle_by_pi, &temp_whole);
            //printf("angle mod %f\n", angle_by_pi);
            if (angle_by_pi >= -0.25 && angle_by_pi < 0.25) {
                //look left
                render_request.used_texture = TEXTURE_ASSET_ID::ARCHER_L;
            } else if (angle_by_pi >= 0.25 && angle_by_pi < 0.75) {
                //look up
                render_request.used_texture = TEXTURE_ASSET_ID::ARCHER_U;
            } else if (angle_by_pi >= 0.75 || angle_by_pi < -0.75) {
                //look right
                render_request.used_texture = TEXTURE_ASSET_ID::ARCHER_R;
            } else if (angle_by_pi >= -0.75 && angle_by_pi < -0.25) {
                //look down
                render_request.used_texture = TEXTURE_ASSET_ID::ARCHER_D;
            }
        }
    }

}

void RenderSystem::doTexturedRender(const GLuint program, const RenderRequestSingle& render_request) const {
	const GLint in_position_loc = glGetAttribLocation(program, "in_position");
	const GLint in_texcoord_loc = glGetAttribLocation(program, "in_texcoord");
	gl_has_errors();
	assert(in_texcoord_loc >= 0);

	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE,
	                      sizeof(TexturedVertex), static_cast<void *>(nullptr));
	gl_has_errors();

	glEnableVertexAttribArray(in_texcoord_loc);
	glVertexAttribPointer(
		in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex),
		reinterpret_cast<void *>(sizeof(vec3))); // note the stride to skip the preceeding vertex position

	// Enabling and binding texture to slot 0
	glActiveTexture(GL_TEXTURE0);
	gl_has_errors();

	const GLuint texture_id =
			texture_gl_handles[static_cast<GLuint>(render_request.used_texture)];

	glBindTexture(GL_TEXTURE_2D, texture_id);
	gl_has_errors();
}

void RenderSystem::drawTexturedMesh(const Entity entity,
                                    mat3 &projection,
                                    Transform& transform,
                                    const RenderRequestSingle& render_request) const {

	const auto used_effect_enum = static_cast<GLuint>(render_request.used_effect);
	assert(used_effect_enum != static_cast<GLuint>(EFFECT_ASSET_ID::EFFECT_COUNT));
	const GLuint program = effects[used_effect_enum];

	// Setting shaders
	glUseProgram(program);
	gl_has_errors();

	assert(render_request.used_geometry != GEOMETRY_BUFFER_ID::GEOMETRY_COUNT);
	const GLuint vbo = vertex_buffers[static_cast<GLuint>(render_request.used_geometry)];
	const GLuint ibo = index_buffers[static_cast<GLuint>(render_request.used_geometry)];

	// Setting vertex and index buffers
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	gl_has_errors();

	// Input data location as in the vertex buffer
	switch (render_request.used_effect) {
		case EFFECT_ASSET_ID::TEXTURED: {
			doTexturedRender(program, render_request);
			break;
		}
		case EFFECT_ASSET_ID::TEXTURED_ATLAS: {
			// Getting uniform location for texture coordinate modification
			const GLuint tex_pos_uloc = glGetUniformLocation(program, "tex_pos");
			const GLuint tex_area_uloc = glGetUniformLocation(program, "tex_area");
			AtlasTexture atlas_texture = atlasLookup.at(render_request.used_texture)[render_request.used_texture_atlas_texture_id];
			glUniform2fv(tex_pos_uloc, 1, reinterpret_cast<float *> (&atlas_texture.tex_pos));
			glUniform2fv(tex_area_uloc, 1, reinterpret_cast<float *> (&atlas_texture.tex_size));
			gl_has_errors();
			// Render Texture
			doTexturedRender(program, render_request);
			break;
		}
		default:
			assert(false && "Type of render request not supported");
	}

	// Getting uniform locations for glUniform* calls
	const GLint color_uloc = glGetUniformLocation(program, "fcolor");
	vec3 color = registry.colors.has(entity) ? registry.colors.get(entity) : vec3(1);
	glUniform3fv(color_uloc, 1, reinterpret_cast<float *>(&color));
	gl_has_errors();

	// Get number of indices from index buffer, which has elements uint16_t
	GLint size = 0;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
	gl_has_errors();

	const GLsizei num_indices = size / sizeof(uint16_t);
	// GLsizei num_triangles = num_indices / 3;

	GLint currProgram;
	glGetIntegerv(GL_CURRENT_PROGRAM, &currProgram);
	// Setting uniform values to the currently bound program
	const GLuint transform_loc = glGetUniformLocation(currProgram, "transform");
	glUniformMatrix3fv(transform_loc, 1, GL_FALSE, reinterpret_cast<float *>(&transform.mat));
	const GLuint projection_loc = glGetUniformLocation(currProgram, "projection");
	glUniformMatrix3fv(projection_loc, 1, GL_FALSE, reinterpret_cast<float *>(&projection));
	const GLuint z_pos_loc = glGetUniformLocation(currProgram, "z_pos");
	glUniform1f(z_pos_loc, render_request.z_position);
	gl_has_errors();
	// Drawing of num_indices/3 triangles specified in the index buffer
	glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
	gl_has_errors();
}


// draw the intermediate texture to the screen, with some distortion to simulate
// water
void RenderSystem::drawToScreen()
{
	// Setting shaders
	// get the water texture, sprite mesh, and program
	glUseProgram(effects[static_cast<GLuint>(EFFECT_ASSET_ID::WATER)]);// TODO: Replace with battle map background?
	gl_has_errors();
	// Clearing backbuffer
	int w, h;
	glfwGetFramebufferSize(window, &w, &h); // Note, this will be 2x the resolution given to glfwCreateWindow on retina displays
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, w, h);
	glDepthRange(0, 10);
	glClearColor(1.f, 0, 0, 1.0);
	glClearDepth(1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	gl_has_errors();
	// Enabling alpha channel for textures
	glDisable(GL_BLEND);
	// glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);

	// Draw the screen texture on the quad geometry
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers[static_cast<GLuint>(GEOMETRY_BUFFER_ID::SCREEN_TRIANGLE)]);
	glBindBuffer(
		GL_ELEMENT_ARRAY_BUFFER,
		index_buffers[static_cast<GLuint>(GEOMETRY_BUFFER_ID::SCREEN_TRIANGLE)]); // Note, GL_ELEMENT_ARRAY_BUFFER associates
																	 // indices to the bound GL_ARRAY_BUFFER
	gl_has_errors();

	// TODO: Replace with battle map background?
	const GLuint water_program = effects[static_cast<GLuint>(EFFECT_ASSET_ID::WATER)];
	// Set clock
	const GLuint time_uloc = glGetUniformLocation(water_program, "time");
	const GLuint dead_timer_uloc = glGetUniformLocation(water_program, "screen_darken_factor");
	glUniform1f(time_uloc, static_cast<float>(glfwGetTime() * 10.0f));
	const ScreenState &screen = registry.screenStates.get(screen_state_entity);
	glUniform1f(dead_timer_uloc, screen.screen_darken_factor);
	gl_has_errors();
	// Set the vertex position and vertex texture coordinates (both stored in the
	// same VBO)
	const GLint in_position_loc = glGetAttribLocation(water_program, "in_position");
	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), static_cast<void *>(nullptr));
	gl_has_errors();


	// Bind our texture in Texture Unit 0
	glActiveTexture(GL_TEXTURE0);

	glBindTexture(GL_TEXTURE_2D, off_screen_render_buffer_color);
	gl_has_errors();
	// Draw
	glDrawElements(
		GL_TRIANGLES, 3, GL_UNSIGNED_SHORT,
		nullptr); // one triangle = 3 vertices; nullptr indicates that there is
				  // no offset from the bound index buffer
	gl_has_errors();
}

// Render our game world
// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-14-render-to-texture/
void RenderSystem::draw()
{
	// Getting size of window
	int w, h;
	glfwGetFramebufferSize(window, &w, &h); // Note, this will be 2x the resolution given to glfwCreateWindow on retina displays

	// First render to the custom framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
	gl_has_errors();
	// Clearing backbuffer
	glViewport(0, 0, w, h);
	glDepthRange(0.00001, 10);
	glClearColor(0, 0, 0, 1.0);
	glClearDepth(10.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST); // native OpenGL does not work with a depth buffer
							  // and alpha blending, one would have to sort
							  // sprites back to front
	gl_has_errors();
	mat3 projection_2D = createProjectionMatrix();
	// Draw all textured meshes that have a position and size component
	// TODO: rework this to enable more influence on order of rendering
	for (std::size_t i = 0; i < registry.renderRequests.size(); ++i) {
		const Entity entity = registry.renderRequests.entities[i];
		RenderRequest& request = registry.renderRequests.components[i];
		if (registry.invisibles.has(entity)) {
			continue;
		}
		// calculate base transform;
		vec2 position;
		vec2 scale;
		float angle;
		bool direction_sprite;
		if (registry.motions.has(entity)) {
			const Motion &motion = registry.motions.get(entity);
			position = motion.position;
			angle = motion.angle;
			scale = motion.scale;
			direction_sprite = motion.use_direction_sprite;
		} else if (registry.stationaries.has(entity)) {
			const Stationary &stationary = registry.stationaries.get(entity);
			position = stationary.position;
			angle = stationary.angle;
			scale = stationary.scale;
			direction_sprite = stationary.use_direction_sprite;
		} else {
			assert(false && "RenderRequest does not have a position");
		}
		// dispatch render request
		if (RenderRequestSingle *single_request = std::get_if<RenderRequestSingle>(&request)) {
			Transform transform;
			transform.translate(position);
			applyTextureRotation(*single_request, transform, entity, angle, direction_sprite);
			transform.scale(scale);
			drawTexturedMesh(entity, projection_2D, transform, *single_request);
		} else if (RenderRequestMulti *multi_request = std::get_if<RenderRequestMulti>(&request)) {
			for (auto & [render_request, stationary] : multi_request->requests) {
				Transform transform;
				transform.translate(stationary.position + position);
				applyTextureRotation(render_request, transform, entity, stationary.angle + angle, stationary.use_direction_sprite);
				transform.scale(scale);
				drawTexturedMesh(entity, projection_2D, transform, render_request);
			}
		}
	}

	// Truly render to the screen
	drawToScreen();

	// Render ImGui Stuff
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	// flicker-free display with a double buffer
	glfwSwapBuffers(window);
	gl_has_errors();
}

constexpr mat3 RenderSystem::createProjectionMatrix() {
	// Fake projection matrix, scales with respect to window coordinates
	constexpr float left = 0.f;
	constexpr float top = 0.f;

	constexpr float right = static_cast<float>(window_width_px);
	constexpr float bottom = static_cast<float>(window_height_px);

	constexpr float sx = 2.f / (right - left);
	constexpr float sy = 2.f / (top - bottom);
	constexpr float tx = -(right + left) / (right - left);
	constexpr float ty = -(top + bottom) / (top - bottom);
	return {{sx, 0.f, 0.f}, {0.f, sy, 0.f}, {tx, ty, 1.f}};
}

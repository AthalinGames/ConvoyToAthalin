
#define GL3W_IMPLEMENTATION
#include <gl3w.h>

// stlib
#include <chrono>

// internal
#include <memory>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "systems/physics_system.hpp"
#include "systems/render_system.hpp"
#include "systems/world_system.hpp"

using Clock = std::chrono::high_resolution_clock;

// Entry point
int main(const int argc, char* argv[]) {
	bool windowed = false;
	for (int arg_no = 0; arg_no < argc; ++arg_no) {
		if (!strcmp(argv[arg_no], "--window") || !strcmp(argv[arg_no], "-w")) {
			windowed = true;
		}
	}

	// Global systems
	RenderSystem render_system;
	PhysicsSystem physics_system;
	WorldSystem world_system;

	// Initializing window
	GLFWwindow* window = world_system.create_window(windowed);
	if (!window) {
		// Time to read the error message
		printf("Press any key to exit");
		getchar();
		return EXIT_FAILURE;
	}

	// initialize the main systems
	render_system.init(window);
	world_system.init(&render_system);

	// variable timestep loop
	auto t = Clock::now();
	while (!world_system.is_over()) {
		// Processes system messages, if this wasn't present the window would become unresponsive
		glfwPollEvents();

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Calculating elapsed times in milliseconds from the previous iteration
		auto now = Clock::now();
		const float elapsed_ms =
			static_cast<float>((std::chrono::duration_cast<std::chrono::microseconds>(now - t)).count()) / 1000;
		t = now;

		world_system.step(elapsed_ms);
		physics_system.step(elapsed_ms);

		world_system.handle_collisions();
		world_system.handle_post_collision_actions();

		render_system.draw();
	}

	return EXIT_SUCCESS;
}

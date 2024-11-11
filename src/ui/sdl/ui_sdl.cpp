#include <gb/ui/ui.h>
#include <gb/utils/sdl_log.h>

#include <glad/gl.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include <format>
#include <stdexcept>
#include <string_view>

namespace gb::ui::sdl {

struct SDLGui : UI {
	static constexpr std::string_view name = "gui";

	SDLGui(int argc, const char* const argv[]) {
		if(argc < 4 || argc > 5) {
			const char* binary_name = argv[0] ? argv[0] : "<binary>";
			throw std::invalid_argument(std::format("Usage: {} gui <boot rom> <game rom> [save data]", binary_name));
		}

		gb::logging::init_sdl_logging();

		GB_SDL_checked(SDL_Init(SDL_INIT_VIDEO));
	}

	void main_loop() override {
		{ // simple white window, based on https://lazyfoo.net/tutorials/SDL/01_hello_SDL/index2.php
			SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
			SDL_Window* window = GB_SDL_checkptr(SDL_CreateWindow("GB", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN));
			SDL_GLContext context = GB_SDL_checkptr(SDL_GL_CreateContext(window));
			const int version = gladLoadGL(reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress));
			GB_log_info("OpenGL version {}.{}", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));

			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO();
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
			// io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
			io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
			ImGui::StyleColorsDark();
			if(!ImGui_ImplSDL2_InitForOpenGL(window, context)) throw GB_exc("");
			if(!ImGui_ImplOpenGL3_Init()) throw GB_exc("");

			SDL_Event e;
			bool quit = false;
			bool show_imgui_demo = true;
			bool show_custom_window = true;
			while( quit == false ){
				while( SDL_PollEvent( &e ) ){
					ImGui_ImplSDL2_ProcessEvent(&e);
					// TODO: filter keyboard/mouse events based on whether imgui is active
					if( e.type == SDL_QUIT ) quit = true;
					if(e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE && e.window.windowID == SDL_GetWindowID(window)) quit = true;
				}

				ImGui_ImplOpenGL3_NewFrame();
				ImGui_ImplSDL2_NewFrame();
				ImGui::NewFrame();
				if(show_imgui_demo) ImGui::ShowDemoWindow(&show_imgui_demo);

				if(show_custom_window) {
					ImGui::Begin("foo", &show_custom_window, ImGuiWindowFlags_MenuBar);
					if(ImGui::BeginMenuBar()) {
						if (ImGui::MenuItem("Close", "Ctrl+W"))  { show_custom_window = false; }
						ImGui::EndMenuBar();
					}
					ImGui::End();
				}

				glClearColor(0.7f, 0.9f, 0.1f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT);

				ImGui::Render();
				ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

				SDL_GL_SwapWindow(window);

				if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
				{
					ImGui::UpdatePlatformWindows();
					ImGui::RenderPlatformWindowsDefault();
					SDL_GL_MakeCurrent(window, context);
				}

				SDL_Delay(1);
			}

			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplSDL2_Shutdown();
			ImGui::DestroyContext();

			SDL_GL_DeleteContext(context);
			SDL_DestroyWindow(window);
		}
	}

	~SDLGui() override {
		SDL_Quit();
	}
};

static auto registration [[maybe_unused]] = (UI::register_ui_type(SDLGui::name, [](int argc, const char* const argv[]){ return std::make_unique<SDLGui>(argc, argv); }), 0);

}
#include <gb/gb.h>
#include <gb/ui/ui.h>
#include <gb/utils/load_file.h>
#include <gb/utils/sdl_log.h>

#include <glad/gl.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include <format>
#include <optional>
#include <stdexcept>
#include <string_view>

namespace gb::ui::sdl {

namespace {

constexpr std::optional<joypad::joypad_bits> translate_keycode(const SDL_Keysym sym) {
	using enum joypad::joypad_bits;
	switch(sym.scancode) {
		case SDL_SCANCODE_UP: return dpad_up;
		case SDL_SCANCODE_DOWN: return dpad_down;
		case SDL_SCANCODE_LEFT: return dpad_left;
		case SDL_SCANCODE_RIGHT: return dpad_right;
		case SDL_SCANCODE_SPACE: return select;
		case SDL_SCANCODE_RETURN: return start;
		case SDL_SCANCODE_X: return a;
		case SDL_SCANCODE_Z: return b;
		default: return std::nullopt;
	}
}

}

// contains useful debugging state.
// TODO: this should be ported to tui mode?
// TODO: color binary bits differently in show8 so you can see flickers easily.
struct Debugger {
	bool visible{false}; // can be toggled by host ui.

	void handle_frame(const gb::gameboy_emulator& emulator) {
		if(!visible) return;

		constexpr static auto show8 = [](const std::string_view label, uint8_t value){
			ImGui::Text("%s: \t[%s]", label.data(), std::format("{0:#04x} = {0:#010b}", value).c_str());
		};
		#define SHOW_MMU(x) show8(std::format(#x " ({:#06x})", memory::addrs::##x), emulator.mmu.get<memory::addrs::##x>());

		ImGui::Begin("Debugger", &visible);
		if(ImGui::TreeNode("CPU")) {
			ImGui::TextUnformatted(emulator.cpu.dump_state().c_str());
			ImGui::TreePop();
		}
		if(ImGui::TreeNode("PPU")) {
			ImGui::TextUnformatted(emulator.ppu.dump_state().c_str());
			ImGui::TreePop();
		}
		if(ImGui::TreeNode("MMU")) {
			SHOW_MMU(INTERRUPT_FLAG);
			SHOW_MMU(INTERRUPT_ENABLE);
			ImGui::TreePop();
		}
		if(ImGui::TreeNode("Joypad")) {
			SHOW_MMU(JOYPAD);
			for(uint8_t i = 0; i<8; i++) {
				const auto joypad_enum = static_cast<joypad::joypad_bits>(i);
				ImGui::TextUnformatted(std::format("{}: {}", joypad_enum, emulator.get_joypad().read_button(joypad_enum)).c_str());
			}
			ImGui::TreePop();
		}
		ImGui::End();
	}
	#undef SHOW_MMU
};

struct SDLGui : UI {
	static constexpr std::string_view name = "gui";

	SDLGui(int argc, const char* const argv[]) {
		IMGUI_CHECKVERSION();
		if(argc < 4 || argc > 5) {
			const char* binary_name = argv[0] ? argv[0] : "<binary>";
			throw std::invalid_argument(std::format("Usage: {} gui <boot rom> <game rom> [save data]", binary_name));
		}
		auto bootrom = gb::load_file(argv[2]);
		auto cartridgerom = gb::load_file(argv[3]);
		std::optional<std::vector<uint8_t>> savedata;
		if(argc >= 5) savedata = gb::load_file(argv[4]);
		log_info("Loaded files");
		emulator.emplace(std::move(bootrom), std::move(cartridgerom), std::move(savedata));

		gb::logging::init_sdl_logging();

		sdl_checked(SDL_Init(SDL_INIT_VIDEO));
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		ImGui::StyleColorsDark();
		// init SDL + OpenGL
		window = sdl_checkptr(SDL_CreateWindow("GB", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 3 * ppu::LCD_WIDTH, 3 * ppu::LCD_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN));
		context = sdl_checkptr(SDL_GL_CreateContext(window));
		const int version = gladLoadGL(reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress));
		log_info("OpenGL version {}.{}", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));

		if(!ImGui_ImplSDL2_InitForOpenGL(window, context)) throw_exc();
		if(!ImGui_ImplOpenGL3_Init()) throw_exc();
	}

	int main_loop() override {
		const ImGuiIO& io = ImGui::GetIO();
		
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		GLuint fboId = 0;
		glGenFramebuffers(1, &fboId);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, fboId);
		glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

		bool quit = false;
		while( quit == false ){
			for(SDL_Event e; SDL_PollEvent( &e );){ // handle SDL events
				ImGui_ImplSDL2_ProcessEvent(&e);
				// TODO: filter keyboard/mouse events based on whether imgui is active
				switch(e.type) {
					case SDL_QUIT:
						quit = true;
						break;
					case SDL_WINDOWEVENT:
						if(e.window.event == SDL_WINDOWEVENT_CLOSE && e.window.windowID == SDL_GetWindowID(window)) quit = true;
						break;
					case SDL_KEYDOWN:
						if(io.WantCaptureKeyboard) break;
						if(e.key.repeat) break;
						if(const auto translated = translate_keycode(e.key.keysym); translated) emulator->press(*translated);
						if(e.key.keysym.scancode == SDL_SCANCODE_D) {
							debugger.visible = !debugger.visible;
						}
						break;
					case SDL_KEYUP:
						if(io.WantCaptureKeyboard) break;
						if(const auto translated = translate_keycode(e.key.keysym); translated) emulator->release(*translated);
						break;
				}
			}

			// TODO: if this throws, freeze on current screen and let us poke around in debugger?
			const auto frame_begin = SDL_GetPerformanceCounter();
			emulator->run_frame();
			const auto frame_end = SDL_GetPerformanceCounter();
			gb::log_debug("frame took {} ms", static_cast<double>(frame_end - frame_begin) * 1000 / SDL_GetPerformanceFrequency());

			// Render GB screen
			prepare_texture();
			int viewportWidth, viewportHeight;
			SDL_GL_GetDrawableSize(window, &viewportWidth, &viewportHeight);
			glBlitFramebuffer(0, 0, ppu::LCD_WIDTH, ppu::LCD_HEIGHT, 0, 0, viewportWidth, viewportHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);

			{ // IMGui
				ImGui_ImplOpenGL3_NewFrame();
				ImGui_ImplSDL2_NewFrame();
				ImGui::NewFrame();

				debugger.handle_frame(*emulator);

				ImGui::Render();
				ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			}

			SDL_GL_SwapWindow(window);

			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
				SDL_GL_MakeCurrent(window, context);
			}

			SDL_Delay(1);
			// TODO: figure out how to sync <60 Hz emulation to 60 hz screen (more important once audio works)
		}

		return 0;
	}

	~SDLGui() override {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();
		if(context) SDL_GL_DeleteContext(context);
		if(window) SDL_DestroyWindow(window);
		SDL_Quit();
	}

private:
	/// render the game boy screen to a texture.
	/// colors may not be accurate.
	/// TODO apply postprocessing with a shader?
	void prepare_texture(){
		const auto& frame = emulator->ppu.cur_frame();
		std::array<std::array<uint16_t, ppu::LCD_WIDTH>, ppu::LCD_HEIGHT> texdata; // RGBA 5551
		for(size_t row = 0; row < ppu::LCD_HEIGHT; row++) {
			for(size_t col = 0; col < ppu::LCD_WIDTH; col++) {
				const auto& framepix = frame[row][col];
				auto& texpix = texdata[(ppu::LCD_HEIGHT - 1) - row][col];
				const uint16_t translate = 21 - (framepix.raw * 7);
				const auto translate_dim = translate >> 1; // make red/blue dimmer
				texpix = static_cast<uint16_t>((translate_dim << 11) | (translate << 6) | (translate_dim << 1) | 1);
			}
		}
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB5_A1, ppu::LCD_WIDTH, ppu::LCD_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, texdata.front().data());
	}

	std::optional<gb::gameboy_emulator> emulator;
	Debugger debugger;
	SDL_Window* window{nullptr};
	SDL_GLContext context{nullptr};
};

static auto registration [[maybe_unused]] = (UI::register_ui_type(SDLGui::name, [](int argc, const char* const argv[]){ return std::make_unique<SDLGui>(argc, argv); }), 0);

}
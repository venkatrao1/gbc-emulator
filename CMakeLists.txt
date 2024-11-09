cmake_minimum_required(VERSION 3.26) # the one I have installed

project(Project)

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

foreach(SDL_DISABLED_FLAGS
	ATOMIC RENDER HIDAPI POWER FILE CPUINFO FILESYSTEM LOCALE # subsystems
	OPENGLES RPI COCOA DIRECTX VIVANTE VULKAN METAL OFFSCREEN # other video backends (using OpenGL)
	TEST
)
	option("SDL_${SDL_DISABLED_FLAGS}" OFF)
endforeach()

FetchContent_Declare(
    SDL2
	URL https://github.com/libsdl-org/SDL/archive/refs/tags/release-2.30.9.zip
	SYSTEM
)

FetchContent_Declare(
	DearImGui
	URL https://github.com/ocornut/imgui/archive/refs/tags/v1.91.5-docking.zip
	SYSTEM
)

FetchContent_MakeAvailable(
	SDL2
	DearImGui
)

include_directories(.) # for headers and ipp
include_directories(include)

# compiler flags
if(MSVC)
	# TODO: more compiler flags, debug vs release builds?
	add_compile_options(/W4 /WX) # warning level 4, treat warnings as errors
	add_compile_options(/we5262) # disable falling through by default - the CPU is one big switch statement, and we don't want to fall through there.
	add_link_options(/LTCG)
	if(CMAKE_BUILD_TYPE STREQUAL "Release")
		add_compile_options(/GL /Gr)
		# add_compile_options(/arch:AVX2 /favor:INTEL64) # for my machine specifically. Ideally I should have a cmake test for this?
		# add_compile_options(/fp:fast) # unsafe but faster hopefully
	endif()
else()
	message(WARNING "No compiler flags set, I only have compiled on MSVC!")
endif()

# for IntelliSense to use
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

# add executables
add_executable(app)
target_compile_features(app PUBLIC cxx_std_20)

# SDL2main must come before SDL2
if(TARGET SDL2::SDL2main)
    target_link_libraries(app PRIVATE SDL2::SDL2main)
endif()
target_link_libraries(app PRIVATE SDL2::SDL2)

add_subdirectory(src)
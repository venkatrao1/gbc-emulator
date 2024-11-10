add_subdirectory(sdl)
add_subdirectory(tui)

target_sources(
	app
	PRIVATE
	ui.cpp
)
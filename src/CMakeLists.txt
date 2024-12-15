add_subdirectory(memory)
add_subdirectory(ui)
add_subdirectory(utils)

target_sources(
	app
	PRIVATE
	joypad.cpp
	main.cpp
)
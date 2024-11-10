add_subdirectory(memory)
add_subdirectory(ui)
add_subdirectory(utils)

target_sources(
	app
	PRIVATE
	main.cpp
)
add_subdirectory(memory)
add_subdirectory(utils)

target_sources(
	app
	PRIVATE
	main.cpp
)
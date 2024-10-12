cmake_minimum_required(VERSION 3.26) # the one I have installed

project(Project)

# SFML
set(SFML_STATIC_LIBRARIES true)
find_package(SFML 2.6.1 COMPONENTS window graphics system audio network)
link_libraries(
	# Link all SFML libraries, because SFML isn't very big anyway
	sfml-graphics
	sfml-window
	sfml-audio
	sfml-network
	sfml-system
)

# Eigen (potentially)
# if(NOT Eigen3_INCLUDE_DIR)
# 	message(SEND_ERROR "No Eigen3_INCLUDE_DIR specified!")
# endif()
# include_directories("${Eigen3_INCLUDE_DIR}")

# TODO: multiple target support probably will not be good here, when to use add vs target?

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(.) # for headers and ipp
include_directories(include)

# compiler flags
if(MSVC)
	# TODO: more compiler flags, debug vs release builds?
	add_compile_options(/W4 /WX) # warning level 4, treat warnings as errors
	if(CMAKE_BUILD_TYPE STREQUAL "Release")
		add_compile_options(/GL /Gr)
		add_compile_options(/arch:AVX2 /favor:INTEL64) # for my machine specifically. Ideally I should have a cmake test for this?
		# add_compile_options(/fp:fast) # unsafe but faster hopefully
	endif()
else()
	message(AUTHOR_WARNING "No compiler flags set, I only have compiled on MSVC!")
endif()

# for IntelliSense to use
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

# add executables
add_executable(app)
target_compile_features(app PUBLIC cxx_std_20)

add_subdirectory(src)
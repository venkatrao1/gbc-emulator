add_subdirectory(cartridge)

target_sources(
	app
	PRIVATE
	mmu.cpp
	bootrom.cpp
)
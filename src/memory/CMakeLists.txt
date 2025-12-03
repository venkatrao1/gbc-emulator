add_subdirectory(cartridge)

target_sources(
	app
	PRIVATE
	bootrom.cpp
	mmu.cpp
	serial.cpp
)
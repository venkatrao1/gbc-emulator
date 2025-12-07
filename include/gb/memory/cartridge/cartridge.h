#pragma once

#include <gb/memory/memory_map.h>
#include <gb/memory/cartridge/mappers/mbc1.h>
#include <gb/memory/cartridge/mappers/no_mapper.h>

#include <cstdint>
#include <optional>
#include <variant>
#include <vector>
#include <span>
#include <string>


namespace gb::memory {

template<typename T>
concept Mapper = requires(std::span<const uint8_t> rom, std::optional<std::span<const uint8_t>> save_data, T mapper) {
	requires std::constructible_from<T, decltype(rom), decltype(save_data)>;

	{ mapper.read(uint16_t{}) } -> std::same_as<uint8_t>;
	{ mapper.write(uint16_t{}, uint8_t{}) } -> std::same_as<void>;

	// for save RAM
	{ std::as_const(mapper).dump_save_data() } -> std::convertible_to<std::optional<std::vector<uint8_t>>>;
	
	// TODO: for debugging mappers
	// { std::as_const(mapper).dump_state() } -> std::string;
};

class Cartridge {
	std::variant<
		mappers::NoMapper,
		mappers::MBC1
	> mapper_variant;
	static_assert(requires {std::visit([](auto mapper){ static_assert(Mapper<decltype(mapper)>); }, mapper_variant);});

public: 
	// TODO save data.
	Cartridge(std::span<const uint8_t> rom, std::optional<std::span<const uint8_t>> save_data);

	// for GB
	uint8_t read(uint16_t addr) const { return std::visit([addr](auto mapper){return mapper.read(addr); }, mapper_variant); };
	void write(uint16_t addr, uint8_t data) { std::visit([addr, data](auto mapper){mapper.write(addr, data);}, mapper_variant); };

	auto dump_save_data() const { return std::visit([](auto mapper) -> std::optional<std::vector<uint8_t>> { return mapper.dump_save_data(); }, mapper_variant); }

	// for external (not by the emulated CPU) use
	std::string title() const {
		std::string ret;
		for(uint16_t addr = addrs::TITLE_BEGIN; addr < addrs::TITLE_END; ++addr) {
			const char c = read(addr);
			if(c == '\0') break;
			ret.push_back(c);
		}
		return ret;
	}

	using mapper_variant_t = decltype(mapper_variant);
};

}
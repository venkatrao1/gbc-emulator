#pragma once

#include <gb/memory/memory_map.h>
#include <gb/utils/log.h>

#include <array>
#include <cstdint>
#include <span>
#include <sstream>

namespace gb::apu {

// APU
// other emulators' approaches:
// - SameBoy appears to generate samples at output rate using band-limited sound synth
//     - https://github.com/LIJI32/SameBoy/blob/master/Core/apu.c#L9C45-L9C80
// - Gambatte generates samples at 2Mhz, then downsamples?

// Original behavior:
// 1. every TCLK, generates a sample
//   a. each DAC is 0-15; added gives 0-60 for each channel, then scaled by 1-8 by volume to 0-480,
//      then scaled by volume register (x1-8 = 0-3840)
// 2. resample to output freq
// 3. scaled by volume knob (TODO - use emulator setting here)
// 4. high pass filter

// current SW behavior:
// 1. accumulate samples into a single bucket at each tclk, samples come from step 1 above
//   - note, this will mean some samples are smaller, but I think this is fine because harmonics are outside audible range?
// 2. scale above integer by volume knob and high pass filter
// 3. push out sample

constexpr std::array<uint8_t, 4> PULSE_DUTY_CYCLES{
	0b1111'1110, 0b0111'1110, 0b0111'1000, 0b1000'0001
};

namespace addrs {
using memory::addrs::AUDIOS_BEGIN, memory::addrs::AUDIOS_END;
constexpr uint16_t NR10{0xFF10}, NR11{0xFF11}, NR12{0xFF12}, NR13{0xFF13}, NR14{0xFF14};
constexpr uint16_t NR21{0xFF16}, NR22{0xFF17}, NR23{0xFF18}, NR24{0xFF19};
constexpr uint16_t NR30{0xFF1A}, NR31{0xFF1B}, NR32{0xFF1C}, NR33{0xFF1D}, NR34{0xFF1E};
constexpr uint16_t NR41{0xFF20}, NR42{0xFF21}, NR43{0xFF22}, NR44{0xFF23};
constexpr uint16_t NR50{0xFF26}, NR51{0xFF25}, NR52{0xFF26};
constexpr uint16_t WAVETABLE_RAM_BEGIN{0xFF30}, WAVETABLE_RAM_END{0xFF40};
}

// when reading from audio regs, or this with last 8-bit value to simulate write-only bits
constexpr std::array<uint8_t, addrs::WAVETABLE_RAM_BEGIN - addrs::AUDIOS_BEGIN> AUDIO_REG_READBACK_MASKS = []() consteval {
	using namespace addrs;
	std::array<uint8_t, WAVETABLE_RAM_BEGIN - AUDIOS_BEGIN> ret;
	std::fill(ret.begin(), ret.end(), 0xFF);

	const auto ptr = [&ret](uint16_t addr){
		return ret.data() + (addr  - AUDIOS_BEGIN);
	};
	// source: https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware
	*ptr(NR10) = 0x80; *ptr(NR11) = 0x3F; *ptr(NR12) = 0x00; *ptr(NR13) = 0xFF; *ptr(NR14) = 0xBF;
	*ptr(NR21) = 0x3F; *ptr(NR22) = 0x00; *ptr(NR23) = 0xFF; *ptr(NR24) = 0xBF;
	*ptr(NR30) = 0x7F; *ptr(NR31) = 0xFF; *ptr(NR32) = 0x9F; *ptr(NR33) = 0xFF; *ptr(NR34) = 0xBF;
					   *ptr(NR41) = 0xFF; *ptr(NR42) = 0x00; *ptr(NR43) = 0x00; *ptr(NR44) = 0xBF;
	*ptr(NR50) = 0x00; *ptr(NR51) = 0x00; *ptr(NR52) = 0x70; // NR52 lowest bits set by APU

	return ret;
}();

// driven by MMU, as things are triggered by writes
struct APU {
	APU() {
		reset();
	}

	void reset() {
		log_debug("Resetting APU");

		// init memory and regs
		using namespace addrs;
		reg<NR52>() = 0; // turn APU off
		// kind of a hack - write 0 to all regs as proxy for resetting
		for(auto addr = NR10; addr < NR52; ++addr) {
			write(addr, 0);
		}
	}

	void tclk_tick() {
		using namespace addrs;
		if(!apu_enabled()) {
			return;
		}
	}

	uint8_t read(uint16_t addr) const {
		using namespace addrs;
		if(addr < AUDIOS_BEGIN || addr >= AUDIOS_END) throw_exc();
		if(addr >= WAVETABLE_RAM_BEGIN) {
			return wave_table[addr - WAVETABLE_RAM_BEGIN];
		} else {
			auto idx = addr - AUDIOS_BEGIN; 
			return audio_regs[idx] | AUDIO_REG_READBACK_MASKS[idx];
		}
	}

	void write(uint16_t addr, uint8_t data) {
		using namespace addrs;
		if(addr < AUDIOS_BEGIN || addr >= AUDIOS_END) throw_exc();

		if(addr >= WAVETABLE_RAM_BEGIN) {
			wave_table[addr - WAVETABLE_RAM_BEGIN] = data;
			return;
		}

		uint8_t& mem = audio_regs[addr - AUDIOS_BEGIN];

		const bool was_enabled = apu_enabled();
		if(addr == NR52) {
			// always writable, even if not enabled
			mem = data;
			if(apu_enabled() && !was_enabled) log_info("Enabling APU");
			else if(was_enabled && !apu_enabled()) reset();
			return;
		}

		if(!was_enabled) [[unlikely]] {
			log_warn("Ignoring write to {:#06x} while APU off", addr);
			return;
		}

		mem = data;
		// TODO triggering logic here
	}

private:
	// get last written value (no masking)
	template<uint16_t Addr>
	auto& reg() {
		static_assert(Addr >= addrs::AUDIOS_BEGIN && Addr <= addrs::NR52);
		return audio_regs[Addr - addrs::AUDIOS_BEGIN];
	}
	
	std::array<uint8_t, AUDIO_REG_READBACK_MASKS.size()> audio_regs{};
	std::array<uint8_t, addrs::WAVETABLE_RAM_END - addrs::WAVETABLE_RAM_BEGIN> wave_table{}; // TODO: supposedly this should be uninitialized memory

	bool apu_enabled() { return reg<addrs::NR52>() & 0x80; }

	// TODO: cgb audio sampling
};

}
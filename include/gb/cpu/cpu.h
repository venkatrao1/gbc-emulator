#pragma once

#include <gb/memory/mmu.h>
#include <gb/utils/log.h>

#include "regs.h"

#include <array>
#include <format>

namespace gb::cpu {

// the game boy CPU.
struct CPU {
	constexpr CPU(memory::MMU& mmuIn) : mmu{mmuIn} {}

	// @ return number of M-cycles taken by this instruction.
	// TODO: this should probably be some sort of coroutine, whether that's a cpp20 coroutine
	// or by saving sub-instruction state inside here.
	uint64_t fetch_execute() {
		const uint8_t opcode = mmu.read(pc++);
		uint64_t cycles = 1;

		// The gb opcodes are easy to decode as octal.
		// https://gbdev.io/gb-opcodes/optables/octal
		const uint8_t op_low3bits = opcode & 0b111;
		const uint8_t op_upper5bits = opcode >> 3;

		// common helpers for decoding
		const auto BC_DE_HL_SP = std::array{&bc, &de, &hl, &sp}; 
		const auto BC_DE_HL_AF = std::array{&bc, &de, &hl, &af};

		const auto ld_imm16 = [this, &cycles]() -> uint16_t {
			uint16_t ret = mmu.read(pc++);
			cycles += 2;
			return ret | (mmu.read(pc++) << 8);
		};

		// very common decoding pattern - r/w one of 7 regs, or from mem[hl].
		// I'm calling this r8 to match gbdev.io, despite the fact that [hl] is not a register
		const std::array<uint8_t*, 8> BCDEHL_NULLPTR_A{&b(), &c(), &d(), &e(), &h(), &l(), nullptr, &a()};
		const auto read_r8 = [this, &cycles, &BCDEHL_NULLPTR_A](uint8_t b3) -> uint8_t {
			if(b3 > 7) throw GB_exc("");
			if(b3 == 6) {
				cycles++;
				return mmu.read(hl);
			}
			return *(BCDEHL_NULLPTR_A[b3]);
		};
		const auto write_r8 = [this, &cycles, &BCDEHL_NULLPTR_A](uint8_t b3, uint8_t data) -> void {
			if(b3 > 7) throw GB_exc("");
			if(b3 == 6) {
				cycles++;
				mmu.write(hl, data);
				return;
			}
			*(BCDEHL_NULLPTR_A[b3]) = data;
		};

		// used for JR, RET, JP, CALL
		const auto get_flag = [this](uint8_t b2) -> bool {
			switch(b2) {
				case 0: return !flag_z(); // NZ
				case 1: return flag_z(); // Z
				case 2: return !flag_c(); // NC
				case 3: return flag_c(); // C
				default: throw GB_exc("");
			}
		};
		
		// note: normally I would nest things further but this will be pretty deeply indented if I do that.
		// hence things like if(x) switch(y) {}.
		// TODO: see if marking some cases unreachable causes better codegen (once all instructions implemented.)
		if(op_upper5bits < 010) switch(op_low3bits) { // < 0o100
			case 0: {
				if(op_upper5bits < 4) {
					break;
				} else { // JR <flag>, e8
					const bool should_jump = get_flag(op_upper5bits & 3);
					const auto offset = static_cast<int8_t>(mmu.read(pc++));
					cycles++;
					if(should_jump) {
						pc += offset;
						cycles++;
					}
					return cycles;
				}
			}
			case 1:
				if(op_upper5bits & 1) { // ADD HL, r16
					break; // TODO
				} else { // LD r16, n16 
					*(BC_DE_HL_SP[op_upper5bits >> 1]) = ld_imm16();
					return cycles;
				}
			case 2: switch(op_upper5bits) {
				case 04: // LD [HL+], A
					mmu.write(hl++, a());
					cycles++;
					return cycles;
				case 06: // LD [HL-], A
					mmu.write(hl--, a());
					cycles++;
					return cycles;
			}
			case 6: { // LD r8, n8
				const auto n8 = mmu.read(pc++);
				cycles++;
				write_r8(op_upper5bits & 7, n8);
				return cycles;
			}
		} else if (op_upper5bits < 030) { // 0o100 <= op < 0o300
			if(opcode == 0167) {
				throw GB_exc("Halt unimplemented!\n{}", dump_state());
			}
			// all 127 of these opcodes (except halt) load r8.
			const auto r8 = read_r8(op_low3bits);
			switch(op_upper5bits) {
				case 025: // XOR A, q8
					a() ^= r8;
					flag_z(a() == 0), flag_n(0), flag_h(0), flag_c(0);
					return cycles;
			}
		} else switch(op_low3bits) { // op >= 0o300 (note that PREFIX is in here, so this includes the 2-byte bitwise ops)
			case 3: switch(op_upper5bits) {
				case 031: { // PREFIX - bitwise ops!
					const uint8_t bit_op = mmu.read(pc++);
					++cycles;
					const uint8_t bit_op_lower3bits = bit_op & 7;
					const uint8_t bit_op_upper5bits = bit_op >> 3;
					const uint8_t bit_op_b3 = bit_op_upper5bits & 7; 

					if(bit_op_upper5bits < 010) { // op <= 0o100 - these ops modify r8 in place

					} else if (bit_op_upper5bits < 020) { // BIT b3, r8
						const uint8_t r8 = read_r8(bit_op_lower3bits);
						flag_z((r8 & (1 << bit_op_b3)) == 0), flag_n(0), flag_h(1);
						return cycles;
					} else { // RES/SET b3, R8
					}

					throw GB_exc(
						"Unrecognized bitwise opcode: {:#04x} == octal {:#3o}\n"
						"CPU dump:\n"
						"{}",
						bit_op, bit_op, dump_state()
					);
				}
			}
		}
	
		throw GB_exc(
			"Unrecognized opcode: {:#04x} == octal {:#3o}\n"
			"CPU dump:\n"
			"{}",
			opcode, opcode, dump_state()
		);
	}

	std::string dump_state() const {
		return std::format(
			"AF[{:#06x}] BC[{:#06x}] DE[{:#06x}] HL[{:#06x}]\n"
			"SP[{:#06x}] PC[{:#06x}]\n"
			"Z[{:b}] N[{:b}] H[{:b}] C[{:b}]",
			uint16_t{af}, uint16_t{bc}, uint16_t{de}, uint16_t{hl},
			uint16_t{sp}, uint16_t{pc},
			flag_z(), flag_n(), flag_h(), flag_c()
		);
	}

private:	
	// regs - TODO seed if needed.
	Reg16 af{0xCAFE}, bc{0xCAFE}, de{0xCAFE}, hl{0xCAFE};
	Reg16 sp{0xCAFE}, pc{};

	// not a huge fan of the 1 letter identifiers tbh, but they make sense for CPU regs.
	constexpr uint8_t& a() { return af.hi; }
	constexpr uint8_t& f() { return af.lo; }
	constexpr uint8_t& b() { return bc.hi; }
	constexpr uint8_t& c() { return bc.lo; }
	constexpr uint8_t& d() { return de.hi; }
	constexpr uint8_t& e() { return de.lo; }
	constexpr uint8_t& h() { return hl.hi; }
	constexpr uint8_t& l() { return hl.lo; }

	constexpr static auto Z_BIT = 7;
	constexpr static auto N_BIT = 6;
	constexpr static auto H_BIT = 5;
	constexpr static auto C_BIT = 4;

	// flag getters/setters.
	constexpr bool flag_z() const { return af.lo & (1 << Z_BIT); }
	constexpr bool flag_n() const { return af.lo & (1 << N_BIT); }
	constexpr bool flag_h() const { return af.lo & (1 << H_BIT); }
	constexpr bool flag_c() const { return af.lo & (1 << C_BIT); }
	constexpr void flag_z(bool newVal) { af.lo = (af.lo & ~(1 << Z_BIT)) | (newVal << Z_BIT); }
	constexpr void flag_n(bool newVal) { af.lo = (af.lo & ~(1 << N_BIT)) | (newVal << N_BIT); }
	constexpr void flag_h(bool newVal) { af.lo = (af.lo & ~(1 << H_BIT)) | (newVal << H_BIT); }
	constexpr void flag_c(bool newVal) { af.lo = (af.lo & ~(1 << C_BIT)) | (newVal << C_BIT); }

	memory::MMU& mmu; // TODO: consider using CRTP instead
};

}
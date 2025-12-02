#pragma once

#include <gb/memory/mmu.h>
#include <gb/utils/log.h>

#include "regs.h"

#include <array>
#include <bitset>
#include <format>

namespace gb::cpu {

// the game boy CPU.
struct CPU {
	CPU(memory::MMU& mmuIn) : mmu{mmuIn} {}

	// @return number of M-cycles taken by this instruction.
	// TODO: this should probably be some sort of coroutine, whether that's a cpp20 coroutine
	// or by saving sub-instruction state inside here.
	uint64_t fetch_execute() {
		uint64_t cycles = 0;

		// common helpers for decoding
		const auto BC_DE_HL_SP = std::array{&bc, &de, &hl, &sp};
		const auto BC_DE_HL_AF = std::array{&bc, &de, &hl, &af};

		// read/write functions make cycle counting easier and code terser.
		// they are small but used so often the savings is probably worth it.
		const auto read = [this, &cycles](uint16_t addr) -> uint8_t {
			cycles++;
			return mmu.read(addr);
		};

		const auto write = [this, &cycles](uint16_t addr, uint8_t data) -> void {
			cycles++;
			mmu.write(addr, data);
		};

		const auto ld_imm8 = [this, &read]() -> uint8_t { return read(pc++); };

		const auto ld_imm16 = [&ld_imm8]() -> uint16_t {
			uint16_t ret = ld_imm8();
			return ret | (static_cast<uint16_t>(ld_imm8()) << 8);
		};

		// very common decoding pattern - r/w one of 7 regs, or from mem[hl].
		// I'm calling this r8 to match gbdev.io, despite the fact that [hl] is not a register
		const std::array<uint8_t*, 8> BCDEHL_NULLPTR_A{&b(), &c(), &d(), &e(), &h(), &l(), nullptr, &a()};
		const auto read_r8 = [this, &read, &BCDEHL_NULLPTR_A](uint8_t b3) -> uint8_t {
			if(b3 > 7) throw_exc();
			if(b3 == 6) return read(hl);
			else return *(BCDEHL_NULLPTR_A[b3]);
		};
		const auto write_r8 = [this, &write, &BCDEHL_NULLPTR_A](uint8_t b3, uint8_t data) -> void {
			if(b3 > 7) throw_exc();
			if(b3 == 6) write(hl, data);
			else *(BCDEHL_NULLPTR_A[b3]) = data;
		};

		// used for JR, RET, JP, CALL
		const auto get_flag = [this](uint8_t b2) -> bool {
			switch(b2) {
				case 0: return !flag_z(); // NZ
				case 1: return flag_z(); // Z
				case 2: return !flag_c(); // NC
				case 3: return flag_c(); // C
				default: throw_exc();
			}
		};

		const auto push16 = [this, &write, &cycles](const Reg16& reg) -> void {
			cycles++;
			write(--sp, reg.hi);
			write(--sp, reg.lo);
		};
		
		const auto requested_interrupt = get_interrupt(); 
		if(halted && !requested_interrupt.has_value()) {
			return 1; // sleeping
		}

		// halt increments PC when waking up (if it exits immediately, halt bug occurs.)
		const uint8_t opcode = ld_imm8();
		if(halted) {
			pc--; // PC points at byte after halt AND so does opcode! (halt bug if !IME)
			halted = false;
			if(!IME) [[unlikely]] {
				log_info("Halt bug triggered! {}", dump_state());
			}
		}

		if(IME) {
			if(requested_interrupt.has_value()) {
				IME = false;
				IME_enable_pending = false;
				const uint16_t next_addr = 0x40 + (8 * static_cast<uint8_t>(*requested_interrupt));
				--pc; ++cycles; // interrupt servicing happens after fetch opcode, backtrack one instruction
				log_debug("Servicing interrupt {}, PC={:#06x}, jumping to {:#06x}", *requested_interrupt, pc, next_addr);
				push16(pc);
				// TODO: a second, higher prio interrupt can handle between the start of this routine and here, and could override this one.
				pc = next_addr;
				return cycles;
			}
		}
		if(IME_enable_pending) {
			IME = true;
			IME_enable_pending = false;
		}

		if(!visited[pc]) {
			log_debug("New PC: {:#06x}", pc);
			visited[pc] = true;
		}

		// The gb opcodes are easy to decode as octal.
		// https://gbdev.io/gb-opcodes/optables/octal
		const uint8_t op_low3bits = opcode & 0b111;
		const uint8_t op_upper5bits = opcode >> 3;

		// note: normally I would nest things further but this will be pretty deeply indented if I do that.
		// hence things like if(x) switch(y) {}.
		// TODO: see if marking some cases unreachable causes better codegen (once all instructions implemented.)
		if(op_upper5bits < 010) switch(op_low3bits) { // < 0o100
			case 0:
				if(op_upper5bits < 3) switch(op_upper5bits) {
					case 0: // NOP
						return cycles;
				} else { // JR <flag>, e8 // JR e8
					const bool should_jump = op_upper5bits == 3 ? true : get_flag(op_upper5bits & 3);
					const auto offset = static_cast<int8_t>(ld_imm8());
					if(should_jump) {
						pc += offset;
						cycles++;
					}
					return cycles;
				}
				break;
			case 1:
				if(op_upper5bits & 1) { // ADD HL, r16
					const auto& r16 = *(BC_DE_HL_SP[op_upper5bits >> 1]);
					flag_n(0), flag_h(((hl & 0xFFF) + (r16 & 0xFFF)) > 0xFFF), flag_c((hl + r16) > 0xFFFF);
					++cycles;
					hl += r16;
					return cycles;
				} else { // LD r16, n16
					*(BC_DE_HL_SP[op_upper5bits >> 1]) = ld_imm16();
					return cycles;
				}
			case 2: {
				const uint16_t addr = [this, op_upper5bits]{ switch(op_upper5bits >> 1) {
					default: // unused
					case 0: return bc;
					case 1: return de;
					case 2: return hl++;
					case 3: return hl--;
				}}();
				if(op_upper5bits & 1) a() = read(addr); // LD A, [r16]
				else write(addr, a()); // LD [r16], A
				return cycles;
			}
			case 3: { // INC r16 / DEC r16
				Reg16& reg = *(BC_DE_HL_SP[op_upper5bits >> 1]);
				if(op_upper5bits & 1) --reg; // DEC
				else ++reg; // INC
				cycles++;
				return cycles;
			}
			case 4: { // INC r8
				uint8_t val = read_r8(op_upper5bits);
				++val;
				write_r8(op_upper5bits, val);
				flag_z(val == 0), flag_n(0), flag_h((val & 0xF) == 0x0);
				return cycles;
			}
			case 5: { // DEC R8
				uint8_t val = read_r8(op_upper5bits);
				--val;
				write_r8(op_upper5bits, val);
				flag_z(val == 0), flag_n(1), flag_h((val & 0xF) == 0xF);
				return cycles;
			}
			case 6: { // LD r8, n8
				write_r8(op_upper5bits & 7, ld_imm8());
				return cycles;
			}
			case 7: if(op_upper5bits < 4) {
				flag_z(0), flag_n(0), flag_h(0);
				switch(op_upper5bits) {
					case 0: // RLCA
						a() = std::rotl(a(), 1);
						flag_c(a() & 1);
						return cycles;
					case 1: // RRCA
						flag_c(a() & 1);
						a() = std::rotr(a(), 1);
						return cycles;
					case 2: { // RLA
						const bool flagc_next = a() & 0x80;
						a() = (a() << 1) | static_cast<uint8_t>(flag_c());
						flag_c(flagc_next);
						return cycles;
					}
					case 3: { // RRA
						const bool flagc_next = a() & 1;
						a() = (a() >> 1) | (flag_c() << 7);
						flag_c(flagc_next);
						return cycles;
					}
				}
			} else switch(op_upper5bits) {
				case 4: // DAA
					break;
				case 5: // CPL
					flag_n(1), flag_h(1);
					a() = ~a();
					return cycles;
				case 6: // SCF
					break;
				case 7: // CCF
					break;
			}
		} else if (op_upper5bits < 020) { // 0o100 <= op < 0o200 - LD r8, r8
			if(constexpr static uint8_t OPCODE_HALT{0166}; opcode == OPCODE_HALT) {
				log_debug("Halting, IE = {:08b}, IF = {:08b}", mmu.get<memory::addrs::INTERRUPT_ENABLE>(), mmu.get<memory::addrs::INTERRUPT_FLAG>());
				halted = true;
				return cycles;
			}
			write_r8(op_upper5bits & 7, read_r8(op_low3bits));
			return cycles;
		} else if (op_upper5bits < 030 || op_low3bits == 6) { // 0o200 <= op < 0o300 (also 0o3X6) - bitwise ops and compare
			const auto r8 = op_upper5bits < 030 ? read_r8(op_low3bits) : ld_imm8();
			switch(op_upper5bits & 7) {
				case 0: // ADD A, r8 // ADD A, n8
					flag_h(((a() & 0xF) + (r8 & 0xF) > 0xF)), flag_c((a() + r8) > 0xFF);
					a() += r8;
					flag_z(a() == 0), flag_n(0);
					return cycles;
				case 1: { // ADC A, r8 // ADC A, n8
					const bool c_old = flag_c();
					flag_h(((a() & 0xF) + (r8 & 0xF) + c_old) > 0xF), flag_c((a() + r8 + c_old) > 0xFF);
					a() += r8 + c_old;
					flag_z(a() == 0), flag_n(0);
					return cycles;
				}
				case 2: // SUB A, r8 // SUB A, n8
					flag_z(a() == r8), flag_n(1), flag_h((a() & 0xF) < (r8 & 0xF)), flag_c(r8 > a());
					a() -= r8;
					return cycles;
				case 4: // AND A, r8 // AND A, n8
					a() &= r8;
					flag_z(a() == 0), flag_n(0), flag_h(1), flag_c(0);
					return cycles;
				case 5: // XOR A, r8 // XOR A, n8
					a() ^= r8;
					flag_z(a() == 0), flag_n(0), flag_h(0), flag_c(0);
					return cycles;
				case 6: // OR A, r8 // OR A, n8
					a() |= r8;
					flag_z(a() == 0), flag_n(0), flag_h(0), flag_c(0);
					return cycles;
				case 7: // CP A, r8 // CP A, n8
					flag_z(a() == r8), flag_n(1), flag_h((a() & 0xF) < (r8 & 0xF)), flag_c(r8 > a()); // same flags as sub
					return cycles;
			}
		} else { // op >= 0o300, (op & 7) != 6 (note that PREFIX is in here, so this includes the 2-byte bitwise ops)
			const auto pop16 = [this, &read]() -> uint16_t {
				const auto lo = read(sp++);
				return lo | (static_cast<uint16_t>(read(sp++)) << 8);
			};
			switch(op_low3bits) {
				case 0: if(op_upper5bits < 034) { // RET <flag>
					cycles++;
					if(get_flag(op_upper5bits & 3)) pc = pop16();
					return cycles;
				} else switch(op_upper5bits) {
					case 034: // LD [0xFF00+imm8], A
						write(0xFF00 + ld_imm8(), a());
						return cycles;
					case 035: throw_exc();
					case 036: // LD A, [0xFF00+imm8]
						a() = read(0xFF00 + ld_imm8());
						return cycles;
					case 037: throw_exc();
				};
				case 1: if(op_upper5bits & 1) switch(op_upper5bits) {
					case 033: // RETI
						IME = true;
						[[fallthrough]];
					case 031: // RET
						pc = pop16();
						cycles++;
						return cycles;
					case 035: // JP HL
						pc = hl;
						return cycles;
					default: throw_exc();
				} else { // POP r16
					*(BC_DE_HL_AF[(op_upper5bits >> 1) & 3]) = pop16();
					return cycles;
				}
				case 2: if(op_upper5bits < 034) { // JP <flag>, a16
					const auto next_addr = ld_imm16();
					if(get_flag(op_upper5bits & 3)) {
						cycles++;
						pc = next_addr;
					}
					return cycles;
				} else switch(op_upper5bits) {
					case 034: // LD [0xFF00+C], A
						write(0xFF00 + c(), a());
						return cycles;
					case 035: // LD [a16], A
						write(ld_imm16(), a());
						return cycles;
					case 036: // LD A, [0xFF00+C]
						a() = read(0xFF00 + c());
						return cycles;
					case 037: // LD A, [a16]
						a() = read(ld_imm16());
						return cycles;
				}
				case 3: switch(op_upper5bits) {
					case 030: // JP a16
						pc = ld_imm16();
						cycles++;
						return cycles;
					case 031: { // PREFIX - bitwise ops!
						const uint8_t bit_op = ld_imm8();
						const uint8_t bit_op_lower3bits = bit_op & 7;
						const uint8_t bit_op_upper5bits = bit_op >> 3;
						const uint8_t bit_op_b3 = bit_op_upper5bits & 7;

						if(bit_op_upper5bits < 010) { // op <= 0o100 - these ops modify r8 in place
							uint8_t data = read_r8(bit_op_lower3bits);
							switch(bit_op_b3) {
								case 0: // RLC r8
									data = std::rotl(data, 1);
									flag_c(data & 1);
									break;
								case 1: // RRC r8
									flag_c(data & 1);
									data = std::rotr(data, 1);
									break;
								case 2: { // RL r8
									const bool flagc_next = data & 0x80;
									data = (data << 1) | static_cast<uint8_t>(flag_c());
									flag_c(flagc_next);
									break;
								}
								case 3: { // RR r8
									const bool flagc_next = data & 1;
									data = (data >> 1) | (flag_c() << 7);
									flag_c(flagc_next);
									break;
								}
								case 4: // SLA r8
									flag_c(data & 0x80);
									data <<= 1;
									break;
								case 5: // SRA r8
									flag_c(data & 1);
									data = (std::bit_cast<int8_t>(data) >> 1);
									break;
								case 6: // SWAP r8
									flag_c(0);
									data = (data << 4) | (data >> 4); // swap nybbles
									break;
								case 7: // SRL r8
									flag_c(data & 1);
									data >>= 1;
									break;
							}
							write_r8(bit_op_lower3bits, data);
							flag_z(data == 0), flag_n(0), flag_h(0);
							return cycles;
						} else if (bit_op_upper5bits < 020) { // BIT b3, r8
							const uint8_t r8 = read_r8(bit_op_lower3bits);
							flag_z((r8 & (1 << bit_op_b3)) == 0), flag_n(0), flag_h(1);
							return cycles;
						} else { // RES/SET b3, R8
							const uint8_t new_bitvalue = -((bit_op_upper5bits >> 3) & 1); // 255 if set, 0 if rst
							write_r8(bit_op_lower3bits, mask_combine(1 << bit_op_b3, read_r8(bit_op_lower3bits), new_bitvalue));
							return cycles;
						}

						throw_exc(
							"Unrecognized bitwise opcode: {:#04x} == octal {:#03o}\n"
							"CPU dump:\n"
							"{}",
							bit_op, bit_op, dump_state()
						);
					}
					case 036: // DI
						IME_enable_pending = false;
						IME = false;
						return cycles;
					case 037: // EI
						IME_enable_pending = true;
						return cycles;
				}
					break;
				case 5: if(op_upper5bits == 031) { // CALL a16
					const auto addr = ld_imm16();
					push16(pc);
					pc = addr;
					return cycles;
				} else if((op_upper5bits & 1) == 0) { // PUSH r16
					push16(*(BC_DE_HL_AF[(op_upper5bits >> 1) & 3]));
					return cycles;
				} else break; // illegal instruction
				case 7: // RST
					push16(pc);
					pc = opcode & 0x38;
					return cycles;
			}
		}

		throw_exc(
			"Unrecognized opcode: {:#04x} == octal {:#03o}\n",
			opcode, opcode
		);
	}

	std::string dump_state() const {
		return std::format(
			"AF[{:#06x}] BC[{:#06x}] DE[{:#06x}] HL[{:#06x}]\n"
			"SP[{:#06x}] PC[{:#06x}]\n"
			"Z[{:b}] N[{:b}] H[{:b}] C[{:b}]",
			af, bc, de, hl,
			sp, pc,
			flag_z(), flag_n(), flag_h(), flag_c()
		);
	}

private:
	// regs - TODO seed if needed.
	Reg16 af{0xCAFE}, bc{0xCAFE}, de{0xCAFE}, hl{0xCAFE};
	Reg16 sp{0xCAFE}, pc{};

	std::bitset<0x10000> visited{};

	bool IME{false}; // interrupt master enable
	bool IME_enable_pending{false};
	bool halted{false};

	// not a huge fan of the 1 letter identifiers tbh, but they make sense for CPU regs.
	[[nodiscard]] constexpr uint8_t& a() { return af.hi; }
	[[nodiscard]] constexpr uint8_t& f() { return af.lo; }
	[[nodiscard]] constexpr uint8_t& b() { return bc.hi; }
	[[nodiscard]] constexpr uint8_t& c() { return bc.lo; }
	[[nodiscard]] constexpr uint8_t& d() { return de.hi; }
	[[nodiscard]] constexpr uint8_t& e() { return de.lo; }
	[[nodiscard]] constexpr uint8_t& h() { return hl.hi; }
	[[nodiscard]] constexpr uint8_t& l() { return hl.lo; }

	constexpr static auto Z_BIT = 7;
	constexpr static auto N_BIT = 6;
	constexpr static auto H_BIT = 5;
	constexpr static auto C_BIT = 4;

	// flag getters/setters.
	[[nodiscard]] constexpr bool flag_z() const { return af.lo & (1 << Z_BIT); }
	[[nodiscard]] constexpr bool flag_n() const { return af.lo & (1 << N_BIT); }
	[[nodiscard]] constexpr bool flag_h() const { return af.lo & (1 << H_BIT); }
	[[nodiscard]] constexpr bool flag_c() const { return af.lo & (1 << C_BIT); }
	constexpr void flag_z(bool newVal) { af.lo = (af.lo & ~(1 << Z_BIT)) | (newVal << Z_BIT); }
	constexpr void flag_n(bool newVal) { af.lo = (af.lo & ~(1 << N_BIT)) | (newVal << N_BIT); }
	constexpr void flag_h(bool newVal) { af.lo = (af.lo & ~(1 << H_BIT)) | (newVal << H_BIT); }
	constexpr void flag_c(bool newVal) { af.lo = (af.lo & ~(1 << C_BIT)) | (newVal << C_BIT); }

	[[nodiscard]] std::optional<memory::interrupt_bits> get_interrupt() {
		const uint8_t requested = mmu.get<memory::addrs::INTERRUPT_ENABLE>() & mmu.get<memory::addrs::INTERRUPT_FLAG>();
		if(const auto lowest_set_bit = std::countr_zero(requested); lowest_set_bit < 5) {
			mmu.get<memory::addrs::INTERRUPT_FLAG>() ^= (1 << lowest_set_bit);
			return static_cast<memory::interrupt_bits>(lowest_set_bit);
		}
		else return std::nullopt;
	}

	memory::MMU& mmu; // TODO: consider using CRTP instead
};

}
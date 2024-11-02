#pragma once

#include <concepts>
#include <cstdint>

namespace gb {

template<std::unsigned_integral T>
[[nodiscard]] constexpr T mask_combine(const uint64_t mask, const T in0, const T in1) {
	return (mask & in1) | (~mask & in0);
}

}
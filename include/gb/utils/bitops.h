#pragma once

#include <concepts>
#include <cstdint>
#include <type_traits>

namespace gb {

template<std::unsigned_integral T>
[[nodiscard]] constexpr T mask_combine(const std::type_identity_t<T> mask, const T in0, const T in1) {
	return (mask & in1) | (~mask & in0);
}

}
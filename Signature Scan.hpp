#pragma once

#include <cstdint>
#include <cstddef>

namespace Scanner {
	uintptr_t scanPattern(std::uint8_t *base, const std::size_t size, char *pattern, char *mask);
}
#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace PatternScan
{
	std::optional<std::uintptr_t> FindInMainModule(std::string_view pattern);
	std::uintptr_t ResolveRip(std::uintptr_t instruction, std::ptrdiff_t operandOffset);
}

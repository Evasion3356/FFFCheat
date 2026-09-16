#pragma once

#include "..\external\RDR-Classes\script\scrProgram.hpp"

#include <cstdint>

namespace GamePointers
{
	rage::scrProgram** GetScriptPrograms();
	rage::scrProgram* FindScriptProgram(std::uint32_t scriptHash);
}

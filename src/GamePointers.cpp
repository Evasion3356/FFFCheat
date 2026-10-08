#include "GamePointers.h"

#include "FFFCheatLog.h"
#include "PatternScan.h"

namespace
{
	constexpr const char* kScriptProgramsPattern = "C1 EF 0E 85 FF 74 21";
	constexpr std::ptrdiff_t kScriptProgramsOperandOffset = -0x13;
	constexpr std::size_t kScriptProgramCount = 160;
}

namespace FFFCheat::GamePointers
{
	rage::scrProgram** GetScriptPrograms()
	{
		static rage::scrProgram** cached = []() -> rage::scrProgram**
		{
			const auto match = FFFCheat::PatternScan::FindInMainModule(kScriptProgramsPattern);
			if (!match)
			{
				Log::Write("script-program table signature was not found");
				return nullptr;
			}

			const auto resolved = FFFCheat::PatternScan::ResolveRip(*match, kScriptProgramsOperandOffset) + 0xC8;
			Log::Write("script-program table resolved");
			return reinterpret_cast<rage::scrProgram**>(resolved);
		}();

		return cached;
	}

	rage::scrProgram* FindScriptProgram(std::uint32_t scriptHash)
	{
		auto programs = GetScriptPrograms();
		if (!programs)
			return nullptr;

		for (std::size_t i = 0; i < kScriptProgramCount; ++i)
		{
			if (programs[i] && programs[i]->m_NameHash == scriptHash)
				return programs[i];
		}

		return nullptr;
	}
}

#include "FFFCheat.h"

#include "GamePointers.h"
#include "Log.h"

#include "..\external\RDR-Classes\rage\joaat.hpp"

#include <windows.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>

namespace
{
	constexpr std::uint32_t kFilletScriptHash = rage::Joaat("fillet_sp");
	// func_657 begins at 0x1921A. Its PUSH_CONST_0 follows the five-byte
	// ENTER prologue at 0x1921F.
	constexpr std::uint32_t kReturnFalseOpcodeOffset = 0x1921F;
	constexpr std::uint8_t kPushConstZero = 47;
	constexpr std::uint8_t kPushConstOne = 9;
	constexpr std::uint8_t kLeave = 80;
	constexpr std::array<std::uint8_t, 9> kExpectedReturnFalseFunction{
		34, 1, 3, 0, 0, kPushConstZero, kLeave, 1, 1
	};

	struct PatchedProgram
	{
		rage::scrProgram* program = nullptr;
		std::uint8_t* page = nullptr;
		std::uint8_t* target = nullptr;
	};

	std::unique_ptr<PatchedProgram> g_patch;

	bool WriteByte(std::uint8_t* address, std::uint8_t value)
	{
		DWORD oldProtect = 0;
		if (!VirtualProtect(address, 1, PAGE_EXECUTE_READWRITE, &oldProtect))
			return false;

		*address = value;
		VirtualProtect(address, 1, oldProtect, &oldProtect);
		return true;
	}

	// The game owns the code pages and frees them with the program when the
	// script unloads, so the patch is made in place. Swapping in a cloned
	// page table makes the game tear down memory its allocator never made.
	bool Apply(rage::scrProgram* program)
	{
		if (!program || !program->IsValid())
			return false;

		const std::uint32_t pageCount = program->GetNumCodePages();
		if (!program->m_CodeBlocks || pageCount == 0 ||
			kReturnFalseOpcodeOffset >= program->m_CodeSize)
			return false;

		const std::uint32_t functionPage = (kReturnFalseOpcodeOffset - 5) >> 14;
		const std::uint32_t functionOffset = (kReturnFalseOpcodeOffset - 5) & 0x3FFF;
		const std::uint32_t page = kReturnFalseOpcodeOffset >> 14;
		const std::uint32_t offset = kReturnFalseOpcodeOffset & 0x3FFF;
		if (functionPage != page || page >= pageCount || !program->m_CodeBlocks[page] ||
			functionOffset + kExpectedReturnFalseFunction.size() > 0x4000)
			return false;

		std::uint8_t* pageBase = program->m_CodeBlocks[page];
		const auto* function = &pageBase[functionOffset];
		if (!std::equal(kExpectedReturnFalseFunction.begin(), kExpectedReturnFalseFunction.end(), function))
		{
			Log::Write("fillet_sp bytecode guard failed; patch not applied");
			return false;
		}

		auto* target = &pageBase[offset];
		if (!WriteByte(target, kPushConstOne))
		{
			Log::Write("fillet_sp patch failed: VirtualProtect");
			return false;
		}

		g_patch = std::make_unique<PatchedProgram>();
		g_patch->program = program;
		g_patch->page = pageBase;
		g_patch->target = target;
		Log::Write("fillet_sp patched: any valid button now counts as correct");
		return true;
	}

	void Restore(rage::scrProgram* liveProgram)
	{
		if (!g_patch)
			return;

		// Only touch the byte if the same program and page are still live;
		// otherwise the game has already freed them.
		if (liveProgram == g_patch->program && liveProgram->m_CodeBlocks &&
			liveProgram->m_CodeBlocks[kReturnFalseOpcodeOffset >> 14] == g_patch->page &&
			*g_patch->target == kPushConstOne)
			WriteByte(g_patch->target, kPushConstZero);

		g_patch.reset();
		Log::Write("fillet_sp patch restored");
	}
}

namespace FFFCheat
{
	void OnTick()
	{
		auto liveProgram = GamePointers::FindScriptProgram(kFilletScriptHash);
		if (g_patch)
		{
			// Compare pointers only; the old program may already be freed.
			if (liveProgram == g_patch->program && liveProgram->m_CodeBlocks &&
				liveProgram->m_CodeBlocks[kReturnFalseOpcodeOffset >> 14] == g_patch->page)
				return;

			g_patch.reset();
			Log::Write("fillet_sp patch discarded after script unload");
		}

		Apply(liveProgram);
	}

	void Shutdown()
	{
		Restore(GamePointers::FindScriptProgram(kFilletScriptHash));
	}
}

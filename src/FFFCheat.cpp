#include "FFFCheat.h"

#include "GamePointers.h"
#include "Log.h"

#include "..\external\RDR-Classes\rage\joaat.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

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
		std::uint8_t** originalPages = nullptr;
		std::unique_ptr<std::uint8_t*[]> patchedPages;
		std::uint32_t pageCount = 0;

		~PatchedProgram()
		{
			for (std::uint32_t page = 0; page < pageCount; ++page)
				delete[] patchedPages[page];
		}
	};

	std::unique_ptr<PatchedProgram> g_patch;

	void Restore(rage::scrProgram* liveProgram = nullptr)
	{
		if (!g_patch)
			return;

		if (liveProgram == g_patch->program && liveProgram->m_CodeBlocks == g_patch->patchedPages.get())
			liveProgram->m_CodeBlocks = g_patch->originalPages;

		g_patch.reset();
		Log::Write("fillet_sp patch restored");
	}

	bool Apply(rage::scrProgram* program)
	{
		if (!program || !program->IsValid())
			return false;

		const std::uint32_t pageCount = program->GetNumCodePages();
		if (!program->m_CodeBlocks || pageCount == 0 ||
			kReturnFalseOpcodeOffset >= program->m_CodeSize)
			return false;

		auto patch = std::make_unique<PatchedProgram>();
		patch->program = program;
		patch->originalPages = program->m_CodeBlocks;
		patch->pageCount = pageCount;
		patch->patchedPages = std::make_unique<std::uint8_t*[]>(pageCount);

		for (std::uint32_t page = 0; page < pageCount; ++page)
		{
			const std::uint32_t pageSize = program->GetCodePageSize(page);
			patch->patchedPages[page] = new std::uint8_t[pageSize];
			std::memcpy(patch->patchedPages[page], program->GetCodePage(page), pageSize);
		}

		const std::uint32_t functionPage = (kReturnFalseOpcodeOffset - 5) >> 14;
		const std::uint32_t functionOffset = (kReturnFalseOpcodeOffset - 5) & 0x3FFF;
		const std::uint32_t page = kReturnFalseOpcodeOffset >> 14;
		const std::uint32_t offset = kReturnFalseOpcodeOffset & 0x3FFF;
		const std::uint32_t pageSize = program->GetCodePageSize(page);
		if (functionPage != page || functionOffset + kExpectedReturnFalseFunction.size() > pageSize)
			return false;

		auto* target = &patch->patchedPages[page][offset];
		const auto* function = &patch->patchedPages[functionPage][functionOffset];
		if (!std::equal(kExpectedReturnFalseFunction.begin(), kExpectedReturnFalseFunction.end(), function))
		{
			Log::Write("fillet_sp bytecode guard failed; patch not applied");
			return false;
		}

		*target = kPushConstOne;
		program->m_CodeBlocks = patch->patchedPages.get();
		g_patch = std::move(patch);
		Log::Write("fillet_sp patched: any valid button now counts as correct");
		return true;
	}
}

namespace FFFCheat
{
	void OnTick()
	{
		if (g_patch)
		{
			auto liveProgram = GamePointers::FindScriptProgram(kFilletScriptHash);
			if (liveProgram == g_patch->program)
			{
				if (liveProgram->m_CodeBlocks == g_patch->patchedPages.get())
					return;

				Restore(liveProgram);
			}
			else
			{
				// The script program can be destroyed when fillet_sp unloads;
				// do not dereference the stale pointer in that case.
				g_patch.reset();
				Log::Write("fillet_sp patch discarded after script unload");
				return;
			}
		}

		Apply(GamePointers::FindScriptProgram(kFilletScriptHash));
	}

	void Shutdown()
	{
		Restore(GamePointers::FindScriptProgram(kFilletScriptHash));
	}
}

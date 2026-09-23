#include "FFFCheat.h"

#include "Config.h"
#include "GamePointers.h"
#include "Log.h"

#include "..\external\RDR-Classes\rage\joaat.hpp"

#include <windows.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace
{
	constexpr std::uint32_t kFilletScriptHash = rage::Joaat("fillet_sp");

	// Opcodes (RDR build 1491.50 numbering).
	constexpr std::uint8_t kEnter = 34;
	constexpr std::uint8_t kLeave = 80;
	constexpr std::uint8_t kJump = 104;
	constexpr std::uint8_t kPushConstZero = 47;
	constexpr std::uint8_t kPushConstOne = 9;

	using Bytes = std::vector<std::uint8_t>;

	struct Guard
	{
		std::uint32_t offset;
		Bytes expected;
	};

	// Every guard must match byte for byte before any patch is written; a
	// mismatch means the installed script differs from the traced build.
	struct PatchDef
	{
		const char* name;
		bool Config::Values::* enabledBy;
		std::vector<Guard> guards;
		std::uint32_t offset;
		Bytes replacement;
	};

	const std::vector<PatchDef>& PatchDefs()
	{
		static const std::vector<PatchDef> defs{
			// func_657 begins at 0x1921A; PUSH_CONST_0 follows the five-byte
			// ENTER prologue at 0x1921F. Return true instead of false so any
			// valid button counts as the expected one.
			{
				"any-button",
				&Config::Values::AnyButtonCounts,
				{ { 0x1921A, { kEnter, 1, 3, 0, 0, kPushConstZero, kLeave, 1, 1 } } },
				0x1921F,
				{ kPushConstOne }
			},
			// func_479 rejects a press as a miss when it comes before the prompt
			// (f_86 > f_87) or before the "input window" anim event fires, which
			// includes the moment right after a flourish. At 0xFE02 the miss block
			// begins (PUSH_CONST_1, LOCAL_U8_LOAD 1); replace those three bytes
			// with a jump to the function's LEAVE at 0xFEBA so the press is
			// ignored. Jump operand = 0xFEBA - 0xFE05 = 0xB5.
			{
				"ignore-early-press",
				&Config::Values::IgnoreEarlyPress,
				{
					// f_86 > f_87 || !HAS_ANIM_EVENT_FIRED(...), through the
					// start of the miss block.
					{ 0xFDD8, { 39, 86, 102, 1, 102, 0, 24, 80, 2, 99, 202, 39, 87, 32, 106, 5, 139, 20, 0,
						102, 1, 102, 0, 24, 80, 2, 23, 202, 55, 87, 222, 30, 224, 3, 9, 0, 99, 5, 48,
						139, 47, 0, 9, 102, 1, 102, 0, 24, 80, 2, 99, 202, 108 } },
					// End of the miss block: J to LEAVE (0xFE31 + 0x89 = 0xFEBA).
					{ 0xFE2E, { kJump, 137, 0 } },
					{ 0xFEBA, { kLeave, 2, 0 } }
				},
				0xFE02,
				{ kJump, 0xB5, 0 }
			}
		};
		return defs;
	}

	struct AppliedPatch
	{
		const char* name = "";
		std::uint32_t page = 0;
		std::uint8_t* pageBase = nullptr;
		std::uint8_t* target = nullptr;
		Bytes original;
		Bytes replacement;
	};

	struct PatchedProgram
	{
		rage::scrProgram* program = nullptr;
		std::vector<AppliedPatch> patches;
	};

	std::unique_ptr<PatchedProgram> g_patch;

	// The live program Apply() last refused (guard mismatch, bad layout).
	// Compared by pointer only, like StillLive(), so a refused program isn't
	// re-checked -- and the INI re-read, and the log re-written -- every
	// frame for as long as the player sits at the table.
	rage::scrProgram* g_rejectedProgram = nullptr;
	std::uint8_t* g_rejectedFirstPage = nullptr;

	bool WriteBytes(std::uint8_t* address, const Bytes& bytes)
	{
		DWORD oldProtect = 0;
		if (!VirtualProtect(address, bytes.size(), PAGE_EXECUTE_READWRITE, &oldProtect))
			return false;

		std::copy(bytes.begin(), bytes.end(), address);
		VirtualProtect(address, bytes.size(), oldProtect, &oldProtect);
		return true;
	}

	// Resolves a code offset to a pointer, requiring the whole range to sit
	// inside one code page.
	std::uint8_t* Resolve(rage::scrProgram* program, std::uint32_t offset, std::size_t length,
		std::uint32_t& page)
	{
		page = offset >> 14;
		const std::uint32_t offsetInPage = offset & 0x3FFF;
		if (offset >= program->m_CodeSize || page >= program->GetNumCodePages() ||
			!program->m_CodeBlocks[page] || offsetInPage + length > 0x4000 ||
			offset + length > program->m_CodeSize)
			return nullptr;

		return &program->m_CodeBlocks[page][offsetInPage];
	}

	void Rollback(std::vector<AppliedPatch>& patches)
	{
		for (auto& p : patches)
			WriteBytes(p.target, p.original);
		patches.clear();
	}

	// The game owns the code pages and frees them with the program when the
	// script unloads, so the patches are made in place. Swapping in a cloned
	// page table makes the game tear down memory its allocator never made.
	bool Apply(rage::scrProgram* program)
	{
		if (!program || !program->IsValid() || !program->m_CodeBlocks ||
			program->GetNumCodePages() == 0)
			return false;

		const Config::Values config = Config::Load();

		// Validate every enabled patch's guards before writing anything.
		for (const auto& def : PatchDefs())
		{
			if (!(config.*def.enabledBy))
				continue;

			for (const auto& guard : def.guards)
			{
				std::uint32_t page = 0;
				const auto* bytes = Resolve(program, guard.offset, guard.expected.size(), page);
				if (!bytes || !std::equal(guard.expected.begin(), guard.expected.end(), bytes))
				{
					Log::Write(std::string("fillet_sp bytecode guard failed (") + def.name +
						"); patches not applied");
					return false;
				}
			}
		}

		auto applied = std::make_unique<PatchedProgram>();
		applied->program = program;
		for (const auto& def : PatchDefs())
		{
			if (!(config.*def.enabledBy))
				continue;

			AppliedPatch patch;
			patch.name = def.name;
			patch.target = Resolve(program, def.offset, def.replacement.size(), patch.page);
			if (!patch.target)
			{
				Rollback(applied->patches);
				return false;
			}

			patch.pageBase = program->m_CodeBlocks[patch.page];
			patch.original.assign(patch.target, patch.target + def.replacement.size());
			patch.replacement = def.replacement;
			if (!WriteBytes(patch.target, patch.replacement))
			{
				Log::Write(std::string("fillet_sp patch failed: VirtualProtect (") + def.name + ")");
				Rollback(applied->patches);
				return false;
			}

			applied->patches.push_back(std::move(patch));
		}

		// Kept even when every patch is disabled so OnTick does not re-read the
		// INI every frame; the config is re-read on the next script load.
		g_patch = std::move(applied);
		for (const auto& p : g_patch->patches)
			Log::Write(std::string("fillet_sp patched: ") + p.name);
		return true;
	}

	// True while the program and every patched page are still the live ones
	// and still hold our bytes. Pointers are compared first: the old program
	// may already be freed. Only once they match -- i.e. the memory is the
	// live program's own -- are the bytes read, which catches a reload that
	// the allocator happened to place at the same addresses.
	bool StillLive(rage::scrProgram* liveProgram)
	{
		if (!g_patch || liveProgram != g_patch->program || !liveProgram->m_CodeBlocks)
			return false;

		for (const auto& p : g_patch->patches)
			if (liveProgram->m_CodeBlocks[p.page] != p.pageBase)
				return false;

		for (const auto& p : g_patch->patches)
			if (!std::equal(p.replacement.begin(), p.replacement.end(), p.target))
				return false;

		return true;
	}

	void Restore(rage::scrProgram* liveProgram)
	{
		if (!g_patch)
			return;

		// Only touch the bytes if the same program and pages are still live;
		// otherwise the game has already freed them.
		if (StillLive(liveProgram))
		{
			for (const auto& p : g_patch->patches)
				if (std::equal(p.replacement.begin(), p.replacement.end(), p.target))
					WriteBytes(p.target, p.original);
		}

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
			if (StillLive(liveProgram))
				return;

			g_patch.reset();
			Log::Write("fillet_sp patch discarded after script unload");
		}

		if (!liveProgram)
		{
			g_rejectedProgram = nullptr;
			g_rejectedFirstPage = nullptr;
			return;
		}

		std::uint8_t* firstPage = liveProgram->m_CodeBlocks ? liveProgram->m_CodeBlocks[0] : nullptr;
		if (liveProgram == g_rejectedProgram && firstPage == g_rejectedFirstPage)
			return;

		if (Apply(liveProgram))
		{
			g_rejectedProgram = nullptr;
			g_rejectedFirstPage = nullptr;
		}
		else if (liveProgram->IsValid())
		{
			// Not yet valid means still loading -- retry next frame. A valid
			// program that failed is final until the script reloads.
			g_rejectedProgram = liveProgram;
			g_rejectedFirstPage = firstPage;
		}
	}

	void Shutdown()
	{
		Restore(GamePointers::FindScriptProgram(kFilletScriptHash));
	}
}

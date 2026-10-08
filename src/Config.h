#pragma once

// INI-backed feature toggles (FFFCheat.ini, next to FFFCheat.asi, or in
// %LOCALAPPDATA%\RDR2ASIMods\ when the game folder isn't writable). Backed by
// inipp (external/inipp, a git submodule), which works on plain std streams
// and avoids <filesystem>, unsafe on ScriptHookRDR2's small fiber stack.
#include "FFFCheat.h"

namespace Config
{
	using Values = FFFCheat::Options;

	// Re-reads FFFCheat.ini (creating it with defaults if missing) and
	// returns the values. Missing or unparsable keys keep their defaults.
	Values Load();
}

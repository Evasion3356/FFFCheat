#pragma once

// The Five Finger Fillet patches (see CLAUDE.md), built as a static library
// by FFFCheatLib.vcxproj. Linked by this repo's ASI and by Rampagio, which
// pulls this repo in as a submodule. Script thread only.
namespace FFFCheat
{
	struct Options
	{
		// Any valid Five Finger Fillet button counts as the expected one.
		bool AnyButtonCounts = true;
		// A press made before the prompt shows, or right after a flourish,
		// is ignored instead of counting as a miss.
		bool IgnoreEarlyPress = true;
		// Blanks the placeholder "MESSAGE" text that a game update added to
		// the minigame timer, and which fillet_sp never fills in.
		bool HideTimerMessage = true;
	};

	// Read each time fillet_sp loads, to pick the patches to apply. Without
	// a provider every option is on.
	using OptionsProvider = Options (*)();
	void SetOptionsProvider(OptionsProvider provider);

	// Call every script tick: patches fillet_sp whenever it loads.
	void OnTick();
	// Takes the patches out now; the next OnTick applies them again with
	// the provider's current options (for a host whose options changed).
	void Reapply();
	// Takes the patches out (on unload).
	void Shutdown();
}

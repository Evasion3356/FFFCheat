#include "..\external\ScriptHookSDK\inc\main.h"

#include "FFFCheat.h"

void ScriptMain()
{
	while (true)
	{
		FFFCheat::OnTick();
		WAIT(0);
	}
}

BOOL APIENTRY DllMain(HMODULE hInstance, DWORD reason, LPVOID lpReserved)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		scriptRegister(hInstance, ScriptMain);
	}
	else if (reason == DLL_PROCESS_DETACH)
	{
		// Non-null lpReserved: the process is exiting, so the game's memory
		// is going away with it -- restoring bytes (and writing the log
		// under the loader lock) would only add risk.
		if (!lpReserved)
			FFFCheat::Shutdown();
		scriptUnregister(hInstance);
	}

	return TRUE;
}

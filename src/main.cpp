#include "..\..\ScriptHookSDK\inc\main.h"

#include "FFFCheat.h"

void ScriptMain()
{
	while (true)
	{
		FFFCheat::OnTick();
		WAIT(0);
	}
}

BOOL APIENTRY DllMain(HMODULE hInstance, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		scriptRegister(hInstance, ScriptMain);
	}
	else if (reason == DLL_PROCESS_DETACH)
	{
		FFFCheat::Shutdown();
		scriptUnregister(hInstance);
	}

	return TRUE;
}

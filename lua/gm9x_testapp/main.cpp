#include <Windows.h>

extern "C" __declspec(dllexport) void* CreateInterface(const char*,int*)
{
	return NULL;
}

BOOL APIENTRY DllMain(HINSTANCE,DWORD fdwReason,LPVOID)
{
	if(fdwReason==DLL_PROCESS_ATTACH)
		MessageBoxA(NULL,"Loaded!","gm9x_testapp",MB_OK);
}
#include <Windows.h>
#include "tier1/interface.h"
#include "filesystem.h"

IFileSystem* filesystem = NULL;

DWORD WINAPI StartThread(LPVOID lpArg)
{
	CSysModule* sysFileSystemSteam = NULL,*sysTestApp = NULL;
	if(!Sys_LoadInterface("filesystem_steam",FILESYSTEM_INTERFACE_VERSION,
		&sysFileSystemSteam,reinterpret_cast<void**>(&filesystem)))
	{
		Warning("filesystem_steam "FILESYSTEM_INTERFACE_VERSION" not found!\n");
		FreeLibraryAndExitThread((HMODULE)lpArg,1);
	}

	filesystem->PrintSearchPaths();
	if(!(sysTestApp = filesystem->LoadModule("gm9x_testapp","GAMEBIN",false)))
		Warning("Failed loading gm9x_testapp!\n");
	filesystem->UnloadModule(sysTestApp);
	FreeLibraryAndExitThread((HMODULE)lpArg,0);
	return 0;
}

BOOL APIENTRY DllMain(HINSTANCE hDll,DWORD fdwReason,LPVOID)
{
	if(fdwReason==DLL_PROCESS_ATTACH)
		CreateThread(0,0,StartThread,(LPVOID)hDll,0,0);
	return TRUE;
}
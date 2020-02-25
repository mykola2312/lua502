#include "chook.h"
#include "tier0/memdbgon.h"

CHook::CHook(DWORD Func,SIZE_T Size)
{
	pFunc = Func;
	DetourSize = Size;

	OrigBytes = (PBYTE)malloc(Size);
}

CHook::~CHook()
{
	if(hooked)
		UnHookFunction();
}

DWORD CHook::HookFunction(DWORD Hook)
{
	DWORD dwProtect;
	pHook = Hook;
	hooked = true;

	VirtualProtect((LPVOID)pFunc,DetourSize,PAGE_EXECUTE_READWRITE,&dwProtect);

	memcpy(OrigBytes,(LPVOID)pFunc,DetourSize);
	memset((LPVOID)pFunc,'\x90',DetourSize);

	*(PBYTE)pFunc = 0xE9;
	*(PDWORD)(pFunc+1) = (pHook - pFunc) - 5;

	pOrig = (DWORD)VirtualAlloc(NULL,DetourSize+5,MEM_RESERVE | MEM_COMMIT,PAGE_EXECUTE_READWRITE);

	memcpy((LPVOID)pOrig,OrigBytes,DetourSize);

	*(PBYTE)(pOrig+DetourSize) = 0xE9;
	*(PDWORD)(pOrig+DetourSize+1) = ((pFunc+DetourSize) - pOrig) - DetourSize - 5;

	VirtualProtect((LPVOID)pFunc,DetourSize,dwProtect,NULL);

	return pOrig;
}

DWORD CHook::UnHookFunction()
{
	DWORD dwProtect;
	hooked = false;

	VirtualProtect((LPVOID)pFunc,DetourSize,PAGE_EXECUTE_READWRITE,&dwProtect);

	memcpy((LPVOID)pFunc,OrigBytes,DetourSize);
	free(OrigBytes);

	VirtualFree((LPVOID)pOrig,DetourSize+5,MEM_RELEASE | MEM_DECOMMIT);
	
	return pFunc;
}
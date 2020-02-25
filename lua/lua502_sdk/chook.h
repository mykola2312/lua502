#ifndef __CHOOK_H
#define __CHOOK_H

#include <Windows.h>
#include <string.h>
#include <malloc.h>

class CHook
{
public:
	CHook(DWORD Func,SIZE_T Size);
	~CHook();

	DWORD HookFunction(DWORD Hook);
	DWORD UnHookFunction();
private:
	bool hooked;
	DWORD pFunc;
	DWORD pHook;
	DWORD pOrig;
	SIZE_T DetourSize;
	PBYTE OrigBytes;
};

#endif
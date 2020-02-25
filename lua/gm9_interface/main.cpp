#include <Windows.h>
#include <TlHelp32.h>
#include <stdio.h>

typedef void* (*GetInterfaceFn_t)();
typedef struct interfacereg_s {
	GetInterfaceFn_t m_fnGetInterface;
	const char* m_pName;
	struct interfacereg_s* m_pNext;
} interfacereg_t;

void DumpInterfaces(PMODULEENTRY32 pModEntry,FILE* fDump)
{
	DWORD dwFunc;
	interfacereg_t* pInterfaceReg;

	if(!(dwFunc = (DWORD)GetProcAddress(pModEntry->hModule,"CreateInterface"))) return;
	fprintf(fDump,"==== %s ====\n",pModEntry->szModule);
	if(*(PWORD)((char*)dwFunc+0x03) != 0x3D8B){
		fputs("Signature failed\n",fDump);
		return;
	}
	if(!(pInterfaceReg = **(interfacereg_t***)((char*)dwFunc+0x05))){
		fputs("interfacereg_t null\n",fDump);
		return;
		return;
	}
	do {
		fprintf(fDump,"\t%s %p\n",pInterfaceReg->m_pName,(DWORD)
			((BYTE*)pInterfaceReg->m_fnGetInterface-pModEntry->modBaseAddr));
	} while(pInterfaceReg = pInterfaceReg->m_pNext);
}

DWORD WINAPI StartThread(LPVOID lpArg)
{
	FILE* fDump;
	fopen_s(&fDump,"gm9_interface.txt","wb");

	MODULEENTRY32 modEntry;
	HANDLE hSnapshot = NULL;
	DWORD dwCurProcId = GetCurrentProcessId();

	modEntry.dwSize = sizeof(MODULEENTRY32);
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,dwCurProcId);
	if(!hSnapshot) return 1;
	if(Module32First(hSnapshot,&modEntry))
	{
		do {
			if(modEntry.th32ProcessID!=dwCurProcId) continue;
			DumpInterfaces(&modEntry,fDump);
		} while(Module32Next(hSnapshot,&modEntry));
	}
	CloseHandle(hSnapshot);
	fclose(fDump);
	FreeLibraryAndExitThread((HMODULE)lpArg,0);
	return 0;
}

BOOL APIENTRY DllMain(HINSTANCE hDll,DWORD fdwReason,LPVOID lpArg)
{
	if(fdwReason==DLL_PROCESS_ATTACH)
		CreateThread(0,0,StartThread,(LPVOID)hDll,0,0);
	return TRUE;
}
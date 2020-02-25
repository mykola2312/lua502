#include <Windows.h>
#include <map>
#include "utlvector.h"
#include "utlsymbol.h"
#include "utlmemory.h"
#include "interface.h"
//#define GMOD9

#ifdef GMOD9
#	define FILESYSTEM_INTERFACEVERSION "VFileSystem017"
#	define FILESYSTEM_MODULE "filesystem_steam.dll"
#	define VPRINTPATHS 65
#else
#	define FILESYSTEM_INTERFACEVERSION "VFileSystem022"
#	define FILESYSTEM_MODULE "filesystem_stdio.dll"
#	define VPRINTPATHS 63
#endif
//Offset 0x48

#undef Msg
typedef void (*Msg_t)(const char*,...);
typedef void (__thiscall* PrintSearchPaths_t)(void* thisptr);
typedef HMODULE (__thiscall* LoadModule_t)(void* thisptr,const char* pName);
Msg_t g_pMsg;

class CSearchPath
{
public:
	int m_iStoreId;
	void* m_pPathInfo;
	bool m_bIsRemotePath;
	CUtlSymbol m_Path;
};

#define LOADMODULE_SIG "\x81\xEC\x00\x00\x00\x00\x53\x55\x56\x57\x8B\xF9\x8D\x44\x24\x28"
#define LOADMODULE_MASK "xx????xxxxxxxxxx"

typedef struct {
	const char* m_pSig;
	const char* m_pMask;
	const char* m_pName;
} signature_t;

signature_t g_zSigs[] = {
	{"\x81\xEC\x00\x00\x00\x00\x8B\x84\x24\x00\x00\x00\x00\x53\x55\x56\x57\x8B\xF1\x50\x89\x74\x24\x1C",
	"xx????xxx????xxxxxxxxxxx","CBaseFileSystem::LoadModule"},
	{"\x81\xEC\x00\x00\x00\x00\x53\x55\x56\x57\x8B\xF9\x8D\x44\x24\x28","xx????xxxxxxxxxx",
	"CBaseFileSystem::LoadModule__ALT"},
	{"\x81\xEC\x00\x00\x00\x00\x8B\x84\x24\x00\x00\x00\x00\x53\x55\x56\x57\x33\xFF\x3B\xC7\x8B\xF1",
	"xx????xxx????xxxxxxxxxx","CBaseFileSystem::CacheFileCRCs_R"},
	{NULL,NULL,NULL}
};

bool SigCheck(char* pMem,const char* pSig,const char* pMask)
{
	size_t iLen = strlen(pMask);
	for(int i = 0; i < iLen; i++)
		if(pMask[i] == 'x' && pMem[i] != pSig[i]) return false;
	return true;
}

DWORD WINAPI StartThread(LPVOID lpArg)
{
	DWORD dwFileSystem = 0;
	CSysModule* pFileSystemModule;
	g_pMsg = (Msg_t)GetProcAddress(GetModuleHandle("tier0.dll"),"Msg");
	if(!(dwFileSystem = (DWORD)((CreateInterfaceFn)GetProcAddress(GetModuleHandle(
		FILESYSTEM_MODULE),"CreateInterface"))(FILESYSTEM_INTERFACEVERSION,NULL)))
	{
		g_pMsg("FileSystem not found!\n");
		FreeLibraryAndExitThread((HMODULE)lpArg,1);
	}
	((PrintSearchPaths_t)((*(PDWORD*)dwFileSystem)[VPRINTPATHS]))((void*)dwFileSystem);
	PDWORD pdwVTable = *(PDWORD*)(dwFileSystem);
	for(int i = 0; i < 100; i++)
	{
		for(signature_t* pSig = g_zSigs; pSig->m_pMask; pSig++)
		{
			if(SigCheck((char*)pdwVTable[i],pSig->m_pSig,pSig->m_pMask))
				g_pMsg("VFunc %d %s\n",i,pSig->m_pName);
		}
	}

	//((LoadModule_t)((*(PDWORD*)dwFileSystem)[29]))((void*)dwFileSystem,"gm9x_testapp");
	FreeLibraryAndExitThread((HMODULE)lpArg,0);
	return 0;
}

BOOL APIENTRY DllMain(HINSTANCE hDll,DWORD fdwReason,LPVOID lpArg)
{
	if(fdwReason==DLL_PROCESS_ATTACH)
		CreateThread(0,0,StartThread,(LPVOID)hDll,0,0);
	return TRUE;
}
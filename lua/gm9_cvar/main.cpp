#include <Windows.h>
#include "sigscan.h"
#include "tier0/dbg.h"
#include "tier1.h"
#include "filesystem.h"
#include "icvar.h"
#include "igameevents.h"
#include "keyvalues.h"
#include "tier0/memdbgon.h"

#define APPFACTORY_SIG "\x8B\x44\x24\x04\x8B\x0D\x00\x00\x00\x00\x50\xE8\x00\x00\x00\x00\x8B\x4C\x24\x08\x85\xC9\x74\x09\x33\xD2\x85\xC0\x0F\x94\xC2\x89\x11\xC3"
#define APPFACTORY_MASK "xxxxxx????xx????xxxxxxxxxxxxxxxxxx"

IGameEventManager* gameeventmanager;
IFileSystem* filesystem;
ICvar* cvar;
ICvar* g_pCVar;

const char* g_pCvarFlags[] = {
	"FCVAR_UNREGISTERED", "FCVAR_LAUNCHER",
	"FCVAR_GAMEDLL", "FCVAR_CLIENTDLL",
	"FCVAR_MATERIAL_SYSTEM", "FCVAR_PROTECTED",
	"FCVAR_SPONLY", "FCVAR_ARCHIVE",
	"FCVAR_NOTIFY", "FCVAR_USERINFO",
	"FCVAR_PRINTABLEONLY", "FCVAR_UNLOGGED",
	"FCVAR_NEVER_AS_STRING", "FCVAR_REPLICATED",
	"FCVAR_CHEAT", "FCVAR_STUDIORENDER",
	"FCVAR_DEMO", "FCVAR_DONTRECORD",
	"FCVAR_PLUGIN", "FCVAR_DATACACHE",
	"FCVAR_TOOLSYSTEM", "FCVAR_FILESYSTEM",
	"FCVAR_NOT_CONNECTED", "FCVAR_SOUNDSYSTEM",
	"FCVAR_ARCHIVE_XBOX", "FCVAR_INPUTSYSTEM",
	"FCVAR_NETWORKSYSTEM", "FCVAR_VPHYSICS"
};

extern "C" ICvar* GetCVarIF()
{
	return cvar;
}

void GetCvarFlags(ConCommandBase* pBase,char* szBuf)
{
	int flags = pBase->m_nFlags&(~FCVAR_NON_ENGINE);
	for(int i = 0; i < 28; i++)
	{
		if(!((flags>>i)&1)) continue;
		//if(!pBase->IsBitSet(i)) continue;
		if(!i) V_strcpy(szBuf,g_pCvarFlags[i]);
		else sprintf(szBuf,"%s %s",szBuf,g_pCvarFlags[i]);
	}
}

CON_COMMAND(fire_event,"Fire test event")
{
	KeyValues* pEvent = new KeyValues("eventPlayerSpawnProp");
	pEvent->SetString("props","maps/gm_construct.bsp");
	gameeventmanager->FireEvent(pEvent);
}

class SCvarAccessor : public IConCommandBaseAccessor
{
public:
	virtual bool RegisterConCommandBase(ConCommandBase* pCmd)
	{
		Msg("Registering %s\n",pCmd->GetName());

		pCmd->AddFlags(FCVAR_PLUGIN);
		pCmd->SetNext(NULL);
		cvar->RegisterConCommandBase(pCmd);
		return true;
	}
};
static SCvarAccessor s_CvarAccessor;

DWORD WINAPI StartThread(LPVOID lpArg)
{
	CreateInterfaceFn fnAppFactory;
	DECLARE_SIGSCAN(fnAppFactory,"engine.dll",APPFACTORY_SIG,
		APPFACTORY_MASK,0,SigScan::CSigScan::SIG_FUNCTION);
	SigScan::Scan();

	gameeventmanager = (IGameEventManager*)fnAppFactory(
		INTERFACEVERSION_GAMEEVENTSMANAGER,0);
	filesystem = (IFileSystem*)fnAppFactory(FILESYSTEM_INTERFACE_VERSION,0);
	cvar = (ICvar*)fnAppFactory(VENGINE_CVAR_INTERFACE_VERSION,0);
	if(!gameeventmanager || !filesystem || !cvar)
	{
		Warning("Linking with appfactory failed!\n");
		return 1;
	}

	char* pFlags = (char*)malloc(1024);

	ConCommandBase* pNext = cvar->GetCommands();
	FileHandle_t fLog = filesystem->Open("cvardump.txt","wb","MOD");
	while(pNext)
	{
		bool bIsCommand = pNext->IsCommand();
		GetCvarFlags(pNext,pFlags);
		filesystem->FPrintf(fLog,"%s %s %p %s\n",(bIsCommand?"ConCommand":"ConVar"),
			pNext->GetName(),(bIsCommand
				?	(void*)((ConCommand*)pNext)->m_fnCommandCallback:
					pNext),pFlags);
		V_memset(pFlags,'\0',1024);
		pNext = (ConCommandBase*)pNext->GetNext();
	}

	//0x18
	Msg("ConCommand callback %d\n",offsetof(ConCommand,m_fnCommandCallback));
	ConCommandBaseMgr::OneTimeInit(&s_CvarAccessor);

	free(pFlags);
	filesystem->Close(fLog);
	return 0;
}

BOOL APIENTRY DllMain(HINSTANCE hDll,DWORD fdwReason,LPVOID)
{
	if(fdwReason==DLL_PROCESS_ATTACH)
		CreateThread(0,0,StartThread,(LPVOID)hDll,0,0);
	return TRUE;
}
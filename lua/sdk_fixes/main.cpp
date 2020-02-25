#define GAME_DLL
#include "cbase.h"
#include "main.h"
#include "plugin.h"
#include "sigscan.h"
#include "chook.h"
#include "dlls/iplayerinfo.h"
#include "filesystem.h"
#include "eiface.h"
#include "threadtools.h"
#include "utlvector.h"
#include "vphysics_interface.h"
#include "steam/steam_api.h"
#include "inetchannelinfo.h"
#include "inetchannel.h"
#include "inetmessage.h"
#include "netmessage.h"
#include "bitbuf.h"
#include "baselib.h"
#include "memdbgon.h"

class CBaseClient;
static bool _IsSecurePath(const char* pPath);
bool IsValidModel_Internal(const char* pPath);
bool IsValidModel_Hook(const char*);
void Nocollide_Hook(IPhysicsObject*,IPhysicsObject*);
bool __fastcall Process_Hook(void* thisptr,void*);

typedef struct {
	char name[260];
	char value[260];
} cvar_t;

typedef struct {
	void* vtable;
	bool m_bReliable;
	void* chan;
	void* handler;
	CUtlVector<cvar_t> cvars;
} net_setconvar_t;

typedef struct {
	void* p1;
	void* p2;
} pair_t;

inline bool IsPairsEqual(pair_t& a,pair_t& b)
{
	return ((a.p1 == b.p1) || (a.p1 == b.p2)
		|| (a.p2 == b.p1) || (a.p2 == b.p2));
}

typedef bool (__thiscall *PrcSetConVar_t)(CBaseClient*,net_setconvar_t*);
static bool __fastcall PrcSetConVar_Hook(CBaseClient*,void*,net_setconvar_t*);

typedef void (*Nocollide_t)(void*,void*);
typedef bool (__thiscall* Process_t)(void*);
typedef void (*Disconnect_t)(void*,const char*,...);

DECLARE_PLUGIN(CSDKFixes)
	virtual bool Load(CreateInterfaceFn,
		CreateInterfaceFn);
	virtual bool LuaInit(lua_State*);
	virtual void Unload();
	virtual void LevelShutdown();

	static IsValidModel_t s_pIsValidModel;
	static PrcSetConVar_t s_pPrcSetConVar;
	static Nocollide_t s_pNocollide;
	static Process_t s_pProcess;
	static Disconnect_t s_pDisconnect;

	CUtlVector<pair_t> m_Nocollides;
private:
	CHook* m_pIsValidModelHook;
	CHook* m_pSetConVarHook;
	CHook* m_pNocollideHook;
	CHook* m_pProcessHook;
END_PLUGIN(CSDKFixes,"sdk_fixes");

IsValidModel_t CSDKFixes::s_pIsValidModel = NULL;
PrcSetConVar_t CSDKFixes::s_pPrcSetConVar = NULL;
Nocollide_t CSDKFixes::s_pNocollide = NULL;
Process_t CSDKFixes::s_pProcess = NULL;
Disconnect_t CSDKFixes::s_pDisconnect = NULL;
IPlayerInfoManager* playerinfomanager = NULL;
IVEngineServer* engine = NULL;
IFileSystem* filesystem = NULL;

class CGameServer;

class CBaseClient
{
public:
	inline edict_t* GetEdict()
	{
		return *((edict_t**)this+145);
	}

	inline int GetUserId()
	{
		return *((int*)this+5);
	}

	inline const char* GetVar(const char* pKey)
	{
		return m_pConVars->GetString(pKey);
	}

	inline INetChannel* GetNetChannel()
	{
		return *((INetChannel**)this+54);
	}

	inline int GetSignonState()
	{
		return *((int*)this+55);
	}

	inline char* GetName()
	{
		return (char*)this+24;
	}

	void SetName(const char* pName)
	{
		strncpy(((char*)this+24),pName,32);
	}

	inline CSteamID* GetSteamId()
	{
		return *(CSteamID**)((char*)this+120);
	}

	inline void SetVar(const char* pKey,
		const char* pValue)
	{
		m_pConVars->SetString(pKey,pValue);
		//m_bConvarsChanged = false;
	}

	inline void SetConVarsChanged(bool bVal)
	{
		m_bConvarsChanged = bVal;
	}

	inline int GetClientIndex()
	{
		return *(int*)((char*)this+0xC);
	}

	char m_szTmp[160];
	KeyValues* m_pConVars;
	bool m_bConvarsChanged;
	CGameServer* m_Server;
};

class CGameServer
{
public:
	CBaseClient* GetClient(edict_t* pEdict)
	{
		for(int i = 0; i < m_Clients.Count(); i++)
			if(m_Clients[i]->GetEdict() == pEdict)
				return m_Clients[i];
		return NULL;
	}

	char m_szTmp[252];
	int m_nUserId;
	int m_nMaxclients;
	int m_nSpawnCount;
	float m_fTickInterval;
	CUtlVector<CBaseClient*> m_Clients;
};

CGameServer* gameserver = NULL;

bool CSDKFixes::Load(CreateInterfaceFn appFactory,
	CreateInterfaceFn gameFactory)
{
	DWORD dwIsValidModel,dwSetConVar,dwCall,dwNocollide,dwProcess;
	filesystem = (IFileSystem*)appFactory(
		FILESYSTEM_INTERFACE_VERSION,0);
	engine = (IVEngineServer*)appFactory(
		INTERFACEVERSION_VENGINESERVER,0);
	playerinfomanager = (IPlayerInfoManager*)gameFactory(
		INTERFACEVERSION_PLAYERINFOMANAGER,0);

	if(!filesystem || !engine || !playerinfomanager)
	{
		Warning("Failed appFactory/gameFactory (%p %p %p)\n",
			filesystem,engine,playerinfomanager);
		return false;
	}

	DECLARE_SIGSCAN(dwIsValidModel,"server.dll",
		ISVALIDMODEL_SIG,ISVALIDMODEL_MASK,0,
			SigScan::CSigScan::SIG_FUNCTION);
	DECLARE_SIGSCAN(dwCall,"server.dll",PCALL_SIG,
		PCALL_MASK,0,SigScan::CSigScan::SIG_FUNCTION);
	DECLARE_SIGSCAN(gameserver,"engine.dll",GAMESERVER_SIG,
		GAMESERVER_MASK,GAMESERVER_OFFSET,
			SigScan::CSigScan::SIG_PTR);
	DECLARE_SIGSCAN(dwSetConVar,"engine.dll",SETCONVAR_SIG,
		SETCONVAR_MASK,0,SigScan::CSigScan::SIG_FUNCTION);
	DECLARE_SIGSCAN(dwProcess,"engine.dll",CLCPROCESS_SIG,
		CLCPROCESS_MASK,0,SigScan::CSigScan::SIG_FUNCTION);
	DECLARE_SIGSCAN(s_pDisconnect,"engine.dll",CLDISCONNECT_SIG,
		CLDISCONNECT_MASK,0,SigScan::CSigScan::SIG_FUNCTION);
	SigScan::Scan();
	dwNocollide = FetchCall(dwCall);

	Msg("gameserver %p\n",gameserver);
	m_pIsValidModelHook = new CHook(dwIsValidModel,
		ISVALIDMODEL_HOOKSIZE);
	s_pIsValidModel = (IsValidModel_t)m_pIsValidModelHook
		->HookFunction((DWORD)IsValidModel_Hook);

	m_pSetConVarHook = new CHook(dwSetConVar,
		SETCONVAR_HOOKSIZE);
	s_pPrcSetConVar = (PrcSetConVar_t)m_pSetConVarHook
		->HookFunction((DWORD)&PrcSetConVar_Hook);

	m_pNocollideHook = new CHook(dwNocollide,PCALL_HOOKSIZE);
	s_pNocollide = (Nocollide_t)m_pNocollideHook
		->HookFunction((DWORD)&Nocollide_Hook);

	m_pProcessHook = new CHook(dwProcess,CLCPROCESS_HOOKSIZE);
	s_pProcess = (Process_t)m_pProcessHook
		->HookFunction((DWORD)&Process_Hook);
	Msg("GetPlayerUserid %p\n",(*(void***)engine)[15]);
	return true;
}

bool CSDKFixes::LuaInit(lua_State* L)
{
	CLuaLibrary::Init(L);
	return true;
}

void CSDKFixes::Unload()
{
	if(m_pIsValidModelHook)
		delete m_pIsValidModelHook;
	if(m_pSetConVarHook)
		delete m_pSetConVarHook;
	if(m_pNocollideHook)
		delete m_pNocollideHook;
}

void CSDKFixes::LevelShutdown()
{
	m_Nocollides.RemoveAll();
}

typedef enum {
	VALUE_NONE,
	VALUE_MODEL,
	VALUE_MATERIAL,
} vartype_t;

typedef struct {
	const char* pName;
	const char* pDefault;
	vartype_t type;
} userinfo_t;

userinfo_t g_UserInfos[] = {
	{"gm_wheel_model","models/props_junk/sawblade001a.mdl",VALUE_MODEL},
	{NULL,NULL,VALUE_NONE}
};

static bool __fastcall PrcSetConVar_Hook(CBaseClient* pClient,
	void* edx,net_setconvar_t* pMsg)
{
	bool bOk = CSDKFixes::s_pPrcSetConVar(pClient,pMsg);
	for(int i = 0; i < pMsg->cvars.Count(); i++)
	{
		for(userinfo_t* j = g_UserInfos; j->pName; j++)
		{
			cvar_t& cv = pMsg->cvars[i];
			if(strcmp(cv.name,j->pName)
				|| j->type != VALUE_MODEL)
				continue;
			if(!IsValidModel_Internal(cv.value))
			{
				Msg("Client %d tried to send invalid user value (%s)!\n",
					pClient->GetUserId(),cv.name);
				strcpy_s(cv.value,j->pDefault);
			}
		}
	}
	return bOk;
}

inline bool _CheckExtension(const char* pPath,
	const char* pExt)
{
	const char* pTmp;
	if(!(pTmp = strrchr(pPath,'.')))
		return false;
	return !strcmp(pTmp,pExt);
}

static bool _IsSecurePath(const char* pPath)
{
	static char s_cBanned[] = {
		'\t','\r','\b','\n',':'};
	size_t uLen = strlen(pPath);
	if(pPath[0] == '/' 
		|| pPath[0] == '\\')
		return false;
	for(size_t i = 0; i < uLen; i++)
	{
		if(pPath[i] == s_cBanned[
			i%sizeof(s_cBanned)])
			return false;
		if(pPath[i] == '.' && pPath[i+1] == '.')
			return false;
	}

	return !_CheckExtension(pPath,".bsp");
}

bool IsValidModel_Internal(const char* pPath)
{
	if(!_IsSecurePath(pPath)) return false;
	if(!_CheckExtension(pPath,".mdl")) return false;
	if(!filesystem->FileExists(pPath))
		return false;
	return true;
}

bool IsValidModel_Hook(const char* pPath)
{
	if(!IsValidModel_Internal(pPath))
		return false;
	return CSDKFixes::s_pIsValidModel(pPath);
}

void Nocollide_Hook(IPhysicsObject* obj1,IPhysicsObject* obj2)
{
	pair_t pr;
	pr.p1 = obj1;
	pr.p2 = obj2;
	for(int i = 0; i < s_CSDKFixes.m_Nocollides.Count(); i++)
	{
		pair_t pr2 = s_CSDKFixes.m_Nocollides[i];
		if(IsPairsEqual(pr,pr2)) return;
	}
	s_CSDKFixes.m_Nocollides.AddToTail(pr);
	CSDKFixes::s_pNocollide(obj1,obj2);
}

const char* RenderSteamId(CSteamID* id)
{
	static char szBuf[64];
	uint64 sid = id->ConvertToUint64();
	int x,y,z;
	//STEAM_X:Y:Z
	x = sid>>61;
	y = sid&1;
	z = (sid>>1)&0x7FFFFFFF;
	Q_snprintf(szBuf,64,"STEAM_%d:%d:%d",x,y,z);
	return szBuf;
}

bool __fastcall Process_Hook(void* thisptr,void*)
{
	INetMessage* pMsg = (INetMessage*)thisptr;
	INetChannel* netchan = pMsg->GetNetChannel();
	for(int i = 0; i < gameserver->m_Clients.Count(); i++)
	{
		CBaseClient* cl = gameserver->m_Clients[i];
		if(cl->GetNetChannel() == netchan)
		{
			if(cl->GetSignonState() > 3)
			{
				char szBuf[128];
				Q_snprintf(szBuf,sizeof(szBuf),"Player %s (%s) attempted to bug sprays, lets kick him\n",
					cl->GetName(),RenderSteamId(cl->GetSteamId()));

				Log(szBuf);
				CSDKFixes::s_pDisconnect((char*)cl+0x4,szBuf);
				return false;
			}
		}
	}

	return CSDKFixes::s_pProcess(thisptr);
}

DECLARE_LIBRARY("_player")
DECLARE_TABLE(_G)

inline CBaseClient* GetPlayer(lua_State* L)
{
	edict_t* pPly;
	CBaseClient* pClient;
	if(!(pPly = engine->PEntityOfEntIndex((int)lua_tonumber(L,1))))
		luaL_error(L,"Wrong player index!");
	if(!(pClient = gameserver->GetClient(pPly)))
		luaL_error(L,"Edict isn't a client (%d)!",(int)lua_tonumber(L,1));
	return pClient;
}

DECLARE_FUNCTION(_G,_SetClientConVar_String)
{
	luaL_checktype(L,1,LUA_TNUMBER);
	luaL_checktype(L,2,LUA_TSTRING);
	luaL_checktype(L,3,LUA_TSTRING);
	CBaseClient* pPlayer = GetPlayer(L);
	pPlayer->SetVar(lua_tostring(L,2),lua_tostring(L,3));
	pPlayer->SetConVarsChanged(true);
	return 0;
}

DECLARE_FUNCTION(_G,_PlayerGetName)
{
	luaL_checktype(L,1,LUA_TNUMBER);
	lua_pushstring(L,GetPlayer(L)->GetVar("name"));
	return 1;
}

DECLARE_FUNCTION(_G,_PlayerSetName)
{
	luaL_checktype(L,1,LUA_TNUMBER);
	luaL_checktype(L,2,LUA_TSTRING);
	CBaseClient* pPlayer = GetPlayer(L);
	pPlayer->SetVar("name",lua_tostring(L,2));
	pPlayer->SetConVarsChanged(true);
	return 0;
}
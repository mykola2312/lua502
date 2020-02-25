#define GAME_DLL
#include "plugin.h"
#include "tier1.h"
#include "eiface.h"
#include "baselib.h"
#include "cbase.h"
#include "pplib.h"
#include "main.h"

DECLARE_PLUGIN(CSDKPPCore)
	virtual bool Load(CreateInterfaceFn,CreateInterfaceFn);
	virtual void Unload();
	
	virtual void LevelShutdown();
	virtual void ClientActive(edict_t*);
	virtual void ClientDisconnect(edict_t*);

	virtual bool LuaInit(lua_State*);
	virtual void OnSWEPLuaCreate(lua_State* L);

	CUtlVector<object_t> m_Objects;
END_PLUGIN(CSDKPPCore,"sdk_ppcore");

IVEngineServer* engine;

inline uint32 ToAccountId(const char* pSteamId)
{
	uint32 account;
	sscanf(pSteamId,"STEAM_%*d:%*d:%d",&account);
	return account;
}

DLL_EXPORT int GetObjectOwner(EHANDLE hObject)
{
	for(int i = 0; i < s_CSDKPPCore.m_Objects.Count(); i++)
	{
		object_t obj = s_CSDKPPCore.m_Objects[i];
		if(obj.m_hEntity.GetEntryIndex() == hObject.GetEntryIndex())
			return obj.m_iAccount;
	}
	return 0;
}

DLL_EXPORT int GetPlayerAccountId(edict_t* pPly)
{
	return ToAccountId(engine->GetPlayerNetworkIDString(pPly));
}

DLL_EXPORT CUtlVector<object_t>* GetAllObjects()
{
	return &s_CSDKPPCore.m_Objects;
}

bool CSDKPPCore::Load(CreateInterfaceFn engineFn,CreateInterfaceFn gameFn)
{
	ConnectTier1Libraries(&engineFn,1);
	if(!(engine = (IVEngineServer*)engineFn(INTERFACEVERSION_VENGINESERVER,0)))
		return false;
	return true;
}

void CSDKPPCore::OnSWEPLuaCreate(lua_State* L)
{
}

void CSDKPPCore::Unload()
{
	DisconnectTier1Libraries();
}

void CSDKPPCore::LevelShutdown()
{
	m_Objects.RemoveAll();
}

void CSDKPPCore::ClientActive(edict_t* pPly)
{
	int account = GetPlayerAccountId(pPly);
	for(int i = 0; i < m_Objects.Count();)
	{
		object_t obj = m_Objects[i];
		if(!INDEXENT(obj.m_hEntity.GetEntryIndex()))
		{
			m_Objects.Remove(i);
			continue;
		}
		else if(obj.m_iAccount == account)
		{
			GetProperty(pPly,obj.m_iType)
				->AddToTail(obj.m_hEntity);
		}
		i++;
	}
}

void CSDKPPCore::ClientDisconnect(edict_t* pPly)
{
	object_t obj;
	obj.m_iAccount = GetPlayerAccountId(pPly);
	for(int i = 0; i < MaxType; i++)
	{
		obj.m_iType = i;
		EntityVector_t* pEnts = GetProperty(pPly,i);
		for(int j = 0; j < pEnts->Count(); j++)
		{
			EHANDLE hnd = pEnts->Element(j);
			if(INDEXENT(hnd.GetEntryIndex()))
			{
				obj.m_hEntity = hnd;
				m_Objects.AddToTail(obj);
			}
		}
	}
}

bool CSDKPPCore::LuaInit(lua_State* L)
{
	CLuaLibrary::Init(L);
	return true;
}
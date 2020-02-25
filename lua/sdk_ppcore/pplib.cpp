#define GAME_DLL
#include "cbase.h"
#include "baselib.h"
#include "hl2mp_dll/hl2mp_player.h"
#include "hl2mp/hl2mp_player_shared.h"
#include "basehandle.h"
#include "pplib.h"
#include "main.h"

DECLARE_LIBRARY("pp");
DECLARE_TABLE(pp);

static int s_iOffsets[] = {
	0x1094,
	0x10AC,
	0x10C4,
	0x10F4,
	0x10DC,
	0x110C,
	0x1124,
	0x113C,
	0x1154,
	0x107C,
};

DLL_EXPORT EntityVector_t* GetProperty(edict_t* pEdict,int iType)
{
	CBaseEntity* pPly;
	if(!(pPly = GetContainingEntity(pEdict)))
		return NULL;
	return (EntityVector_t*)((char*)pPly+s_iOffsets[iType]);
}

DLL_EXPORT void FixupProperty(EntityVector_t* pProperty)
{
	for(int i = 0; i < pProperty->Count();)
	{
		if(!INDEXENT(pProperty->Element(i).GetEntryIndex()))
			pProperty->Remove(i);
		else i++;
	}
}

inline CBaseEntity* ToEntity(lua_State* L,int idx = 1)
{
	int iEnt = lua_tonumber(L,idx);
	CBaseEntity* pPly;
	edict_t* pEdict;
	if(!(pEdict = INDEXENT(iEnt)))
		return NULL;
	if(!(pPly = GetContainingEntity(pEdict)))
		return NULL;
	//if(!pPly->IsPlayer()) return NULL;
	return pPly;
}

#define DECLARE_OBJECT(type)													\
DECLARE_FUNCTION(pp,Get##type)													\
{																				\
	lua_newtable(L);															\
	CBaseEntity* pPly;															\
	if(!(pPly = ToEntity(L)))													\
		return 1;																\
	CUtlVector<EHANDLE >* pVec													\
		= (CUtlVector<EHANDLE >*)((char*)pPly+s_iOffsets[type]);				\
	for(int i = 0; i < pVec->Count();)											\
	{																			\
		int iEnt = pVec->Element(i).GetEntryIndex();							\
		if(INDEXENT(iEnt))														\
		{																		\
			lua_pushnumber(L,iEnt);												\
			lua_rawseti(L,-2,i++ + 1);											\
		}																		\
		else pVec->Remove(i);													\
	}																			\
	return 1;																	\
}																				\
DECLARE_FUNCTION(pp,Add##type)													\
{																				\
	CBaseEntity* pPly,*pEnt;													\
	if(!(pPly = ToEntity(L)))													\
		return 0;																\
	if(!(pEnt = ToEntity(L,2)))													\
		return 0;																\
	CUtlVector<EHANDLE >* pVec													\
		= (CUtlVector<EHANDLE >*)												\
					((char*)pPly+s_iOffsets[type]);								\
	pVec->AddToTail(EHANDLE(pEnt));												\
	return 0;																	\
}

//enum {
//	Ragdolls,
//	Props,
//	Balloons,
//	Effects,
//	Sprites,
//	Emitters,
//	Wheels,
//	Npcs,
//	Dynamites,
//	Thrusters
//};

DECLARE_OBJECT(Ragdolls)
DECLARE_OBJECT(Props)
DECLARE_OBJECT(Balloons)
DECLARE_OBJECT(Effects)
DECLARE_OBJECT(Sprites)
DECLARE_OBJECT(Emitters)
DECLARE_OBJECT(Wheels)
DECLARE_OBJECT(Npcs)
DECLARE_OBJECT(Dynamites)
DECLARE_OBJECT(Thrusters)

DECLARE_FUNCTION(pp,GetObjectAccount)
{
	luaL_checktype(L,1,LUA_TNUMBER);

	int idx = lua_tonumber(L,1);
	if(!INDEXENT(idx))
		lua_pushnumber(L,0);
	else
	{
		lua_pushnumber(L,GetObjectOwner(
			EHANDLE::FromIndex(idx)));
	}
	return 1;
}

DECLARE_FUNCTION(pp,GetPlayerAccountid)
{
	luaL_checktype(L,1,LUA_TNUMBER);
	edict_t* pPly;
	if(!(pPly = INDEXENT(lua_tonumber(L,1))))
		lua_pushnumber(L,0);
	else
		lua_pushnumber(L,GetPlayerAccountId(pPly));
	return 1;
}

//DLL_EXPORT CUtlVector<object_t>* GetAllObjects();
DECLARE_FUNCTION(pp,GetAccountObjects)
{
	luaL_checktype(L,1,LUA_TNUMBER);
	luaL_checktype(L,2,LUA_TNUMBER);
	lua_newtable(L);
	int iAcc = lua_tonumber(L,1);
	int iType = lua_tonumber(L,2);
	if(iType >= MaxType) return 1;
	CUtlVector<object_t>* pObjs = GetAllObjects();
	int idx = 1;
	for(int i = 0; i < pObjs->Count(); i++)
	{
		object_t obj = pObjs->Element(i);
		if(obj.m_iAccount == iAcc 
			&& obj.m_iType == iType)
		{
			lua_pushnumber(L,obj.m_hEntity.GetEntryIndex());
			lua_rawseti(L,-2,idx++);
		}
	}
	return 1;
}

DECLARE_FUNCTION(pp,GetPlayerObjects)
{
	luaL_checktype(L,1,LUA_TNUMBER);
	luaL_checktype(L,2,LUA_TNUMBER);
	lua_newtable(L);
	edict_t* pEdict;
	int iType = lua_tonumber(L,2);
	if(iType >= MaxType) return 1;
	if(!(pEdict = INDEXENT(lua_tonumber(L,1))))
		return 1;
	int idx = 1;
	EntityVector_t* pEnts = GetProperty(pEdict,iType);
	for(int i = 0; i < pEnts->Count(); i++)
	{
		int iEnt = pEnts->Element(i).GetEntryIndex();
		if(INDEXENT(iEnt))
		{
			lua_pushnumber(L,iEnt);
			lua_rawseti(L,-2,idx++);
		}
	}
	return 1;
}
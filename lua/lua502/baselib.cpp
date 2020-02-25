#define __BASELIB_INTERNAL_H
#include <stdio.h>
#include "tier1/strtools.h"
#include "tier0/dbg.h"
#include "baselib.h"
#include "types.h"

/*LUA_API userdata_t* luaL_createuserdata(lua_State* L,
	unsigned short type,void* pData)
{
	userdata_t* pUserData = (userdata_t*)lua_newuserdata(L,sizeof(userdata_t));
	pUserData->m_nType = type;
	pUserData->m_pData = pData;

	lua_pushref(L,type); //Notice that type is reference id!
	lua_setmetatable(L,-2);

	return pUserData;
}*/

LUA_API void* luaL_createuserdata(lua_State* L,size_t sz,CLuaMetaTable* pMeta)
{
	void* pUserData = lua_newuserdata(L,sz+sizeof(unsigned short));
	*(unsigned short*)pUserData = pMeta->m_iTypeID;

	pMeta->PushToStack(L);
	lua_setmetatable(L,-2);

	return (char*)pUserData+sizeof(unsigned short);
}

LUA_API void* luaL_multicheckuserdata(lua_State* L,int idx,int nTypes,...)
{
	void* pUserData = lua_touserdata(L,1);
	int type = *(unsigned short*)pUserData;
	pUserData = (char*)pUserData+sizeof(unsigned short);

	va_list ap;
	va_start(ap,nTypes);
	for(int i = 0; i < nTypes; i++)
	{
		if(type == va_arg(ap,int))
		{
			va_end(ap);
			return pUserData;
		}
	}
	va_end(ap);
	luaL_argerror(L,idx,"Wrong userdata");
	return NULL;
}

LUA_API void luaL_pushfstring(lua_State* L,const char* pFmt,...)
{
	char szBuf[MAX_PATH];
	va_list ap;

	va_start(ap,pFmt);
	V_vsnprintf(szBuf,MAX_PATH,pFmt,ap);
	va_end(ap);

	lua_pushstring(L,szBuf);
}

LUA_API int lua_refobj(lua_State* L)
{
	char szName[16];
	static int iID = 0;
	sprintf(szName,"lua502ref_%04x",++iID);
	lua_pushstring(L,szName);
	lua_pushvalue(L,-2);
	lua_settable(L,LUA_REGISTRYINDEX);
	lua_pop(L,1);
	return iID;
}

LUA_API void lua_unrefobj(lua_State* L,int id)
{
	char szName[16];
	sprintf(szName,"lua502ref_%04x",id);
	lua_pushstring(L,szName);
	lua_pushnil(L);
	lua_settable(L,LUA_REGISTRYINDEX);
}

LUA_API void lua_pushref(lua_State* L,int id)
{
	char szName[16];
	sprintf(szName,"lua502ref_%04x",id);
	lua_pushstring(L,szName);
	lua_gettable(L,LUA_REGISTRYINDEX);
}

CLuaFunction::CLuaFunction(CLuaTable* pTable,const char* pName,
	lua_CFunction fnFunc)
{
	m_pName = pName;
	m_fnFunc = fnFunc;
	pTable->Add(this);
}

CLuaTable::CLuaTable(CLuaLibrary* pLib,const char* pName,
	bool bRegister)
{
	m_pName = pName;
	m_bRegister = bRegister;
	pLib->Add(this);
}

int luaf_unk_tostring(lua_State* L)
{
	luaL_checktype(L,1,LUA_TUSERDATA);
	UInt16* pUserData = (UInt16*)lua_touserdata(L,1);
	luaL_pushfstring(L,"%s %p",luaf_typename(*pUserData),
		(char*)pUserData+sizeof(UInt16));
	return 1;
}

void CLuaTable::Add(CLuaFunction* pFunc)
{
	pFunc->m_pNext = m_pLast;
	m_pLast = pFunc;
}

//Still work
void CLuaTable::Register(lua_State* L)
{
	CLuaFunction* pFunc;
	if(!(pFunc = m_pLast)) return;

	if(!V_strcmp(m_pName,"_G"))
	{
		do {
			lua_register(L,pFunc->m_pName,
				pFunc->m_fnFunc);
		} while((pFunc=pFunc->m_pNext));
	}
	else
	{
		lua_pushstring(L,m_pName);
		lua_newtable(L);
		do {
			lua_pushstring(L,pFunc->m_pName);
			lua_pushcfunction(L,pFunc->m_fnFunc);
			lua_settable(L,-3);
		} while((pFunc=pFunc->m_pNext));
		lua_settable(L,LUA_GLOBALSINDEX);
	}
}

void CLuaTable::Unregister(lua_State* L)
{
	CLuaFunction* pFunc = m_pLast;
	if(!V_strcmp(m_pName,"_G"))
	{
		while(pFunc)
		{
			lua_pushstring(L,pFunc->m_pName);
			lua_pushnil(L);
			lua_settable(L,LUA_GLOBALSINDEX);
			pFunc = pFunc->m_pNext;
		}
	}
	else
	{
		lua_pushstring(L,m_pName);
		lua_pushnil(L);
		lua_settable(L,LUA_GLOBALSINDEX);
	}
}

CLuaMetaTable::CLuaMetaTable(CLuaLibrary* pLib,
	unsigned short nTypeID,CLuaTable* pParent)
	: CLuaTable(pLib,NULL)
{
	m_iTypeID = (int)nTypeID;
	m_pParent = pParent;
}

void CLuaMetaTable::Register(lua_State* L)
{
	CLuaFunction* pFunc;
	bool bHasIndex = false,bHasToString = false;
	if(!(pFunc = m_pLast)) return;

	sprintf(m_szMetaName,"metatable_%04d",m_iTypeID);

	lua_pushstring(L,m_szMetaName);
	lua_newtable(L);
	do {
		if(!V_strcmp(pFunc->m_pName,"__index"))
			bHasIndex = true;
		if(!V_strcmp(pFunc->m_pName,"__tostring"))
			bHasToString = true;
		lua_pushstring(L,pFunc->m_pName);
		lua_pushcfunction(L,pFunc->m_fnFunc);
		lua_settable(L,-3);
	} while((pFunc=pFunc->m_pNext));
	if(m_pParent)
	{
		pFunc = m_pParent->m_pLast;
		while(pFunc)
		{
			if(!V_strcmp(pFunc->m_pName,"__index"))
				bHasIndex = true;
			if(!V_strcmp(pFunc->m_pName,"__tostring"))
				bHasToString = true;
			lua_pushstring(L,pFunc->m_pName);
			lua_pushcfunction(L,pFunc->m_fnFunc);
			lua_settable(L,-3);
			pFunc = pFunc->m_pNext;
		}
	}
	if(!bHasIndex)
	{
		lua_pushstring(L,"__index");
		lua_pushvalue(L,-2);
		lua_settable(L,-3);
	}
	if(!bHasToString)
	{
		lua_pushstring(L,"__tostring");
		lua_pushcfunction(L,luaf_unk_tostring);
		lua_settable(L,-3);
	}
	lua_settable(L,LUA_REGISTRYINDEX);
}

void CLuaMetaTable::Unregister(lua_State* L)
{
	lua_pushstring(L,m_szMetaName);
	lua_pushnil(L);
	lua_settable(L,LUA_REGISTRYINDEX);
}

void CLuaMetaTable::PushToStack(lua_State* L)
{
	lua_pushstring(L,m_szMetaName);
	lua_gettable(L,LUA_REGISTRYINDEX);
}

CLuaLibrary* CLuaLibrary::s_pLast = NULL;

CLuaLibrary::CLuaLibrary(const char* pName)
{
	m_pName = pName;
	m_pLastTable = NULL;

	m_pNextLibrary = s_pLast;
	s_pLast = this;
}

void CLuaLibrary::Add(CLuaTable* pTable)
{
	pTable->m_pNext = m_pLastTable;
	m_pLastTable = pTable;
}

void CLuaLibrary::Register(lua_State* L)
{
	CLuaTable* pTable;
	if(!(pTable = m_pLastTable)) return;
	do {
		if(!pTable->m_bRegister) continue;
		pTable->Register(L);
	} while((pTable=pTable->m_pNext));
}

void CLuaLibrary::Unregister(lua_State* L)
{
	CLuaTable* pTable;
	if(!(pTable = m_pLastTable)) return;
	do {
		if(!pTable->m_bRegister) continue;
		pTable->Unregister(L);
	} while((pTable=pTable->m_pNext));
}

void CLuaLibrary::Init(lua_State* L)
{
	CLuaLibrary* pLib;
	if(!(pLib = s_pLast)) return;
	do {
		pLib->Register(L);
	} while((pLib=pLib->m_pNextLibrary));
}

void CLuaLibrary::Exit(lua_State* L)
{
	CLuaLibrary* pLib;
	if(!(pLib = s_pLast)) return;
	do {
		pLib->Unregister(L);
	} while((pLib=pLib->m_pNextLibrary));
}

void CLuaLibrary::Dump()
{
	CLuaLibrary* pLib;
	CLuaTable* pTable;
	CLuaFunction* pFunc;

	pLib = s_pLast;
	while(pLib)
	{
		Msg("library %s\n",pLib->m_pName);
		pTable = pLib->m_pLastTable;
		while(pTable)
		{
			Msg("\ttable %s\n",pTable->m_iTypeID>1024
				? pTable->m_pName:"(metatable)");
			pFunc = pTable->m_pLast;
			while(pFunc)
			{
				Msg("\t\tfunction %s %p\n",pFunc->m_pName,
					pFunc->m_fnFunc);
				pFunc = pFunc->m_pNext;
			}
			pTable = pTable->m_pNext;
		}
		pLib = pLib->m_pNextLibrary;
	}
}
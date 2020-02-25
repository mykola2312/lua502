#include "glua.h"
#include "plugin.h"
#include "baselib.h"
#include "types.h"
#include "luapointer.h"

static int s_iType = LUAPLUGIN;

DECLARE_LIBRARY("mainlib");
/*DECLARE_METATABLE(metaplugin,s_iType);

static luaplugin_t* luaf_makeplugin(lua_State* L,luaplugin_t* pPlug)
{
	luaplugin_t* pPlug2 = (luaplugin_t*)luaL_createuserdata(L,sizeof(luaplugin_t),
		&g_LuaTable_metaplugin);
	V_memcpy(pPlug2,pPlug,sizeof(luaplugin_t)); //Protect plugin
	return pPlug2;
}

DECLARE_FUNCTION(metaplugin,__tostring)
{
	luaplugin_t* pPlug = (luaplugin_t*)luaf_checkuserdata(L,1,&g_LuaTable_metaplugin);
	if(!pPlug) lua_pushnil(L);
	else luaL_pushfstring(L,"luaplugin_t %s",pPlug->m_pName);
	return 1;
}

DECLARE_FUNCTION(metaplugin,__eq)
{
	luaplugin_t* pOne,*pTwo;
	if(!(pOne = (luaplugin_t*)luaf_checkuserdata(L,1,&g_LuaTable_metaplugin))
		|| (pTwo = (luaplugin_t*)luaf_checkuserdata(L,2,&g_LuaTable_metaplugin)))
		lua_pushboolean(L,false);
	else lua_pushboolean(L,(pOne->m_hModule==pTwo->m_hModule));
	return 1;
}

DECLARE_FUNCTION(metaplugin,__gc)
{
	return 0;
}

DECLARE_FUNCTION(metaplugin,GetName)
{
	luaplugin_t* pPlug = (luaplugin_t*)luaf_checkuserdata(L,1,
		&g_LuaTable_metaplugin);
	if(!pPlug) lua_pushnil(L);
	else lua_pushstring(L,pPlug->m_pName);
	return 1;
}

DECLARE_FUNCTION(metaplugin,GetAPIVersion)
{
	luaplugin_t* pPlug = (luaplugin_t*)luaf_checkuserdata(L,1,
		&g_LuaTable_metaplugin);
	if(!pPlug) lua_pushnil(L);
	else lua_pushnumber(L,pPlug->m_iPluginAPI);
	return 1;
}

DECLARE_FUNCTION(metaplugin,GetInitExit)
{
	luaplugin_t* pPlug = (luaplugin_t*)luaf_checkuserdata(L,1,
		&g_LuaTable_metaplugin);
	if(!pPlug)
	{
		lua_pushnil(L);
		lua_pushnil(L);
	}
	else
	{
		luaf_makepointer(L,pPlug->m_fnInit);
		luaf_makepointer(L,pPlug->m_fnExit);
	}
	return 2;
}

DECLARE_FUNCTION(metaplugin,GetNext)
{
	luaplugin_t* pPlug = (luaplugin_t*)luaf_checkuserdata(L,1,
		&g_LuaTable_metaplugin);
	if(!pPlug)
	{
		lua_pushnil(L);
		lua_pushnil(L);
	}
	else
	{
		if(!pPlug->m_pNext)
		{
			lua_pushnil(L);
			lua_pushboolean(L,false);
		}
		else
		{
			luaf_makeplugin(L,pPlug->m_pNext);
			lua_pushboolean(L,true);
		}
	}
	return 2;
}

DECLARE_FUNCTION(metaplugin,Unload)
{
	luaplugin_t* pPlug = (luaplugin_t*)luaf_checkuserdata(L,1,
		&g_LuaTable_metaplugin);
	luaplugin_t* pIter = NULL,*pTarget = NULL;
	int iRet = 0;
	if(!pPlug)
	{
		lua_pushnil(L);
		lua_pushnil(L);
		return 2;
	}

	pIter = g_pLua502->FirstPlugin();
	while(pIter)
	{
		if(pPlug->m_fnInit == pIter->m_fnInit
			&& pPlug->m_fnExit == pIter->m_fnExit
			&& !V_strcmp(pPlug->m_pName,pIter->m_pName))
		{
			pTarget = pIter;
			break;
		}
		pIter = pIter->m_pNext;
	}

	if(!pTarget)
	{
		lua_pushnil(L);
		lua_pushnil(L);
		return 2;
	}

	lua_pushboolean(L,(!g_pLua502->UnloadPlugin(pTarget,&iRet)));
	lua_pushnumber(L,iRet);
	return 2;
}*/

DECLARE_TABLE(lua502);

//DECLARE_FUNCTION(lua502,GetAPIVersion)
//{
//	lua_pushnumber(L,LUAPLUGIN_API);
//	return 1;
//}

DECLARE_FUNCTION(lua502,Print)
{
	g_pLua502->Print("%s\n",lua_tostring(L,1));
	return 0;
}

DECLARE_FUNCTION(lua502,RunString)
{
	const char* pErr = NULL,*pCode;
	size_t uLen;
	luaL_checktype(L,1,LUA_TSTRING);
	
	pCode = lua_tostring(L,1);
	uLen = lua_strlen(L,1);

	if((pErr=g_pLua502->RunString(pCode,uLen,"LUA")))
		lua_pushstring(L,pErr);
	else lua_pushnil(L);
	return 1;
}

DECLARE_FUNCTION(lua502,RunFile)
{
	luaL_checktype(L,1,LUA_TSTRING);
	luafile_err_t iErr;
	const char* pErr;
	char szLuaPath[MAX_PATH];

	sprintf(szLuaPath,"lua/%s",lua_tostring(L,1));
	if(!CLua502Plugin::IsPathSecure(szLuaPath))
	{
		luaL_error(L,"Path may not contain specific symbols!");
		lua_pushnil(L);
		lua_pushnil(L);
		return 2;
	}

	if((iErr = g_pLua502->RunFile(szLuaPath,&pErr)))
	{
		lua_pushnumber(L,iErr);
		if(iErr == LUAFILE_LUA_ERROR)
			lua_pushstring(L,pErr);
		else lua_pushnil(L);
		return 2;
	}

	lua_pushnil(L);
	lua_pushnil(L);
	return 2;
}

DECLARE_FUNCTION(lua502,GetLuaInterface)
{
	luaf_makepointer(L,g_pLua502->GetLuaInterface());
	return 1;
}

DECLARE_FUNCTION(lua502,LoadPlugin)
{
	int iPlugin = 0;
	luaL_checktype(L,1,LUA_TSTRING);
	if(g_pLua502->LoadPlugin(lua_tostring(L,1),
		&iPlugin))
		lua_pushnumber(L,iPlugin);
	else lua_pushnil(L);
	return 1;
}

DECLARE_FUNCTION(lua502,UnloadPlugin)
{
	luaL_checktype(L,1,LUA_TNUMBER);
	g_pLua502->UnloadPlugin(
		lua_tonumber(L,1));
	return 0;
}

DECLARE_FUNCTION(lua502,GetRegistry)
{
	lua_pushvalue(L,LUA_REGISTRYINDEX);
	return 1;
}

/*class CMainLibrary : public CLuaLibrary
{
public:
	CMainLibrary() : CLuaLibrary("main")
	{
		m_iRefOpenScript = 0;
	}

	virtual void Register(lua_State* L)
	{
		lua_pushstring(L,"_OpenScript");
		lua_rawget(L,LUA_GLOBALSINDEX);
		m_iRefOpenScript = lua_refobj(L);

		lua_pushstring(L,"print");
		lua_pushcfunction(L,g_lua502Print.m_fnFunc);
		lua_rawset(L,LUA_GLOBALSINDEX);

		lua_pushstring(L,"_OpenScript");
		lua_pushcfunction(L,g_lua502RunFile.m_fnFunc);
		lua_rawset(L,LUA_GLOBALSINDEX);
	}

	virtual void Unregister(lua_State* L)
	{
		lua_pushstring(L,"_OpenSript");
		lua_pushref(L,m_iRefOpenScript);
		lua_rawset(L,LUA_GLOBALSINDEX);

		lua_pushstring(L,"print");
		lua_pushnil(L);
		lua_rawset(L,LUA_GLOBALSINDEX);

		lua_unrefobj(L,m_iRefOpenScript);
	}

	int m_iRefOpenScript;
};

static CMainLibrary g_MainLibrary;*/
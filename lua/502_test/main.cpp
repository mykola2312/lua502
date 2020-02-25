#include <Windows.h>
#include <stdio.h>
#include "lauxlib.h"
#include "lua502.h"
#include "appframework/iappsystemgroup.h"
#include "tier1/strtools.h"
#include "tier1/interface.h"
#include "tier1/utlvector.h"
#include "tier1/utldict.h"

int Init(lua_State*);
int Exit(lua_State*);
DECLARE_GMOD9_PLUGIN2(Init,Exit,"testmodule");

/*class CAppSystemGroup
{
public:
	virtual int LoadModule( const char *pDLLName ) = 0;
	virtual IAppSystem *AddSystem( int module, const char *pInterfaceName ) = 0;
	virtual void *FindSystem( const char *pSystemName ) = 0;
	virtual CreateInterfaceFn GetFactory() = 0;

	CUtlVector<CSysModule*> m_Modules;
	CUtlVector<IAppSystem*> m_Systems;
	CUtlDict<int, unsigned short> m_SystemDict;
};*/

int lua_MessageBox(lua_State* L)
{
	luaL_checktype(L,1,LUA_TSTRING);
	luaL_checktype(L,2,LUA_TSTRING);
	luaL_checktype(L,3,LUA_TNUMBER);

	lua_pushnumber(L,MessageBox(NULL,lua_tostring(L,1),
		lua_tostring(L,2),(UINT)lua_tonumber(L,3)));
	return 1;
}

int lua_strcmp(lua_State* L)
{
	luaL_checktype(L,1,LUA_TSTRING);
	luaL_checktype(L,2,LUA_TSTRING);

	lua_pushnumber(L,V_strcmp(lua_tostring(L,1),
		lua_tostring(L,2)));
	return 1;
}

int lua_EngineError(lua_State* L)
{
	luaL_checktype(L,1,LUA_TSTRING);
	Error("%s",lua_tostring(L,1));
	return 0;
}

typedef void* (*GetInterfaceFn_t)();
typedef struct interfacereg_s {
	GetInterfaceFn_t m_fnGetInterface;
	const char* m_pName;
	struct interfacereg_s* m_pNext;
} interfacereg_t;

void DumpCreateInterface(CreateInterfaceFn fnFactory)
{
	interfacereg_t* pReg = **(interfacereg_t***)((char*)fnFactory+0x05);
	while(pReg)
	{
		Msg("\t%s %p\n",pReg->m_pName,pReg->m_fnGetInterface());
		pReg = pReg->m_pNext;
	}
}

typedef CreateInterfaceFn (*GetFactoryFn)();

void DumpAppFactoryGroup(CAppSystemGroup* pAppSystemGroup,int tier = 0)
{
	unsigned short i = pAppSystemGroup->m_SystemDict.First();
	Msg("== TIER %d ==\npAppSystemGroup %p\n",tier,pAppSystemGroup);
	CUtlVector<int> LegalSystems;
	while(pAppSystemGroup->m_SystemDict.IsValidIndex(i))
	{
		const char* pName = pAppSystemGroup->m_SystemDict.GetElementName(i);
		int iAppID;
		LegalSystems.AddToTail((iAppID = pAppSystemGroup->m_SystemDict.Element(i)));

		IAppSystem* pAppSystem = pAppSystemGroup->m_Systems.Element(iAppID);
		Msg("\t%s %p\n",(pName?pName:"(null)"),pAppSystem);
		i = pAppSystemGroup->m_SystemDict.Next(i);
	}

	if(pAppSystemGroup->m_pParentAppSystem)
	{
		if(pAppSystemGroup->m_nErrorStage != CAppSystemGroup::NONE) return;
		DumpAppFactoryGroup(pAppSystemGroup->m_pParentAppSystem,tier+1);
	}
}

int Init(lua_State* L)
{
	lua_pushcfunction(L,lua_MessageBox);
	lua_setglobal(L,"MessageBox");

	lua_pushcfunction(L,lua_strcmp);
	lua_setglobal(L,"strcmp");

	lua_pushcfunction(L,lua_EngineError);
	lua_setglobal(L,"EngineError");

	g_pLua502->Print("Hello World from ILua502!\n");
	g_pLua502->Print("AppFactory %p GameFactory %p\n",
		g_PluginInfo.m_fnAppFactory,g_PluginInfo.m_fnGameFactory);

	CAppSystemGroup* pAppSystemGroup = **(CAppSystemGroup***)(
		(char*)g_PluginInfo.m_fnAppFactory+0x06);
	DumpAppFactoryGroup(pAppSystemGroup);
	return 0;
}

int Exit(lua_State* L)
{
	lua_pushnil(L);
	lua_setglobal(L,"MessageBox");

	lua_pushnil(L);
	lua_setglobal(L,"strcmp");

	lua_pushnil(L);
	lua_setglobal(L,"EngineError");
	return 0;
}
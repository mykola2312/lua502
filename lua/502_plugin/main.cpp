#include "plugin.h"

DECLARE_PLUGIN(CTestPlugin)
	virtual bool GlobalInit(ILua502*);
	virtual bool LuaInit(lua_State*);

	virtual void LevelInit(const char*);
	virtual void ClientPutInServer(edict_t*,const char*);
END_PLUGIN(CTestPlugin,"testplugin");

bool CTestPlugin::GlobalInit(ILua502* pLua502)
{
	BaseClass::GlobalInit(pLua502);
	Msg("CTestPlugin::GlobalInit!\n");
	return true;
}

bool CTestPlugin::LuaInit(lua_State* L)
{
	BaseClass::LuaInit(L);

	lua_pushnumber(L,1234);
	lua_setglobal(L,"magic");

	Msg("CTestPlugin::LuaInit!\n");
	return true;
}

void CTestPlugin::LevelInit(const char* pLevel)
{
	Msg("CTestPlugin::LevelInit %s\n",pLevel);
}

void CTestPlugin::ClientPutInServer(edict_t* pEdict,
	const char* pName)
{
	Msg("Player %s (%) put in server!\n",pEdict,pName);
}
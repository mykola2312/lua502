#include "plugin.h"

ILua502* g_pLua502 = NULL;

CBasePlugin::CBasePlugin()
{
}

bool CBasePlugin::Load(CreateInterfaceFn,CreateInterfaceFn)
{
	return true;
}

void CBasePlugin::Unload()
{
}

void CBasePlugin::Pause()
{
}

void CBasePlugin::UnPause()
{
}

const char* CBasePlugin::GetPluginDescription()
{
	return s_pPluginName;
}

void CBasePlugin::LevelInit(const char*)
{
}

void CBasePlugin::ServerActivate(edict_t*,int,int)
{
}

void CBasePlugin::GameFrame(bool)
{
}

void CBasePlugin::LevelShutdown()
{
}

void CBasePlugin::ClientActive(edict_t*)
{
}

void CBasePlugin::ClientDisconnect(edict_t*)
{
}

void CBasePlugin::ClientPutInServer(edict_t*,const char*)
{
}

void CBasePlugin::SetCommandClient(int iIndex)
{
	m_iCommandClientIndex = iIndex;
}

void CBasePlugin::ClientSettingsChanged(edict_t*)
{
}

PLUGIN_RESULT CBasePlugin::ClientConnect(bool*,edict_t*,
	const char*,const char*,char*,int)
{
	return PLUGIN_CONTINUE;
}

PLUGIN_RESULT CBasePlugin::ClientCommand(edict_t*)
{
	return PLUGIN_CONTINUE;
}

PLUGIN_RESULT CBasePlugin::NetworkIDValidated(const char*,
	const char*)
{
	return PLUGIN_CONTINUE;
}

bool CBasePlugin::GlobalInit(ILua502* pLua502)
{
	g_pLua502 = pLua502;
	return true;
}

bool CBasePlugin::LuaInit(lua_State*)
{
	return true;
}

void CBasePlugin::OnSWEPLuaCreate(lua_State*)
{
}
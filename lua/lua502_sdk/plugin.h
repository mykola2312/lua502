#ifndef __PLUGIN_H
#define __PLUGIN_H

#include "glua.h"
#include "networkvar.h"

class CBasePlugin : public ILuaPluginCallbacks
{
public:
	CBasePlugin();

	virtual bool Load(CreateInterfaceFn,CreateInterfaceFn);
	virtual void Unload();
	virtual void Pause();
	virtual void UnPause();
	virtual const char* GetPluginDescription();

	virtual void LevelInit(const char*);
	virtual void ServerActivate(edict_t*,int,int);
	virtual void GameFrame(bool);
	virtual void LevelShutdown();

	virtual void ClientActive(edict_t*);
	virtual void ClientDisconnect(edict_t*);
	virtual void ClientPutInServer(edict_t*,const char*);
	virtual void SetCommandClient(int);
	virtual void ClientSettingsChanged(edict_t*);
	
	virtual PLUGIN_RESULT ClientConnect(bool*,edict_t*,const char*,
		const char*,char*,int);
	virtual PLUGIN_RESULT ClientCommand(edict_t*);
	virtual PLUGIN_RESULT NetworkIDValidated(const char*,
		const char*);

	virtual bool GlobalInit(ILua502*);
	virtual bool LuaInit(lua_State*);
	virtual void OnSWEPLuaCreate(lua_State*);
private:
	int m_iCommandClientIndex;
	static const char* s_pPluginName;
};

#define DECLARE_PLUGIN(dllName)											\
	class dllName : public CBasePlugin									\
	{																	\
	public:																\
		DECLARE_CLASS(dllName,CBasePlugin);
#define END_PLUGIN(dllName,plugName)									\
	};																	\
	const char* CBasePlugin::s_pPluginName = plugName;					\
	static dllName s_##dllName;											\
	EXPOSE_SINGLE_INTERFACE_GLOBALVAR(dllName,IServerPluginCallbacks,	\
		INTERFACEVERSION_ISERVERPLUGINCALLBACKS,s_##dllName)

extern ILua502* g_pLua502;

#endif
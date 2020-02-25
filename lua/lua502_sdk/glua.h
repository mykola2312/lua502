#ifndef __GLUA_H
#define __GLUA_H

#include "engine/iserverplugin.h"
extern "C"
{
	#include "lua.h"
	#include "lualib.h"
}

class ILua502;
class ILuaPluginCallbacks : public IServerPluginCallbacks
{
public:
	virtual bool GlobalInit(ILua502*) = 0;
	virtual bool LuaInit(lua_State*) = 0;
	virtual void OnSWEPLuaCreate(lua_State*) = 0;
};

class CLuaInterface
{
public:
	lua_State* m_pL;
};

typedef enum {
	LUAFILE_OK = 0,
	LUAFILE_MEM_FAULT,
	LUAFILE_LUA_ERROR,
	LUAFILE_DUMP_FAULT,
	LUAFILE_LFC_FAULT,
} luafile_err_t;

typedef struct {
	char m_szName[128];
	bool m_bDisabled;
	ILuaPluginCallbacks* m_pCallbacks;
	int m_iVersion;
	CSysModule* m_pModule;
} luaplugin_t;

class ILua502Callbacks
{
public:
	virtual int OnLuaPanic(lua_State* L) = 0;
	virtual int OnLuaError(lua_State* L) = 0;

	virtual void OnPluginLoad(luaplugin_t* pPlug) = 0;
	virtual void OnPluginUnload(luaplugin_t* pPlug) = 0;
	
	virtual void OnLuaCreate(lua_State* L) = 0;
	virtual void OnSWEPLuaCreate(CLuaInterface*) = 0;
};

class ILua502
{
public:
	virtual bool LoadPlugin(const char* pName,int* pPlugin) = 0;
	virtual void UnloadPlugin(int iPlugin) = 0;

	virtual luaplugin_t* GetPlugin(int iPlugin) = 0;
	virtual const char* RunString(const char* pCode,size_t len,const char* pName) = 0;

	virtual CLuaInterface* GetLuaInterface() = 0;
	virtual void Print(const char* fmt,...) = 0;

	virtual luafile_err_t RunFile(const char* pFile,const char** ppErr = NULL,
		lua_State* pL = NULL) = 0;

	virtual void SetLuaCallbacks(ILua502Callbacks* pCallbacks) = 0;
	virtual ILua502Callbacks* GetLuaCallbacks() = 0;

	virtual void Lock() = 0;
	virtual void Unlock() = 0;
};
#define INTERFACEVERSION_LUA502 "LUA502_001"

LUA_API int lua502_atpanic(lua_State* L);
LUA_API int lua502_aterror(lua_State* L);

#endif
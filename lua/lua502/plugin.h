#ifndef __PLUGIN_H
#define __PLUGIN_H

#include "glua.h"
#include "luafile.h"
#include "chook.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#include "tier0/threadtools.h"
#include "tier1/utlvector.h"
#include "tier1/utlmap.h"
#include "eiface.h"

#define RUNFILE_SIG "\x81\xEC\x00\x00\x00\x00\x55\x8B\xAC\x24\x00\x00\x00\x00\x8D\x54\x24\x08\x56\x89\x4C\x24\x08\x8B\xC5\x2B\xD5\xEB\x03\x8D\x49\x00\x8A\x08"
#define RUNFILE_MASK "xx????xxxx????xxxxxxxxxxxxxxxxxxxx"
#define RUNFILE_HOOKSIZE 6

#define PLUGINLOAD_SIG "\x81\xEC\x00\x00\x00\x00\x53\x8B\x9C\x24\x00\x00\x00\x00\x56\x68\x00\x00\x00\x00\x8D\x44\x24\x0C"
#define PLUGINLOAD_MASK "xx????xxxx????xx????xxxx"

#define PLUGINUNLOAD_SIG "\x56\x8B\xF1\x8B\x8E\x00\x00\x00\x00\x57\x33\xFF\x3B\xCF\x74\x11\x8B\x01\xFF\x50\x00\x89\xBE\x00\x00\x00\x00\x89\xBE\x00\x00\x00\x00\x8B\x86\x00\x00\x00\x00\x3B\xC7\x89\xBE\x00\x00\x00\x00\x74\x0C\x8B\x0D\x00\x00\x00\x00\x8B\x11\x50\xFF\x52\x00\xF6\x44\x24\x0C\x01\x89\xBE\x00\x00\x00\x00\x74\x09"
#define PLUGINUNLOAD_MASK "xxxxx????xxxxxxxxxxx?xx????xx????xx????xxxx????xxxx????xxxxx?xxxxxxx????xx"

#define SETSCRIPT_SIG "\x81\xEC\x00\x00\x00\x00\x57\x8B\xF9\x8B\x87\x00\x00\x00\x00\x85\xC0\x74\x25"
#define SETSCRIPT_MASK "xx????xxxxx????xxxx"
#define SETSCRIPT_HOOKSIZE 6

#define SWEPLUAOFFSET 0x6E4

typedef bool (__thiscall* FnRunFile)(const char*);
typedef int (*FnCreateGlobalLua)(void);
typedef int (__thiscall* FnSetScript)(void*,const char*);

typedef bool (__thiscall* FnPluginLoad)(void*,const char*);
typedef void (__thiscall* FnPluginUnload)(void*);

typedef struct {
	void** m_pVTable;
	CUtlVector<luaplugin_t*> m_pPlugins;
} pluginhelpers_t;

class CLua502Plugin : public IServerPluginCallbacks, public ILua502, public ILua502Callbacks
{
public:
	CLua502Plugin();

	virtual bool  Load( CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory  );
	virtual void  Unload( void );
	virtual void  Pause( void );
	virtual void  UnPause( void );  
	virtual const char* GetPluginDescription( void );      
	virtual void  LevelInit( const char* pMapName );
	virtual void  ServerActivate( edict_t *pEdictList, int edictCount, int clientMax );
	virtual void  GameFrame( bool simulating );
	virtual void  LevelShutdown( void );
	virtual void  ClientActive( edict_t *pEntity );
	virtual void  ClientDisconnect( edict_t *pEntity );
	virtual void  ClientPutInServer( edict_t *pEntity, char const *playername );
	virtual void  SetCommandClient( int index );
	virtual void  ClientSettingsChanged( edict_t *pEdict );
	virtual PLUGIN_RESULT ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen );
	virtual PLUGIN_RESULT ClientCommand( edict_t *pEntity/*, void* args */); //--From version 003
	virtual PLUGIN_RESULT NetworkIDValidated( const char *pszUserName, const char *pszNetworkID );

	virtual void Load2();
	virtual void LoadPlugins();

	/*
	--From version 003
	virtual void  OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue );
	virtual void  OnEdictAllocated( edict_t *edict );
	virtual void  OnEdictFreed( edict_t *edict  ); */

	//ILua502
	virtual bool LoadPlugin(const char* pName,
		int* pPlugin);
	virtual void UnloadPlugin(int iPlugin);
	virtual const char* RunString(const char* pCode,size_t len,const char* pName);
	const char* RunStringEx(lua_State* _L,const char* pCode,size_t len,const char* pName);
	virtual luaplugin_t* GetPlugin(int iPlugin);

	virtual CLuaInterface* GetLuaInterface();
	virtual void Print(const char* pFmt,...);

	virtual luafile_err_t RunFile(const char* pFile,const char** ppErr = NULL,
		lua_State* pL = NULL);

	virtual void SetLuaCallbacks(ILua502Callbacks* pCallbacks);
	virtual ILua502Callbacks* GetLuaCallbacks();

	virtual void Lock();
	virtual void Unlock();

	void LoadConfigs();
	void LoadScripts();

	//ILua502Callbacks

	virtual int OnLuaPanic(lua_State* L);
	virtual int OnLuaError(lua_State* L);

	virtual void OnPluginLoad(luaplugin_t* pPlug);
	virtual void OnPluginUnload(luaplugin_t* pPlug);

	virtual void OnLuaCreate(CLuaInterface*);
	virtual void OnSWEPLuaCreate(CLuaInterface*);

	static bool IsPathSecure(const char* pPath);

	CLuaInterface* m_pLuaInterface;
	ILua502Callbacks* m_pCallbacks;
	CUtlMap<int,luaplugin_t*> m_Plugins;

	lua_CFunction m_fnOldPanic;
	CHook* m_pRunFileHook;
	CThreadMutex m_LuaMutex;

	FnSetScript m_fnSetScript;
	FnCreateGlobalLua m_fnCreateGlobalLua;
	CHook* m_pCreateGlobalLua;
	CHook* m_pSetScript;

	CreateInterfaceFn m_fnAppFactory;
	CreateInterfaceFn m_fnGameFactory;

	//Settings
	bool m_bLuaUseCache;

	static FnPluginLoad s_fnPluginLoad;
	static FnPluginUnload s_fnPluginUnload;
};

/*
unsigned long hash(unsigned char* s)
{
	unsigned long h = 5381;
	unsigned char c;
	while((c=*s++))
		h+=(h<<5)+c;
	return h;
}
*/

extern CLua502Plugin g_Lua502Plugin;
extern ILua502* g_pLua502;

extern IVEngineServer* engine;
extern IFileSystem* filesystem;

LUA_API int lua502_atpanic(lua_State* L);
LUA_API int lua502_aterror(lua_State* L);

#endif
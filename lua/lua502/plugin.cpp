#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "plugin.h"
#include "sigscan.h"
#include "svar.h"
#include "luafile.h"
#include "baselib.h"
#include "eiface.h"
#include "tier1/interface.h"
#include "tier0/dbg.h"
#include "tier0/memdbgon.h"

CLua502Plugin g_Lua502Plugin;
ILua502* g_pLua502 = (ILua502*)&g_Lua502Plugin;
IFileSystem* filesystem = NULL;
IVEngineServer* engine = NULL;
pluginhelpers_t* pluginhelpers = NULL;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CLua502Plugin,IServerPluginCallbacks,
	INTERFACEVERSION_ISERVERPLUGINCALLBACKS,g_Lua502Plugin);
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CLua502Plugin,ILua502,INTERFACEVERSION_LUA502,g_Lua502Plugin);

bool __fastcall RunFileHook(CLuaInterface*,void*,const char*);
int __fastcall SetScriptHook(void*,void*,const char*);
int CreateGlobalLuaHook();
static FnRunFile s_fnRunFile;
static CLuaInterface** s_ppLuaInterface;
static DWORD s_dwCreateGLUA;

FnPluginLoad CLua502Plugin::s_fnPluginLoad = NULL;
FnPluginUnload CLua502Plugin::s_fnPluginUnload = NULL;

CLua502Plugin::CLua502Plugin() : m_Plugins(DefLessFunc(int))
{
	m_pLuaInterface = NULL;
	m_pRunFileHook = NULL;
}

bool CLua502Plugin::Load(CreateInterfaceFn InterFn,CreateInterfaceFn GameFn)
{
	DWORD dwSetScript;
	/*CSysModule* sysFileSystemSteam = NULL;
	if(!Sys_LoadInterface("filesystem_steam",FILESYSTEM_INTERFACE_VERSION,
		&sysFileSystemSteam,(void**)&filesystem))
	{
		Warning("filesystem not found!\n");
		return false;
	}*/
	m_fnAppFactory = InterFn;
	m_fnGameFactory = GameFn;

	ConnectTier1Libraries(&m_fnAppFactory,1);

	if(!(filesystem = (IFileSystem*)m_fnAppFactory(FILESYSTEM_INTERFACE_VERSION,0)))
	{
		Print(FILESYSTEM_INTERFACE_VERSION " not found!\n");
		Unlock();
		return false;
	}

	if(!(engine = (IVEngineServer*)m_fnAppFactory(INTERFACEVERSION_VENGINESERVER,0)))
	{
		Print("IVEngineServer not found!\n");
		Unlock();
		return false;
	}

	if(!(pluginhelpers = (pluginhelpers_t*)m_fnAppFactory(
		INTERFACEVERSION_ISERVERPLUGINHELPERS,0)))
	{
		Print("IServerPluginHelpers not found!\n");
		Unlock();
		return false;
	}

	/*DECLARE_SIGSCAN(m_pLuaInterface,"server.dll",GETLUAINTERFACE_SIG,GETLUAINTERFACE_MASK,
		GETLUAINTERFACE_OFFSET,SigScan::CSigScan::SIG_VARIABLE);*/
	/*DECLARE_SIGSCAN(s_ppLuaInterface,"server.dll",CREATEGLUA_SIG,CREATEGLUA_MASK,
		CREATEGLUA_OFFSET,SigScan::CSigScan::SIG_PTR);*/
	DECLARE_SIGSCAN(s_dwCreateGLUA,"server.dll",CREATEGLUA_SIG,CREATEGLUA_MASK,
		0,SigScan::CSigScan::SIG_FUNCTION);
	DECLARE_SIGSCAN(s_fnRunFile,"server.dll",RUNFILE_SIG,RUNFILE_MASK,0,
		SigScan::CSigScan::SIG_FUNCTION);
	DECLARE_SIGSCAN(dwSetScript,"server.dll",SETSCRIPT_SIG,SETSCRIPT_MASK,0,
		SigScan::CSigScan::SIG_FUNCTION);
	DECLARE_SIGSCAN(s_fnPluginLoad,"engine.dll",PLUGINLOAD_SIG,
		PLUGINLOAD_MASK,0,SigScan::CSigScan::SIG_FUNCTION);
	DECLARE_SIGSCAN(s_fnPluginUnload,"engine.dll",PLUGINUNLOAD_SIG,
		PLUGINUNLOAD_MASK,0,SigScan::CSigScan::SIG_FUNCTION);
	SigScan::Scan();

	s_ppLuaInterface = *(CLuaInterface***)((char*)s_dwCreateGLUA+CREATEGLUA_OFFSET);

	m_pCreateGlobalLua = new CHook(s_dwCreateGLUA,CREATEGLUA_HOOKSIZE);
	m_fnCreateGlobalLua = (FnCreateGlobalLua)
		m_pCreateGlobalLua->HookFunction((DWORD)CreateGlobalLuaHook);
	m_pSetScript = new CHook(dwSetScript,SETSCRIPT_HOOKSIZE);
	m_fnSetScript = (FnSetScript)m_pSetScript
		->HookFunction((DWORD)SetScriptHook);

	ConVar_Register();
	LoadConfigs();
	LoadPlugins();
	return true;
}

void CLua502Plugin::Load2()
{
	m_pLuaInterface = *s_ppLuaInterface;
	Lock();
	SetLuaCallbacks(this);
	//CLuaFile::ClearCache();
	if(!CLuaFile::Init())
	{
		Warning("Lua cache initialization failed!\n");
		Unlock();
		return;
	}
	m_fnOldPanic = lua_atpanic(m_pLuaInterface->m_pL,lua502_atpanic);

	int iNewTop,iTop = lua_gettop(m_pLuaInterface->m_pL);
	CLuaLibrary::Init(m_pLuaInterface->m_pL);
	iNewTop = lua_gettop(m_pLuaInterface->m_pL);

	Print("iTop %d iNewTop %d\n",iTop,iNewTop);
	//MessageBox(NULL,m_pLuaInterface ? "m_pLuaInterface OK" : "m_pLuaInterface FAIL","Lua502",MB_OK);
	Print("Loaded\nm_pLuaInterface %p lua_State %p\n",
		m_pLuaInterface,m_pLuaInterface->m_pL);

	CLuaLibrary::Dump();

	if(!m_pRunFileHook)
	{
		m_pRunFileHook = new CHook((DWORD)s_fnRunFile,RUNFILE_HOOKSIZE);
		m_pRunFileHook->HookFunction((DWORD)RunFileHook);
	}

	Unlock();
}

void CLua502Plugin::LoadConfigs()
{
	KeyValues* pKV = new KeyValues("Settings");
	if(!filesystem->FileExists("cfg/lua502.cfg","MOD"))
	{
		m_bLuaUseCache = true;

		pKV->SetInt("LuaUseCache",m_bLuaUseCache);
		pKV->SaveToFile(filesystem,"cfg/lua502.cfg","MOD");
	}
	else
	{
		pKV->LoadFromFile(filesystem,"cfg/lua502.cfg","MOD");
		m_bLuaUseCache = !!pKV->GetInt("LuaUseCache");
	}
}

void CLua502Plugin::LoadPlugins()
{
	FileFindHandle_t fFind;
	const char* pPath = NULL;
	char szBuff[MAX_PATH];

	if((pPath=filesystem->FindFirstEx("lua/bin/*.dll","MOD",&fFind)))
	{
		do {
			sprintf(szBuff,"gmod9/lua/bin/%s",pPath);
			Print("Plugin %s\n",pPath);
			LoadPlugin(szBuff,NULL);
		} while((pPath=filesystem->FindNext(fFind)));
		filesystem->FindClose(fFind);
	}
}

void CLua502Plugin::LoadScripts()
{
	FileFindHandle_t fFind;
	const char* pFile,*pErr;
	char szFile[MAX_PATH];
	if((pFile = filesystem->FindFirstEx("lua/lua502/*.lua","MOD",&fFind)))
	{
		do {
			sprintf(szFile,"lua/lua502/%s",pFile);
			if(RunFile(szFile,&pErr))
				Warning("Failed run scripts %s: %s\n",szFile,pErr?pErr:"no-error");
		} while((pFile = filesystem->FindNext(fFind)));
		filesystem->FindClose(fFind);
	}
}

void CLua502Plugin::Unload()
{
	CLuaLibrary::Exit(m_pLuaInterface->m_pL);

	for(size_t i = 0; i < m_Plugins.Count(); i++)
		UnloadPlugin(i);
	lua_atpanic(m_pLuaInterface->m_pL,m_fnOldPanic);
	delete m_pRunFileHook;
	delete m_pCreateGlobalLua;
	delete m_pSetScript;
}

void CLua502Plugin::Pause()
{
}

void CLua502Plugin::UnPause()
{
}

const char* CLua502Plugin::GetPluginDescription()
{
	return "GLua9 Interface";
}

void CLua502Plugin::LevelInit(const char*)
{
}

void CLua502Plugin::ServerActivate(edict_t*,int,int)
{
}

void CLua502Plugin::GameFrame(bool)
{
}

void CLua502Plugin::LevelShutdown()
{
}

void CLua502Plugin::ClientActive(edict_t*)
{
}

void CLua502Plugin::ClientDisconnect(edict_t*)
{
}

void CLua502Plugin::ClientPutInServer(edict_t*,const char*)
{
}

void CLua502Plugin::SetCommandClient(int)
{
}

void CLua502Plugin::ClientSettingsChanged(edict_t*)
{
}

PLUGIN_RESULT CLua502Plugin::ClientConnect(bool*,edict_t*,const char*,const char*, char*,int)
{
	return PLUGIN_CONTINUE;
}

PLUGIN_RESULT CLua502Plugin::ClientCommand(edict_t* /*,void**/)
{
	return PLUGIN_CONTINUE;
}

PLUGIN_RESULT CLua502Plugin::NetworkIDValidated(const char*,const char*)
{
	return PLUGIN_CONTINUE;
}

/*void CLua502Plugin::OnQueryCvarValueFinished(QueryCvarCookie_t,edict_t*,EQueryCvarValueStatus,const char*,const char*)
{
}

void CLua502Plugin::OnEdictAllocated(edict_t*)
{
}

void CLua502Plugin::OnEdictFreed(edict_t*)
{
}*/

bool CLua502Plugin::LoadPlugin(const char* pName,
		int* pPlugin)
{
	luaplugin_t* pPlug = new luaplugin_t;
	V_memset(pPlug,'\0',sizeof(luaplugin_t));
	if(!IsPathSecure(pName))
	{
		delete pPlug;
		return false;
	}
	if(!s_fnPluginLoad(pPlug,pName))
	{
		delete pPlug;
		return false;
	}
	if(!pPlug->m_pCallbacks)
	{
		Print("%s wrong callbacks\n",pName);
		delete pPlug;
		return false;
	}
	if(!pPlug->m_pCallbacks->GlobalInit(this))
	{
		Print("%s GlobalInit failed!\n",
			pPlug->m_szName);
		s_fnPluginUnload(this);
		delete pPlug;
		return false;
	}

	int iIndex = pluginhelpers->m_pPlugins.AddToTail(pPlug);
	int iID = m_Plugins.Insert(iIndex,pPlug); //Crash
	if(pPlugin) *pPlugin = iID;
	OnPluginLoad(pPlug);
	return true;
}

void CLua502Plugin::UnloadPlugin(int iPlugin)
{
	luaplugin_t* pPlug;
	if(!(pPlug = GetPlugin(iPlugin)))
		return;
	OnPluginUnload(pPlug);
	s_fnPluginUnload(pPlug);

	int iID = pluginhelpers->m_pPlugins.Find(pPlug);
	if(pluginhelpers->m_pPlugins.IsValidIndex(iID))
		pluginhelpers->m_pPlugins.Remove(iID);
	
	m_Plugins.Remove(iPlugin);
	delete pPlug;
}

luaplugin_t* CLua502Plugin::GetPlugin(int iPlugin)
{
	if(m_Plugins.IsValidIndex(iPlugin))
		return m_Plugins[iPlugin];
	return NULL;
}

const char* CLua502Plugin::RunString(const char* pCode,size_t len,const char* pName)
{
	const char* pRet = NULL;
	lua_State* L = m_pLuaInterface->m_pL;
	Lock();

	lua_pushcfunction(L,lua502_aterror);
	if(luaL_loadbuffer(L,pCode,len,pName))
		pRet = lua_tostring(L,-1);
	else
		lua_pcall(L,0,LUA_MULTRET,lua_gettop(L)-1);

	lua_pop(L,1);
	Unlock();
	return pRet;
}

const char* CLua502Plugin::RunStringEx(lua_State* _L,const char* pCode,size_t len,const char* pName)
{
	const char* pRet = NULL;
	Lock();

	lua_pushcfunction(_L,lua502_aterror);
	if(luaL_loadbuffer(_L,pCode,len,pName))
	{
		pRet = lua_tostring(_L,-1);
		lua_pop(_L,1);
	}
	else
		lua_pcall(_L,0,LUA_MULTRET,lua_gettop(_L)-1);

	lua_pop(_L,1);
	Unlock();
	return pRet;
}

CLuaInterface* CLua502Plugin::GetLuaInterface()
{
	return m_pLuaInterface;
}

void CLua502Plugin::Print(const char* pFmt,...)
{
	char szBuf[256] = {0};
	char szFormat[256] = {0};
	va_list ap;

	Lock();

	va_start(ap,pFmt);
	V_vsnprintf(szBuf,256,pFmt,ap);
	va_end(ap);
	sprintf(szFormat,"[Lua502] %s",szBuf);

	Msg(szFormat);
	if(engine) engine->LogPrint(szFormat);
	Unlock();
}

//lua/file.lua
luafile_err_t CLua502Plugin::RunFile(const char* pFile,const char** ppErr,
		lua_State* pL)
{
	luafile_err_t iErr;
	const char* pErr = NULL;

	if(m_bLuaUseCache)
	{
		Lock();
		CLuaFile luafile((pL?pL:m_pLuaInterface->m_pL),pFile);
		if((iErr = luafile.Load(&pErr)))
		{
			Warning("CLuaFile::Load error %s\n",g_pLuaFileErrors[iErr]);
			if(iErr == LUAFILE_LUA_ERROR)
				Warning("%s\n",pErr?pErr:"error-error");
		} else luafile.Execute();
		if(ppErr) *ppErr = pErr;
		Unlock();
	}
	else
	{
		Lock();
		FileHandle_t fLua = filesystem->Open(pFile,"rb","MOD");
		if(!fLua)
		{
			Warning("File not found!\n");
			*ppErr = "File not found!";
			Unlock();
			return LUAFILE_LFC_FAULT;
		}
		uint32 uSize = filesystem->Size(fLua);
		char* pBuf = new char[uSize];
		filesystem->Read(pBuf,uSize,fLua);
		filesystem->Close(fLua);
		if((pErr=RunStringEx((pL?pL:m_pLuaInterface->m_pL),pBuf,uSize,"CLuaFile")))
		{
			Warning("%s\n",pErr?pErr:"error-error");
			if(ppErr) *ppErr = pErr;
			delete[] pBuf;
			Unlock();
			return LUAFILE_LUA_ERROR;
		}
		delete[] pBuf;
		Unlock();
		return LUAFILE_OK;
	}
	return iErr;
}

void CLua502Plugin::SetLuaCallbacks(ILua502Callbacks* pCallbacks)
{
	Lock();
	m_pCallbacks = pCallbacks;
	Unlock();
}

ILua502Callbacks* CLua502Plugin::GetLuaCallbacks()
{
	return m_pCallbacks;
}

void CLua502Plugin::Lock()
{
	m_LuaMutex.Lock();
}

void CLua502Plugin::Unlock()
{
	m_LuaMutex.Unlock();
}

//ILua502Callbacks

int CLua502Plugin::OnLuaPanic(lua_State* L)
{
	const char* pStr = lua_tostring(L,1);
	Error("Lua Panic!\n%s",pStr?pStr:"(null)");
	return 0;
}

int CLua502Plugin::OnLuaError(lua_State* L)
{
	const char* pStr = lua_tostring(L,1);
	Warning("%s\n",pStr?pStr:"(null)\n");
	return 0;
}

void CLua502Plugin::OnPluginLoad(luaplugin_t* pPlug)
{
	Msg("Plugin %s (%p) loaded\n",pPlug->m_szName,
		pPlug);
}

void CLua502Plugin::OnPluginUnload(luaplugin_t* pPlug)
{
	Msg("Plugin %s (%p) unloaded\n",pPlug->m_szName,
		pPlug);
}

void CLua502Plugin::OnLuaCreate(CLuaInterface* pLua)
{
	for(int i = m_Plugins.FirstInorder();
		m_Plugins.IsValidIndex(i); i = m_Plugins.NextInorder(i))
		m_Plugins[i]->m_pCallbacks->LuaInit(pLua->m_pL);
	LoadScripts();
}

void CLua502Plugin::OnSWEPLuaCreate(CLuaInterface* pLua)
{
	for(int i = m_Plugins.FirstInorder();
		m_Plugins.IsValidIndex(i); i = m_Plugins.NextInorder(i))
		m_Plugins[i]->m_pCallbacks->OnSWEPLuaCreate(pLua->m_pL);
}

bool CLua502Plugin::IsPathSecure(const char* pPath)
{
	int iLen;
	char szBannedChars[] = {'\t','\r','\t'};

	if(!(iLen = V_strlen(pPath))) return false;
	if(V_strstr(pPath,"..") || V_strstr(pPath,":")) return false;
	if(*(unsigned short*)pPath == 0x5C5C) return false;
	for(int i = 0; i < iLen; i++)
		if(pPath[i] && pPath[i] <= 0x1F) return false; //Ditch up control chars
	return true;
}

LUA_API int lua502_atpanic(lua_State* L)
{
	return g_Lua502Plugin.OnLuaPanic(L);
}

LUA_API int lua502_aterror(lua_State* L)
{
	return g_Lua502Plugin.OnLuaError(L);
}

bool __fastcall RunFileHook(CLuaInterface* pInterface,void* edx,
	const char* pName)
{
	const char* pErr = NULL;
	char szBuf[MAX_PATH] = {0};
	sprintf(szBuf,"lua/%s",pName);
	return (g_pLua502->RunFile(szBuf,&pErr,
		pInterface->m_pL) == 0);
}

int __fastcall SetScriptHook(void* thisptr,void*,const char* pFile)
{
	int iRet = g_Lua502Plugin.m_fnSetScript(thisptr,pFile);
	CLuaInterface* pLua = *(CLuaInterface**)((char*)thisptr+SWEPLUAOFFSET);
	if(pLua) g_Lua502Plugin.OnSWEPLuaCreate(pLua);
	return iRet;
}

int CreateGlobalLuaHook()
{
	bool bPrev = (*s_ppLuaInterface!=NULL);
	int iRet = g_Lua502Plugin.m_fnCreateGlobalLua();
	if(bPrev) g_Lua502Plugin.Load2();
	g_Lua502Plugin.OnLuaCreate(*s_ppLuaInterface);
	return iRet;
}
#include <Windows.h>
#include <stdlib.h>
#include "sigscan.h"
extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lstate.h"
}

#define GETLUAINTERFACE_SIG "\xA1\x00\x00\x00\x00\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x81\xEC\x00\x00\x00\x00"
#define GETLUAINTERFACE_MASK "x????xxxxxxxxxxxxx????"

class CLuaInterface
{
public:
	lua_State* m_pL;
};
CLuaInterface* g_pLuaInterface;
DECLARE_SIGSCAN(g_pLuaInterface,"server.dll",GETLUAINTERFACE_SIG,GETLUAINTERFACE_MASK,1,SigScan::CSigScan::SIG_VARIABLE);

DWORD WINAPI StartThread(LPVOID lpArg)
{
	SigScan::Scan();
	const char* pLua = "Msg(string.format(\"Hello World\\n\"))";

	lua_pushnumber(g_pLuaInterface->m_pL,sizeof(lua_State));
	lua_setglobal(g_pLuaInterface->m_pL,"lua_State__sizeof");

	if(luaL_loadbuffer(g_pLuaInterface->m_pL,pLua,strlen(pLua),"gm9_internal"))
	{
		const char* pErr = lua_tostring(g_pLuaInterface->m_pL,-1);
		lua_pop(g_pLuaInterface->m_pL,1);
		MessageBoxA(NULL,pErr,"Lua Error",MB_ICONHAND);
	}
	else lua_pcall(g_pLuaInterface->m_pL,0,0,0);

	return 0;
}

BOOL APIENTRY DllMain(HINSTANCE hDll,DWORD fdwReason,LPVOID lpArg)
{
	if(fdwReason==DLL_PROCESS_ATTACH) 
		CreateThread(0,0,StartThread,0,0,0);
	return TRUE;
}
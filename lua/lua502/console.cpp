#define GAME_DLL
#include "eiface.h"
#include "convar.h"
#include "glua.h"
#include "plugin.h"
#include "luafile.h"
#include "tier0/icommandline.h"

extern IVEngineServer* engine;
static ConVar lua502_luapath("lua502_luapath","MOD");

CON_COMMAND(lua502_plugin,"Lua502 Plugin Interfae")
{
	if(engine->Cmd_Argc() < 1) return;
	const char* pCmd = engine->Cmd_Argv(1);
	if(!V_stricmp(pCmd,"load"))
	{
		int iPlug;
		const char* pName = engine->Cmd_Argv(2);
		if(!g_pLua502->LoadPlugin(pName,&iPlug))
			g_pLua502->Print("Failed load plugin %s\n",
				pName);
		else 
			g_pLua502->Print("Plugin %s successfully loaded!\n",iPlug);
	}
	else if(!V_stricmp(pCmd,"unload"))
		g_pLua502->UnloadPlugin(V_atoi(
			engine->Cmd_Argv(2)));
	else if(!V_stricmp(pCmd,"list"))
	{
		luaplugin_t* pPlug = NULL;
		for(int i = 0; (pPlug = g_pLua502
			->GetPlugin(i++));)
			Msg("%d: %s (%d %p)\n",(i-1),
				pPlug->m_szName,pPlug->m_iVersion,
				pPlug->m_pCallbacks);
	}
}

CON_COMMAND(lua502_run,"Run lua")
{
	const char* pErr,*pCode = engine->Cmd_Args();
	int iLen = strlen(pCode);
	if(iLen<1) return;
	if((pErr=g_pLua502->RunString(pCode,iLen,"lua502_run")))
		Msg("%s\n",pErr);
}

CON_COMMAND(lua502_runfile,"Run lua file")
{
	if(engine->Cmd_Argc()<2) return;
	if(g_pLua502->RunFile(engine->Cmd_Args()))
		Warning("ILua502::RunFile failed!\n");
}

CON_COMMAND(lua502_cache,"Cache Interface")
{
	if(engine->Cmd_Argc() < 1) return;
	const char* pCmd = engine->Cmd_Argv(1);
	if(!V_stricmp(pCmd,"list"))
	{
		Msg("name size time\n");
		for(int i = 0; i < CLuaFile::s_Cache.Size(); i++)
		{
			CLuaFile::cachenode_t& cache = CLuaFile::s_Cache[i];
			Msg("%s %d %d\n",cache.m_szName,cache.m_uSize,
				cache.m_lTime);
		}
	}
	else if(!V_stricmp(pCmd,"forceupdate"))
		CLuaFile::UpdateCacheFile();
	else if(!V_stricmp(pCmd,"clear"))
		CLuaFile::ClearCache();
}

void DebugMsg(const char* pFmt,...)
{
	char szBuffer[128];
	va_list ap;

	va_start(ap,pFmt);
	V_vsnprintf(szBuffer,128,pFmt,ap);
	va_end(ap);

	OutputDebugString(szBuffer);
}

static void value2string(lua_State* L,char* pBuffer)
{
	const char* pValue;
	switch(lua_type(L,-1))
	{
	case LUA_TFUNCTION:
		sprintf(pBuffer,"function 0x%08X",lua_tocfunction(L,-1));
		break;
	case LUA_TBOOLEAN:
		sprintf(pBuffer,lua_toboolean(L,-1)?"true":"false");
		break;
	case LUA_TLIGHTUSERDATA:
		sprintf(pBuffer,"userdata 0x%08X",lua_touserdata(L,-1));
		break;
	default:
		if((pValue = lua_tostring(L,-1))) V_strcpy(pBuffer,pValue);
		else V_strcpy(pBuffer,"(null)");
		break;
	}
}

static void DumpTable(lua_State* L,FileHandle_t fLog,int iLevel,
	int iMaxLevel = 2)
{
	char szValue[128] = {0};
	const char* pKey;

	if(iLevel==iMaxLevel) return;
	if(!iLevel)
	{
		for(int i = 0; i < iLevel; i++)
			filesystem->FPrintf(fLog,"\t");
		filesystem->FPrintf(fLog,"(root)\n");
	}
	else
	{
		const char* pKey = lua_tostring(L,-2);
		filesystem->FPrintf(fLog,"|");
		for(int i = 0; i < iLevel; i++)
			filesystem->FPrintf(fLog,"\t");
		filesystem->FPrintf(fLog,"%s\n",pKey?pKey:"(null)");
	}

	lua_pushnil(L); //Nil key
	while(lua_next(L,-2)!=0)
	{
		//if(lua_tostring(L,-2)=="_G") continue;
		if(!(pKey = lua_tostring(L,-2))) pKey = "(null)";
		if(*pKey == '0')
		{
			lua_pop(L,1);
			Msg("Possibly error detected\n");
			return;
		}

		if(*(unsigned short*)pKey == 0x455F
			|| *(unsigned short*)pKey == 0x475F
			|| *(unsigned short*)pKey == 0x525F
			|| *(unsigned int*)pKey == 0x5F544341
			|| !V_strcmp(pKey,"g_EvenHooks")
			|| !V_strcmp(pKey,"gLuaThinkFunctions")
			|| *pKey == 'g')
		{
			lua_pop(L,1);
			continue;
		}

		if(lua_type(L,-1) == LUA_TTABLE)
		{
			DebugMsg("Recursive table %s\n",pKey);
			DumpTable(L,fLog,iLevel+1,iMaxLevel);
		}
		else
		{
			value2string(L,szValue);
			for(int i = 0; i < (iLevel); i++)
				filesystem->FPrintf(fLog,"\t");
			filesystem->FPrintf(fLog,"|\t");
			DebugMsg("[%s] = %s\n",pKey,szValue);
			filesystem->FPrintf(fLog,"[%s] = %s\n",
				pKey,szValue);
		}
		lua_pop(L,1); //Pop value
	}
}

CON_COMMAND(lua502_gdump,"Dump _G table, levels specified")
{
	FileHandle_t fLog = filesystem->Open("gdump.log","wb","MOD");
	int iMaxLevel = atoi(engine->Cmd_Argv(1));
	if(!iMaxLevel) iMaxLevel = 2;
	lua_State* L = g_pLua502->GetLuaInterface()->m_pL;

	lua_pushvalue(L,LUA_GLOBALSINDEX);
	DumpTable(L,fLog,0,iMaxLevel);
	lua_pop(L,1);
	filesystem->Close(fLog);
}

CON_COMMAND(lua502_kdump,"Dump KeyValues")
{
	KeyValues* kv = new KeyValues("kdump");
	kv->LoadFromFile(filesystem,"kval.txt","MOD");
	KeyValues* pIter = kv->GetFirstSubKey();
	if(!pIter) return;
	do {
		Msg("%s %s\n",pIter->GetName(),pIter->GetString());
	} while((pIter=pIter->GetNextKey()));
	kv->deleteThis();
}

CON_COMMAND(lua502_ndump,"Make KeyValue dump")
{
	KeyValues* kv = new KeyValues("root");

	KeyValues* pSub1 = new KeyValues("group1");
	KeyValues* pSub2 = new KeyValues("group2");

	KeyValues* pKey1 = pSub1->CreateNewKey();
	KeyValues* pKey2 = pSub2->CreateNewKey();

	pKey1->SetName("Name1");
	pKey1->SetStringValue("Value1");

	pKey2->SetName("Name1");
	pKey2->SetStringValue("Value1");

	kv->AddSubKey(pSub1);
	kv->AddSubKey(pSub2);

	kv->SaveToFile(filesystem,"ndump.txt","MOD");
	kv->deleteThis();
}

CON_COMMAND(crashme,"")
{
	PDWORD pdwVal = NULL;
	*pdwVal = 0xDEADBEEF;
}

CON_COMMAND(whileloop,"")
{
	while(1)
	{
		__asm nop;
	}
}
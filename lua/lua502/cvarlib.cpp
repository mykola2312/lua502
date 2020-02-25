#include "tier1/convar.h"
#include "icvar.h"
#include "types.h"
#include "plugin.h"
#include "baselib.h"
#include "cvarlib.h"

DECLARE_LIBRARY("cvar");
DECLARE_TABLE_DUMMY(concommandbase);

DECLARE_FUNCTION(concommandbase,__tostring)
{
	cmdbase_ud_t* ud = (cmdbase_ud_t*)luaL_multicheckuserdata(L,1,
		2,CONVAR,COMMAND);
	if(!ud) return 0;
	luaL_pushfstring(L,"%s %s (%p)",ud->m_pBase->IsCommand()?"ConCommand":"ConVar",
		ud->m_pBase->GetName(),ud->m_pBase);
	return 1;
}

DECLARE_FUNCTION(concommandbase,__gc)
{
	cmdbase_ud_t* ud = (cmdbase_ud_t*)luaL_multicheckuserdata(L,1,
		2,CONVAR,COMMAND);
	if(!ud) return 0;
	if(ud->m_bCustom)
	{
		if(ud->m_pBase->IsCommand())
			delete (CLuaConCommand*)ud->m_pCmd;
		else delete ud->m_pConVar;
	}
	return 0;
}

DECLARE_FUNCTION(concommandbase,GetName)
{
	//ConCommandBase** ppBase = (ConCommandBase**)luaL_multicheckuserdata(L,1,3,
		//CONVAR,COMMAND),*pBase;
	cmdbase_ud_t* ud = (cmdbase_ud_t*)luaL_multicheckuserdata(L,1,2,
		CONVAR,COMMAND);
	if(!ud) return 0;
	lua_pushstring(L,ud->m_pBase->GetName());
	return 1;
}

DECLARE_FUNCTION(concommandbase,GetHelpText)
{
	cmdbase_ud_t* ud = (cmdbase_ud_t*)luaL_multicheckuserdata(L,1,2,
		CONVAR,COMMAND);
	if(!ud) return 0;

	lua_pushstring(L,ud->m_pBase->GetHelpText());
	return 1;
}

DECLARE_FUNCTION(concommandbase,GetNext)
{
	cmdbase_ud_t* ud = (cmdbase_ud_t*)luaL_multicheckuserdata(L,1,2,
		CONVAR,COMMAND);
	ConCommandBase* pNext;
	if(!ud) return 0;

	if((pNext = (ConCommandBase*)ud->m_pBase->GetNext()))
		luaf_makecmdbase(L,ud->m_pBase);
	else lua_pushnil(L);
	return 1;
}

DECLARE_FUNCTION(concommandbase,IsCommand)
{
	cmdbase_ud_t* ud = (cmdbase_ud_t*)luaL_multicheckuserdata(L,1,2,
		CONVAR,COMMAND);
	if(!ud) return 0;

	lua_pushboolean(L,ud->m_pBase->IsCommand());
	return 1;
}

DECLARE_FUNCTION(concommandbase,IsRegistered)
{
	cmdbase_ud_t* ud = (cmdbase_ud_t*)luaL_multicheckuserdata(L,1,2,
		CONVAR,COMMAND);
	if(!ud) return 0;

	lua_pushboolean(L,ud->m_pBase->IsRegistered());
	return 1;
}

DECLARE_FUNCTION(concommandbase,IsBitSet)
{
	cmdbase_ud_t* ud = (cmdbase_ud_t*)luaL_multicheckuserdata(L,1,2,
		CONVAR,COMMAND);
	if(!ud) return 0;

	lua_pushboolean(L,ud->m_pBase->IsBitSet(lua_tonumber(L,2)));
	return 1;
}

DECLARE_FUNCTION(concommandbase,AddFlags)
{
	cmdbase_ud_t* ud = (cmdbase_ud_t*)luaL_multicheckuserdata(L,1,2,
		CONVAR,COMMAND);
	if(!ud) return 0;

	ud->m_pBase->AddFlags(lua_tonumber(L,2));
	return 0;
}

DECLARE_METATABLE_INHERITOR(convar,CONVAR,concommandbase);

DECLARE_FUNCTION(convar,GetValue)
{
	cmdbase_ud_t* ud = (cmdbase_ud_t*)luaf_checkuserdata(L,1,&g_LuaTable_convar);
	if(!ud) return 0;

	lua_pushstring(L,ud->m_pConVar->GetString());
	return 1;
}

DECLARE_FUNCTION(convar,SetValue)
{
	cmdbase_ud_t* ud = (cmdbase_ud_t*)luaf_checkuserdata(L,1,&g_LuaTable_convar);
	if(!ud) return 0;

	ud->m_pConVar->SetValue(lua_tostring(L,2));
	return 0;
}

DECLARE_FUNCTION(convar,SetBool)
{
	cmdbase_ud_t* ud = (cmdbase_ud_t*)luaL_multicheckuserdata(L,1,2,
		CONVAR,COMMAND);
	if(!ud) return 0;

	ud->m_pConVar->SetValue(!!lua_toboolean(L,2));
	return 0;
}

DECLARE_FUNCTION(convar,SetInt)
{
	cmdbase_ud_t* ud = (cmdbase_ud_t*)luaf_checkuserdata(L,1,&g_LuaTable_convar);
	if(!ud) return 0;

	ud->m_pConVar->SetValue((int)lua_tonumber(L,2));
	return 0;
}

DECLARE_FUNCTION(convar,SetFloat)
{
	cmdbase_ud_t* ud = (cmdbase_ud_t*)luaf_checkuserdata(L,1,&g_LuaTable_convar);
	if(!ud) return 0;

	ud->m_pConVar->SetValue((float)lua_tonumber(L,2));
	return 0;
}

DECLARE_FUNCTION(convar,GetBool)
{
	cmdbase_ud_t* ud = (cmdbase_ud_t*)luaf_checkuserdata(L,1,&g_LuaTable_convar);
	if(!ud) return 0;

	lua_pushboolean(L,ud->m_pConVar->GetBool());
	return 1;
}

DECLARE_FUNCTION(convar,GetInt)
{
	cmdbase_ud_t* ud = (cmdbase_ud_t*)luaf_checkuserdata(L,1,&g_LuaTable_convar);
	if(!ud) return 0;

	lua_pushnumber(L,ud->m_pConVar->GetInt());
	return 1;
}

DECLARE_FUNCTION(convar,GetFloat)
{
	cmdbase_ud_t* ud = (cmdbase_ud_t*)luaf_checkuserdata(L,1,&g_LuaTable_convar);
	if(!ud) return 0;

	lua_pushnumber(L,ud->m_pConVar->GetFloat());
	return 1;
}

DECLARE_FUNCTION(convar,GetDefault)
{
	cmdbase_ud_t* ud = (cmdbase_ud_t*)luaf_checkuserdata(L,1,&g_LuaTable_convar);
	if(!ud) return 0;

	lua_pushstring(L,ud->m_pConVar->GetDefault());
	return 1;
}

DECLARE_METATABLE_INHERITOR(concommand,COMMAND,concommandbase);

DECLARE_FUNCTION(concommand,Dispatch)
{
	char szCommand[MAX_PATH] = {0};
	cmdbase_ud_t* ud = (cmdbase_ud_t*)luaf_checkuserdata(L,1,&g_LuaTable_concommand);
	luaL_checktype(L,2,LUA_TSTRING);
	if(!ud) return 0;

	sprintf(szCommand,"%s %s\n",ud->m_pCmd->GetName(),lua_tostring(L,2));
	engine->ServerCommand(szCommand);
	engine->ServerExecute();

	return 0;
}

DECLARE_TABLE(cvar);

DECLARE_FUNCTION(cvar,GetCommands)
{
	ConCommandBase* pBase;
	if((pBase = cvar->GetCommands()))
		lua_pushnil(L);
	else luaf_makecmdbase(L,pBase);
	return 1;
}

DECLARE_FUNCTION(cvar,Find)
{
	ConCommandBase* pBase;
	const char* pName;
	luaL_checktype(L,1,LUA_TSTRING);
	pName = lua_tostring(L,1);

	pBase = cvar->GetCommands();
	while(pBase)
	{
		if(!V_strcmp(pBase->GetName(),pName))
		{
			luaf_makecmdbase(L,pBase);
			return 1;
		}
		pBase = (ConCommandBase*)pBase->GetNext();
	}

	lua_pushnil(L);
	return 1;
}

DECLARE_FUNCTION(cvar,RegisterConCommandBase)
{
	ConCommandBase** ppBase = (ConCommandBase**)luaL_multicheckuserdata(L,
		1,2,CONVAR,COMMAND);
	if(!ppBase) return 0;
	else cvar->RegisterConCommandBase(*ppBase);
	return 0;
}

DECLARE_FUNCTION(cvar,CreateConVar)
{
	int iArgs,iFlags = 0;
	const char* pHelpText = "";
	cmdbase_ud_t* ud;

	luaL_checktype(L,1,LUA_TSTRING);
	luaL_checktype(L,2,LUA_TSTRING);
	iArgs = lua_gettop(L);

	if(iArgs>2)
		pHelpText = lua_tostring(L,3);
	if(iArgs>3)
		iFlags = lua_tonumber(L,4);

	ConVar* pConVar = new ConVar(lua_tostring(L,1),lua_tostring(L,2),
		iFlags,pHelpText);
	ud = luaf_makeconvar(L,pConVar);
	if(ud) ud->m_bCustom = true;
	else lua_pushnil(L);
	return 1;
}

DECLARE_FUNCTION(cvar,CreateConCommand)
{
	int iArgs,iRefID,iFlags = 0;
	const char* pHelpText = "";
	cmdbase_ud_t* ud;

	luaL_checktype(L,1,LUA_TSTRING);
	luaL_checktype(L,2,LUA_TFUNCTION);
	iArgs = lua_gettop(L);

	if(iArgs>2) pHelpText = lua_tostring(L,3);
	if(iArgs>3) iFlags = lua_tonumber(L,4);

	if(!(ud = luaf_makecommand(L,NULL)))
		lua_pushnil(L);
	else
	{
		lua_pushvalue(L,2);
		iRefID = lua_refobj(L);

		ud->m_pCmd = new CLuaConCommand(lua_tostring(L,1),L,iRefID,pHelpText,iFlags);
		ud->m_bCustom = true;
	}
	return 1;
}

DECLARE_TABLE(cmd);

DECLARE_FUNCTION(cmd,Argc)
{
	lua_pushnumber(L,engine->Cmd_Argc());
	return 1;
}

DECLARE_FUNCTION(cmd,Argv)
{
	const char* pStr;

	luaL_checktype(L,1,LUA_TNUMBER);
	pStr = engine->Cmd_Argv(lua_tonumber(L,1));

	lua_pushstring(L,pStr?pStr:"(null)");
	return 1;
}

DECLARE_FUNCTION(cmd,Args)
{
	const char* pStr = engine->Cmd_Args();

	lua_pushstring(L,pStr?pStr:"(null)");
	return 1;
}

cmdbase_ud_t* luaf_makecmdbase(lua_State* L,ConCommandBase* pBase)
{
	if(pBase->IsCommand())
		return luaf_makecommand(L,(ConCommand*)pBase);
	return luaf_makeconvar(L,(ConVar*)pBase);
}

cmdbase_ud_t* luaf_makeconvar(lua_State* L,ConVar* pConVar)
{
	cmdbase_ud_t* ud = (cmdbase_ud_t*)luaL_createuserdata(L,
		sizeof(cmdbase_ud_t),&g_LuaTable_convar);
	ud->m_pConVar = pConVar;
	return ud;
}

cmdbase_ud_t* luaf_makecommand(lua_State* L,ConCommand* pCmd)
{
	cmdbase_ud_t* ud = (cmdbase_ud_t*)luaL_createuserdata(L,
		sizeof(cmdbase_ud_t),&g_LuaTable_convar);
	ud->m_pCmd = pCmd;
	return ud;
}

CLuaConCommand::CLuaConCommand(char const *pName, lua_State* pL, int iRefID, 
	char const *pHelpString,int flags,FnCommandCompletionCallback completionFunc) 
	: ConCommand(pName,NULL,pHelpString,flags,completionFunc)
{
	m_pL = pL;
	m_iRefID = iRefID;
}

void CLuaConCommand::Dispatch(void)
{
	if(m_iRefID == -1) return;

	lua_pushref(m_pL,m_iRefID);
	if(lua_isnil(m_pL,-1))
	{
		Warning("CLuaConCommand %s is unreferenced!\n",GetName());
		m_iRefID = -1;
		return;
	}

	lua_call(m_pL,0,0);
}
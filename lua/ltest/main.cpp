#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

int lua502_err(lua_State* L)
{
	puts(lua_tostring(L,1));
	return 0;
}

void lua502f_tostr(lua_State* L,int idx,char* szBuf)
{
	int iType;

	iType = lua_type(L,idx);
	if(iType == LUA_TTABLE || iType == LUA_TUSERDATA
		|| iType == LUA_TTHREAD || iType == LUA_TLIGHTUSERDATA)
	{
		sprintf(szBuf,"%s %p",lua_typename(L,iType),
			lua_topointer(L,idx));
	}
	else
	{
		const char* pStr = lua_tostring(L,idx);
		strcpy(szBuf,pStr?pStr:"(null)");
	}
}

int lua502_print(lua_State* L)
{
	char szBuf[64] = {0};
	int iArgs = lua_gettop(L);
	if(iArgs>1)
	{
		for(int i = 1; i <= iArgs; i++)
		{
			*szBuf = 0;
			lua502f_tostr(L,i,szBuf);
			printf("%s\t",szBuf);
		}
		putc('\n',stdout);
	} else if(iArgs)
	{
		lua502f_tostr(L,1,szBuf);
		printf("%s\n",lua_tostring(L,1));
	}
	return 0;
}

void lua502f_dumpstack(lua_State* L)
{
	static int s_iStep = 0;

	int iTop = lua_gettop(L);
	char szName[32];

	printf("== DUMP STACK %d ==\n",++s_iStep);
	for(int i = 1; i <= iTop; i++)
	{
		*szName = 0;
		lua502f_tostr(L,i,szName);
		printf("%d: %s\n",i,szName);
	}
}

int lua502_test(lua_State* L)
{
	lua_pushliteral(L,"metatest");
	lua_newtable(L);

	lua502f_dumpstack(L);

	lua_pushliteral(L,"n");
	lua_pushnumber(L,3);

	lua502f_dumpstack(L);

	lua_settable(L,-3);

	lua502f_dumpstack(L);

	lua_settable(L,LUA_REGISTRYINDEX);
	lua502f_dumpstack(L);
	return 0;
}

int main()
{
	lua_State* L;
	char szBuffer[256] = {0};
	int iErrHandler;

	L = lua_open();
	lua_register(L,"test",lua502_test);
	lua_register(L,"print",lua502_print);

	lua_pushvalue(L,LUA_GLOBALSINDEX);
	lua_setglobal(L,"_G");

	lua_pushvalue(L,LUA_REGISTRYINDEX);
	lua_setglobal(L,"_R");

	while(putc('>',stdout) && gets_s<256>(szBuffer))
	{
		lua_pushcfunction(L,lua502_err);
		if(luaL_loadbuffer(L,szBuffer,strlen(szBuffer),"promt"))
			puts(lua_tostring(L,-1));
		else
		{
			iErrHandler = lua_gettop(L);
			lua_pcall(L,0,LUA_MULTRET,iErrHandler-1);
		}
		lua_pop(L,1);
	}

	lua_close(L);
	return 0;
}
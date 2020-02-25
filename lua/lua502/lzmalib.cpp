#include "baselib.h"
#include "lzma/lzma.h"
#include "tier0/memdbgon.h"

DECLARE_LIBRARY("LzmaLib");
DECLARE_TABLE(lzma);

size_t g_uDictSize = 18U;

DECLARE_FUNCTION(lzma,SetDictionarySize)
{
	luaL_checktype(L,1,LUA_TNUMBER);
	g_uDictSize = (size_t)lua_tonumber(L,1);
	return 0;
}

DECLARE_FUNCTION(lzma,Compress)
{
	luaL_checktype(L,1,LUA_TSTRING);

	const char* pStr = lua_tostring(L,1);
	size_t iLen = lua_strlen(L,1),uLen = 0;
	if(!iLen) return 0;

	unsigned char* pCompressed;
	if(!(pCompressed = LZMA_Compress((unsigned char*)pStr,iLen,&uLen,g_uDictSize)))
		lua_pushnil(L);
	else
	{
		lua_pushlstring(L,(const char*)pCompressed,uLen);
		lua_pushnumber(L,uLen);
		free(pCompressed);
		return 2;
	}
	return 1;
}

DECLARE_FUNCTION(lzma,IsCompressed)
{
	luaL_checktype(L,1,LUA_TSTRING);
	unsigned char* pBuf = (unsigned char*)lua_tostring(L,1);
	lua_pushboolean(L,LZMA_IsCompressed(pBuf));
	return 1;
}

DECLARE_FUNCTION(lzma,Uncompress)
{
	luaL_checktype(L,1,LUA_TSTRING);
	size_t uLen = 0;
	unsigned char* pOut,*pBuf = (unsigned char*)lua_tostring(L,1);
	if(!LZMA_IsCompressed(pBuf))
		lua_pushnil(L);
	else
	{
		if(!LZMA_Uncompress(pBuf,&pOut,&uLen)) lua_pushnil(L);
		else
		{
			lua_pushlstring(L,(const char*)pOut,uLen);
			lua_pushnumber(L,uLen);
			free(pOut);
			return 2;
		}
	}
	return 1;
}
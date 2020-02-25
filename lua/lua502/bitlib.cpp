#include "baselib.h"

DECLARE_LIBRARY("bit")
DECLARE_TABLE(bit)

typedef unsigned long lua_Unsigned;

inline lua_Unsigned toU(lua_State* L,int idx)
{
	return (lua_Unsigned)lua_tonumber(L,idx);
}

inline void pushU(lua_State* L,lua_Unsigned U)
{
	lua_pushnumber(L,(lua_Number)U);
}

DECLARE_FUNCTION(bit,lsh)
{
	luaL_checktype(L,1,LUA_TNUMBER);
	luaL_checktype(L,2,LUA_TNUMBER);
	pushU(L,(toU(L,1) << toU(L,2)));
	return 1;
}

DECLARE_FUNCTION(bit,rsh)
{
	luaL_checktype(L,1,LUA_TNUMBER);
	luaL_checktype(L,2,LUA_TNUMBER);
	pushU(L,(toU(L,1) >> toU(L,2)));
	return 1;
}

DECLARE_FUNCTION(bit,bor)
{
	luaL_checktype(L,1,LUA_TNUMBER);
	luaL_checktype(L,2,LUA_TNUMBER);
	pushU(L,(toU(L,1) | toU(L,2)));
	return 1;
}

DECLARE_FUNCTION(bit,band)
{
	luaL_checktype(L,1,LUA_TNUMBER);
	luaL_checktype(L,2,LUA_TNUMBER);
	pushU(L,(toU(L,1) & toU(L,2)));
	return 1;
}

DECLARE_FUNCTION(bit,bnot)
{
	luaL_checktype(L,1,LUA_TNUMBER);
	pushU(L,~toU(L,1));
	return 1;
}

inline int bitmask(unsigned int n)
{
    return (1<<n)-1;
}

inline unsigned int ror(unsigned int num,unsigned int bits)
{
    return ((num&bitmask(bits))<<(32-bits))|(num>>bits);
}

inline unsigned int rol(unsigned int num,unsigned int bits)
{
    return ((num&~bitmask(bits))>>(32-bits))|(num<<bits);
}

DECLARE_FUNCTION(bit,rol)
{
	luaL_checktype(L,1,LUA_TNUMBER);
	luaL_checktype(L,2,LUA_TNUMBER);
	pushU(L,rol(toU(L,1),toU(L,2)));
	return 1;
}

DECLARE_FUNCTION(bit,ror)
{
	luaL_checktype(L,1,LUA_TNUMBER);
	luaL_checktype(L,2,LUA_TNUMBER);
	pushU(L,ror(toU(L,1),toU(L,2)));
	return 1;
}

DECLARE_TABLE(bit64)

DECLARE_FUNCTION(bit64,Make)
{
	luaL_checktype(L,1,LUA_TNUMBER);
	luaL_checktype(L,2,LUA_TNUMBER);

	lua_newtable(L);
	
	lua_pushliteral(L,"high");
	lua_pushvalue(L,1);
	lua_rawset(L,-3);

	lua_pushliteral(L,"low");
	lua_pushvalue(L,2);
	lua_rawset(L,-3);

	return 1;
}

typedef union {
	unsigned long long ull;
	struct {
		unsigned long low;
		unsigned long high;
	} long64;
} lua_Unsigned64;

DECLARE_FUNCTION(bit64,ToString)
{
	luaL_checktype(L,1,LUA_TTABLE);
	lua_Unsigned64 num;
	char szNum[64];

	lua_pushliteral(L,"high");
	lua_rawget(L,1);
	num.long64.high = toU(L,-1);
	
	lua_pushliteral(L,"low");
	lua_rawget(L,1);
	num.long64.low = toU(L,-1);
	lua_pop(L,2);

	sprintf(szNum,"%llu",num.ull);
	lua_pushstring(L,szNum);
	return 1;
}

DECLARE_FUNCTION(bit64,FromString)
{
	luaL_checktype(L,1,LUA_TSTRING);
	lua_Unsigned64 num;

	sscanf(lua_tostring(L,1),"%llu",&num.ull);

	lua_newtable(L);
	
	lua_pushliteral(L,"high");
	pushU(L,num.long64.high);
	lua_rawset(L,-3);

	lua_pushliteral(L,"low");
	pushU(L,num.long64.low);
	lua_rawset(L,-3);

	return 1;
}
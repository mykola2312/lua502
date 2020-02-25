#include "luapointer.h"
#include "baselib.h"
#include "types.h"

DECLARE_LIBRARY("ptrlib");
DECLARE_METATABLE(ptrmeta,POINTER);

/*void luaf_makepointer(lua_State* L,void* pPtr)
{
	luaL_createuserdata(L,s_iType,pPtr);
}*/

void luaf_makepointer(lua_State* L,void* pPtr)
{
	void** ppPtr = (void**)luaL_createuserdata(L,sizeof(void*),
		&g_LuaTable_ptrmeta);
	*ppPtr = pPtr;
}

DECLARE_FUNCTION(ptrmeta,__tostring)
{
	void** ppPtr;
	if((ppPtr=(void**)luaf_checkuserdata(L,1,&g_LuaTable_ptrmeta)))
		luaL_pushfstring(L,"Pointer %p",*ppPtr);
	else lua_pushnil(L);
	return 1;
}

DECLARE_FUNCTION(ptrmeta,__add)
{
	void** pOne = (void**)luaf_checkuserdata(L,1,&g_LuaTable_ptrmeta),
		**pTwo = (void**)luaf_checkuserdata(L,2,&g_LuaTable_ptrmeta);
	if(!pOne || !pTwo) lua_pushnil(L);
	else
		luaf_makepointer(L,((char*)*pOne+(int)*pTwo));
	return 1;
}

DECLARE_FUNCTION(ptrmeta,__sub)
{
	void** pOne = (void**)luaf_checkuserdata(L,1,&g_LuaTable_ptrmeta),
		**pTwo = (void**)luaf_checkuserdata(L,2,&g_LuaTable_ptrmeta);
	if(!pOne || !pTwo) lua_pushnil(L);
	else
		luaf_makepointer(L,((char*)*pOne-(int)*pTwo));
	return 1;
}

DECLARE_FUNCTION(ptrmeta,__eq)
{
	void** pOne = (void**)luaf_checkuserdata(L,1,&g_LuaTable_ptrmeta),
		**pTwo = (void**)luaf_checkuserdata(L,2,&g_LuaTable_ptrmeta);
	if(!pOne || !pTwo) lua_pushboolean(L,false);
	else
		lua_pushboolean(L,(*pOne==*pTwo));
	return 1;
}
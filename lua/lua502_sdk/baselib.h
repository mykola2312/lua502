#ifndef __BASELIB_H
#define __BASELIB_H

#include "lua.hpp"

class CLuaLibrary;
class CLuaFunction;
class CLuaTable
{
public:
	CLuaTable(CLuaLibrary* pLib,
		const char* pName, bool bRegister = true);

	void Add(CLuaFunction* pFunc);
	virtual void Register(lua_State* L);
	virtual void Unregister(lua_State* L);

	union {
		const char* m_pName;
		int m_iTypeID;
	};
	bool m_bRegister;

	CLuaFunction* m_pLast;
	CLuaTable* m_pNext;
};

class CLuaMetaTable : public CLuaTable
{
public:
	CLuaMetaTable(CLuaLibrary* pLib,
		unsigned short nTypeID,
		CLuaTable* pParent = NULL);

	virtual void Register(lua_State* L);
	virtual void Unregister(lua_State* L);
	virtual void PushToStack(lua_State* L);

	CLuaTable* m_pParent;
	char m_szMetaName[16];
};

class CLuaFunction
{
public:
	CLuaFunction(CLuaTable* pTable,const char* pName,
		lua_CFunction fnFunc);

	const char* m_pName;
	lua_CFunction m_fnFunc;
	CLuaFunction* m_pNext;
};

class CLuaLibrary
{
public:
	CLuaLibrary(const char* pName);

	virtual void Register(lua_State* L);
	virtual void Unregister(lua_State* L);

	void Add(CLuaTable* pTable);

	const char* m_pName;

	CLuaTable* m_pLastTable;
	CLuaLibrary* m_pNextLibrary;

	static void Init(lua_State* L);
	static void Exit(lua_State* L);

	static void Dump();

	static CLuaLibrary* s_pLast;
};

typedef unsigned short UInt16;

typedef struct {
	UInt16 m_nType;
	void* m_pData;
} userdata_t;

LUA_API void* luaL_createuserdata(lua_State* L,size_t sz,
	CLuaMetaTable* pMeta);
LUA_API void luaL_pushfstring(lua_State* L,const char* pFmt,...);
inline void* luaf_checkuserdata(lua_State* L,int idx,
	CLuaMetaTable* pMeta)
{
	luaL_checktype(L,idx,LUA_TUSERDATA);
	void* pUD = lua_touserdata(L,idx);
	if(*(unsigned char*)pUD != pMeta->m_iTypeID)
		luaL_argerror(L,idx,"Wrong userdata");
	else return (char*)pUD+sizeof(unsigned short);
	return NULL;
}
LUA_API void* luaL_multicheckuserdata(lua_State* L,int idx,
	int nTypes,...);

//Eats object at top of stack
LUA_API int lua_refobj(lua_State* L);
LUA_API void lua_unrefobj(lua_State* L,int id);
LUA_API void lua_pushref(lua_State* L,int id);

//#undef __BASELIB_INTERNAL_H
#ifndef __BASELIB_INTERNAL_H
#define DECLARE_LIBRARY(name)																		\
	static CLuaLibrary g_LuaLibrary(name);

#define DECLARE_TABLE(tablename)																	\
	static CLuaTable g_LuaTable_##tablename(&g_LuaLibrary,#tablename);

#define DECLARE_TABLE_DUMMY(tablename)																\
	static CLuaTable g_LuaTable_##tablename(&g_LuaLibrary,#tablename,false);

#define DECLARE_METATABLE(tablename,type)															\
	static CLuaMetaTable g_LuaTable_##tablename(&g_LuaLibrary,type);

#define DECLARE_METATABLE_INHERITOR(tablename,type,parent)											\
	static CLuaMetaTable g_LuaTable_##tablename(&g_LuaLibrary,type,&g_LuaTable_##parent);

#define DECLARE_FUNCTION(tablename,funcname)														\
	static int luaf_##tablename##funcname(lua_State*);															\
	static CLuaFunction g_##tablename##funcname(&g_LuaTable_##tablename,#funcname,luaf_##tablename##funcname);	\
	static int luaf_##tablename##funcname(lua_State* L)
#endif

#endif
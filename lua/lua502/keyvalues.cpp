#include "baselib.h"
#include "keyvalues.h"
#include "filesystem.h"

DECLARE_LIBRARY("keyvalues")
DECLARE_TABLE(keyvalues)
extern IFileSystem* filesystem;

//void print_stack(lua_State* L)
//{
//	for(int i = lua_gettop(L); i > 0; i--)
//		Msg("%d: %s (%s)\n",i,lua_tostring(L,i),lua_typename(L,lua_type(L,i)));
//}

void KeyValuesFromTable(lua_State* L,int idx,KeyValues* pKV)
{
	lua_pushvalue(L,idx);
	lua_pushnil(L);
	while(lua_next(L,-2))
	{
		int iKeyType = lua_type(L,-2);
		if(iKeyType != LUA_TSTRING && iKeyType != LUA_TNUMBER && iKeyType != LUA_TTABLE)
		{
			lua_pop(L,1);
			continue;
		}

		lua_pushvalue(L,-2);
		//Key -1, Value -2, Old Key -3, Table -4
		int iType = lua_type(L,-2);

		char* pKey;
		char cSwap = ' ';
		if(iKeyType == LUA_TTABLE)
		{
			cSwap = 'T';
			lua_insert(L,-2);
			iKeyType = lua_type(L,-1);
			iType = lua_type(L,-2);
		}

		if(iKeyType == LUA_TSTRING)
		{
			int iLen = lua_strlen(L,-1)+4;
			pKey = new char[iLen];
			sprintf(pKey,"S%c%s",cSwap,lua_tostring(L,-1));
		}
		else if(iKeyType == LUA_TNUMBER)
		{
			pKey = new char[32];
			memset(pKey,'\0',32);
			sprintf(pKey,"N%c%lf",cSwap,lua_tonumber(L,-1));
		}

		KeyValues* pSub = NULL;
		switch(iType)
		{
		case LUA_TBOOLEAN:
			pKV->SetInt(pKey,lua_tonumber(L,-2));
			break;
		case LUA_TNUMBER:
			pKV->SetFloat(pKey,lua_tonumber(L,-2));
			break;
		case LUA_TFUNCTION:
			/*sprintf(szBuf,"0x%p",lua_tocfunction(L,-2));
			pKV->SetString(pKey,szBuf);
			break;*/
		case LUA_TTHREAD:
		case LUA_TUSERDATA:
		case LUA_TSTRING:
			pKV->SetString(pKey,lua_tostring(L,-2));
			break;
		case LUA_TTABLE:
			if(!lua_rawequal(L,-2,-4))
			{
				pSub = pKV->FindKey(pKey,true);
				KeyValuesFromTable(L,-2,pSub);
			}
			break;
		}
		delete[] pKey;
		lua_pop(L,2);
	}
	lua_pop(L,1);
}

DECLARE_FUNCTION(keyvalues,SaveTable)
{
	luaL_checktype(L,1,LUA_TTABLE);
	luaL_checktype(L,2,LUA_TSTRING);
	luaL_checktype(L,3,LUA_TSTRING);
	KeyValues* pKV = new KeyValues(lua_tostring(L,2));
	KeyValuesFromTable(L,1,pKV);
	pKV->SaveToFile(filesystem,lua_tostring(L,3),"MOD");
	pKV->deleteThis();
	return 0;
}

void KeyValuesToTable(lua_State* L,KeyValues* pKV)
{
	lua_newtable(L);
	for(KeyValues* sub = pKV->GetFirstSubKey(); sub; sub = sub->GetNextKey())
	{
		const char* pName0 = sub->GetName();
		const char* pName = pName0+2;
		bool bSwap = false;
		if(pName0[0] == 'S') 
			lua_pushstring(L,pName);
		else if(pName0[0] == 'N') 
			lua_pushnumber(L,atof(pName));
		bSwap = (pName0[1] == 'T');

		if(sub->GetFirstSubKey())
			KeyValuesToTable(L,sub);
		else
		{
			switch(pKV->GetDataType(pName0))
			{
			case KeyValues::TYPE_STRING:
				lua_pushstring(L,sub->GetString());
				break;
			case KeyValues::TYPE_INT:
				lua_pushnumber(L,sub->GetInt());
				break;
			case KeyValues::TYPE_UINT64:
				lua_pushnumber(L,sub->GetUint64());
				break;
			case KeyValues::TYPE_FLOAT:
				lua_pushnumber(L,sub->GetFloat());
				break;
			default:
				lua_pushnil(L);
			}
		}
		if(bSwap) lua_insert(L,-2);
		lua_settable(L,-3);
	}
}

DECLARE_FUNCTION(keyvalues,LoadTable)
{
	luaL_checktype(L,1,LUA_TSTRING);
	luaL_checktype(L,2,LUA_TSTRING);
	KeyValues* pKV = new KeyValues(lua_tostring(L,1));
	pKV->LoadFromFile(filesystem,lua_tostring(L,2),"MOD");
	KeyValuesToTable(L,pKV);
	pKV->deleteThis();
	return 1;
}
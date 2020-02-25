#include <Windows.h>
#include "threadtools.h"
#include "tier1.h"
#include "dbg.h"
#include <stdarg.h>

void PrintDebug(const char* fmt,...)
{
	char szBuf[512] = {0};
	va_list ap;
	va_start(ap,fmt);

	vsnprintf(szBuf,512,fmt,ap);
	va_end(ap);
	
	Msg("%s",szBuf);
	OutputDebugString(szBuf);
}

class CMyWorker : public CWorkerThread
{
public:
	CMyWorker() : CWorkerThread()
	{
		SetName("MyWorker");
	}

	enum {
		CALL_FUNC,
		EXIT
	};

	void AddRequest(int param)
	{
		m_iParam = param;
		CallWorker(CALL_FUNC);
	}

	int Run()
	{
		unsigned nCall;
		while(WaitForCall(&nCall))
		{
			if(nCall == EXIT)
			{
				Reply(1);
				break;
			}

			int iparam = m_iParam;
			m_iParam = 0;
			Reply(1);
			PrintDebug("Work %d from %d\n",iparam,GetCurrentThreadId());
		}
		return 0;
	}

	int m_iParam;
} g_MyWorker;

class CMyThread : public CThread
{
public:
	CMyThread() : CThread()
	{
		SetName("MyThread");
	}

	int Run()
	{
		PrintDebug("MyThread works!!!");
		return 0;
	}
} g_MyThread;

ICvar* g_pCvar = NULL;

CON_COMMAND(thread_start,"")
{
	PrintDebug("This thread %d\n",GetCurrentThreadId());
	g_MyWorker.Start();
	g_MyThread.Start();
	for(int i = 0; i < 5; i++)
		g_MyWorker.AddRequest(i);
}

void Main()
{
	CreateInterfaceFn EngineFn = Sys_GetFactory("engine.dll");
	g_pCvar = (ICvar*)EngineFn(VENGINE_CVAR_INTERFACE_VERSION,0);
	g_pCvar->RegisterConCommandBase(&thread_start_command);
}

BOOL APIENTRY DllMain(HINSTANCE,DWORD fdw,LPVOID)
{
	if(fdw==DLL_PROCESS_ATTACH)
		Main();
	return TRUE;
}
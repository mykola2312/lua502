#ifndef __VHOOK_H
#define __VHOOK_H

#include <Windows.h>
#include <map>

class VHookStatic
{
public:
	VHookStatic(DWORD dwInterface);
	~VHookStatic();

	DWORD HookFunction(DWORD dwHook,int idx);
	DWORD UnHookFunction(int idx);
	void UnHookAll();
private:
	DWORD* dwVTable;
	std::map<int,DWORD> m_Hooks;
};

class VHook
{
public:
	VHook(DWORD _dwInterface);
	~VHook();

	DWORD HookFunction(DWORD dwHook,int idx);
	DWORD UnHookFunction(int idx);
	void UnHookAll();

	inline PDWORD GetOldVTable()
	{
		return dwOldVTable;
	}

	DWORD dwInterface;
private:
	MEMORY_BASIC_INFORMATION mba;
	DWORD* dwOldVTable;
	DWORD* dwVTable;
	DWORD dwSize;
};

class CEHook
{
public:
	CEHook(DWORD dwInterface);
	~CEHook();
	
	static DWORD HookVTable(PDWORD dwVTable,DWORD dwHook,DWORD dwIndex);
	DWORD BuildDetour(DWORD dwOriginal,DWORD dwHook);
	DWORD HookFunction(DWORD dwHook,DWORD dwIndex);
	void UnHookFunction(DWORD dwIndex);
	void UnHookAll();
private:
	static size_t GateSize;
	PDWORD dwVTable;
	//Index,<Orig,Detour>
	std::map<DWORD,std::pair<DWORD,DWORD> > m_Hooks;
};

#endif
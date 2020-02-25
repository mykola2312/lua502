#include "VHook.h"

size_t CEHook::GateSize = 0;

VHookStatic::VHookStatic(DWORD dwInterface)
{
	dwVTable = *(DWORD**)dwInterface;
}

VHookStatic::~VHookStatic()
{
	UnHookAll();
}

DWORD VHookStatic::HookFunction(DWORD dwHook,int idx)
{
	DWORD dwOld,dwProtect;
	VirtualProtect((LPVOID)&dwVTable[idx],sizeof(DWORD),PAGE_READWRITE,&dwProtect);
	dwOld = dwVTable[idx];
	m_Hooks.insert(std::pair<int,DWORD>(idx,dwOld));
	dwVTable[idx] = dwHook;
	VirtualProtect((LPVOID)&dwVTable[idx],sizeof(DWORD),dwProtect,NULL);
	return dwOld;
}

DWORD VHookStatic::UnHookFunction(int idx)
{
	std::map<int,DWORD>::iterator it;
	it = m_Hooks.find(idx);
	if(it == m_Hooks.end())
		return NULL;
	return HookFunction(it->second,idx);
}

void VHookStatic::UnHookAll()
{
	std::map<int,DWORD>::iterator it;
	for(it = m_Hooks.begin(); it != m_Hooks.end(); ++it)
	{
		HookFunction(it->second,it->first);
		m_Hooks.erase(it);
	}
}

VHook::VHook(DWORD _dwInterface)
{
	dwInterface = _dwInterface;
	dwOldVTable = *(DWORD**)dwInterface;

	dwSize = 2048;
	dwVTable = (DWORD*)VirtualAlloc(NULL,dwSize,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
	UnHookAll();

	*(DWORD**)dwInterface = dwVTable;
}

VHook::~VHook()
{
	*(DWORD**)dwInterface = dwOldVTable;
	VirtualFree((LPVOID)dwVTable,dwSize,MEM_RELEASE|MEM_DECOMMIT);
}

DWORD VHook::HookFunction(DWORD dwHook, int idx)
{
	dwVTable[idx] = dwHook;
	return dwOldVTable[idx];
}

DWORD VHook::UnHookFunction(int idx)
{
	return (dwVTable[idx] = dwOldVTable[idx]);
}

void VHook::UnHookAll()
{
	memcpy((void*)dwVTable,(const void*)dwOldVTable,dwSize);
}

CEHook::CEHook(DWORD dwInterface)
{
	dwVTable = *(PDWORD*)dwInterface;
}
	
CEHook::~CEHook()
{
	UnHookAll();
}
	
DWORD CEHook::HookVTable(PDWORD dwVTable,DWORD dwHook,DWORD dwIndex)
{
	DWORD dwProtect,dwOld;
	VirtualProtect((LPVOID)dwVTable,4096,PAGE_READWRITE,&dwProtect);
	dwOld = dwVTable[dwIndex];
	dwVTable[dwIndex] = dwHook;
	VirtualProtect((LPVOID)dwVTable,4096,dwOld,NULL);
	return dwOld;
}
	
DWORD CEHook::BuildDetour(DWORD dwOriginal,DWORD dwHook)
{
	const char* pGate = "\x58\x51\x50\xE9\x90\x90\x90\x90\x58\x59\x50\xE9\x90\x90\x90\x90";
	DWORD dwOrigRelative,dwHookRelative,dwDetour;
	if(!GateSize)
		GateSize = strlen(pGate);
		
	printf("%d\n",GateSize);
	dwDetour = (DWORD)VirtualAlloc(NULL,GateSize,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);
	memcpy((void*)dwDetour,pGate,GateSize);
		
	//Destination - SourceFunc - CurrentPosition(pos+opcode size)
	dwHookRelative = (DWORD)(dwHook - (DWORD)(dwDetour+8));
	dwOrigRelative = (DWORD)(dwOriginal - (DWORD)(dwDetour+16));
		
	*(PDWORD)(dwDetour+4) = dwHookRelative;
	*(PDWORD)(dwDetour+12) = dwOrigRelative;
	return dwDetour;
}
	
DWORD CEHook::HookFunction(DWORD dwHook,DWORD dwIndex)
{
	DWORD dwOrig = dwVTable[dwIndex];
	DWORD dwDetour = BuildDetour(dwOrig,dwHook);
	HookVTable(dwVTable,dwDetour,dwIndex);
	m_Hooks.insert(std::pair<DWORD,std::pair<DWORD,DWORD> >(dwIndex,std::pair<DWORD,DWORD>(dwOrig,dwDetour)));
	return (DWORD)(dwDetour+8);
}
	
void CEHook::UnHookFunction(DWORD dwIndex)
{
	std::map<DWORD,std::pair<DWORD,DWORD> >::iterator it = m_Hooks.find(dwIndex);
	if(it==m_Hooks.end())
		return;
		
	HookVTable(dwVTable,it->second.first,dwIndex);
	VirtualFree((LPVOID)it->second.second,GateSize,MEM_DECOMMIT|MEM_RELEASE);
}
	
void CEHook::UnHookAll()
{
	std::map<DWORD,std::pair<DWORD,DWORD> >::iterator it;
	for(it = m_Hooks.begin(); it != m_Hooks.end(); ++it)
	{
		HookVTable(dwVTable,it->second.first,it->first);
		VirtualFree((LPVOID)it->second.second,GateSize,MEM_DECOMMIT|MEM_RELEASE);
	}
}
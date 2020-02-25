#include "filesystem.h"
#include <map>

typedef struct {
	HMODULE m_hMod;
	CreateInterfaceFn m_fnFactory;
} factory_t;

void* Sys_GetInterface(const char* pModule,const char* pInterface)
{
	HMODULE hMod = NULL;
	factory_t ft;

	static std::map<const char*,factory_t> s_FactoryList;
	if(s_FactoryList.find(pModule)==s_FactoryList.end())
	{
		if(!(ft.m_hMod = GetModuleHandle(pModule))) return NULL;
		if(!(ft.m_fnFactory = (CreateInterfaceFn)GetProcAddress(
			ft.m_hMod,"CreateInterface"))) return NULL;
	}
	else ft = s_FactoryList[pInterface];

	if(ft.m_fnFactory) return ft.m_fnFactory(pInterface,NULL);
	return NULL;
}
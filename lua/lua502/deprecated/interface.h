#ifndef __INTERFACE_H
#define __INTERFACE_H

#include <stdlib.h>
#include <string.h>

class CInterfaceReg
{
public:
	CInterfaceReg(void* pDst,const char* pName);

	void* m_pDst;
	const char* m_pName;
	CInterfaceReg* m_pNext;

	static CInterfaceReg* s_pLast;
};

extern "C" __declspec(dllexport) void* CreateInterface(const char* pName,int* pStatus);

#define EXPORT_INTERFACE(dst,name) CInterfaceReg dst##_Reg(&dst,name)

#endif
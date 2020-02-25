#include "interface.h"

CInterfaceReg* CInterfaceReg::s_pLast = NULL;

CInterfaceReg::CInterfaceReg(void* pDst,const char* pName)
{
	m_pDst = pDst;
	m_pName = pName;

	m_pNext = s_pLast;
	s_pLast = this;
}

void* CreateInterface(const char* pName,int* pStatus)
{
	CInterfaceReg* pInterfaceReg = CInterfaceReg::s_pLast;
	do {
		if(!strcmp(pInterfaceReg->m_pName,pName))
			return pInterfaceReg->m_pDst;
	} while(pInterfaceReg = pInterfaceReg->s_pLast);
	return NULL;
}
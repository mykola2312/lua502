#include "sigscan.h"

static SigScan::CSigScan* s_pLast;

void SigScan_Notify(SigScan::CSigScan* pSigScan,SigScan::SigNotify_t notify)
{
	char szError[64];
	if(notify==SigScan::SIG_ERROR)
	{
		sprintf(szError,"%s (%p) sigscan failed!",pSigScan->m_pName,pSigScan->m_pdwDest);
		MessageBoxA(NULL,szError,"SigScan",MB_ICONHAND);
		ExitThread(1);
	}
}

SigScan::CSigScan::CSigScan(const char* pName,const char* pDll,const char* pSig,const char* pMask,
				PDWORD pdwDest,DWORD dwOffset = 0,SigType_t type)
{
	m_pPrev = NULL;

	m_pName = pName;
	m_pDll = pDll;
	m_pSig = pSig;
	m_pMask = pMask;
	m_pdwDest = pdwDest;
	m_dwOffset = dwOffset;
	m_Type = type;

	Add(this);
}

void SigScan::Add(CSigScan* pSigScan)
{
	pSigScan->m_pPrev = s_pLast;
	s_pLast = pSigScan;
}

void SigScan::Scan()
{
	CSigScan* pSigScan = s_pLast;
	DWORD dwTrunk;

	do {
		if(!(dwTrunk = SigScan(GetModuleHandle(pSigScan->m_pDll),
			pSigScan->m_pSig,pSigScan->m_pMask)))
		{
			SigScan_Notify(pSigScan,SIG_ERROR);
			continue;
		}

		dwTrunk += pSigScan->m_dwOffset;
		switch(pSigScan->m_Type)
		{
		case CSigScan::SIG_FUNCTION:
			*pSigScan->m_pdwDest = dwTrunk;
			break;
		case CSigScan::SIG_PTR:
			*pSigScan->m_pdwDest = *(PDWORD)(dwTrunk);
			break;
		case CSigScan::SIG_VARIABLE:
			*pSigScan->m_pdwDest = **(PDWORD*)(dwTrunk);
			break;
		}
		SigScan_Notify(pSigScan,SIG_LOG);
	} while((pSigScan = pSigScan->m_pPrev));
}

size_t SigScan::GetModuleSize(HMODULE hDll)
{
	PIMAGE_DOS_HEADER DosHeader;
	PIMAGE_NT_HEADERS NtHeaders;
	
	DosHeader = (PIMAGE_DOS_HEADER)hDll;
	NtHeaders = (PIMAGE_NT_HEADERS)((LONG)hDll+DosHeader->e_lfanew);

	return NtHeaders->OptionalHeader.SizeOfImage;
}

DWORD SigScan::SigScan(HMODULE hDll,const char* sig,const char* mask)
{
	size_t j,size = GetModuleSize(hDll),len = strlen(mask);
	char* mem = (char*)hDll;
	for(size_t i = 0; i < size; i++)
	{
		for(j = 0; j < len; j++)
		{
			if((mask[j] != '?') && (((char*)(mem+i))[j] != sig[j]))
				break;
		}
		
		if(j==len)
			return (DWORD)(mem+i);
	}
	return NULL;
}
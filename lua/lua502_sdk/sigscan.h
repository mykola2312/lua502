#ifndef __SIGSCAN_H
#define __SIGSCAN_H

#include <Windows.h>
#include <stdio.h>

namespace SigScan
{
	class CSigScan
	{
	public:
		typedef enum {
			SIG_FUNCTION,
			SIG_VARIABLE,
			SIG_PTR,
		} SigType_t;

		CSigScan(const char* pName,const char* pDll,const char* pSig,const char* pMask,
				PDWORD pdwDest,DWORD dwOffset,SigType_t type = SIG_FUNCTION);

		CSigScan* m_pPrev;

		const char* m_pName;
		const char* m_pDll;
		const char* m_pSig;
		const char* m_pMask;
	
		PDWORD m_pdwDest;
		DWORD m_dwOffset;
		SigType_t m_Type;
	};

	typedef enum {
		SIG_LOG,
		SIG_ERROR,
	} SigNotify_t;

	size_t GetModuleSize(HMODULE hDll);
	DWORD SigScan(HMODULE hDll,const char* pSig,const char* pMask);

	void Scan();
	void Add(CSigScan* pSigScan);

	//CSigScan* s_pLast;
}

#define DECLARE_SIGSCAN(dst,dll,sig,mask,ofst,type) SigScan::CSigScan dst##_s(#dst,dll,sig,mask,(PDWORD)&dst,ofst,type)

#endif
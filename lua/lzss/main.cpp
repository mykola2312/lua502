#include <Windows.h>
#include <stdio.h>
#include "tier1.h"
#include "tier1/strtools.h"
#include "tier0/platform.h"
#include "lzma/lzma.h"

inline char* allocbuf(size_t z)
{
	char* pData = (char*)malloc(z);
	const char* lel = "YOUSPINMERIGHTROUND";
	size_t k = strlen(lel);
	for(int i = 0; i < z; i++)
		pData[i] = lel[i%k];
	pData[z] = 0;
	return pData;
}

void hexdump(unsigned char* pData,size_t len)
{
	for(size_t i = 0; i < len; i++)
		Msg("%02X ",(pData[i] & 0xFF));
	Msg("\n");
}

int EMain()
{
	char szBuffer[256] = {0};

	Msg("Enter a string: ");
	gets(szBuffer);

	int iLen = V_strlen(szBuffer)+1;
	unsigned int uLen = 0;

	unsigned char* pCompressed;
	if(!(pCompressed = LZMA_Compress((unsigned char*)szBuffer,iLen,&uLen,5U)))
	{
		Error("LZMA_Compress failed!\n");
		getc(stdin);
		return 1;
	}
	Msg("Compressed (%d): ",uLen);
	hexdump(pCompressed,uLen);

	unsigned char* pUncompressed;
	if(!LZMA_Uncompress(pCompressed,&pUncompressed,&uLen))
	{
		Error("LZMA_Uncompress failed!\n");
		getc(stdin);
		return 1;
	}
	free(pCompressed);

	Msg("Uncompressed (%d): %s\n",uLen,pUncompressed);
	free(pUncompressed);
	getc(stdin);
	return 0;
}

#ifdef WIN_DLL
BOOL APIENTRY DllMain(HINSTANCE hDll,DWORD fdwReason,LPVOID)
{
	if(fdwReason==DLL_PROCESS_ATTACH)
		CreateThread(0,0,(LPTHREAD_START_ROUTINE)EMain,0,0,0);
	return TRUE;
}
#else
int main()
{
	return EMain();
}
#endif
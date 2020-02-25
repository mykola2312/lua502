#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "bzlib.h"

WSADATA g_WSA;

#pragma pack(push,1)
typedef struct {
	DWORD dwHdr;
	BYTE bPck;
	DWORD dwChallenge;
} a2schallenge_t;

typedef struct {
	DWORD dwHdr;
	DWORD dwID;
	BYTE bPcks;
	BYTE bCurPck;
	WORD wSplitSize;

	DWORD dwUSize;
	DWORD dwCRC;
} multi_t;

typedef struct {
	DWORD dwHdr;
	BYTE bPck;
	WORD wRules;
} a2srules_t;
#pragma pack(pop)

#define SADDR(adr) ((sockaddr*)&adr)
#define CSADDR(adr) ((const sockaddr*)&adr)

int main(int argc,char** argv)
{
	SOCKET s;
	struct sockaddr_in addr;
	int socklen = sizeof(addr);
	a2schallenge_t chl;
	char* pRules = NULL;
	char szBuf[1500];
	int iSize = 0,iRules = 0,iLen;
	int iPcks = 0;

	WSAStartup(MAKEWORD(2,2),&g_WSA);
	s = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(argv[1]);
	addr.sin_port = htons(atoi(argv[2]));

	chl.dwHdr = (DWORD)-1;
	chl.bPck = 'V';
	chl.dwHdr = (DWORD)-1;
	
	sendto(s,(const char*)&chl,sizeof(chl),0,CSADDR(addr),socklen);
	recvfrom(s,(char*)&chl,sizeof(chl),0,SADDR(addr),&socklen);

	pRules = (char*)calloc(1,1500);
	printf("Challenge 0x%p\n",chl.dwChallenge);
	socklen = sizeof(addr);
	chl.bPck = 'V';
	sendto(s,(const char*)&chl,sizeof(chl),0,CSADDR(addr),socklen);

	multi_t m;
	int iTotalSize = 0;
	do {
		if((iLen = recvfrom(s,szBuf,sizeof(szBuf),0,SADDR(addr),&socklen)) > 0)
		{
			int iPckSize = (iPcks?(sizeof(multi_t)-8):sizeof(multi_t));
			int iCurSize = iLen-iPckSize;
			memcpy(&m,szBuf,iPckSize);
			if(!iPcks)
			{
				iPcks = m.bPcks;
				iTotalSize = m.dwUSize;
				printf("%p\n",iTotalSize);
			}
			printf("Packet %d iLen %d\n",m.bCurPck,iLen);
			
			if(!pRules) pRules = (char*)malloc(iCurSize);
			else pRules = (char*)realloc(pRules,iSize+iCurSize);

			memcpy((pRules+iSize),(char*)szBuf+iPckSize,iCurSize);
			iSize += iCurSize;
		}
	} while(iLen > 0 && m.bCurPck < (iPcks-1));

	/* Uses BZip2! */
	if(m.dwID & 0x80000000)
	{
		char* pOut = (char*)malloc(iTotalSize);
		bz_stream strm;

		memset(&strm,'\0',sizeof(strm));
		strm.next_in = pRules;
		strm.avail_in = iSize;
		strm.next_out = pOut;
		strm.avail_out = iTotalSize;

		BZ2_bzDecompressInit(&strm,0,0);
		BZ2_bzDecompress(&strm);
		BZ2_bzDecompressEnd(&strm);
		puts("Decompressed!\n");

		free(pRules);
		iSize = iTotalSize;
		pRules = pOut;
	}

	FILE* fRules = fopen("rules.bin","wb");
	fwrite(pRules,iSize,1,fRules);
	fclose(fRules);
	return 0;
}
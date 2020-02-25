#ifndef __FILESYSTEM_H
#define __FILESYSTEM_H

#include <Windows.h>

typedef int CSysModule;
typedef int FileFindHandle_t;

#define FILESYSTEM_INTERFACEVERSION "VFileSystem017"
#define FILESYSTEM_MODULE "filesystem_steam.dll"

typedef void* (*CreateInterfaceFn)(const char*,int*);

class IFileSystem
{
public:
	virtual void stub_000() = 0;
	virtual void stub_001() = 0;
	virtual void stub_002() = 0;
	virtual void stub_003() = 0;
	virtual void stub_004() = 0;
	virtual void stub_005() = 0;
	virtual void stub_006() = 0;
	virtual void stub_007() = 0;
	virtual void stub_008() = 0;
	virtual void stub_009() = 0;
	virtual void stub_010() = 0;
	virtual void stub_011() = 0;
	virtual void stub_012() = 0;
	virtual void stub_013() = 0;
	virtual void stub_014() = 0;
	virtual void stub_015() = 0;
	virtual void stub_016() = 0;
	virtual void stub_017() = 0;
	virtual void stub_018() = 0;
	virtual void stub_019() = 0;
	virtual void stub_020() = 0;
	virtual void stub_021() = 0;
	virtual void stub_022() = 0;
	virtual void stub_023() = 0;
	virtual void stub_024() = 0;
	virtual void stub_025() = 0;
	virtual void stub_026() = 0;
	virtual void stub_027() = 0;
	virtual void stub_028() = 0;
	virtual CSysModule* LoadModule(const char *pFileName,
		const char *pPathID = 0, bool bValidatedDllOnly = true) = 0;
	virtual void UnloadModule(CSysModule* pModule) = 0;
	/*virtual CSysModule* LoadModule(const char *pFileName,
		const char *pPathID = 0, bool bValidatedDllOnly = true) = 0;
	virtual void UnloadModule(CSysModule* pModule) = 0;
	virtual const char *FindFirst( const char *pWildCard, FileFindHandle_t *pHandle ) = 0;
	virtual const char *FindNext( FileFindHandle_t handle ) = 0;
	virtual bool FindIsDirectory( FileFindHandle_t handle ) = 0;
	virtual void FindClose( FileFindHandle_t handle ) = 0;
	virtual const char *FindFirstEx( 
		const char *pWildCard, 
		const char *pPathID,
		FileFindHandle_t *pHandle
		) = 0;*/
};

void* Sys_GetInterface(const char* pModule,const char* pInterface);

#endif
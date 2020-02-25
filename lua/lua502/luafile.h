#ifndef __LUAFILE_H
#define __LUAFILE_H

#include "glua.h"
#include "filesystem.h"
#include "tier1/utlvector.h"
#include "lzma/lzma.h"

#include "lapi.h"
#include "lfunc.h"
#include "lmem.h"
#include "lobject.h"
#include "lopcodes.h"
#include "lstring.h"
#include "lundump.h"

#define CACHE_INDEX "cache/index.dat"
#define CACHE_PATHID "MOD"
#define CACHE_FORMAT "cache/%08X.dat"

typedef struct {
	void* m_pMem;
	size_t m_iLen;
} memarea_t;

class CLuaFile
{
public:
	enum luasrc_e {
		LUA_INTERPRET,
		LUA_CACHE,
	};

	typedef struct {
		char m_szName[MAX_PATH];
		unsigned int m_uSize;
		long m_lTime;
		//unsigned long m_ulHash;
	} cachenode_t;

	CLuaFile(lua_State* L,const char* pPath);

	luafile_err_t Load(const char** ppErr);
	void LoadFromFile();
	bool LoadFromCache(const char** pErr);
	const char* Compile(memarea_t* pMemArea);
	bool IsCached();
	void Compress(memarea_t* pMemArea);
	bool Uncompress(size_t* uLen);
	void DumpCache(memarea_t* pMemArea);
	void ReUpdate();
	void Execute();

	cachenode_t m_CacheNode;
	unsigned char* m_pMem;
	lua_State* L;
	int m_iOldCacheNode;
	bool m_bCached;

	//static unsigned long Hash(unsigned char* pData,size_t iLen);
	static bool Init();
	static void ClearCache();
	static bool UpdateCacheFile();
	static CUtlVector<cachenode_t> s_Cache;
};

extern const char* g_pLuaFileErrors[];

#endif
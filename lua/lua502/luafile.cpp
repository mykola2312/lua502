#include "luafile.h"
#include "plugin.h"
#include "tier1/bitbuf.h"
#include "tier0/memdbgon.h"

const char* g_pLuaFileErrors[] = {
	"LUAFILE_OK",
	"LUAFILE_MEM_FAULT",
	"LUAFILE_LUA_ERROR",
	"LUAFILE_DUMP_FAULT",
	"LUAFILE_LFC_FAULT"
};

CUtlVector<CLuaFile::cachenode_t> CLuaFile::s_Cache;

bool CLuaFile::Init()
{
	if(!filesystem->FileExists("cache/",CACHE_PATHID))
		filesystem->CreateDirHierarchy("cache/",CACHE_PATHID);
	if(filesystem->FileExists(CACHE_INDEX,CACHE_PATHID))
	{
		FileHandle_t fCache;
		unsigned int uSize;
		unsigned short nNum;
		void* pMem;

		if(!(fCache = filesystem->Open(CACHE_INDEX,"rb",CACHE_PATHID)))
		{
			Warning("Failed to open " CACHE_INDEX "\n");
			return false;
		}
		if(!(uSize = filesystem->Size(fCache))) return true;
		if(!(pMem = malloc(uSize))) return false;

		filesystem->Read(pMem,uSize,fCache);
		filesystem->Close(fCache);

		bf_read bf = bf_read(pMem,uSize);
		nNum = bf.ReadShort();
		if(!nNum)
		{
			free(pMem);
			Msg("Cache file is empty o.0\n");
			return true;
		}

		for(unsigned short i = 0; i < nNum; i++)
		{
			cachenode_t cache;

			bf.ReadString(cache.m_szName,MAX_PATH);
			cache.m_uSize = bf.ReadLong();
			cache.m_lTime = bf.ReadLong();

			s_Cache.AddToTail(cache);
		}

		free(pMem);
	}
	return true;
}

void CLuaFile::ClearCache()
{
	char szFileName[MAX_PATH];
	for(int i = 0; i < s_Cache.Size(); i++)
	{
		sprintf(szFileName,CACHE_FORMAT,s_Cache[i].m_lTime);
		if(filesystem->FileExists(szFileName,CACHE_PATHID))
			filesystem->RemoveFile(szFileName,CACHE_PATHID);
	}
	s_Cache.RemoveAll();
	filesystem->RemoveFile(CACHE_INDEX,CACHE_PATHID);
}

/*unsigned long CLuaFile::Hash(unsigned char* pData,size_t iLen)
{
	unsigned long h = 5381;
	while(iLen--)
		h+=(h<<5)+*pData++;
	return h;
}*/

CLuaFile::CLuaFile(lua_State* pL,const char* pPath)
{
	if(!filesystem->FileExists(pPath,CACHE_PATHID)) return;
	V_strncpy(m_CacheNode.m_szName,pPath,MAX_PATH);
	this->L = pL;
	m_bCached = false;
}

void CLuaFile::LoadFromFile()
{
	FileHandle_t fFile;
	if(!filesystem->FileExists(m_CacheNode.m_szName))
	{
		Warning("File %s doesn't exists!\n",m_CacheNode.m_szName);
		return;
	}

	m_pMem = (unsigned char*)malloc(m_CacheNode.m_uSize+1);
	m_pMem[m_CacheNode.m_uSize] = 0;
	if(!m_pMem)
	{
		Warning("Not enough memory for lua file!\n");
		return;
	}
	if(!(fFile = filesystem->Open(m_CacheNode.m_szName,"rb",CACHE_PATHID)))
	{
		Warning("Failed to open lua file!\n");
		free(m_pMem);
		return;
	}

	filesystem->Read(m_pMem,m_CacheNode.m_uSize,fFile);
	//m_CacheNode.m_ulHash = Hash(m_pMem,m_CacheNode.m_uSize);
	filesystem->Close(fFile);
}

luafile_err_t CLuaFile::Load(const char** ppErr)
{
	char szFileName[MAX_PATH];
	m_pMem = NULL;
	m_bCached = false;
	m_iOldCacheNode = -1;
	
	//It must be initialized here, not in LoadFromFile
	m_CacheNode.m_uSize = filesystem->Size(
		m_CacheNode.m_szName,CACHE_PATHID);
	m_CacheNode.m_lTime = filesystem->GetFileTime(
		m_CacheNode.m_szName,CACHE_PATHID);
	sprintf(szFileName,CACHE_FORMAT,m_CacheNode.m_lTime);
	if(s_Cache.Size())
	{
		for(int i = 0; i < s_Cache.Size(); i++)
		{
			cachenode_t& cache = s_Cache[i];
			/*if(!V_strcmp(cache.m_szName,m_CacheNode.m_szName)
				&& cache.m_lTime == m_CacheNode.m_lTime)
					m_bCached = true;*/
			if(!V_strcmp(cache.m_szName,m_CacheNode.m_szName))
			{
				m_bCached = true;
				m_iOldCacheNode = ((cache.m_lTime==m_CacheNode.m_lTime)
					&&(cache.m_uSize==m_CacheNode.m_uSize))?-1:i;
				break;
			}
		}
	}
	else m_bCached = false;
	/*if(!m_pMem) LoadFromFile();
				m_bCached = (cache.m_ulHash == m_CacheNode.m_ulHash);*/

	//if m_Cached = false or need update of cached
	if(!m_bCached||m_iOldCacheNode!=-1)
	{
		memarea_t luaCp;
		LoadFromFile();
		if(!m_pMem)
		{
			Warning("FATAL: m_pMem = NULL!\n");
			return LUAFILE_MEM_FAULT;
		}
		luaCp.m_pMem = NULL;
		luaCp.m_iLen = 0;
		const char* pErr = Compile(&luaCp); //In m_pMem must be lua code!
		free(m_pMem); //Free lua code, 'cuz we have compiled lua
		if(pErr)
		{
			Warning("Lua Error: %s\n",pErr);
			return LUAFILE_LUA_ERROR;
		}
		if(!luaCp.m_pMem)
		{
			Warning("lua_dump FATAL ERROR!!!!!\n");
			return LUAFILE_DUMP_FAULT;
		}
		DumpCache(&luaCp);
		free(luaCp.m_pMem);
		//We ready to launch code
	}
	else if(m_iOldCacheNode==-1) //Run from cache if EXACLY same file
	{
		Msg("Found in cache!\n");
		if(!LoadFromCache(ppErr))
		{
			Warning("LoadFromCache failed\n");
			return LUAFILE_LFC_FAULT;
		}
	}
	return LUAFILE_OK;
}

bool CLuaFile::LoadFromCache(const char** pErr)
{
	FileHandle_t fLua;
	char szFileName[MAX_PATH]= {0};
	//sprintf(szFileName,CACHE_FORMAT,m_CacheNode.m_lTime);
	sprintf_s<MAX_PATH>(szFileName,CACHE_FORMAT,m_CacheNode.m_lTime);
	size_t uSize;
	if(!filesystem->FileExists(szFileName,CACHE_PATHID))
	{
		Warning("Cache file %s not found\n",szFileName);
		return false;
	}
	if(!(uSize = filesystem->Size(szFileName,CACHE_PATHID)))
	{
		Warning("Lua cache file is empty!\n");
		return false;
	}
	if(!(m_pMem = (unsigned char*)malloc(uSize)))
	{
		Warning("Allocate memory for lua cache failed!\n");
		return false;
	}
	if(!(fLua = filesystem->Open(szFileName,"rb",CACHE_PATHID)))
	{
		Warning("Failed to open file %s!\n",szFileName);
		return false;
	}
	filesystem->Read(m_pMem,uSize,fLua);
	filesystem->Close(fLua);

	if(!Uncompress(&uSize))
	{
		Warning("Uncompress failed!\n");
		return false;
	}
	//m_pMem holds lua bytecode
	if(luaL_loadbuffer(L,(const char*)m_pMem,uSize,"CLuaFileCached"))
	{
		if(pErr) *pErr = lua_tostring(L,-1);
		lua_pop(L,1);
		free(m_pMem);
		return false;
	}

	free(m_pMem);
	return true;
}

void CLuaFile::Compress(memarea_t* pMemArea)
{
	//We don't need compression for small files
	if(pMemArea->m_iLen<1024) return;
	if(!pMemArea->m_pMem) return;
	size_t uLen;
	unsigned char* pCompressed;
	if((pCompressed=LZMA_Compress((unsigned char*)pMemArea->m_pMem,pMemArea->m_iLen,
		&uLen)))
	{
		free(pMemArea->m_pMem);
		pMemArea->m_pMem = pCompressed;
		pMemArea->m_iLen = uLen;
	}
}

bool CLuaFile::Uncompress(size_t* uLen)
{
	//m_pMem holds compressed stuff
	if(!LZMA_IsCompressed(m_pMem)) return true;
	unsigned char* pMem;
	if(!LZMA_Uncompress(m_pMem,&pMem,uLen))
		return false;
	else
	{
		free(m_pMem);
		m_pMem = pMem;
	}
	return true;
}

void CLuaFile::DumpCache(memarea_t* pMemArea)
{
	char szFileName[MAX_PATH];
	sprintf(szFileName,CACHE_FORMAT,m_CacheNode.m_lTime);

	FileHandle_t fCacheFile;
	if(!(fCacheFile = filesystem->Open(szFileName,"wb",CACHE_PATHID)))
	{
		Warning("Failed to open %s!\n",szFileName);
		return;
	}

	Compress(pMemArea);
	filesystem->Write(pMemArea->m_pMem,pMemArea->m_iLen,fCacheFile);
	filesystem->Close(fCacheFile);

	if(m_iOldCacheNode!=-1)
	{
		Msg("Updating %s...\n",s_Cache[m_iOldCacheNode].m_szName);
		ReUpdate();
	}
	s_Cache.AddToTail(m_CacheNode);
	if(!UpdateCacheFile())
		Warning("Failed to update "CACHE_INDEX,"\n");
}

static int writer(lua_State* L, const void* p, size_t size, void* u)
{
 UNUSED(L);
 memarea_t* pMemArea = (memarea_t*)u;
 if(!pMemArea->m_pMem) pMemArea->m_pMem = malloc(size);
 else pMemArea->m_pMem = realloc(pMemArea->m_pMem,pMemArea->m_iLen+size);
 memcpy(((char*)pMemArea->m_pMem+pMemArea->m_iLen),p,size);
 pMemArea->m_iLen += size;
 return size != 0;
}

/*
static Proto* toproto(lua_State* L, int i)
{
 const Closure* c=(const Closure*)lua_topointer(L,i);
 return c->l.p;
}*/

const char* CLuaFile::Compile(memarea_t* pMemArea)
{
	//m_pMem holds lua code
	const char* pErr = NULL;

	if(luaL_loadbuffer(L,(const char*)m_pMem,V_strlen((const char*)m_pMem),"CLuaFile"))
	{
		pErr = lua_tostring(L,-1);
		lua_pop(L,1);
		return pErr;
	}

	/*Proto* f = toproto(L,-1);
	luaU_dump(L,f,writer,pMemArea);*/
	lua_dump(L,writer,pMemArea);
	return pErr;
}

bool CLuaFile::UpdateCacheFile()
{
	int iSize;
	if(!(iSize=s_Cache.Size())) return true;
	FileHandle_t fCache;
	if(!(fCache = filesystem->Open(CACHE_INDEX,"wb",CACHE_PATHID)))
	{
		Warning("CLuaFile::UpdateCacheFile file "CACHE_INDEX" unavaible\n");
		return false;
	}

	//cachenode_t* pCacheNode = (cachenode_t*)malloc(sizeof(cachenode_t));
	size_t uMaxSize = s_Cache.Size()*sizeof(cachenode_t);
	cachenode_t* pCacheNodes = (cachenode_t*)malloc(uMaxSize);
	bf_write bf = bf_write(pCacheNodes,uMaxSize);
	bf.WriteShort(iSize);
	for(int i = 0; i < iSize; i++)
	{
		cachenode_t& cache = s_Cache[i];
		bf.WriteString(cache.m_szName);
		bf.WriteLong(cache.m_uSize);
		bf.WriteLong(cache.m_lTime);
	}

	filesystem->Write(bf.GetData(),bf.GetNumBytesWritten(),fCache);
	filesystem->Close(fCache);
	free(pCacheNodes);
	return true;
}

void CLuaFile::ReUpdate()
{
	char szFileName[MAX_PATH];
	sprintf(szFileName,CACHE_FORMAT,s_Cache[m_iOldCacheNode].m_lTime);
	if(filesystem->FileExists(szFileName,CACHE_PATHID))
		filesystem->RemoveFile(szFileName,CACHE_PATHID);
	s_Cache.Remove(m_iOldCacheNode);
}

void CLuaFile::Execute()
{
	lua_pushcfunction(L,lua502_aterror);
	lua_insert(L,-2);
	lua_pcall(L,0,LUA_MULTRET,lua_gettop(L)-1);
	lua_pop(L,1);
}
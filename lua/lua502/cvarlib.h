#ifndef __CVARLIB_H
#define __CVARLIB_H

#include "glua.h"
#include "tier1/convar.h"

typedef struct {
	union {
		ConCommandBase* m_pBase;
		ConVar* m_pConVar;
		ConCommand* m_pCmd;
	};
	bool m_bCustom;
} cmdbase_ud_t;

cmdbase_ud_t* luaf_makeconvar(lua_State* L,ConVar* pConVar);
cmdbase_ud_t* luaf_makecommand(lua_State* L,ConCommand* pCommand);
cmdbase_ud_t* luaf_makecmdbase(lua_State* L,ConCommandBase* pBase);

class CLuaConCommand : public ConCommand
{
public:
	CLuaConCommand(char const *pName, lua_State* L, int iRefID, 
									char const *pHelpString = 0, int flags = 0, FnCommandCompletionCallback completionFunc = 0);

	virtual void Dispatch(void);
private:
	lua_State* m_pL;
	int m_iRefID;
};

#endif
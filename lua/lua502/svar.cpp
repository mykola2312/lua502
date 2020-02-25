#include "svar.h"


class SCvarAccessor : public IConCommandBaseAccessor
{
public:
	virtual bool RegisterConCommandBase(ConCommandBase* pCmd)
	{
		Msg("Registering %s\n",pCmd->GetName());

		pCmd->AddFlags(FCVAR_PLUGIN);
		pCmd->SetNext(NULL);
		cvar->RegisterConCommandBase(pCmd);
		return true;
	}
};

static SCvarAccessor g_SCvarAccessor;

void ConVar_Register()
{
	ConCommandBaseMgr::OneTimeInit(&g_SCvarAccessor);
	Msg("%s\n",ConCommandBase::s_pConCommandBases->IsRegistered() ? "registered" : "not-registered");
	Msg("ICvar::RegisterConCommandBase %p\n",(*(void***)g_pCVar)[5]);
}
#ifndef __MAIN_H
#define __MAIN_H

#define GAME_DLL
#include "cbase.h"

typedef struct {
	int m_iAccount;
	int m_iType;
	EHANDLE m_hEntity;
} object_t;

DLL_EXPORT int GetObjectOwner(EHANDLE hObject);
DLL_EXPORT int GetPlayerAccountId(edict_t* pPly);
DLL_EXPORT CUtlVector<object_t>* GetAllObjects();

#endif
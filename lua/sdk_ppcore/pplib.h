#ifndef __PPLIB_H
#define __PPLIB_H

#define GAME_DLL
#include "cbase.h"
typedef CUtlVector<EHANDLE > EntityVector_t;

enum {
	Ragdolls = 0,
	Props,
	Balloons,
	Effects,
	Sprites,
	Emitters,
	Wheels,
	Npcs,
	Dynamites,
	Thrusters,
	MaxType,
};

DLL_EXPORT EntityVector_t* GetProperty(edict_t* pPly,int iType);
DLL_EXPORT void FixupProperty(EntityVector_t* pProperty);

#endif
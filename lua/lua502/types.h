#ifndef __TYPES_H

#include "lua.h"

typedef enum {
	POINTER = 0,
	LUAPLUGIN,
	CONVAR,
	COMMAND,
} luatypes_t;

LUA_API const char* luaf_typename(int type);

#endif
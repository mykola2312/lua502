#include "types.h"

static char* s_TypeNames[] = {
	"Pointer",
	"LuaPluign",
	"ConVar",
	"Command"
};

LUA_API const char* luaf_typename(int type)
{
	return s_TypeNames[type];
}
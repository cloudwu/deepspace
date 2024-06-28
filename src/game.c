#define LUA_LIB

#include <stdint.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"

static int
ltest(lua_State *L) {
	lua_pushinteger(L, 42);
	return 1;
}

LUAMOD_API int
luaopen_game_core(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "test", ltest },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}
#define LUA_LIB

#include <lua.h>

#define GAME_MODULE(name) LUAMOD_API int luaopen_game_##name(lua_State *L)
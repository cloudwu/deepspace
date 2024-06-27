#define LUA_LIB

#include <stdint.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"

#define FLOOR_SIZE 256
#define FLOOR_CAP (FLOOR_SIZE * FLOOR_SIZE)
#define FLOOR_EMPTY -1

struct floor_tile {
	uint8_t x;
	uint8_t y;
};

struct floor_map {
	int map[FLOOR_SIZE][FLOOR_SIZE];
	int size;
	int changes;
	struct floor_tile set[FLOOR_CAP];
};

static int
lfloor_len(lua_State *L) {
	struct floor_map *m = (struct floor_map *)lua_touserdata(L, 1);
	lua_pushinteger(L, m->size);
	return 1;
}

static int
lfloor_index(lua_State *L) {
	struct floor_map *m = (struct floor_map *)lua_touserdata(L, 1);
	int idx = luaL_checkinteger(L, 2);
	if (idx < 1 || idx > m->changes * 2)
		return 0;
	--idx;
	struct floor_tile * t = &m->set[idx / 2];
	if (idx & 1) {
		lua_pushinteger(L, t->y);
	} else {
		lua_pushinteger(L, t->x);
	}
	return 1;
}

static inline int
reset(struct floor_map *m) {
	m->changes = 0;
	return 0;
}

static int
lfloor_set(lua_State *L) {
	struct floor_map *m = (struct floor_map *)lua_touserdata(L, 1);
	if (lua_gettop(L) == 1) {
		return reset(m);
	}
	int x = luaL_checkinteger(L, 2);
	int y = luaL_checkinteger(L, 3);
	int set = lua_toboolean(L, 4);
	if (x < 0 || x >= FLOOR_SIZE) {
		return luaL_error(L, "Invalid x = %d", x);
	}
	if (y < 0 || y >= FLOOR_SIZE) {
		return luaL_error(L, "Invalid y = %d", y);
	}
	int idx = m->map[x][y];
	if (idx < 0) {
		// It's empty
		if (set) {
			
		}
		return 0;
	} else {
		// set
		if (!set
			--m->size;
			struct floor_tile * last = m->set[m->size];
			m->map[last->x][last->y] = idx;
			m->set[idx] = *last;
	}
}

static int
lfloor(lua_State *L) {
	struct floor_map * m = (struct floor_map *)lua_newuserdatauv(L, sizeof(struct floor_map), 0);
	if (luaL_newmetatable(L, "GAME_FLOOR")) {
		luaL_Reg l[] = {
			{ "__len", lfloor_len},
			{ "__call", lfloor_set },
			{ "__index", lfloor_index },
			{ NULL, NULL },
		};
		luaL_setfuncs(L, l, 0);
	}
	m->size = 0;
	m->changes = 0;
	int i,j;
	for (i=0;i<FLOOR_SIZE;i++) {
		for (j=0;j<FLOOR_SIZE;j++) {
			m->map[i][j] = FLOOR_EMPTY;
		}
	}
	return 1;
}

LUAMOD_API int
luaopen_game_core(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "floor", lfloor },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}
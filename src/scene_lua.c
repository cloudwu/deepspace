#define LUA_LIB

#include <lua.h>
#include <lauxlib.h>
#include <string.h>
#include <assert.h>
#include "scene.h"

static int
getfield(lua_State *L, int idx, const char *key) {
	int t = lua_getfield(L, idx, key);
	if (t == LUA_TNIL) {
		return luaL_error(L, "No .%s for %d", key, idx);
	} else if (lua_isinteger(L, -1)) {
		int	r = lua_tointeger(L, -1);
		lua_pop(L, 1);
		return r;
	}
	return luaL_error(L, "Need integer for .%s, It's %s", key, lua_typename(L, t));
}

static size_t
count_size(int layer, int x, int y) {
	size_t meta = sizeof(struct scene) + (layer - 1) * sizeof(slot_t *);
	meta += layer * (x * y) * sizeof(slot_t);
	return meta;
}

static slot_t *
layer_ptr(struct scene *s, int n) {
	size_t meta_size = sizeof(struct scene) + (s->layer_n - 1) * sizeof(slot_t *);
	assert(n >= 0 && n < s->layer_n);
	char * ptr = (char *)s + meta_size;
	ptr += (n << s->shift_x) * s->y * sizeof(slot_t);
	return (slot_t *)ptr;
}

static int
lscene_info(lua_State *L) {
	struct scene *s = (struct scene *)luaL_checkudata(L, 1, "SCENE");
	lua_newtable(L);
	lua_pushinteger(L, 1 << s->shift_x);
	lua_setfield(L, -2, "x");
	lua_pushinteger(L, s->y);
	lua_setfield(L, -2, "y");
	lua_pushinteger(L, s->layer_n);
	lua_setfield(L, -2, "layer");
	return 1;
}

static slot_t *
get_xy(lua_State *L, struct scene *s,  int index, int *ox, int *oy, int *on) {
	int x = luaL_checkinteger(L, index + 1);
	int y = luaL_checkinteger(L, index + 2);
	int n = luaL_checkinteger(L, index + 3);
	if (x < 0 || x >= (1 << s->shift_x) || y < 0 || y >= s->y)
		luaL_error(L, "Invalid (%d x %x) in (%d x %d)", x, y, 1 << s->shift_x, s->y);
	if (n < 0 || n >= s->layer_n)
		luaL_error(L, "Invalid layer %d/%d", n, s->layer_n);
	*ox = x;
	*oy = y;
	*on = n;
	return s->layer[n];
}

static inline struct scene *
get_scene(lua_State *L) {
	return (struct scene *)luaL_checkudata(L, 1, "SCENE");
}

static int
lscene_get(lua_State *L) {
	struct scene *s = get_scene(L);
	int x, y, layer;
	slot_t * grid = get_xy(L, s, 1, &x, &y, &layer);
	lua_pushinteger(L, grid[ (y << s->shift_x) + x]);
	return 1;
}

static int
lscene_set(lua_State *L) {
	struct scene *s = get_scene(L);
	int x, y, layer;
	int v = luaL_checkinteger(L, 2);
	slot_t * grid = get_xy(L, s, 2, &x, &y, &layer);
	int index = (y << s->shift_x) + x;
	lua_pushinteger(L, grid[ index ]);
	grid[ index ] = (slot_t)v;
	return 1;
}

static int
lscene_pathmap(lua_State *L) {
	struct scene *s = get_scene(L);
	luaL_checktype(L, 2, LUA_TTABLE);
	int layer = getfield(L, 2, "block");
	int target = getfield(L, 2, "target");
	struct scene_coord pos;
	pos.x = getfield(L, 2, "x");
	pos.y = getfield(L, 2, "y");
	scene_pathmap(s, layer, pos, target);
	return 0;
}

#define MAX_STEP 4096

static int
lscene_path(lua_State *L) {
	struct scene *s = get_scene(L);
	luaL_checktype(L, 2, LUA_TTABLE);
	int layer = getfield(L, 2, "layer");
	struct scene_coord pos;
	pos.x = getfield(L, 2, "x");
	pos.y = getfield(L, 2, "y");
	struct scene_coord output[MAX_STEP];
	int dist = scene_path(s, layer, pos, MAX_STEP, output);
	int i=0;
	if (dist > 0) {
		for (i=0;i<MAX_STEP-1;i++) {
			if (output[i].x == pos.x && output[i].y == pos.y)
				break;
		}
		int n = i;
		for (i=0;i<=n;i++) {
			lua_pushinteger(L, output[n-i].x);
			lua_rawseti(L, 2, i*2+1);
			lua_pushinteger(L, output[n-i].y);
			lua_rawseti(L, 2, i*2+2);
		}
	}
	int n = lua_rawlen(L, 2);
	i = i*2+1;
	while (i <= n) {
		lua_pushnil(L);
		lua_rawseti(L, 2, i);
		++i;
	}
	lua_pushinteger(L, dist);
	return 1;
}

static int
lscene_build(lua_State *L) {
	struct scene *s = get_scene(L);
	luaL_checktype(L, 2, LUA_TTABLE);
	int layer = getfield(L, 2, "layer");
	int n = lua_rawlen(L, 2);
	struct scene_coord tmp[MAX_STEP];
	struct scene_coord *p = tmp;
	if (n % 2 != 0)
		return luaL_error(L, "Invalid length %d", n);
	n /= 2;
	if (n > MAX_STEP) {
		p = (struct scene_coord *)lua_newuserdatauv(L, n * sizeof(*s), 0);
	}
	int i;
	for (i=0;i<n;i++) {
		if (lua_rawgeti(L, 2, i*2+1) != LUA_TNUMBER)
			return luaL_error(L, "Invalid coord at %d", i*2+1);
		p[i].x = lua_tointeger(L, -1);
		lua_pop(L, 1);
		if (lua_rawgeti(L, 2, i*2+2) != LUA_TNUMBER)
			return luaL_error(L, "Invalid coord at %d", i*2+2);
		p[i].y = lua_tointeger(L, -1);
		lua_pop(L, 1);
	}
	scene_build(s, layer, n, p);
	return 0;
}

static int
shift(int v) {
	int n = 0;
	int t = 1;
	while (t < v) {
		t *= 2;
		++n;
	}
	return n;
}

static int
lscene_new(lua_State *L) {
	luaL_checktype(L, 1, LUA_TTABLE);
	int x = getfield(L, 1, "x");
	int y = getfield(L, 1, "y");
	int layer = getfield(L, 1, "layer");
	if (layer < 1)
		return luaL_error(L, "At least layer 1 (%d)", layer);
	
	x = shift(x);
	
	size_t sz = count_size(layer, 1 << x, y);
	struct scene * s = (struct scene *)lua_newuserdatauv(L, sz, 0);
	memset(s, 0, sz);
	s->shift_x = x;
	s->y = y;
	s->layer_n = layer;
	int i;
	for (i=0;i<layer;i++) {
		s->layer[i] = layer_ptr(s, i);
	}
	if (luaL_newmetatable(L, "SCENE")) {
		luaL_Reg l[] = {
			{ "info", lscene_info },
			{ "get", lscene_get },
			{ "set", lscene_set },
			{ "pathmap", lscene_pathmap },
			{ "path", lscene_path },
			{ "build", lscene_build },
			{ "__index", NULL },
			{ NULL, NULL },
		};
		luaL_setfuncs(L, l, 0);
		lua_pushvalue(L, -1);
		lua_setfield(L, -2, "__index");
	}
	lua_setmetatable(L, -2);
	return 1;
}

LUAMOD_API int
luaopen_game_scene(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "new", lscene_new },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}
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
	if (x < 0 || x >= (1 << s->shift_x) || y < 0 || y >= s->y) {
		return NULL;
	}
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

union endian_t {
	slot_t i;
	char i8[sizeof(slot_t)];
};

static inline int
little_endian() {
	union endian_t u;
	u.i = 1;
	return u.i8[0] == 1;
}

static inline size_t
convert_endian(slot_t s) {
	union endian_t u;
	u.i = s;
	union endian_t r;
	int i;
	for (i=0;i<sizeof(slot_t);i++) {
		r.i8[i] = u.i8[sizeof(slot_t) - i - 1];
	}
	return r.i;
}

struct rect {
	int left;
	int right;
	int top;
	int bottom;
};

static void
bounding_rect(const slot_t *s, int w, int h, struct rect *output) {
	int i,j;
	
	output->left = w;
	output->right = 0;
	output->top = -1;
	output->bottom = 0;
	
	for (i=0;i<h;i++) {
		int left = w, right = 0;
		for (j=0;j<w;j++) {
			if (*s != 0) {
				if (left == w) {
					left = j;
				}
				right = j;
			}
			++s;
		}
		if (left < output->left)
			output->left = left;
		else if (left == w) {
			// empty line
			if (output->bottom == 0)
				output->top = i;
		} else {
			output->bottom = i;
		}
		if (right > output->right)
			output->right = right;
	}
}

static int
lscene_export(lua_State *L) {
	struct scene *s = get_scene(L);
	int n = luaL_checkinteger(L, 2);
	if (n < 0 || n >= s->layer_n)
		luaL_error(L, "Invalid layer %d/%d", n, s->layer_n);
	const slot_t *ptr = s->layer[n];
	struct rect r;
	bounding_rect(ptr, 1 << s->shift_x, s->y, &r);
	if (r.bottom == 0) {
		// empty
		return 0;
	}
//	printf("top = %d bottom = %d left = %d right = %d\n", r.top, r.bottom, r.left, r.right);
	int w = r.right - r.left + 1;
	int h = r.bottom - r.top + 1;
	slot_t *buffer = lua_newuserdatauv(L, w * h * sizeof(slot_t), 0);
	slot_t *tmp = buffer;
	int i,j;
	ptr += r.top << s->shift_x;
	if (little_endian()) {
		for (i=0;i<h;i++) {
			for (j=0;j<w;j++) {
				*tmp = ptr[r.left + j];
				++tmp;
			}
			ptr += 1 << s->shift_x;
		}
	} else {
		for (i=0;i<h;i++) {
			for (j=0;j<w;j++) {
				*tmp = convert_endian(ptr[r.left + j]);
				++tmp;
			}
			ptr += 1 << s->shift_x;
		}
	}
	lua_pushlstring(L, (const char *)buffer, w * h * sizeof(slot_t));
	lua_pushinteger(L, r.left);
	lua_pushinteger(L, r.top);
	lua_pushinteger(L, w);
	lua_pushinteger(L, h);
	
	return 5;
}

static int
lscene_import(lua_State *L) {
	struct scene *s = get_scene(L);
	int n = luaL_checkinteger(L, 2);
	if (n < 0 || n >= s->layer_n)
		return luaL_error(L, "Invalid layer %d/%d", n, s->layer_n);
	int x = luaL_checkinteger(L, 3);
	int y = luaL_checkinteger(L, 4);
	int w = luaL_checkinteger(L, 5);
	int h = luaL_checkinteger(L, 6);
	if (x + w > (1 << s->shift_x)) {
		return luaL_error(L, "Invalid width (%d + %d > %d)", x, w, (1 << s->shift_x));
	}
	if (y + h > s->y) {
		return luaL_error(L, "Invalid height (%d + %d > %d)", y, h, s->y);
	}
	size_t sz;
	const char * data = luaL_checklstring(L, 3, &sz);
	if (sz != w * h) {
		return luaL_error(L, "Invalid size (%d * %d != %d)", w, h, (int)sz);
	}
	slot_t *ptr = s->layer[n];
	const slot_t *src = (const slot_t *)data;
	memset(ptr, 0, s->y << s->shift_x);
	int i,j;
	ptr += y << s->shift_x;
	if (little_endian()) {
		for (i=0;i<h;i++) {
			for (j=0;j<w;j++) {
				ptr[x + j] = *src;
				++src;
			}
			ptr += 1 << s->shift_x;
		}
	} else {
		for (i=0;i<h;i++) {
			for (j=0;j<w;j++) {
				ptr[x + j] = convert_endian(*src);
				++src;
			}
			ptr += 1 << s->shift_x;
		}
	}
	return 0;
}

static int
lscene_clear(lua_State *L) {
	struct scene *s = get_scene(L);
	int n = luaL_checkinteger(L, 2);
	if (n < 0 || n >= s->layer_n)
		luaL_error(L, "Invalid layer %d/%d", n, s->layer_n);
	slot_t *ptr = s->layer[n];
	memset(ptr, 0, (1 << s->shift_x) * s->y * sizeof(slot_t));
	return 0;
}

static int
lscene_get(lua_State *L) {
	struct scene *s = get_scene(L);
	int x, y, layer;
	slot_t * grid = get_xy(L, s, 1, &x, &y, &layer);
	if (grid == NULL)
		return 0;
	lua_pushinteger(L, grid[ (y << s->shift_x) + x]);
	return 1;
}

static int
lscene_set(lua_State *L) {
	struct scene *s = get_scene(L);
	int x, y, layer;
	int v = luaL_checkinteger(L, 2);
	slot_t * grid = get_xy(L, s, 2, &x, &y, &layer);
	if (grid == NULL) {
		luaL_error(L, "Invalid (%d x %d) in (%d x %d)", x, y, 1 << s->shift_x, s->y);
	}
	int index = (y << s->shift_x) + x;
	lua_pushinteger(L, grid[ index ]);
	grid[ index ] = (slot_t)v;
	return 1;
}

#define MAX_STEP 4096

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
lscene_debug(lua_State *L) {
	struct scene *scene = get_scene(L);
	int layer = luaL_checkinteger(L, 2);
	if (layer < 0 || layer >= scene->layer_n)
		return luaL_error(L, "Invalid layer = %d", layer);
	
	luaL_Buffer b;
	luaL_buffinit(L, &b);
	slot_t *s = scene->layer[layer];

	int i,j;
	int x = 1 << scene->shift_x;
	for (i=0;i<scene->y;i++) {
		for (j=0;j<x;j++) {
			char tmp[32];
			snprintf(tmp, sizeof(tmp), "%4d", *s++);
			luaL_addstring(&b, tmp);
		}
		luaL_addchar(&b, '\n');
	}
	luaL_pushresult(&b);
	return 1;
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
			{ "import", lscene_import },
			{ "export", lscene_export },
			{ "clear", lscene_clear },
			{ "get", lscene_get },
			{ "set", lscene_set },
			{ "debug", lscene_debug },
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
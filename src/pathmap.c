#include <lua.h>
#include <lauxlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "scene.h"
#include "lgame.h"

#define PATHMAP_LUAKEY "GAME_PATHMAP"
#define CACHE_SLOTS 31

struct pathmap {
	int x;
	int y;
	int shift;
	uint64_t cache_index[CACHE_SLOTS];
	slot_t *layer;
	struct scene cache;
};

static size_t
pathmap_size(int x, int y) {
	int layer = CACHE_SLOTS;
	size_t meta = sizeof(struct pathmap) + (layer - 1) * sizeof(slot_t *);
	meta += layer * (x * y) * sizeof(slot_t);
	return meta;
}

static inline void
check_layer(struct scene *S, int layer) {
	assert(layer >= 0 && layer < S->layer_n);	
}

struct pathmap_context {
	int x;
	int y;
	int shift;
	int id;
	slot_t * block;
	slot_t * target;
};

static inline slot_t *
get_coord(struct pathmap_context *C, slot_t *layer, struct scene_coord p) {
	if (p.x < 0 || p.y < 0 || p.x >= C->x || p.y >= C->y)
		return NULL;
	return layer + (p.y << C->shift) + p.x;
}

static void
pathmap_context_init(struct pathmap_context *C, struct pathmap *P, int target_layer) {
	C->x = P->x;
	C->y = P->y;
	C->shift = P->shift;
	check_layer(&P->cache, target_layer);
	C->block = P->layer;
	C->target = P->cache.layer[target_layer];
	C->id = 0;
}

static const int neighbor[8*3] = {
	 0, -1, 5,	// N
	-1,  0, 5,	// W
	 1,  0,	5,	// E
	 0,  1,	5,	// S
	-1, -1, 7,	// NW
	 1, -1, 7,	// NE
	-1,  1, 7,	// SW
	 1,  1,	7,	// SE
};

static inline int
is_block(struct pathmap_context *C, int x, int y) {
	return C->block[(y << C->shift) + x] != C->id;
}

static void
render(struct pathmap_context *C, struct scene_coord pos, int dist) {
	slot_t * s = get_coord(C, C->block, pos);
	if (s == NULL || *s != C->id)
		return;
	slot_t * t = s + (C->target - C->block);
	if (*t != 0 && *t <= dist)
		return;
	*t = dist;
	int i;
	for (i=0;i<8*3;i+=3) {
		struct scene_coord t = pos;
		if (neighbor[i+2] == 7) {
			// test diagonal
			if (!is_block(C, pos.x, pos.y + neighbor[i+1]) ||
				!is_block(C, pos.x + neighbor[i+0], pos.y)) {
				t.x += neighbor[i+0];
				t.y += neighbor[i+1];
				render(C, t, dist + neighbor[i+2]);
			}
		} else {
			t.x += neighbor[i+0];
			t.y += neighbor[i+1];
			render(C, t, dist + neighbor[i+2]);		
		}
	}
}

static void
gen_pathmap(struct pathmap *P, int n, struct scene_coord pos[], int target_layer) {
	struct pathmap_context C;
	pathmap_context_init(&C, P, target_layer);
	memset(C.target, 0, C.x * C.y * sizeof(slot_t));
	if (n == 0)
		return;
	int i;
	for (i=0;i<n;i++) {
		slot_t * s = get_coord(&C, C.block, pos[i]);
		if (s) {
			C.id = *s;
			render(&C, pos[i], 1);
		}
	}
}

static inline slot_t *
get_dist(struct scene *S, int layer, struct scene_coord p) {
	if (p.x < 0 || p.y < 0 || p.x >= (1 << S->shift_x) || p.y >= S->y)
		return NULL;
	return S->layer[layer] + (p.y << S->shift_x) + p.x;
}

static inline int
nearest(struct scene *S, int layer, struct scene_coord *p, int dist) {
	int i;
	struct scene_coord c = *p;
	int min_dist = dist;
	int stride = 1 << S->shift_x;
	for (i=0;i<8*3;i+=3) {
		struct scene_coord t = c;
		t.x += neighbor[i+0];
		t.y += neighbor[i+1];
		slot_t *s = get_dist(S, layer, t);
		if (s != NULL && *s != 0 && *s < min_dist) {
			if (neighbor[i+2] == 7) {
				// test diagonal
				slot_t *n1 = s - neighbor[i+0];
				slot_t *n2 = s - neighbor[i+1] * stride;
				if (*n1 != 0 && *n2 != 0) {
					min_dist = *s;
					*p = t;
				}
			} else {
				min_dist = *s;
				*p = t;
			}
		}
	}
	return min_dist;
}

static inline int
block(struct scene *S, int layer, struct scene_coord p0, struct scene_coord p1) {
	int x1, x2, y1, y2;
	if (p0.x > p1.x) {
		x1 = p1.x;
		x2 = p0.x;
	} else {
		x1 = p0.x;
		x2 = p1.x;
	}

	if (p0.y > p1.y) {
		y1 = p1.y;
		y2 = p0.y;
	} else {
		y1 = p0.y;
		y2 = p1.y;
	}
	
	int shift = S->shift_x;
	
	slot_t *s = S->layer[layer] + (y1 << shift);
	
	int i,j;
	for (i=y1;i<=y2;i++) {
		for (j=x1;j<=x2;j++) {
			if (s[j] == 0)
				return 1;
		}
		s += 1 << shift;
	}
	return 0;
}

// 0 : term
static int
find_next(struct scene *S, int layer, struct scene_coord *inout) {
	slot_t *s = S->layer[layer] + (inout->y << S->shift_x) + inout->x;
	int dist = *s;
	struct scene_coord c = *inout;
	dist = nearest(S, layer, &c, dist);
	if (c.x == inout->x && c.y == inout->y)
		return 0;
	struct scene_coord last_noblock = c;
	for (;;) {
		int next_dist = nearest(S, layer, &c, dist);
		if (next_dist == dist || block(S, layer, *inout, c)) {
			break;
		} else {
			last_noblock = c;
			dist = next_dist;
		}
	}
	*inout = last_noblock;
	return 1;
}	

static int
get_path(struct pathmap *P, int layer, struct scene_coord target, int n, struct scene_coord p[]) {
	check_layer(&P->cache, layer);
	if (target.x < 0 || target.y < 0 || target.x >= P->x || target.y >= P->y)
		return 0;
	slot_t * slot = get_dist(&P->cache, layer, target);
	if (slot == NULL)
		return 0;
	int dist = *slot;
	if (dist == 0)
		return 0;
	
	int idx = n;
	struct scene_coord tmp = target;
	while (idx > 0 && find_next(&P->cache, layer, &tmp)) {
		--idx;
		p[idx] = tmp;
	}
	int len = n - idx;
	memmove(p, &p[idx], len * sizeof(p[0]));
	if (len < n) {
		p[len] = target;
	}
	return dist;
}

static struct pathmap *
getP(lua_State *L) {
	return (struct pathmap *)luaL_checkudata(L, 1, PATHMAP_LUAKEY);
}

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

#define MAX_STEP 4096

static int
path_to_table(lua_State *L, int index, struct pathmap *P, int layer, struct scene_coord pos) {
	struct scene_coord output[MAX_STEP];
	int dist = get_path(P, layer, pos, MAX_STEP, output);
	int i=0;
	if (dist > 0) {
		for (i=0;i<MAX_STEP-1;i++) {
			if (output[i].x == pos.x && output[i].y == pos.y)
				break;
		}
		int n = i;
		for (i=0;i<n;i++) {
			lua_pushinteger(L, output[i].x);
			lua_rawseti(L, index, i*2+1);
			lua_pushinteger(L, output[i].y);
			lua_rawseti(L, index, i*2+2);
		}
	}
	int n = lua_rawlen(L, index);
	i = i*2+1;
	while (i <= n) {
		lua_pushnil(L);
		lua_rawseti(L, index, i);
		++i;
	}
	lua_pushinteger(L, dist);
	return 1;
}

//  x(24bit) y (24bit) type(1bit)

static inline uint64_t
hash_key(unsigned int x, unsigned int y, int near) {
	uint64_t key = ((uint64_t)x & 0xffffff) << 25 | ((uint64_t)y & 0xffffff) << 1 | (near & 1);
	return key;
}

static int
ldebug(lua_State *L) {
	struct pathmap * P = getP(L);
	int x = luaL_checkinteger(L, 2);
	int y = luaL_checkinteger(L, 3);
	int near = lua_toboolean(L, 4);
	uint64_t key = hash_key(x, y, near);

	int index = key % CACHE_SLOTS;
	if (P->cache_index[index] != key) {
		return 0;
	}
	luaL_Buffer b;
	luaL_buffinit(L, &b);
	slot_t *s = P->cache.layer[index];

	int i,j;
	luaL_addstring(&b, "\n    ");
	for (i=0;i<P->x;i++) {
		char tmp[32];
		snprintf(tmp, sizeof(tmp), "%4d", i);
		luaL_addstring(&b, tmp);
	}
	luaL_addchar(&b, '\n');
	for (i=0;i<P->y;i++) {
		char tmp[32];
		snprintf(tmp, sizeof(tmp), "%4d", i);
		luaL_addstring(&b, tmp);
		for (j=0;j<P->x;j++) {
			char tmp[32];
			snprintf(tmp, sizeof(tmp), "%4d", *s++);
			luaL_addstring(&b, tmp);
		}
		luaL_addchar(&b, '\n');
	}
	luaL_pushresult(&b);
	return 1;
}

static void
check_xy(lua_State *L, struct pathmap *P, int x, int y) {
	if (x < 0 || x >= P->x || y < 0 || y >= P->y)
		luaL_error(L, "Invalid coord (%d,%d)", x, y);
}

static inline slot_t *
get_block(struct pathmap *P, int x, int y) {
	if (x < 0 || y < 0 || x >= P->x || y >= P->y)
		return NULL;
	return P->layer + (y << P->shift) + x;
}

static int
get_pathmap(struct pathmap *P, int x, int y, int near) {
	uint64_t key = hash_key(x, y, near);
	int index = key % CACHE_SLOTS;
	if (P->cache_index[index] != key) {
		P->cache_index[index] = key;

		slot_t * s = get_block(P, x, y);
		int n = 0;
		struct scene_coord temp[8];
		if (s) {
			if (near) {
				int i;
				n = 0;
				for (i=0;i<8;i++) {
					temp[n].x = x + neighbor[i*3+0];
					temp[n].y = y + neighbor[i*3+1];
					slot_t *ns = get_block(P, temp[n].x, temp[n].y);
					if (ns) {
						++n;
					}
				}
			} else {
				n = 1;
				temp[0].x = x;
				temp[0].y = y;
			}
		}
		gen_pathmap(P, n, temp, index);
	}
	return index;		
}

static int
lpath_(lua_State *L, int near) {
	struct pathmap * P = getP(L);
	luaL_checktype(L, 2, LUA_TTABLE);	// start point and path result
	int x = luaL_checkinteger(L, 3);	// end point x
	int y = luaL_checkinteger(L, 4);	// end point y
	check_xy(L, P, x, y);
	int index = get_pathmap(P, x, y, near);
	struct scene_coord pos;
	pos.x = getfield(L, 2, "x");
	pos.y = getfield(L, 2, "y");
	
	return path_to_table(L, 2, P, index, pos);
}

static int
lpath(lua_State *L) {
	return lpath_(L, 0);
}

static int
lpath_near(lua_State *L) {
	return lpath_(L, 1);
}

static int
ldist_(lua_State *L, int near) {
	struct pathmap * P = getP(L);
	int x = luaL_checkinteger(L, 4);	// cache end point x
	int y = luaL_checkinteger(L, 5);	// cache end point y
	check_xy(L, P, x, y);
	int index = get_pathmap(P, x, y, 0);
	struct scene_coord pos;
	pos.x = luaL_checkinteger(L, 2);	// start point x
	pos.y = luaL_checkinteger(L, 3);	// start point y
	if (near) {
		int i;
		int min_dist = 0;
		for (i=7;i>=0;i--) {
			struct scene_coord t;
			t.x = pos.x + neighbor[i*3+0];
			t.y = pos.y + neighbor[i*3+1];
			slot_t *s = get_dist(&P->cache, index, t);
			if (s && *s) {
				if (min_dist == 0 || *s < min_dist)
					min_dist = *s;
			}
		}
		lua_pushinteger(L, min_dist);
	} else {
		slot_t *s = get_dist(&P->cache, index, pos);
		lua_pushinteger(L, s == NULL ? 0 : *s);
	}
	return 1;	
}

static int
ldist(lua_State *L) {
	return ldist_(L, 0);
}

static int
ldist_near(lua_State *L) {
	return ldist_(L, 1);
}

static int
lreset(lua_State *L) {
	struct pathmap * P = getP(L);
	memset(&P->cache_index, ~0, sizeof(P->cache_index));
	return 0;
}

static int
lpathmap_new(lua_State *L) {
	struct scene * s = luaL_checkudata(L, 1, "SCENE");	// todo
	int layer = luaL_checkinteger(L, 2);
	if (!(layer >= 0 && layer < s->layer_n))
		return luaL_error(L, "Invalid layer %d", layer);
	size_t sz = pathmap_size(1 << s->shift_x , s->y);
	struct pathmap *P = (struct pathmap *)lua_newuserdatauv(L, sz, 1);
	lua_pushvalue(L, 1);
	lua_setiuservalue(L, -2, 1);
	P->layer = s->layer[layer];
	P->x = 1 << s->shift_x;
	P->y = s->y;
	P->shift = s->shift_x;
	memset(&P->cache_index, ~0, sizeof(P->cache_index));
	P->cache.shift_x = s->shift_x;
	P->cache.y = s->y;
	P->cache.layer_n = CACHE_SLOTS;
	int sz_map = s->y << s->shift_x;
	slot_t *base = (slot_t *)&P->cache.layer[CACHE_SLOTS];
	int i;
	for (i=0;i<CACHE_SLOTS;i++) {
		P->cache.layer[i] = base;
		base += sz_map;
	}
	if (luaL_newmetatable(L, PATHMAP_LUAKEY)) {
		luaL_Reg l[] = {
			{ "path", lpath },
			{ "dist", ldist },
			{ "path_near", lpath_near },
			{ "dist_near", ldist_near },
			{ "reset", lreset },
			{ "debug", ldebug },
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

GAME_MODULE(pathmap) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "new", lpathmap_new },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}

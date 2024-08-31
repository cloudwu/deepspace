#include <lua.h>
#include <lauxlib.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "lgame.h"
#include "group.h"
#include "list.h"

#define CONTAINER_LUAKEY "GAME_CONTAINER"
#define DEFAULT_STORAGE 128
#define CACHE_SIZE 8191

struct stack {
	int type;
	int number;
};

struct storage_pile {
	int type;
	int stock;
};

LIST_TYPE(storage_pile)

struct box {
	int size;	// = 0 : pile, > 0 storage
	int storage;
};

#define BOX_INDEX 1
#define PILE_INDEX 2
#define STACK_INDEX 3
#define CONTAINER_USERDATA_COUNT 3

struct container {
	struct group G;
	struct list T;
	struct stack stack_cache[CACHE_SIZE];
};

static void
stack_cache_init(struct container *C) {
	int i;
	for (i=0;i<CACHE_SIZE;i++) {
		C->stack_cache[i].number = 0;
	}
}

static int
stack_cache_lookup(lua_State *L, int index, struct container *C, int type) {
	int idx = type % CACHE_SIZE;
	if (C->stack_cache[idx].type == type && C->stack_cache[idx].number != 0) {
		return C->stack_cache[idx].number;
	}
	lua_getiuservalue(L, index, STACK_INDEX);
	if (lua_rawgeti(L, -1, type) != LUA_TNUMBER)
		return luaL_error(L, "Invalid type %d", type);
	int v = lua_tointeger(L, -1);
	if (v <= 0)
		return luaL_error(L, "Invalid type %d", type);
	lua_pop(L, 2);
	C->stack_cache[idx].type = type;
	C->stack_cache[idx].number = v;
	return v;
}

static inline struct container *
getC(lua_State *L) {
	struct container * C = (struct container *)lua_touserdata(L, 1);
	if (C == NULL)
		luaL_error(L, "Invalid box userdata");
	return C;
}

static inline struct box *
add_box(lua_State *L, struct container *C) {
	struct box * c = (struct box *)group_add(L, BOX_INDEX, &C->G);
	c->storage = LIST_EMPTY;
	return c;
}

static int
lcontainer_add(lua_State *L) {
	struct container * C = getC(L);
	struct box * b = add_box(L, C);
	b->size = luaL_optinteger(L, 2, 0);
	if (b->size < 0)
		return luaL_error(L, "size %d should > 0", b->size);
	lua_pushinteger(L, group_handle(&C->G, b));
	return 1;
}

static int
lcontainer_remove(lua_State *L) {
	struct container * C = getC(L);
	int id = luaL_checkinteger(L, 2);
	struct box *c = group_object(&C->G, id);
	if (c == NULL)
		return 0;
	list_clear(storage_pile, &C->T, c->storage);
	group_remove(&C->G, c);
	return 0;
}

static void
export_box(lua_State *L, struct container *C, struct box *b) {
	lua_createtable(L, 0, 3);
	unsigned int id = group_handle(&C->G, b);
	lua_pushinteger(L, id);
	lua_setfield(L, -2, "id");
	if (b->size > 0) {
		lua_pushinteger(L, b->size);
		lua_setfield(L, -2, "size");
	}
	lua_newtable(L);
	int iter = b->storage;
	int idx = 0;
	struct storage_pile *p;
	while ((p = list_each(storage_pile, &C->T, &iter))) {
		lua_pushfstring(L, "%d|%d", p->type, p->stock);
		lua_rawseti(L, -2, ++idx);
	}
	lua_setfield(L, -2, "content");
}

static int
lcontainer_export(lua_State *L) {
	struct container * C = getC(L);
	lua_newtable(L);
	struct box * iter = NULL;
	int i = 0;
	while ((iter = group_each(&C->G, iter))) {
		export_box(L, C, iter);
		lua_rawseti(L, -2, ++i);
	}
	return 1;
}

static int
import_key(lua_State *L, const char *key) {
	lua_getfield(L, -1, key);
	int isnum = 0;
	int v = lua_tointegerx(L, -1, &isnum);
	if (!isnum) {
		return luaL_error(L, "Not an integer for .%s", key);
	}
	lua_pop(L, 1);
	return v;
}

static int
import_key_opt(lua_State *L, const char *key, int opt) {
	lua_getfield(L, -1, key);
	int isnum = 0;
	int v = lua_tointegerx(L, -1, &isnum);
	if (!isnum) {
		if (lua_type(L, -1) == LUA_TNIL) {
			lua_pop(L, 1);
			return opt;
		}
		return luaL_error(L, "Not an integer for .%s", key);
	}
	lua_pop(L, 1);
	return v;
}

static void
init_pile(lua_State *L, int index, struct container *C, struct box *b, const char *content) {
	struct storage_pile *p = list_add(storage_pile, &C->T, &b->storage, PILE_INDEX);
	int n = sscanf(content, "%d|%d", &p->type, &p->stock);
	if (n != 2 || p->type < 0 || p->stock < 0) {
		luaL_error(L, "Invalid content %s", content);
	}
}

static int
lcontainer_import(lua_State *L) {
	struct container * C = getC(L);
	list_reset(storage_pile, &C->T);
	group_clear(&C->G);
	if (lua_isnoneornil(L, 2))
		return 0;
	luaL_checktype(L, 2, LUA_TTABLE);
	int idx = 0;
	while (lua_geti(L, 2, ++idx) == LUA_TTABLE) {
		int id = import_key(L, "id");
		struct box *b = group_import(L, 1, &C->G, id);
		if (b == NULL) {
			return luaL_error(L, "Can't import box id = %d", id);
		}
		b->storage = LIST_EMPTY;
		b->size = import_key_opt(L, "size", 0);
		if (b->size < 0) {
			return luaL_error(L, "box %d size %d should > 0", id, b->size);
		}
		if (lua_getfield(L, -1, "content") != LUA_TTABLE) {
			return luaL_error(L, "box %d no content", group_handle(&C->G, b));
		}
		int pile_n = 0;
		while (lua_geti(L, -1, ++pile_n) == LUA_TSTRING) {
			const char * content = lua_tostring(L, -1);
			init_pile(L, 1, C, b, content);
			lua_pop(L, 1);
		}
		if (lua_type(L, -1) != LUA_TNIL) {
			return luaL_error(L, "Invalid content table for box %d", group_handle(&C->G, b));
		}
		lua_pop(L, 2);
	}
	return 0;
}

static int
lcontainer_content(lua_State *L) {
	struct container * C = getC(L);
	int id = luaL_checkinteger(L, 2);
	struct box *c = group_object(&C->G, id);
	if (c == NULL)
		return 0;
	lua_newtable(L);
	int iter = c->storage;
	struct storage_pile *p;
	while ((p = list_each(storage_pile, &C->T, &iter))) {
		lua_pushinteger(L, p->stock);
		lua_rawseti(L, -2, p->type);
	}
	return 1;
}

static struct storage_pile *
find_pile(struct container * C, struct box *c, int type) {
	int iter = c->storage;
	struct storage_pile *p;
	while ((p = list_each(storage_pile, &C->T, &iter))) {
		if (p->type == type) {
			return p;
		}
	}
	return NULL;
}

static int
lcontainer_stock(lua_State *L) {
	struct container * C = getC(L);
	int id = luaL_checkinteger(L, 2);
	struct box *c = group_object(&C->G, id);
	if (c == NULL) {
		return 0;
	}
	if (lua_isnoneornil(L, 3)) {
		struct storage_pile *p = list_object(storage_pile, &C->T, c->storage);
		if (p) {
			lua_pushinteger(L, p->stock);
			lua_pushinteger(L, p->type);
			return 2;
		}
	} else {
		int type = luaL_checkinteger(L, 3);
		struct storage_pile *p = find_pile(C, c, type);
		if (p) {
			lua_pushinteger(L, p->stock);
			return 1;
		}
	}
	return 0;
}

static inline struct storage_pile *
add_pile(lua_State *L, int index, struct container * C, struct box *c) {
	struct storage_pile *p = list_add(storage_pile, &C->T, &c->storage, PILE_INDEX);
	return p;
}

static int
put_storage(lua_State *L, struct container *C, struct box *b, int type, int count) {
	struct storage_pile *p;
	int iter = b->storage;
	struct storage_pile *found = NULL;
	int size = b->size;
	while ((p = list_each(storage_pile, &C->T, &iter))) {
		if (p->type == type) {
			found = p;
		} else {
			int stack = stack_cache_lookup(L, 1, C, p->type);
			size -= (p->stock + stack - 1) / stack;
			if (size <= 0) {
				if (size == 0) {
					lua_pushinteger(L, 0);
					return 1;
				}
				return luaL_error(L, "Invalid storage stock");
			}
		}
	}
	if (!found) {
		found = add_pile(L, 1, C, b);
		found->type = type;
	}
	int stack = stack_cache_lookup(L, 1, C, type);
	int space = size * stack - found->stock;
	if (space < count)
		count = space;
	found->stock += count;
	lua_pushinteger(L, count);
	return 1;
}

static int
lcontainer_put(lua_State *L) {
	struct container * C = getC(L);
	int id = luaL_checkinteger(L, 2);
	struct box *c = group_object(&C->G, id);
	if (c == NULL) {
		lua_pushinteger(L, 0);
		return 1;
	}
	int type = luaL_checkinteger(L, 3);
	int count = luaL_checkinteger(L, 4);
	if (count <= 0) {
		if (count < 0) {
			return luaL_error(L, "Invalid count %d", count);
		}
		lua_pushinteger(L, 0);
		return 1;
	}
	if (c->size > 0) {
		return put_storage(L, C, c, type, count);
	}
	struct storage_pile *p = find_pile(C, c, type);
	if (p == NULL) {
		p = add_pile(L, 1, C, c);
		p->type = type;
		p->stock = count;
	} else {
		p->stock += count;
	}
	lua_pushinteger(L, count);
	return 1;
}

static int
lcontainer_take(lua_State *L) {
	struct container * C = getC(L);
	int id = luaL_checkinteger(L, 2);
	struct box *c = group_object(&C->G, id);
	if (c == NULL) {
		lua_pushinteger(L, 0);
		return 1;
	}
	int type = luaL_checkinteger(L, 3);
	int count = luaL_checkinteger(L, 4);
	if (count <= 0) {
		if (count < 0) {
			return luaL_error(L, "Can't take %d", count);
		}
		return 0;
	}
	struct storage_pile *p = find_pile(C, c, type);
	if (p == NULL) {
		lua_pushinteger(L, 0);
		return 1;
	}
	if (p->stock < count) {
		count = p->stock;
	}
	p->stock -= count;
	if (p->stock == 0) {
		list_remove(storage_pile, &C->T, &c->storage, p);
	}
	lua_pushinteger(L, count);
	return 1;
}

static int
check_type(struct container * C, struct box *c, int type) {
	struct storage_pile *p = find_pile(C, c, type);
	if (p == NULL)
		return 0;
	return p->stock;
}

static int
lcontainer_find(lua_State *L) {
	struct container * C = getC(L);
	int type = luaL_checkinteger(L, 2);
	if (!lua_istable(L, 3)) {
		lua_settop(L, 2);
		lua_newtable(L);
	}
	int i;
	int result_n = 0;
	int result = 0;
	struct box *iter = NULL;
	while ((iter = group_each(&C->G, iter))) {
		int n = check_type(C, iter, type);
		if (n > 0) {
			result += n;
			unsigned int id = group_handle(&C->G, iter);
			lua_pushinteger(L, id);
			lua_seti(L, 3, ++result_n);
		}
	}
	int size = lua_rawlen(L, 3);
	for (i = result_n; i < size; i++) {
		lua_pushnil(L);
		lua_seti(L, 3, i+1);
	}
	lua_pushinteger(L, result);
	lua_pushvalue(L, 3);
	return 2;
}

static int
lcontainer_list(lua_State *L) {
	struct container * C = getC(L);
	if (!lua_istable(L, 2)) {
		lua_settop(L, 1);
		lua_newtable(L);
	}
	struct box *iter = NULL;
	int result_n = 0;
	while ((iter = group_each(&C->G, iter))) {
		if (iter->size > 0) {
			unsigned int id = group_handle(&C->G, iter);
			lua_pushinteger(L, id);
			lua_seti(L, 2, ++result_n);
		}
	}
	int size = lua_rawlen(L, 2);
	int i;
	for (i = result_n; i < size; i++) {
		lua_pushnil(L);
		lua_seti(L, 2, i+1);
	}
	return 1;
}

static int
lcontainer_info(lua_State *L) {
	struct container * C = getC(L);
	int id = luaL_checkinteger(L, 2);
	struct box *c = group_object(&C->G, id);
	if (c == NULL)
		return luaL_error(L, "No id %d", id);
	lua_newtable(L);
	lua_pushinteger(L, c->size);
	lua_setfield(L, -2, "size");
	int iter = c->storage;
	int index = 0;
	struct storage_pile *pile;
	while ((pile = list_each(storage_pile, &C->T, &iter))) {
		lua_newtable(L);
		lua_pushinteger(L, pile->type);
		lua_setfield(L, -2, "type");
		lua_pushinteger(L, pile->stock);
		lua_setfield(L, -2, "stock");
		lua_rawseti(L, -2, ++index);
	}
	return 1;
}

static int
lcontainer_new(lua_State *L) {
	luaL_checktype(L, 1, LUA_TTABLE);	// stack table
	struct container *C = (struct container *)lua_newuserdatauv(L, sizeof(*C), CONTAINER_USERDATA_COUNT);
	group_init(L, -1, BOX_INDEX, &C->G, sizeof(struct box));
	list_init(storage_pile, &C->T, PILE_INDEX);
	stack_cache_init(C);
	lua_pushvalue(L, 1);
	lua_setiuservalue(L, -2, STACK_INDEX);
	if (luaL_newmetatable(L, CONTAINER_LUAKEY)) {
		luaL_Reg l[] = {
			{ "add", lcontainer_add },
			{ "remove", lcontainer_remove },
			{ "content", lcontainer_content },
			{ "stock", lcontainer_stock },
			{ "put", lcontainer_put },
			{ "take", lcontainer_take },
			{ "export", lcontainer_export },
			{ "import", lcontainer_import },
			{ "find", lcontainer_find },
			{ "list", lcontainer_list },
			{ "info", lcontainer_info },	// for debug
			{ "__index", NULL },
			{ NULL, NULL },
		};
		luaL_setfuncs(L, l, 0);
		lua_pushvalue(L, -1);
		lua_setfield(L, -2, "__index");
	}
	lua_setmetatable(L, -2);
	return 1;
};

GAME_MODULE(container) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "new", lcontainer_new },
		{ NULL, NULL },
	};
		
	luaL_newlib(L, l);
	return 1;
}

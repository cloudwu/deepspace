#include <lua.h>
#include <lauxlib.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "lgame.h"
#include "group.h"
#include "list.h"

#define DEFAULT_STORAGE 128

struct storage_pile {
	int type;
	int cap;
	int stack;
	int stock;
	int book;
	int reserve;
};

LIST_TYPE(storage_pile)

struct box {
	int size;	// >0 : input/output ; == 0 : output ; < 0 : input
	int storage;
};

struct container {
	struct group G;
	struct list T;
};

static inline struct container *
getC(lua_State *L) {
	struct container * C = (struct container *)lua_touserdata(L, 1);
	if (C == NULL)
		luaL_error(L, "Invalid box userdata");
	return C;
}

static inline struct box *
add_box(lua_State *L, struct container *C) {
	struct box * c = (struct box *)group_add(L, 1, &C->G);
	c->storage = LIST_EMPTY;
	return c;
}

static int
container_add(lua_State *L) {
	struct container * C = getC(L);
	struct box * b = add_box(L, C);
	if (lua_isboolean(L, 2)) {
		// true: input false: output
		if (lua_toboolean(L, 2)) {
			b->size = -1;
		} else {
			b->size = 0;
		}
	} else {
		b->size = luaL_checkinteger(L, 2);
	}
	lua_pushinteger(L, group_handle(&C->G, b));
	return 1;
}

static int
container_remove(lua_State *L) {
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
	} else {
		lua_pushboolean(L, 1);
		if (b->size == 0) {
			lua_setfield(L, -2, "output");
		} else {
			lua_setfield(L, -2, "input");
		}
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
container_export(lua_State *L) {
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

static int
import_key_boolean(lua_State *L, const char *key) {
	int t = lua_getfield(L, -1, key);
	if (t == LUA_TNIL || t == LUA_TBOOLEAN) {
		int r = lua_toboolean(L, -1);
		lua_pop(L, 1);
		return r;
	}
	return luaL_error(L, "Not an boolean for .%s", key);
}

static void
init_pile(lua_State *L, int index, struct container *C, struct box *b, const char *content) {
	struct storage_pile *p = list_add(storage_pile, &C->T, &b->storage, 2);
	int n = sscanf(content, "%d|%d", &p->type, &p->stock);
	if (n != 2) {
		luaL_error(L, "Invalid content %s", content);
	}
}

static int
container_import(lua_State *L) {
	struct container * C = getC(L);
	list_reset(storage_pile, &C->T);
	group_clear(&C->G);
	luaL_checktype(L, 2, LUA_TTABLE);
	int idx = 0;
	while (lua_geti(L, 2, ++idx) == LUA_TTABLE) {
		int id = import_key(L, "id");
		struct box *b = group_import(L, 1, &C->G, id);
		if (b == NULL) {
			return luaL_error(L, "Can't import id = %d", id);
		}
		b->storage = LIST_EMPTY;
		b->size = import_key_opt(L, "size", 0);
		if (b->size == 0) {
			if (import_key_boolean(L, "input")) {
				b->size = -1;
			} else if (!import_key_boolean(L, "output")) {
				return luaL_error(L, "box %d should be input or output", id);
			}
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
container_content(lua_State *L) {
	struct container * C = getC(L);
	int id = luaL_checkinteger(L, 2);
	struct box *c = group_object(&C->G, id);
	if (c == NULL)
		return 0;
	lua_newtable(L);
	int iter = c->storage;
	struct storage_pile *p;
	int idx = 0;
	while ((p = list_each(storage_pile, &C->T, &iter))) {
		lua_pushinteger(L, p->type);
		lua_rawseti(L, -2, ++idx);
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
container_stock(lua_State *L) {
	struct container * C = getC(L);
	int id = luaL_checkinteger(L, 2);
	struct box *c = group_object(&C->G, id);
	if (c == NULL)
		return luaL_error(L, "No id %d", id);
	int type = luaL_checkinteger(L, 3);
	struct storage_pile *p = find_pile(C, c, type);
	if (p) {
		lua_pushinteger(L, p->stock);
		lua_pushinteger(L, p->cap);
		return 2;
	}
	return 0;
}

static int
container_put(lua_State *L) {
	struct container * C = getC(L);
	int id = luaL_checkinteger(L, 2);
	struct box *c = group_object(&C->G, id);
	if (c == NULL)
		return luaL_error(L, "No id %d", id);
	int type = luaL_checkinteger(L, 3);
	int count = luaL_checkinteger(L, 4);
	int cancel = lua_toboolean(L, 5);
	struct storage_pile *p = find_pile(C, c, type);
	if (p == NULL)
		return luaL_error(L, "Reserve first");
	if (p->reserve < count) {
		return luaL_error(L, "Reserve = %d put = %d", p->reserve , count);
	}
	p->reserve -= count;
	if (!cancel) {
		p->stock += count;
	}
	return 0;
}

static inline struct storage_pile *
add_pile(lua_State *L, int index, struct container * C, struct box *c) {
	return list_add(storage_pile, &C->T, &c->storage, 2);
}

static int
reserve_storage(lua_State *L, int index, struct container * C, struct box *c, int type, int count, int stack) {
	int size = c->size;
	struct storage_pile *pile = find_pile(C, c, type);
	if (pile) {
		// found
		if (stack != pile->stack) {
			return luaL_error(L, "Invalid stack %d != %d", stack, pile->stack);
		}
		int space = size * stack - pile->stock;
		if (space < count) {
			assert(space >= 0);
			count = space;
		}
		pile->stock += count;
		pile->reserve += count;
	} else {
		// not found, add new pile
		int space = size * stack;
		if (space <= 0) {
			assert(space == 0);
			return 0;
		}
		pile = add_pile(L, index, C, c);
		if (space < count) {
			count = space;
		}
		pile->type = type;
		pile->reserve = count;
		pile->stack = stack;
	}
	lua_pushinteger(L, count);
	return 1;
}

static int
reserve_inout(lua_State *L, int index, struct container * C, struct box *c, int type, int count) {
	struct storage_pile *pile = find_pile(C, c, type);
	if (pile) {
		// found
		int space = pile->cap - pile->stock - pile->reserve;
		if (space < count) {
			if (space <= 0)
				count = 0;
			else
				count = space;
		}
	} else {
		// not found
		if (c->size < 0) {
			return luaL_error(L, "input box should setcap first");
		}
		// It's output box
		pile = add_pile(L, index, C, c);
		pile->type = type;
	}
	pile->reserve += count;
	lua_pushinteger(L, count);
	return 1;
}

static int
container_reserve(lua_State *L) {
	struct container * C = getC(L);
	int id = luaL_checkinteger(L, 2);
	struct box *c = group_object(&C->G, id);
	if (c == NULL)
		return luaL_error(L, "No id %d", id);
	int type = luaL_checkinteger(L, 3);
	int count = luaL_checkinteger(L, 4);
	if (c->size > 0) {
		int stack = luaL_checkinteger(L, 5);
		return reserve_storage(L, 1, C, c, type, count, stack);
	} else {
		return reserve_inout(L, 1, C, c, type, count);
	}
}

static int
container_setcap(lua_State *L) {
	struct container * C = getC(L);
	int id = luaL_checkinteger(L, 2);
	struct box *c = group_object(&C->G, id);
	if (c == NULL)
		return luaL_error(L, "No id %d", id);
	int type = luaL_checkinteger(L, 3);
	int cap = luaL_checkinteger(L, 4);
	struct storage_pile *p = find_pile(C, c, type);
	if (p) {
		p->cap = cap;
	} else {
		p = add_pile(L, 1, C, c);
		p->type = type;
		p->cap = cap;
	}
	return 0;
}

static int
container_setstack(lua_State *L) {
	struct container * C = getC(L);
	luaL_checktype(L, 2, LUA_TTABLE);
	struct box *b = NULL;
	while ((b = group_each(&C->G, b))) {
		if (b->size > 0) {
			struct storage_pile *p;
			int iter = b->storage;
			while ((p = list_each(storage_pile, &C->T, &iter))) {
				if (lua_rawgeti(L, 2, p->type) != LUA_TNUMBER) {
					return luaL_error(L, "Type %d has no stack number", p->type);
				}
				p->stack = lua_tointeger(L, -1);
				lua_pop(L, 1);
			}
		}
	}
	return 0;
}

static int
container_book(lua_State *L) {
	struct container * C = getC(L);
	int id = luaL_checkinteger(L, 2);
	struct box *c = group_object(&C->G, id);
	if (c == NULL)
		return luaL_error(L, "No id %d", id);
	int type = luaL_checkinteger(L, 3);
	int count = luaL_checkinteger(L, 4);
	struct storage_pile *p = find_pile(C, c, type);
	if (p == NULL) {
		lua_pushinteger(L, 0);
		return 1;
	}
	int stock = p->stock - p->book;
	assert(stock >= 0);
	if (stock < count)
		count = stock;
	p->book += count;
	lua_pushinteger(L, count);
	return 1;	
}

static int
container_take(lua_State *L) {
	struct container * C = getC(L);
	int id = luaL_checkinteger(L, 2);
	struct box *c = group_object(&C->G, id);
	if (c == NULL)
		return luaL_error(L, "No id %d", id);
	int type = luaL_checkinteger(L, 3);
	int count = luaL_checkinteger(L, 4);
	int cancel = lua_toboolean(L, 5);
	if (count <= 0) {
		if (count < 0) {
			return luaL_error(L, "Can't take %d", count);
		}
		return 0;
	}
	struct storage_pile *p = find_pile(C, c, type);
	if (p == NULL)
		return luaL_error(L, "No type %d in %d", type, id);
	if (p->book < count) {
		return luaL_error(L, "Book = %d put = %d", p->book , count);
	}
	p->book -= count;
	if (!cancel) {
		p->stock -= count;
		if (p->stock == 0 && c->size > 0) {
			list_remove(storage_pile, &C->T, &c->storage, p);
		}
	}
	return 0;
}

static int
check_type(struct container * C, struct box *c, int type) {
	struct storage_pile *p = find_pile(C, c, type);
	if (p == NULL)
		return 0;
	int n = p->stock - p->book;
	return n;
}

static int
container_find(lua_State *L) {
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
lcontainer_new(lua_State *L) {
	struct container *C = (struct container *)lua_newuserdatauv(L, sizeof(*C), 2);
	group_init(L, -1, 1, &C->G, sizeof(struct box));
	list_init(storage_pile, &C->T, 2);
	if (luaL_newmetatable(L, "GAME_CONTAINER")) {
		luaL_Reg l[] = {
			{ "add", container_add },
			{ "remove", container_remove },
			{ "content", container_content },
			{ "setstack", container_setstack },
			{ "setcap", container_setcap },
			{ "stock", container_stock },
			{ "put", container_put },
			{ "reserve", container_reserve },
			{ "book", container_book },
			{ "take", container_take },
			{ "export", container_export },
			{ "import", container_import },
			{ "find", container_find },
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

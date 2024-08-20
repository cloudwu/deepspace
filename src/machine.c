#include <lua.h>
#include <lauxlib.h>

#include "lgame.h"
#include "powergrid.h"
#include "container.h"
#include "group.h"
#include "list.h"

struct machine {
	int recipe;
	int input_box;	// for logistics
	int output_box;	// for logistics
	int appliance;	// for powergrid
	int productivity;	// 0x10000 : 100%
	int power;	// for powergrid, set by recipe
	int worktime;	// set by recipe
	int input_recipe;
	int output_recipe;
	uint64_t worktick;	// fixnumber ( worktick * 0x10000 => worktime ) 
};

struct recipe {
	int type;
	int count;
};

LIST_TYPE(recipe)

#define MACHINE_MACHINE 1
#define MACHINE_RECIPE 2
#define MACHINE_COUNT 2

struct machine_arena {
	struct group M;
	struct list R;	// recipe
};

// set productivity to turn on/off the machine
// init machine with recipe { .id, .power .worktime .input, .output }

static struct machine_arena *
getM(lua_State *L) {
	struct machine_arena *M = (struct machine_arena *)lua_touserdata(L, 1);
	if (M == NULL)
		luaL_error(L, "Need userdata machine");
	return M;
}

static lua_Integer
get_key(lua_State *L, int index, const char *key) {
	lua_getfield(L, index, key);
	int isnum = 0;
	lua_Integer r = lua_tointegerx(L, -1, &isnum);
	if (!isnum) {
		return luaL_error(L, ".%s is not an integer", key);
	}
	lua_pop(L, 1);
	return r;
}

static int
opt_key(lua_State *L, int index, const char *key, int opt) {
	if (lua_getfield(L, index, key) == LUA_TNIL) {
		lua_pop(L, 1);
		return opt;
	}
	int isnum = 0;
	lua_Integer r = lua_tointegerx(L, -1, &isnum);
	if (!isnum) {
		return luaL_error(L, ".%s is not an integer", key);
	}
	lua_pop(L, 1);
	return r;
}

static int
get_fixnumber(lua_State *L, int index, const char *key) {
	if (lua_getfield(L, index, key) != LUA_TNUMBER) {
		return luaL_error(L, ".%s is not a number", key);
	}
	double v = lua_tonumber(L, -1);
	lua_pop(L, 1);
	int fv = (int)(v * 0x10000 + 0.5);
	return fv;
}

static int
lmachine_add(lua_State *L) {
	struct machine_arena *M = getM(L);
	struct machine *m = (struct machine *)group_add(L, 1, &M->M);
	m->recipe = 0;
	m->input_box = 0;
	m->output_box = 0;
	m->appliance = 0;
	m->power = 0;
	m->worktime = 0;
	m->worktick = 0;
	m->productivity = 0;
	m->input_recipe = LIST_EMPTY;
	m->output_recipe = LIST_EMPTY;
	lua_pushinteger(L, group_handle(&M->M, m));
	return 1;
}

static int
lmachine_init(lua_State *L) {
	struct machine_arena *M = getM(L);
	int id = luaL_checkinteger(L, 2);
	luaL_checktype(L, 3, LUA_TTABLE);	// { .input, .output , .appliance }
	struct machine *m = (struct machine *)group_object(&M->M, id);
	if (m == NULL)
		return luaL_error(L, "Invalid machine id %d", id);
	m->input_box = opt_key(L, 3, "input", 0);
	m->output_box = get_key(L, 3, "output");	// void is also a type
	m->appliance = opt_key(L, 3, "appliance", 0);
	if (m->appliance == 0 && m->power != 0)
		return luaL_error(L, "Need .appliance for power %d", m->power);
	return 0;
}

static int
set_recipe(lua_State *L, struct list *R) {
	int head = LIST_EMPTY;
	int index = 0;
	while (lua_geti(L, -1, ++index) == LUA_TTABLE) {
		struct recipe *r = list_add(recipe, R, &head, MACHINE_RECIPE);
		r->type = get_key(L, -1, "type");
		r->count = get_key(L, -1, "count");
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	return head;
}

static int
lset_recipe(lua_State *L) {
	struct machine_arena *M = getM(L);
	int id = luaL_checkinteger(L, 2);
	luaL_checktype(L, 3, LUA_TTABLE);	// recipe table
	struct machine *m = (struct machine *)group_object(&M->M, id);
	if (m == NULL)
		return luaL_error(L, "Invalid machine id %d", id);
	m->recipe = get_key(L, 3, "id");
	m->power = opt_key(L, 3, "power", 0);
	m->worktime = get_key(L, 3, "worktime");
	m->productivity = get_fixnumber(L, 3, "productivity");

	list_clear(recipe, &M->R, m->input_recipe);
	list_clear(recipe, &M->R, m->output_recipe);
	m->input_recipe = LIST_EMPTY;
	m->output_recipe = LIST_EMPTY;
	
	if (lua_getfield(L, 3, "input") == LUA_TTABLE) {
		m->input_recipe = set_recipe(L, &M->R);
		lua_pop(L, 1);
	}
	if (lua_getfield(L, 3, "output") == LUA_TTABLE) {
		m->output_recipe = set_recipe(L, &M->R);
		lua_pop(L, 1);
	}
	return 0;
}

static int
lmachine_remove(lua_State *L) {
	struct machine_arena *M = getM(L);
	int id = luaL_checkinteger(L, 2);
	struct machine *m = (struct machine *)group_object(&M->M, id);
	if (m == NULL)
		return 0;
	lua_createtable(L, 0, 3);
	lua_pushinteger(L, m->input_box);
	lua_setfield(L, -2, "input");
	lua_pushinteger(L, m->output_box);
	lua_setfield(L, -2, "output");
	lua_pushinteger(L, m->appliance);
	lua_setfield(L, -2, "appliance");
	group_remove(&M->M, m);
	list_clear(recipe, &M->R, m->input_recipe);
	list_clear(recipe, &M->R, m->output_recipe);
	return 1;
}

static int
lset_productivity(lua_State *L) {
	struct machine_arena *M = getM(L);
	int id = luaL_checkinteger(L, 2);
	struct machine *m = (struct machine *)group_object(&M->M, id);
	if (m == NULL)
		return luaL_error(L, "Invalid machine id %d", id);
	double v = luaL_checknumber(L, 3);
	m->productivity = (int)(v * 0x10000 + 0.5);
	return 0;
}

static void
export_table_with_id(lua_State *L, int h) {
	lua_newtable(L);
	lua_pushinteger(L, h);
	lua_setfield(L, -2, "id");
}

static int
lmachine_export(lua_State *L) {
	struct machine_arena *M = getM(L);
	lua_newtable(L);
	struct machine * obj = NULL;
	int from_index = 1;
	while ((obj = group_each(&M->M, obj))) {
		int h = group_handle(&M->M, obj);
		export_table_with_id(L, h);
		lua_pushinteger(L, obj->recipe);
		lua_setfield(L, -2, "recipe");
		lua_pushinteger(L, obj->input_box);
		lua_setfield(L, -2, "input");
		lua_pushinteger(L, obj->output_box);
		lua_setfield(L, -2, "output");
		lua_pushinteger(L, obj->appliance);
		lua_setfield(L, -2, "appliance");
		lua_pushinteger(L, obj->productivity);
		lua_setfield(L, -2, "productivity");
		lua_pushinteger(L, obj->worktick);
		lua_setfield(L, -2, "worktick");
		lua_rawseti(L, -2, from_index++);
	}
	return 1;
}

static void
import_machine(lua_State *L, struct group *M, int recipe_index) {
	int h = get_key(L, -1, "id");
	struct machine *m = (struct machine *)group_import(L, -1, M, h);
	if (m == NULL)
		luaL_error(L, "Can't import machine id = %d", h);
	m->recipe = get_key(L, -1, "recipe");
	m->input_box = get_key(L, -1, "input");
	m->output_box = get_key(L, -1, "output");
	m->appliance = get_key(L, -1, "appliance");
	m->productivity = get_key(L, -1, "productivity");
	m->worktick = get_key(L, -1, "worktick");
	// set recipe
	if (lua_geti(L, recipe_index, m->recipe) != LUA_TTABLE) {
		luaL_error(L, "No recipe %d", m->recipe);
	}
	m->power = opt_key(L, -1, "power", 0);
	m->worktime = get_key(L, -1, "worktime");
	m->input_recipe = LIST_EMPTY;
	m->output_recipe = LIST_EMPTY;
	lua_pop(L, 1);
}

static int
lmachine_import(lua_State *L) {
	struct machine_arena *M = getM(L);
	luaL_checktype(L, 2, LUA_TTABLE);	// data
	luaL_checktype(L, 3, LUA_TTABLE);	// recipe
	group_clear(&M->M);
	list_reset(recipe, &M->R);
	int index = 0;
	while (lua_geti(L, 2, ++index) == LUA_TTABLE) {
		import_machine(L, &M->M, 3);
		lua_pop(L, 1);
	}
	return 0;
}

static void
add_pair(lua_State *L, luaL_Buffer *b, int head, struct list *R) {
	if (head == LIST_EMPTY)
		return;
	struct recipe *r;
	while ((r = list_each(recipe, R, &head))) {
		lua_pushfstring(L, "%d*%d+", r->type, r->count);		
		luaL_addvalue(b);
	}
	luaL_buffsub(b, 1);	
}

static void
push_formula(lua_State *L, struct machine *m, struct list *R) {
	luaL_Buffer b;
	luaL_buffinit(L, &b);
	add_pair(L, &b, m->input_recipe, R);
	luaL_addchar(&b, '=');
	add_pair(L, &b, m->output_recipe, R);
	luaL_pushresult(&b);
}

static int
lmachine_info(lua_State *L) {
	struct machine_arena *M = getM(L);
	int id = luaL_checkinteger(L, 2);
	struct machine *m = (struct machine *)group_object(&M->M, id);
	if (m == NULL)
		luaL_error(L, "Can't import machine id = %d", id);
	lua_newtable(L);
	lua_pushinteger(L, m->recipe);
	lua_setfield(L, -2, "recipe");
	lua_pushinteger(L, m->input_box);
	lua_setfield(L, -2, "input");
	lua_pushinteger(L, m->output_box);
	lua_setfield(L, -2, "output");
	lua_pushinteger(L, m->appliance);
	lua_setfield(L, -2, "appliance");
	lua_pushnumber(L, (double)m->productivity/0x10000);
	lua_setfield(L, -2, "productivity");
	lua_pushinteger(L, m->worktime);
	lua_setfield(L, -2, "worktime");
	lua_pushnumber(L, (double)m->worktick/0x10000);
	lua_setfield(L, -2, "worktick");
	push_formula(L, m, &M->R);
	lua_setfield(L, -2, "formula");
	return 1;
}

static int
output_full(struct machine *m, struct container *C, struct list *R) {
	struct recipe *r;
	int iter = m->output_recipe;
	while ((r = list_each(recipe, R, &iter))) {
		int c = container_put(C, m->output_box, r->type, r->count, 1);
		if (c != r->count) {
			// full
			return 1;
		}
	}
	
	// put all
	iter = m->output_recipe;
	while ((r = list_each(recipe, R, &iter))) {
		container_put(C, m->output_box, r->type, r->count, 0);
	}
	return 0;
}

static int
input_enough(struct machine *m, struct container *C, struct list *R) {
	struct recipe *r;
	int iter = m->input_recipe;
	while ((r = list_each(recipe, R, &iter))) {
		int c = container_take(C, m->input_box, r->type, r->count, 1);
		if (c != r->count) {
			// not enough
			return 0;
		}
	}
	return 1;
}

static void
input_load(struct machine *m, struct container *C, struct list *R) {
	struct recipe *r;
	int iter = m->input_recipe;
	while ((r = list_each(recipe, R, &iter))) {
		container_take(C, m->input_box, r->type, r->count, 0);
	}
}

static void
machine_work(struct machine *m, struct list *R, struct powergrid *P, struct container *C) {
	if (m->productivity == 0)	// stop
		return;
	if (m->worktick >= ((uint64_t)m->worktime << 16)) {
		// finish a circle
		if (output_full(m, C, R)) {
			return;			
		}
		// output succ, reset worktick
		m->worktick = 0;
	}
	if (m->worktick == 0) {
		// start, check input
		if (!input_enough(m, C, R))
			return;
	}
	if (m->appliance) {
		// need power
		struct capacitance * cap = powergrid_get_level(P, m->appliance);
		if (cap->level) {
			// not enough energy (The first tick is free)
			return;						
		}
		cap->level = m->power;
	}
	if (m->worktick == 0) {
		input_load(m, C, R);
	}
	m->worktick += m->productivity;
}

static int
lmachine_tick(lua_State *L) {
	struct machine_arena *M = getM(L);
	struct powergrid *P = luaL_checkudata(L, 2, POWERGRID_LUAKEY);
	struct container *C = luaL_checkudata(L, 3, CONTAINER_LUAKEY);

	struct machine *m = NULL;
	while((m = (struct machine *)group_each(&M->M, m))) {
		machine_work(m, &M->R, P, C);
	}
	return 0;
}

static int
lmachine_new(lua_State *L) {
	struct machine_arena *M = (struct machine_arena *)lua_newuserdatauv(L, sizeof(struct machine_arena), MACHINE_COUNT);
	group_init(L, -1, MACHINE_MACHINE, &M->M, sizeof(struct machine));
	list_init(recipe, &M->R, MACHINE_RECIPE);
	if (luaL_newmetatable(L, "GAME_MACHINE")) {
		luaL_Reg l[] = {
			{ "add", lmachine_add },
			{ "init", lmachine_init },
			{ "set_recipe", lset_recipe },
			{ "remove", lmachine_remove },
			{ "set_productivity", lset_productivity },
			{ "tick", lmachine_tick },
			{ "export", lmachine_export },
			{ "import", lmachine_import },
			{ "info", lmachine_info }, // for debug
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

GAME_MODULE(machine) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "new", lmachine_new },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}

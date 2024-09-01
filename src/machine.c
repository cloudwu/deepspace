#include <lua.h>
#include <lauxlib.h>
#include <string.h>

#include "lgame.h"
#include "powergrid.h"
#include "group.h"
#include "list.h"

struct machine {
	int appliance;	// for powergrid
	int productivity;	// 0x10000 : 100%
	int power;	// for powergrid, set by recipe
	int worktime;	// set by recipe
	int working;	// in working list ?
	uint64_t worktick;	// fixnumber ( worktick * 0x10000 => worktime ) 
};

struct working {
	int id;
};

LIST_TYPE(working)

struct machine_arena {
	struct group M;
	struct list working;
	int working_list;
};

#define MACHINE_MACHINE 1
#define MACHINE_WORKING 2
#define MACHINE_COUNT 2

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
opt_fixnumber(lua_State *L, int index, const char *key, int opt) {
	int t = lua_getfield(L, index, key);
	if (t == LUA_TNIL) {
		lua_pop(L, 1);
		return opt;
	}
	if (t != LUA_TNUMBER) {
		return luaL_error(L, ".%s is not a number", key);
	}
	double v = lua_tonumber(L, -1);
	lua_pop(L, 1);
	int fv = (int)(v * 0x10000 + 0.5);
	return fv;
}

//#define list_each(T, t, iter) list_each_(t, iter, LIST_SIZEOF(T), LIST_OFFSETOF(T))

static int
lmachine_add(lua_State *L) {
	struct machine_arena *M = getM(L);
	struct machine *m = (struct machine *)group_add(L, 1, &M->M);
	m->appliance = luaL_optinteger(L, 2, 0);
	m->power = 0;
	m->worktime = 0;
	m->worktick = 0;
	m->productivity = 0;
	int id = group_handle(&M->M, m);
	struct working *w = list_add(working, &M->working, &M->working_list, MACHINE_WORKING);
	w->id = id;
	m->working = 1;
	lua_pushinteger(L, id);
	return 1;
}

static int
lset_recipe(lua_State *L) {
	struct machine_arena *M = getM(L);
	int id = luaL_checkinteger(L, 2);
	luaL_checktype(L, 3, LUA_TTABLE);	// recipe table
	struct machine *m = (struct machine *)group_object(&M->M, id);
	if (m == NULL)
		return luaL_error(L, "Invalid machine id %d", id);
	m->power = opt_key(L, 3, "power", 0);
	if (m->appliance != 0 && m->power == 0)
		return luaL_error(L, "appliance need power");
	if (m->power > 0) {
		struct powergrid *P = luaL_checkudata(L, 4, POWERGRID_LUAKEY);
		if (m->appliance == 0)
			return luaL_error(L, "No appliance but power = %d", m->power);
		struct capacitance *c = powergrid_get_level(P, m->appliance);
		if (c == NULL)
			return luaL_error(L, "No appliance id %d", m->appliance);
		c->level = m->power;		
	}
	m->worktime = get_key(L, 3, "worktime");
	m->productivity = opt_fixnumber(L, 3, "productivity", 0);
	return 0;
}

static int
lmachine_remove(lua_State *L) {
	struct machine_arena *M = getM(L);
	int id = luaL_checkinteger(L, 2);
	struct machine *m = (struct machine *)group_object(&M->M, id);
	if (m == NULL)
		return 0;
	if (m->appliance) {
		lua_pushinteger(L, m->appliance);
		group_remove(&M->M, m);
		return 1;
	} else {
		group_remove(&M->M, m);
		return 0;
	}
}

static void
set_working(lua_State *L, struct machine_arena *M, struct machine *m) {
	if (m->working)
		return;
	m->working = 1;
	int id = group_handle(&M->M, m);
	struct working *w = list_add(working, &M->working, &M->working_list, MACHINE_WORKING);
	w->id = id;
}

static int
lset_productivity(lua_State *L) {
	struct machine_arena *M = getM(L);
	int id = luaL_checkinteger(L, 2);
	struct machine *m = (struct machine *)group_object(&M->M, id);
	if (m == NULL)
		return luaL_error(L, "Invalid machine id %d", id);
	double v = luaL_checknumber(L, 3);
	if (m->productivity == 0)
		set_working(L, M, m);
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
		if (obj->appliance != 0) {
			lua_pushinteger(L, obj->appliance);
			lua_setfield(L, -2, "appliance");
		}
		lua_pushinteger(L, obj->worktick);
		lua_setfield(L, -2, "worktick");
		lua_rawseti(L, -2, from_index++);
	}
	return 1;
}

static void
import_machine(lua_State *L, struct machine_arena *M, int data_index) {
	int h = get_key(L, data_index, "id");
	struct machine *m = (struct machine *)group_import(L, data_index, &M->M, h);
	if (m == NULL)
		luaL_error(L, "Can't import machine id = %d", h);
	struct working *w = list_add(working, &M->working, &M->working_list, MACHINE_WORKING);
	w->id = h;
	m->working = 1;
	m->appliance = opt_key(L, data_index, "appliance", 0);
	m->worktick = get_key(L, data_index, "worktick");
	m->productivity = 0;
	m->power = 0;
	m->worktime = 0;
}

static int
lmachine_import(lua_State *L) {
	struct machine_arena *M = getM(L);
	group_clear(&M->M);
	list_reset(working, &M->working);
	M->working_list = LIST_EMPTY;
	if (lua_isnoneornil(L, 2))
		return 0;
	luaL_checktype(L, 2, LUA_TTABLE);	// data
	int index = 0;
	while (lua_geti(L, 2, ++index) == LUA_TTABLE) {
		import_machine(L, M, -1);
		lua_pop(L, 1);
	}
	return 0;
}

static int
lmachine_info(lua_State *L) {
	struct machine_arena *M = getM(L);
	int id = luaL_checkinteger(L, 2);
	struct machine *m = (struct machine *)group_object(&M->M, id);
	if (m == NULL)
		luaL_error(L, "Can't import machine id = %d", id);
	if (lua_isstring(L, 3)) {
		const char *key = lua_tostring(L, 3);
		if (strcmp(key, "workprocess") == 0) {
			if (m->worktick == 0) {
				lua_pushnumber(L, 0);
			} else {
				double worktick = (double)m->worktick/0x10000;
				lua_pushnumber(L, worktick / m->worktime);
			}
		} else if (strcmp(key, "worktick") == 0) {
			lua_pushnumber(L, (double)m->worktick/0x10000);
		} else if (strcmp(key, "worktime") == 0) {
			lua_pushinteger(L, m->worktime);
		} else if (strcmp(key, "productivity") == 0) {
			lua_pushnumber(L, (double)m->productivity/0x10000);
		} else if (strcmp(key, "appliance") == 0) {
			lua_pushinteger(L, m->appliance);
		} else {
			return luaL_error(L, "Invalid key %s", key);
		}
		return 1;
	}
	lua_newtable(L);
	lua_pushinteger(L, m->appliance);
	lua_setfield(L, -2, "appliance");
	lua_pushnumber(L, (double)m->productivity/0x10000);
	lua_setfield(L, -2, "productivity");
	lua_pushinteger(L, m->worktime);
	lua_setfield(L, -2, "worktime");
	lua_pushnumber(L, (double)m->worktick/0x10000);
	lua_setfield(L, -2, "worktick");
	return 1;
}

static int
machine_work(struct machine *m, struct powergrid *P) {
	if (m->appliance) {
		// need power
		struct capacitance * cap = powergrid_get_level(P, m->appliance);
		if (cap->level) {
			return 0;
		}
		cap->level = m->power;
	}
	if (m->productivity == 0)	// stop
		return 1;
	if (m->worktick >= ((uint64_t)m->worktime << 16)) {
		// finish
		return 1;
	}
	m->worktick += m->productivity;
	return 0;
}

static int
lmachine_tick(lua_State *L) {
	struct machine_arena *M = getM(L);
	struct powergrid *P = luaL_checkudata(L, 2, POWERGRID_LUAKEY);
	
	int iter = M->working_list;
	struct working *w = NULL;
	while ((w = list_each(working, &M->working, &iter))) {
		struct machine *m = (struct machine *)group_object(&M->M, w->id);
		if (m == NULL) {
			// machine delete
			iter = list_remove(working, &M->working, &M->working_list, w);
		} else {
			int stop = machine_work(m, P);
			if (stop) {
				m->working = 0;
				iter = list_remove(working, &M->working, &M->working_list, w);
			}
		}
	}
	return 0;
}

static int
lmachine_status(lua_State *L) {
	struct machine_arena *M = getM(L);
	int id = luaL_checkinteger(L, 2);
	struct machine *m = (struct machine *)group_object(&M->M, id);
	if (m == NULL)
		return luaL_error(L, "Invalid machine id %d", id);
	if (m->worktick >= ((uint64_t)m->worktime << 16)) {
		lua_pushstring(L, "finish");
		return 1;
	}
	if (m->appliance) {
		struct powergrid *P = luaL_checkudata(L, 3, POWERGRID_LUAKEY);
		struct capacitance *c = powergrid_get_level(P, m->appliance);
		if (c == NULL)
			return luaL_error(L, "No appliance id %d", m->appliance);
		if (c->level != m->power) {
			// no power
			lua_pushstring(L, "nopower");
			return 1;
		}
	}
	if (m->productivity == 0) {
		lua_pushstring(L, "stop");
		return 1;
	}
	lua_pushstring(L, "working");
	return 1;
}

static int
lmachine_reset(lua_State *L) {
	struct machine_arena *M = getM(L);
	int id = luaL_checkinteger(L, 2);
	struct machine *m = (struct machine *)group_object(&M->M, id);
	if (m == NULL)
		return luaL_error(L, "Invalid machine id %d", id);
	m->worktick = 0;
	printf("Machine reset\n");
	set_working(L, M, m);
	return 0;
}

static int
lmachine_new(lua_State *L) {
	struct machine_arena *M = (struct machine_arena *)lua_newuserdatauv(L, sizeof(struct machine_arena), MACHINE_COUNT);
	group_init(L, -1, MACHINE_MACHINE, &M->M, sizeof(struct machine));
	list_init(working, &M->working, MACHINE_WORKING);
	M->working_list = LIST_EMPTY;
	if (luaL_newmetatable(L, "GAME_MACHINE")) {
		luaL_Reg l[] = {
			{ "add", lmachine_add },
			{ "remove", lmachine_remove },
			{ "set_recipe", lset_recipe },
			{ "set_productivity", lset_productivity },
			{ "status", lmachine_status },
			{ "reset", lmachine_reset },
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

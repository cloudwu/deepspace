#include <lua.h>
#include <lauxlib.h>

#include "lgame.h"
#include "powergrid.h"
#include "group.h"

struct machine {
	int appliance;	// for powergrid
	int productivity;	// 0x10000 : 100%
	int power;	// for powergrid, set by recipe
	int worktime;	// set by recipe
	uint64_t worktick;	// fixnumber ( worktick * 0x10000 => worktime ) 
};

struct machine_arena {
	struct group M;
};

#define MACHINE_MACHINE 1
#define MACHINE_COUNT 1

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
	m->appliance = luaL_optinteger(L, 2, 0);
	m->power = 0;
	m->worktime = 0;
	m->worktick = 0;
	m->productivity = 0;
	lua_pushinteger(L, group_handle(&M->M, m));
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
	m->productivity = get_fixnumber(L, 3, "productivity");
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
	m->appliance = get_key(L, -1, "appliance");
	m->productivity = get_key(L, -1, "productivity");
	m->worktick = get_key(L, -1, "worktick");
	m->power = opt_key(L, -1, "power", 0);
	m->worktime = get_key(L, -1, "worktime");
	lua_pop(L, 1);
}

static int
lmachine_import(lua_State *L) {
	struct machine_arena *M = getM(L);
	luaL_checktype(L, 2, LUA_TTABLE);	// data
	luaL_checktype(L, 3, LUA_TTABLE);	// recipe
	group_clear(&M->M);
	int index = 0;
	while (lua_geti(L, 2, ++index) == LUA_TTABLE) {
		import_machine(L, &M->M, 3);
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

static void
machine_work(struct machine *m, struct powergrid *P) {
	if (m->productivity == 0)	// stop
		return;
	if (m->worktick >= ((uint64_t)m->worktime << 16)) {
		// finish
		return;
	}
	if (m->appliance) {
		// need power
		struct capacitance * cap = powergrid_get_level(P, m->appliance);
		if (cap->level) {
			return;						
		}
		cap->level = m->power;
	}
	m->worktick += m->productivity;
}

static int
lmachine_tick(lua_State *L) {
	struct machine_arena *M = getM(L);
	struct powergrid *P = luaL_checkudata(L, 2, POWERGRID_LUAKEY);

	struct machine *m = NULL;
	while((m = (struct machine *)group_each(&M->M, m))) {
		machine_work(m, P);
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
	if (m->productivity == 0) {
		lua_pushstring(L, "stop");
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
	return 0;
}

static int
lmachine_new(lua_State *L) {
	struct machine_arena *M = (struct machine_arena *)lua_newuserdatauv(L, sizeof(struct machine_arena), MACHINE_COUNT);
	group_init(L, -1, MACHINE_MACHINE, &M->M, sizeof(struct machine));
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

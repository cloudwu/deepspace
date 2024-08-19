#include <lua.h>
#include <lauxlib.h>
#include <stdint.h>

#include "lgame.h"
#include "group.h"

struct capacitance {
	uint32_t level;
};

struct battery {
	uint64_t level;
	uint64_t cap;
	uint32_t power;
	uint32_t charge;
};

struct powergrid {
	struct group generator;
	struct group appliance;
	struct group battery;
};

#define GENERATOR_INDEX 1
#define APPLIANCE_INDEX 2
#define BATTERY_INDEX 3
#define GROUP_COUNT 3

static inline int
build_index(int type, int id) {
	return (id << 2) | (type & 3);	
}

static inline int
handle_type(lua_Integer h) {
	return h & 3;
}

static inline int
handle_id(lua_Integer h) {
	return h >> 2;
}

static inline int
check_index(lua_State *L, int type, lua_Integer id) {
	if (type != (id & 3))
		return luaL_error(L, "Invalid type %d for %x", type, id);
	return id >> 2;
}

static void
push_handle(lua_State *L, int type, struct group *g, void *obj) {
	unsigned int h = group_handle(g, obj);
	lua_pushinteger(L, build_index(type, h));
}

static struct powergrid *
getP(lua_State *L) {
	struct powergrid *P = (struct powergrid *)lua_touserdata(L, 1);
	if (P == NULL)
		luaL_error(L, "Need userdata powergrid");
	return P;
}

static int
lgenerator_add(lua_State *L) {
	struct powergrid *P = getP(L);
	struct capacitance * obj = (struct capacitance *)group_add(L, 1, &P->generator);
	obj->level = 0;
	push_handle(L, GENERATOR_INDEX, &P->generator, obj);
	return 1;
}

static int
lappliance_add(lua_State *L) {
	struct powergrid *P = getP(L);
	struct capacitance * obj = (struct capacitance *)group_add(L, 1, &P->appliance);
	obj->level = 0;
	push_handle(L, APPLIANCE_INDEX, &P->appliance, obj);
	return 1;
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
lbattery_add(lua_State *L) {
	struct powergrid *P = getP(L);
	struct battery * obj = (struct battery *)group_add(L, 1, &P->battery);
	obj->level = 0;
	obj->cap = get_key(L, 2, "cap");
	obj->power = get_key(L, 2, "power");
	obj->charge = get_key(L, 2, "charge");
	
	push_handle(L, BATTERY_INDEX, &P->battery, obj);
	return 1;
}

static int
lremove(lua_State *L) {
	struct powergrid *P = getP(L);
	lua_Integer id = luaL_checkinteger(L, 2);
	struct group *g = NULL;
	switch (handle_type(id)) {
	case GENERATOR_INDEX:
		g = &P->generator;
		break;
	case APPLIANCE_INDEX:
		g = &P->appliance;
		break;
	case BATTERY_INDEX:
		g = &P->battery;
		break;
	default:
		return luaL_error(L, "Invalid handle type %x", id);
	}
	group_remove(g, group_object(g, handle_id(id)));
	return 0;
}

static void *
get_object(lua_State *L, struct group *g, int h) {
	void *r = group_object(g, h);
	if (r == NULL)
		luaL_error(L, "Invalid id %d", h);
	return r;
}

static int
lset_level(lua_State *L) {
	struct powergrid *P = getP(L);
	lua_Integer id = luaL_checkinteger(L, 2);
	int level = luaL_checkinteger(L, 3);
	int h = handle_id(id);
	struct capacitance *c;
	struct battery *b;
	switch (handle_type(id)) {
	case GENERATOR_INDEX:
		c = (struct capacitance *)get_object(L, &P->generator, h);
		c->level = level;
		break;
	case APPLIANCE_INDEX:
		c = (struct capacitance *)get_object(L,  &P->appliance, h);
		c->level = level;
		break;
	case BATTERY_INDEX:
		b = (struct battery *)get_object(L,  &P->battery, h);
		b->level = level;
		break;
	default:
		return luaL_error(L, "Invalid handle type %x", id);
	}
	return 0;
}

static void
export_table_with_id(lua_State *L, int h) {
	lua_newtable(L);
	lua_pushinteger(L, h);
	lua_setfield(L, -2, "id");
}

static int
export_capacitance(lua_State *L, struct group *g, int type, int from_index) {
	struct capacitance * obj = NULL;
	while ((obj = group_each(g, obj))) {
		int h = build_index(type, group_handle(g, obj));
		export_table_with_id(L, h);
		lua_pushinteger(L, obj->level);
		lua_setfield(L, -2, "level");
		lua_rawseti(L, -2, from_index++);
	}
	return from_index;
}

static int
lexport(lua_State *L) {
	struct powergrid *P = getP(L);
	lua_newtable(L);
	int from_index = 1;
	from_index = export_capacitance(L, &P->generator, GENERATOR_INDEX, from_index);
	from_index = export_capacitance(L, &P->appliance, APPLIANCE_INDEX, from_index);
	struct battery * obj = NULL;
	while ((obj = group_each(&P->battery, obj))) {
		int h = build_index(BATTERY_INDEX, group_handle(&P->battery, obj));
		export_table_with_id(L, h);
		lua_pushinteger(L, obj->level);
		lua_setfield(L, -2, "level");
		lua_pushinteger(L, obj->cap);
		lua_setfield(L, -2, "cap");
		lua_pushinteger(L, obj->power);
		lua_setfield(L, -2, "power");
		lua_pushinteger(L, obj->charge);
		lua_setfield(L, -2, "charge");
		lua_rawseti(L, -2, from_index++);
	}
	return 1;	
}

static int
limport(lua_State *L) {
	struct powergrid *P = getP(L);
	luaL_checktype(L, 2, LUA_TTABLE);
	group_clear(&P->generator);
	group_clear(&P->appliance);
	group_clear(&P->battery);
	int index = 0;
	while(lua_geti(L, 2, ++index) == LUA_TTABLE) {
		int id = get_key(L, -1, "id");
		int h = handle_id(id);
		struct capacitance *c;
		struct battery *b;
		switch (handle_type(id)) {
		case GENERATOR_INDEX:
			c = (struct capacitance *)group_import(L, 1, &P->generator, h);
			c->level = get_key(L, -1, "level");
			break;
		case APPLIANCE_INDEX:
			c = (struct capacitance *)group_import(L, 1, &P->appliance, h);
			c->level = get_key(L, -1, "level");
			break;
		case BATTERY_INDEX:
			b = (struct battery *)group_import(L, 1, &P->battery, h);
			b->level = get_key(L, -1, "level");
			b->cap = get_key(L, -1, "cap");
			b->power = get_key(L, -1, "cap");
			b->charge = get_key(L, -1, "charge");
			break;
		default:
			return luaL_error(L, "Invalid id %d for [%d]", id, index);
		}
		lua_pop(L, 1);
	}
	return 0;
}

struct power_context {
	uint64_t total_output;
	uint64_t total_need;
	uint64_t battery_output;
	uint64_t battery_consume;
};

static uint64_t
capacitance_stat(struct group *g) {
	struct capacitance *c = NULL;
	uint64_t t = 0;
	while ((c = group_each(g, c))) {
		t += c->level;		
	}
	return t;
}

static void
battery_charge(struct power_context *ctx, struct group *g) {
	ctx->battery_output = 0;
	uint64_t need = 0;
	struct battery *b = NULL;
	while ((b = (struct battery *)group_each(g, b))) {
		uint64_t e = b->cap - b->level;
		if (e > b->charge)
			e = b->charge;
		need += e;
	}
	uint64_t extra = ctx->total_output - ctx->total_need;
	
	if (need == 0 || extra == 0) {
		// no battery
		ctx->battery_consume = 0;
		return;
	}
	if (need <= extra) {
		// charge all
		b = NULL;
		while ((b = (struct battery *)group_each(g, b))) {
			b->level += b->charge;
			if (b->level > b->cap) {
				b->level = b->cap;
			}
		}
		ctx->battery_consume = need;
	} else {
		// part charge
		uint64_t scale = (extra << 32) / need;
		if (scale == 0) {
			// too little enery
			ctx->battery_consume = 0;
			return;
		}
		uint64_t charge = 0;
		b = NULL;
		while ((b = (struct battery *)group_each(g, b))) {
			uint64_t e = b->cap - b->level;
			if (e > b->charge)
				e = b->charge;
			e = (e * scale) >> 32;
			b->level += e;
			charge += e;
		}
		ctx->battery_consume = charge;
	}
}

static void
battery_discharge(struct power_context *ctx, struct group *g) {
	ctx->battery_consume = 0;
	uint64_t energy = 0;	
	struct battery *b = NULL;
	while ((b = (struct battery *)group_each(g, b))) {
		uint64_t e = b->power;
		if (e > b->level)
			e = b->level;
		energy += e;
	}
	uint64_t need = ctx->total_need - ctx->total_output;
	if (energy == 0 || need == 0) {
		// no power
		ctx->battery_output = 0;
		return;
	}
	if (energy <= need) {
		// discharge all
		b = NULL;
		while ((b = (struct battery *)group_each(g, b))) {
			if (b->power > b->level)
				b->level = 0;
			else
				b->level -= b->power;
		}
		ctx->battery_output = energy;
	} else {
		// part discharge
		uint64_t scale = (need << 32) / energy;
		if (scale == 0) {
			ctx->battery_output = 0;
			return;
		}
		uint64_t output = 0;
		b = NULL;
		while ((b = (struct battery *)group_each(g, b))) {
			uint64_t e = b->power;
			if (e > b->level)
				e = b->level;
			e = (e * scale) >> 32;
			b->level -= e;
			output += e;
		}
		ctx->battery_output = output;
	}
}

static void
generate_discharge(struct power_context *ctx, struct group *g) {
	if (ctx->total_output == 0)	// no power
		return;
	uint64_t total_need = ctx->total_need + ctx->battery_consume - ctx->battery_output;
	if (total_need == 0) // no need
		return;
	if (ctx->total_output <= total_need) {
		// discharge all
		struct capacitance * c = NULL;
		while ((c = (struct capacitance *)group_each(g, c))) {
			c->level = 0;
		}
	} else {
		uint64_t scale = (total_need << 32) / ctx->total_output;
		if (scale == 0)
			return;
		scale = ((uint64_t)1 << 32) - scale;
//		printf("discharge scale = %f\n", (double)scale / ((uint64_t)1 << 32));
		struct capacitance * c = NULL;
		while ((c = (struct capacitance *)group_each(g, c))) {
			c->level = (c->level * scale) >> 32;
		}
	}
}

static void
comsume(struct power_context *ctx, struct group *g) {
	if (ctx->total_need == 0)	// no need
		return;
	uint64_t total_energy = ctx->total_output + ctx->battery_output - ctx->battery_consume;
	if (total_energy == 0) // no power
		return;
	if (ctx->total_need <= total_energy) {
		struct capacitance * c = NULL;
		while ((c = (struct capacitance *)group_each(g, c))) {
			c->level = 0;
		}
	} else {
		uint64_t scale = (total_energy << 32) / ctx->total_need;
		if (scale == 0)
			return;
		scale = ((uint64_t)1 << 32) - scale;
		struct capacitance * c = NULL;
		while ((c = (struct capacitance *)group_each(g, c))) {
			c->level = (c->level * scale) >> 32;
		}
	}
}

//#include <stdio.h>

static int
ltick(lua_State *L) {
	struct powergrid *P = getP(L);
	struct power_context ctx;
	ctx.total_output = capacitance_stat(&P->generator);
	ctx.total_need = capacitance_stat(&P->appliance);
	if (ctx.total_output >= ctx.total_need) {
		battery_charge(&ctx, &P->battery);
	} else {
		battery_discharge(&ctx, &P->battery);
	}
//	printf("output = %lld need = %lld battery %lld/%lld\n", 
//		ctx.total_output,
//		ctx.total_need,
//		ctx.battery_output,
//		ctx.battery_consume
//	);
	generate_discharge(&ctx, &P->generator);
	comsume(&ctx, &P->appliance);
	return 0;	
}

static int
lpowergrid_new(lua_State *L) {
	struct powergrid *P = (struct powergrid *)lua_newuserdatauv(L, sizeof(struct powergrid), GROUP_COUNT);
	group_init(L, -1, GENERATOR_INDEX, &P->generator, sizeof(struct capacitance));
	group_init(L, -1, APPLIANCE_INDEX, &P->appliance, sizeof(struct capacitance));
	group_init(L, -1, BATTERY_INDEX, &P->battery, sizeof(struct battery));
	if (luaL_newmetatable(L, "GAME_POWERGRID")) {
		luaL_Reg l[] = {
			{ "generator_add", lgenerator_add },
			{ "appliance_add", lappliance_add },
			{ "battery_add", lbattery_add },
			{ "remove", lremove },
			{ "set_level", lset_level },	// for debug
			{ "tick", ltick },
			{ "export", lexport },
			{ "import", limport },
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

GAME_MODULE(powergrid) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "new", lpowergrid_new },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}

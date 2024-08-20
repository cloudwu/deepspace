#ifndef game_powergrid_h
#define game_powergrid_h

#include <stdint.h>
#include "gamecapi.h"
#define POWERGRID_LUAKEY "GAME_POWERGRID"

struct powergrid;

struct capacitance {
	uint32_t level;
};

struct powergrid_api {
	struct capacitance* (*get_level)(struct powergrid *P, int handle);
};

static inline struct capacitance *
powergrid_get_level(struct powergrid *P, int handle) {
	return GAME_CAPI(powergrid, P)->get_level(P, handle);
}

#endif

#ifndef game_container_h
#define game_container_h

#include "gamecapi.h"
#define CONTAINER_LUAKEY "GAME_CONTAINER"

struct container;

struct container_api {
	int (*put)(struct container *C, int id, int type, int count, int dryrun);
	int (*take)(struct container *C, int id, int type, int count, int dryrun);
};

static inline int
container_put(struct container *C, int id, int type, int count, int dryrun) {
	return GAME_CAPI(container, C)->put(C, id, type, count, dryrun);
}

static inline int
container_take(struct container *C, int id, int type, int count, int dryrun) {
	return GAME_CAPI(container, C)->take(C, id, type, count, dryrun);
}

#endif

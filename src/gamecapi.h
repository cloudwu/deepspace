#ifndef game_capi_h
#define game_capi_h

struct game_capi {
	void * api_table;
};

#define GAME_CAPI(T, obj) ((struct T##_api *)(((struct game_capi *)obj)->api_table))

#endif


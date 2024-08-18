#ifndef object_group_h
#define object_group_h

#include <lua.h>

struct handle;

struct group {
	int stride;
	int uv;
	void *data;
	struct handle *index;
};

void group_init(lua_State *L, int index, int uv, struct group *, int stride);
void group_clear(struct group *);
void * group_add(lua_State *L, int index, struct group *);
void * group_import(lua_State *L, int index, struct group *, unsigned int id);
void * group_object(struct group *, unsigned int id);
void group_remove(struct group *, void *obj);
unsigned int group_handle(struct group *, void *obj);
void * group_each(struct group *, void *iter);

#endif

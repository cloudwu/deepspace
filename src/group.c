#include <lua.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "handle.h"
#include "group.h"

#define DEFAULT_BITS 7

static inline size_t
group_size(int bits, int stride) {
	size_t data_size = stride << bits;
	size_t index_size = handle_size(bits);
	return data_size + index_size;
}

void
group_init(lua_State *L, int idx, int uv, struct group *G, int stride) {
	idx = lua_absindex(L, idx);
	char * ptr = lua_newuserdatauv(L, group_size(DEFAULT_BITS, stride), 0);
	G->uv = uv;
	G->stride = stride;
	G->data = (void *)ptr;
	G->index = (struct handle *)(ptr + (stride << DEFAULT_BITS));
	handle_init(G->index, NULL, DEFAULT_BITS, handle_size(DEFAULT_BITS));
	lua_setiuservalue(L, idx, uv);
}

void
group_clear(struct group *G) {
	handle_clear(G->index);
}

static inline void *
handle_ptr(struct group *G, unsigned int h) {
	int mask = (1 << G->index->bits) - 1;
	return (void *)((char *)G->data + (h & mask) * G->stride);
}

static void
expand_handle(lua_State *L, int idx, struct group *G) {
	int bits = G->index->bits + 1;
	char * ptr = lua_newuserdatauv(L, group_size(bits, G->stride), 0);
	void * data = (void *)ptr;
	struct handle * index = (struct handle *)(ptr + (G->stride << bits));
	handle_init(index, G->index, bits, handle_size(bits));
	unsigned int iter = 0;
	int mask_old = (1 << G->index->bits) - 1;
	int mask_new = (1 << index->bits) - 1;
	int stride = G->stride;
	while ((iter = handle_each(G->index, iter))) {
		int index_old = iter & mask_old;
		int index_new = iter & mask_new;
		memcpy((char *)data + index_new * stride, (char *)G->data + index_old * stride, stride);
	}
	lua_setiuservalue(L, idx, G->uv);
	G->data = data;
	G->index = index;
}

void *
group_add(lua_State *L, int idx, struct group *G) {
	unsigned int h = handle_new(G->index);
	if (h == 0) {
		idx = lua_absindex(L, idx);
		expand_handle(L, idx, G);
		h = handle_new(G->index);
		assert(h > 0);
	}
	return handle_ptr(G, h);
}

void *
group_import(lua_State *L, int idx, struct group *G, unsigned int id) {
	idx = lua_absindex(L, idx);
	int err;
	while ((err = handle_import(G->index, id, 0))) {
		if (err < 0)
			return NULL;
		expand_handle(L, idx, G);
	}
	return group_object(G, id);
}

void *
group_object(struct group *G, unsigned int id) {
	if (handle_invalid(G->index, id))
		return NULL;
	return handle_ptr(G, id);
}

unsigned int
group_handle(struct group *G, void *obj) {
	int index = ((char *)obj - (char *)G->data) / G->stride;
	return handle_index(G->index, index);
}

void
group_remove(struct group *G, void *obj) {
	handle_remove(G->index, group_handle(G, obj));
}

void *
group_each(struct group *G, void *iter) {
	unsigned int h;
	if (iter == NULL) {
		h = 0;
	} else {
		h = group_handle(G, iter);
	}
	h = handle_each(G->index, h);
	if (h == 0)
		return NULL;
	return handle_ptr(G, h);
}



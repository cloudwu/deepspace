#ifndef object_array_h
#define object_array_h

#include <lua.h>
#include <stdlib.h>

#define CACHE_SIZE 8191

struct hash_cache {
	int index[CACHE_SIZE];		
};

struct array {
	int id;
	int sorted;
	int n;
	int cap;
	struct hash_cache hash;
	void *s;
};

#define ARRAY_TYPE(T) struct array_##T { int id; struct T data; };
#define ARRAY_SIZEOF(T) sizeof(struct array_##T)
#define ARRAY_OFFSETOF(T) offsetof(struct array_##T, data)

#define array_init(T, a, uv) array_init_(L, -1, uv, a, ARRAY_SIZEOF(T))
void array_init_(lua_State *L, int index, int uv, struct array *, size_t stride);

#define array_add(T, a, uv) (struct T*)array_add_(L, 1, uv, a, ARRAY_SIZEOF(T), ARRAY_OFFSETOF(T))
void * array_add_(lua_State *L, int index, int uv, struct array *, size_t stride, size_t off);

#define array_import(T, a, id, uv) (struct T*)array_import_(L, 1, uv, a, id, ARRAY_SIZEOF(T), ARRAY_OFFSETOF(T))
void * array_import_(lua_State *L, int index, int uv, struct array *, int id, size_t stride, size_t off);

#define array_find(T, a, id) (struct T*)array_find_(a, id, ARRAY_SIZEOF(T), ARRAY_OFFSETOF(T))
void * array_find_(struct array *, int id, size_t stride, size_t off);

#define array_remove(T, a, obj) array_remove_(a, obj, ARRAY_SIZEOF(T), ARRAY_OFFSETOF(T))
void array_remove_(struct array *, void *obj, size_t stride, size_t off);

#define array_id(T, obj) array_id_(obj, ARRAY_OFFSETOF(T))
static inline int
array_id_(void *obj, size_t off) {
	char * ptr = (char *)obj;
	int * id = (int *)(ptr - off);
	return *id;
}

void array_reset(struct array *A);

#define array_each(T, A, obj) array_each_(A, obj, ARRAY_SIZEOF(T), ARRAY_OFFSETOF(T))
void * array_each_(struct array *A, void *obj, size_t stride, size_t off);

#endif

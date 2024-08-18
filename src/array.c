#include "array.h"
#include <string.h>
#include <assert.h>

#define DEFAULT_SIZE 128

struct array_slot {
	int id;
};

static inline int
inthash(int p) {
    int h = (2654435761 * p) % CACHE_SIZE;
    return h;
}

static inline int *
hash_cache_lookup(struct hash_cache *h, int id) {
	int key = inthash(id);
	return &h->index[key];	
}

static int
compar_slot(const void *a, const void *b) {
	const struct array_slot *ca = (const struct array_slot *)a;
	const struct array_slot *cb = (const struct array_slot *)b;
	return ca->id - cb->id;
}

static void inline
sort_array(struct array *A, size_t stride) {
	if (!A->sorted) {
		// sort
		qsort(A->s, A->n, stride, compar_slot);
		A->sorted = 1;
	}
}

static inline struct array_slot *
index_(struct array *A, int n, size_t stride) {
	return (struct array_slot *)((char *)A->s + n * stride);
}

static inline void *
array_data(struct array_slot *s, size_t off) {
	return (void *)((char *)s + off);
}

void
array_reset(struct array *A) {
	A->n = 0;
	A->sorted = 1;
	A->id = 0;
}

static void *
add_id(lua_State *L, int index, int uv, struct array *A, int id, size_t stride, size_t off) {
	void * obj = array_add_(L, index, uv, A, stride, off);
	char * ptr = (char *)obj;
	int *v = (int *)(ptr - off);
	*v = id;
	return obj;
}

void
array_init_(lua_State *L, int index, int uv, struct array *A, size_t stride) {
	index = lua_absindex(L, index);
	A->id = 0;
	A->cap = DEFAULT_SIZE;
	A->n = 0;
	A->sorted = 1;
	A->s = lua_newuserdatauv(L, stride * A->cap, 0);
	lua_setiuservalue(L, index, uv);
}

void *
array_add_(lua_State *L, int index, int uv, struct array *A, size_t stride, size_t off) {
	if (A->n >= A->cap) {
		// expand container
		index = lua_absindex(L, index);
		int cap = (A->cap * 3) / 2 + 1;
		void *temp = lua_newuserdatauv(L, stride * cap, 0);
		memcpy(temp, A->s, stride * A->cap);	
		lua_setiuservalue(L, index, uv);
		A->s = temp;
	}
	int n = A->n++;
	struct array_slot * obj = index_(A, n, stride);
	void * data = array_data(obj, off);
	memset(data, 0, stride - off);
	obj->id = ++A->id;
	return data;
}

static inline struct array_slot *
find_with_id(struct array *A, int id, size_t stride) {
	int *h = hash_cache_lookup(&A->hash, id);
	int index = *h;
	if (index < 0 || index >= A->n)
		index = A->n / 2;
	struct array_slot *obj = index_(A, index, stride);
	if (obj->id == id) {
		return obj;
	}
	sort_array(A, stride);
	int begin = 0;
	int end = A->n;
	int mid = index;
	while (begin < end) {
		struct array_slot *c = index_(A, mid, stride);
		if (c->id == id) {
			*h = mid;
			return c;
		} else if (c->id < id) {
			begin = mid + 1;
		} else {
			end = mid;
		}
		mid = (begin + end) / 2;
	}
	return NULL;
}

void *
array_import_(lua_State *L, int index, int uv, struct array *A, int id, size_t stride, size_t off) {
	if (id >= A->id) {
		void * obj = add_id(L, index, uv, A, id, stride, off);
		A->id = id;
		return obj;
	}
	struct array_slot * r = find_with_id(A, id, stride);
	if (r) {
		return r;
	}
	int old_id = A->id;
	void * obj = add_id(L, index, uv, A, id, stride, off);
	A->id = old_id;
	A->sorted = 0;
	return obj;
}

void *
array_find_(struct array *A, int id, size_t stride, size_t off) {
	struct array_slot *s = find_with_id(A, id, stride);
	if (s)
		return array_data(s, off);
	return NULL;
}

void
array_remove_(struct array *A, void *obj, size_t stride, size_t off) {
	assert(obj);	
	char * ptr = (char *)obj - off;
	int top = --A->n;
	memcpy(ptr, index_(A, top, stride), stride);
	A->sorted = 0;
}

void *
array_each_(struct array *A, void *obj, size_t stride, size_t off) {
	int index;
	if (obj == NULL) {
		sort_array(A, stride);
		if (A->n == 0)
			return NULL;
		return array_data((struct array_slot *)(A->s), off);
	}
	char *ptr = (char *)obj - off;
	assert(ptr >= (char *)A->s);
	index = (ptr - (char *)A->s) / stride + 1;
	if (index >= A->n)
		return NULL;
	struct array_slot * r = index_(A, index, stride);
	return array_data(r, off);
}

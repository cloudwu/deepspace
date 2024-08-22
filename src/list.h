#ifndef object_list_h
#define object_list_h

#include <lua.h>
#include <stdlib.h>

struct list {
	int cap;
	int freelist;
	void *s;
};

#define LIST_TYPE(T) struct list_##T { int next; struct T data; };
#define LIST_SIZEOF(T) sizeof(struct list_##T)
#define LIST_OFFSETOF(T) offsetof(struct list_##T, data)
#define LIST_EMPTY (-1)

#define list_init(T, t, uv) list_init_(L, -1, uv, t, LIST_SIZEOF(T))
void list_init_(lua_State *L, int index, int uv, struct list *, size_t stride);

#define list_reset(T, t) list_reset_(t, LIST_SIZEOF(T))
void list_reset_(struct list *, size_t stride);

#define list_object(T, t, id) (struct T *)list_object_(t, id, LIST_SIZEOF(T), LIST_OFFSETOF(T))
static inline void *
list_object_(struct list *T, int n, size_t stride, size_t off) {
	if (n < 0)
		return NULL;
	return (void *)((char *)T->s + n * stride + off);
}

#define list_each(T, t, iter) list_each_(t, iter, LIST_SIZEOF(T), LIST_OFFSETOF(T))
static inline void *
list_each_(struct list *T, int *n, size_t stride, size_t off) {
	if (*n == LIST_EMPTY)
		return NULL;
	char * ptr = (char *)T->s + *n * stride;
	*n = *(int *)ptr;
	return (void *)(ptr + off);
}

#define list_remove(T, t, head, p) list_remove_(t, head, p, LIST_SIZEOF(T), LIST_OFFSETOF(T))
void list_remove_(struct list *T, int *head, void *p, size_t stride, size_t off);

#define list_clear(T, t, head) list_clear_(t, head, LIST_SIZEOF(T))
void list_clear_(struct list *T, int head, size_t stride);

#define list_add(T, t, head, uv) (struct T *)list_add_(L, 1, uv, t, head,  LIST_SIZEOF(T), LIST_OFFSETOF(T))
void * list_add_(lua_State *L, int index, int uv, struct list *T, int *head, size_t stride, size_t off);

#endif
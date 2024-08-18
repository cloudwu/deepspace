#include "list.h"
#include <assert.h>
#include <string.h>

#define DEFAULT_SIZE 128

struct list_slot {
	int next;
};

static inline struct list_slot *
index_(struct list *T, int n, size_t stride) {
	return (struct list_slot *)((char *)T->s + n * stride);
}

static void
init_slots(struct list *T, size_t stride) {
	int n = T->cap;
	int i;
	for (i = 0; i<n-1; i++) {
		struct list_slot *s = index_(T, i, stride);
		s->next = i + 1;
	}
	struct list_slot *s = index_(T, n-1, stride);
	s->next = LIST_EMPTY;
}

static void
init_list(lua_State *L, int index, int uv, struct list * T, int n, size_t stride) {
	struct list tmp;
	tmp.s = lua_newuserdatauv(L, stride * n, 0);
	tmp.cap = n;
	
	init_slots(&tmp, stride);

	if (T->s == NULL) {
		*T = tmp;
		T->freelist = 0;
	} else {
		assert(T->freelist == LIST_EMPTY);
		assert(T->cap <= n);
		memcpy(tmp.s, T->s, T->cap * stride);
		T->s = tmp.s;
		T->freelist = T->cap;
		T->cap = n;
	}
	
	lua_setiuservalue(L, index, uv);
}

static int
new_slot(lua_State *L, int index, int uv, struct list *T, size_t stride) {
	int freelist = T->freelist;
	if (freelist == LIST_EMPTY) {
		init_list(L, index, uv, T, (T->cap + 1) * 3 / 2, stride);
		freelist = T->freelist;
		assert(freelist != LIST_EMPTY);
	}
	struct list_slot *p = index_(T, freelist, stride);
	T->freelist = p->next;
	memset(p, 0, stride);
	p->next = LIST_EMPTY;
	return freelist;
}

void
list_init_(lua_State *L, int index, int uv, struct list *T, size_t stride) {
	T->s = NULL;
	init_list(L, index, uv, T, DEFAULT_SIZE, stride);
}

void
list_reset_(struct list *T, size_t stride) {
	init_slots(T, stride);
	T->freelist = 0;
}

static void
remove_last(struct list *T, int *head, size_t stride) {
	int storage = *head;
	if (storage == LIST_EMPTY)
		return;
	struct list_slot *current = index_(T, storage, stride);
	if (current->next == LIST_EMPTY) {
		current->next = T->freelist;
		T->freelist = storage;
		*head = LIST_EMPTY;
		return;
	}
	for (;;) {
		struct list_slot *next = index_(T, current->next, stride);
		if (next->next == LIST_EMPTY) {
			next->next = T->freelist;
			T->freelist = current->next;
			current->next = LIST_EMPTY;
			return;
		}
		current = next;
	}
}

void
list_remove_(struct list *T, int *head, void *ptr, size_t stride, size_t off) {
	struct list_slot *p = (struct list_slot *)((char *)ptr - off);
	int next = p->next;
	if (next == LIST_EMPTY) {
		remove_last(T, head, stride);
		return;
	}
	struct list_slot *np = index_(T, next, stride);
	memcpy(p, np, stride);
	np->next = T->freelist;
	T->freelist = next;
}

void *
list_add_(lua_State *L, int index, int uv, struct list *T, int *head, size_t stride, size_t off) {
	int slot_index = new_slot(L, index, uv, T, stride);
	struct list_slot *p = index_(T, slot_index, stride);
	p->next = *head;
	*head = slot_index;
	return (void *)((char *)p + off);
}

void
list_clear_(struct list *T, int head, size_t stride) {
	if (head != LIST_EMPTY) {
		struct list_slot * current = index_(T, head, stride);
		while (current->next != LIST_EMPTY) {
			current = index_(T, current->next, stride);
		}
		current->next = T->freelist;
		T->freelist = head;
	}
}

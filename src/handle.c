#include "handle.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define HANDLE_USED 0x80000000

size_t
handle_size(int bits) {
	int n = 1 << bits;
	return sizeof(struct handle) + sizeof(unsigned int) * (n - 1);
}

void
handle_clear(struct handle *H) {
	int n = (1 << H->bits);
	int i;
	for (i=1;i<n-1;i++) {
		H->h[i] = i+1;
	}
	H->h[0] = 1 + n;
	H->h[n-1] = 0;
	H->freelist = 0;
}

void
handle_init(struct handle *H, struct handle *old, int bits, size_t sz) {
	assert(sz >= handle_size(bits));
	int n = (1 << bits);
	H->bits = bits;
	
	if (old == NULL) {
		handle_clear(H);
	} else {
		assert(bits > old->bits);
		int c = 1 << (bits - old->bits);
		memset(H->h, 0, sizeof(unsigned int) * n);
		int i;
		// rehash
		int old_n = 1 << old->bits;
		int mask = (1 << H->bits) - 1;
		int old_mask = old_n -1;
		for (i=0;i<old_n;i++) {
			unsigned int v = old->h[i];
			if (v & HANDLE_USED) {
				int index = v & mask;
				H->h[index] = v;
				// mark lower slots
				unsigned int base = ((v & ~HANDLE_USED) & ~mask);
				int old_index = index & old_mask;
				int j;
				for (j = 0;	j < c ; j++) {
					if (old_index < index) {
						H->h[old_index] = base + mask + 1;
					} else if (old_index > index) {
						H->h[old_index] = base;
					}
					old_index += old_mask + 1;										
				}
			}
		}
		// link free
		int free_head = -1;
		int free_current = -1;
		
		for (i=0;i<n;i++) {
			if (!(H->h[i] & HANDLE_USED)) {
				if (free_head < 0) {
					free_head = free_current = i;
				} else {
					H->h[free_current] |= i;
					free_current = i;
				}
			}
		}
		// set free list
		H->h[free_current] = free_head;
		H->freelist = free_head;
	}
}

unsigned int
handle_new(struct handle *H) {
	int free_node = H->freelist;
	unsigned int v = H->h[free_node];
	if (v & HANDLE_USED)
		return 0;
	int mask = (1 << H->bits) - 1;
	int next = v & mask;
	unsigned int next_v = H->h[next];
	H->h[free_node] = (v & ~mask) | (next_v & mask);
	unsigned int r = (next_v & ~mask) | next;
	H->h[next] = r | HANDLE_USED;
	return r;
}

// 0 succ
// 1 need expand
// -1 error
int
handle_import(struct handle *H, unsigned int id, int invalid) {
	int mask = (1 << H->bits) - 1;
	int index = id & mask;
	unsigned int v = H->h[index];
	if (v & HANDLE_USED) {
		if ((v & ~HANDLE_USED) < id)
			return 1;
		return -1;
	}
	if (invalid) {
		if ((id & ~mask) > (v & ~mask)) {
			H->h[index] = (id & ~mask) | (v & mask);
		}
	} else {
		// remove index
		int prev = H->freelist;
		int next;
		while ((next = H->h[prev] & mask) != index) {
			prev = next;
		}
		H->h[prev] = (H->h[prev] & ~mask) | (v & mask);
		H->h[index] = id | HANDLE_USED;
	}
	return 0;
}

int
handle_invalid(struct handle *H, unsigned int id) {
	int mask = (1 << H->bits) - 1;
	return H->h[id & mask] != (id | HANDLE_USED);
}

void
handle_remove(struct handle *H, unsigned int id) {
	if (handle_invalid(H, id))
		return;
	int mask = (1 << H->bits) - 1;
	int inc = mask + 1;
	id = (id + inc) & ~HANDLE_USED;
	if (id == 0)
		id = inc;
	int index = id & mask;
	unsigned int last = H->h[H->freelist];
	if (last & HANDLE_USED) {
		// full
		H->freelist = index;
		H->h[index] = (id & ~mask) | index;
	} else {
		int next = last & mask;
		H->h[H->freelist] = (last & ~mask) | index;
		H->h[index] = (id & ~mask) | next;
		H->freelist = index;
	}
}

unsigned int
handle_each(struct handle *H, unsigned int last) {
	int n = 1 << H->bits;
	if (last == 0) {
		// first
		int i;
		for (i=0;i<n;i++) {
			unsigned int v = H->h[i];
			if (v & HANDLE_USED) {
				return v & ~HANDLE_USED;
			}
		}
		return 0;
	}
	int mask = (1 << H->bits) - 1;
	int index = last & mask;
	while (++index < n) {
		unsigned int v = H->h[index];
		if (v & HANDLE_USED) {
			return v & ~HANDLE_USED;
		}
	}
	return 0;
}

unsigned int
handle_each_available(struct handle *H, unsigned int last) {
	int mask = (1 << H->bits) - 1;
	if (last == 0) {
		// first
		unsigned int v = H->h[H->freelist];
		if (v & HANDLE_USED)
			return 0;
		return (v & ~mask) | H->freelist;
	}
	int index = last & mask;
	unsigned int v = H->h[index];
	if (v & HANDLE_USED)
		return 0;
	int next = v & mask;
	if (next == H->freelist)
		return 0;
	v = H->h[next];
	return (v & ~mask) | next;
}

unsigned int
handle_index(struct handle *H, int index) {
	int mask = (1 << H->bits) - 1;
	unsigned int v = H->h[index & mask];
	if (v & HANDLE_USED)
		return v & ~HANDLE_USED;
	return 0;
}

#ifdef TEST_HANDLE_MAIN

#include <stdio.h>

static void
print_handle(struct handle *H) {
	printf("FREE ");
	unsigned int id = 0;
	while ((id = handle_each_available(H, id))) {
		printf("%u ", id);
	}
	printf("\n");
	
	printf("USED ");
	id = 0;
	while ((id = handle_each(H, id))) {
		printf("%u ", id);
	}
	printf("\n");
}

int
main() {
	size_t sz = handle_size(2);
	struct handle *H = malloc(sz);
	handle_init(H, NULL, 2, sz);
	print_handle(H);
	int i;
	for (i=0;i<4;i++) {
		unsigned int v = handle_new(H);
		printf("Alloc %u\n", v);
		print_handle(H);
	}
	unsigned int id = 1;
	for (i=0;i<4;i++) {
		handle_remove(H, id);
		unsigned int old_id = id;
		id = handle_new(H);
		printf("Realloc %d->%d\n", old_id, id);
	}
	print_handle(H);
	sz = handle_size(4);
	struct handle *H2 = malloc(sz);
	handle_init(H2, H, 4, sz);
	print_handle(H2);
	for (i=0;i<8;i++) {
		unsigned int v = handle_new(H2);
		printf("Alloc %u\n", v);
		print_handle(H2);
	}
	printf("COPY handles\n");
	struct handle *H3 = malloc(sz);
	handle_init(H3, NULL, 4, sz);
	print_handle(H3);
	unsigned int iter = 0;
	while ((iter = handle_each(H2, iter))) {
		handle_import(H3, iter, 0);
	}
	print_handle(H3);
	while ((iter = handle_each_available(H2, iter))) {
		handle_import(H3, iter, 1);
	}
	print_handle(H3);
	return 0;
}

#endif
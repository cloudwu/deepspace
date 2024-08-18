#ifndef object_handle_h
#define object_handle_h

#include <stddef.h>

struct handle {
	int bits;
	int freelist;
	unsigned int h[1];
};

size_t handle_size(int bits);
void handle_init(struct handle *H, struct handle *old, int bits, size_t sz);
void handle_clear(struct handle *H);
unsigned int handle_new(struct handle *H);
int handle_import(struct handle *H, unsigned int id, int invalid);	// 0 succ
unsigned int handle_index(struct handle *H, int index);
int handle_invalid(struct handle *H, unsigned int id);
void handle_remove(struct handle *H, unsigned int id);
unsigned int handle_each(struct handle *H, unsigned int last);
unsigned int handle_each_available(struct handle *H, unsigned int last);

#endif
#include "scene.h"
#include <assert.h>
#include <stdio.h>

struct build_context {
	int shift;
	int x;
	int y;
	int id;
	slot_t *grid;
};

static void
build_init(struct build_context *C, struct scene *S, int layer) {
	C->shift = S->shift_x;
	C->x = 1 << S->shift_x;
	C->y = S->y;
	C->id = -1;
	assert(layer >=0 && layer < S->layer_n);
	C->grid = S->layer[layer];
}

static int
available_id(struct build_context *C) {
	if (C->id >= 0) {
		return C->id++;
	}
	int m = 0;
	slot_t * grid = C->grid;
	int sz = C->y << C->shift;
	int i;
	for (i=0; i<sz; i++) {
		if (grid[i] > m)
			m = grid[i];
	}
	C->id = m + 1;
	return C->id++;
}

static int
neighbor_east(struct build_context *C, struct scene_coord c, struct scene_coord *output, slot_t *grid) {
	if (c.x >= C->x -1)
		return -1;
	output->x = c.x + 1;		
	output->y = c.y;
	return *(grid + 1);
}

static int
neighbor_west(struct build_context *C, struct scene_coord c, struct scene_coord *output, slot_t *grid) {
	if (c.x <= 0)
		return -1;
	output->x = c.x - 1;		
	output->y = c.y;
	return *(grid - 1);
}

static int
neighbor_south(struct build_context *C, struct scene_coord c, struct scene_coord *output, slot_t *grid) {
	if (c.y >= C->y -1)
		return -1;
	output->x = c.x;		
	output->y = c.y + 1;
	return *(grid + (1 << C->shift));
}

static int
neighbor_north(struct build_context *C, struct scene_coord c, struct scene_coord *output, slot_t *grid) {
	if (c.y <= 0)
		return -1;
	output->x = c.x;		
	output->y = c.y - 1;
	return *(grid - (1 << C->shift));
}

typedef int (*neighbor_func)(struct build_context *C, struct scene_coord c, struct scene_coord *output, slot_t *grid);

static const neighbor_func funcs[] = {
	neighbor_east,
	neighbor_west,
	neighbor_south,
	neighbor_north,
};
	
static inline slot_t *
get_coord(struct build_context *C, struct scene_coord p) {
	return C->grid + (p.y << C->shift) + p.x;
}

static void
convert(struct build_context *C, int oid, int id) {
//	printf("Convert %d->%d\n", oid, id);
	int sz = C->y << C->shift;
	int i;
	slot_t * s = C->grid;
	for (i=0;i<sz;i++) {
		if (s[i] == oid)
			s[i] = id;
	}
}

static void
render_(struct build_context *C, struct scene_coord p, int from, int to) {
	slot_t * s = get_coord(C, p);
	if (*s != from)
		return;
	*s = to;
	// east
	struct scene_coord tmp;
	if (p.x < C->x - 1) {
		tmp.x = p.x + 1;
		tmp.y = p.y;
		render_(C, tmp, from, to);
	}
	// west
	if (p.x > 0) {
		tmp.x = p.x - 1;
		tmp.y = p.y;
		render_(C, tmp, from, to);
	}
	// south
	if (p.y < C->y - 1) {
		tmp.x = p.x;
		tmp.y = p.y + 1;
		render_(C, tmp, from, to);
	}
	// north
	if (p.y > 0) {
		tmp.x = p.x;
		tmp.y = p.y - 1;
		render_(C, tmp, from, to);
	}
}

static void
render(struct build_context *C, struct scene_coord p, int id) {
	slot_t * s = get_coord(C, p);
	int oid = *s;
	if (oid == id)
		return;
	render_(C, p, oid, id);
}

static inline void
print_coords(const char *pre, struct scene_coord p[], int n) {
	printf("%s (", pre);
	int i;
	for (i=0;i<n;i++) {
		printf("%d,%d ", p[i].x, p[i].y);
	}
	printf(")\n");
}

static void
build_one(struct build_context *C, struct scene_coord p) {
	slot_t * grid = get_coord(C, p);
	int current = *grid;
	struct scene_coord neighbor;
	int i;
	int nid;
	struct scene_coord connect[4];
	int connect_n = 0;
	struct scene_coord split[4];
	int split_n = 0;
	int del[4];
	int del_n = 0;
	
	for (i=0;i<4;i++) {
		if ((nid = funcs[i](C, p, &neighbor, grid)) >= 0) {
			if (nid != current) {
				connect[connect_n++] = neighbor;
			} else {
				split[split_n++] = neighbor;
			}
		}
	}
//	print_coords("connect", connect, connect_n);
//	print_coords("split", split, split_n);
	if (connect_n > 0) {
		slot_t * neighbor = get_coord(C, connect[0]);
		int id = *neighbor; 
		int oid = *grid;
		if (oid != id && split_n == 0) {
			del[del_n++] = oid;
		}
		*grid = id;
		for (i = 1; i < connect_n; i++) {
			neighbor = get_coord(C, connect[i]);
			int oid = *neighbor;
//			printf("Del %d <= %d\n", oid, id);
			if (oid != id) {
				convert(C, oid, id);
				del[del_n++] = oid;
			}
		}
	} else {
		*grid = available_id(C);
	}
	for (i = 1;i < split_n; i++) {
		slot_t * neighbor = get_coord(C, split[i]);
		if (*neighbor == current) {
			int id;
			if (del_n > 0) {
				id = del[--del_n];
			} else {
				id = available_id(C);
			}
			render(C, split[i], id);
		}
	}
	if (split_n > 1) {
		slot_t * neighbor = get_coord(C, split[0]);
		if (*neighbor != current) {
			del[del_n++] = current;
		}
	}
	if (del_n > 0) {
		int id = available_id(C);
		int i = 0;
		while (i < del_n) {
			if (del[i] >= id) {
				del[i] = del[--del_n];
			} else {
				++i;
			}
		}
		C->id -= del_n + 1;
		id -= del_n;
		for (i=0;i<del_n;i++) {
			convert(C, id+i, del[i]);
		}
	}
}


static inline void
print_map(struct build_context *C) {
	int i,j;
	slot_t *p = C->grid;
	for (i=0;i<C->y;i++) {
		for (j=0;j<C->x;j++) {
			printf("%d ", *p++);
		}
		printf("\n");
	}
}

void
scene_build(struct scene *S, int layer, int n, struct scene_coord p[]) {
	struct build_context C;
	build_init(&C, S, layer);
	int i;
	for (i=0;i<n;i++) {
//		printf("Add %d %d\n", p[i].x, p[i].y);
		build_one(&C, p[i]);
	}
}

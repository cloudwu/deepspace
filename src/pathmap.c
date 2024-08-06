#include "scene.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

static inline void
check_layer(struct scene *S, int layer) {
	assert(layer >= 0 && layer < S->layer_n);	
}

struct pathmap_context {
	int x;
	int y;
	int shift;
	int id;
	slot_t * block;
	slot_t * target;
};

static inline slot_t *
get_coord(struct pathmap_context *C, slot_t *layer, struct scene_coord p) {
	if (p.x < 0 || p.y < 0 || p.x >= C->x || p.y >= C->y)
		return NULL;
	return layer + (p.y << C->shift) + p.x;
}

static void
pathmap_init(struct pathmap_context *C, struct scene *S, int layer, int target_layer) {
	C->x = 1 << S->shift_x;
	C->y = S->y;
	C->shift = S->shift_x;
	check_layer(S, layer);
	check_layer(S, target_layer);
	C->block = S->layer[layer];
	C->target = S->layer[target_layer];
	C->id = 0;
}

static const int neighbor[8*3] = {
	-1, -1, 7,	// NW
	 0, -1, 5,	// N
	 1, -1, 7,	// NE
	-1,  0, 5,	// W
	 1,  0,	5,	// E
	-1,  1, 7,	// SW
	 0,  1,	5,	// S
	 1,  1,	7,	// SE
};

static inline int
is_block(struct pathmap_context *C, int x, int y) {
	return C->block[(y << C->shift) + x] != C->id;
}

static void
render(struct pathmap_context *C, struct scene_coord pos, int dist) {
	slot_t * s = get_coord(C, C->block, pos);
	if (s == NULL || *s != C->id)
		return;
	slot_t * t = s + (C->target - C->block);
	if (*t != 0 && *t <= dist)
		return;
	*t = dist;
	int i;
	for (i=0;i<8*3;i+=3) {
		struct scene_coord t = pos;
		if (neighbor[i+2] == 7) {
			// test diagonal
			if (!is_block(C, pos.x, pos.y + neighbor[i+1]) ||
				!is_block(C, pos.x + neighbor[i+0], pos.y)) {
				t.x += neighbor[i+0];
				t.y += neighbor[i+1];
				render(C, t, dist + neighbor[i+2]);
			}
		} else {
			t.x += neighbor[i+0];
			t.y += neighbor[i+1];
			render(C, t, dist + neighbor[i+2]);		
		}
	}
}

void
scene_pathmap(struct scene *S, int layer, int n, struct scene_coord pos[], int target_layer) {
	struct pathmap_context C;
	pathmap_init(&C, S, layer, target_layer);
	memset(C.target, 0, C.x * C.y * sizeof(slot_t));
	if (n == 0)
		return;
	int i;
	for (i=0;i<n;i++) {
		slot_t * s = get_coord(&C, C.block, pos[i]);
		if (s) {
			C.id = *s;
			render(&C, pos[i], 1);
		}
	}
}

static inline slot_t *
get_dist(struct scene *S, int layer, struct scene_coord p) {
	if (p.x < 0 || p.y < 0 || p.x >= (1 << S->shift_x) || p.y >= S->y)
		return NULL;
	return S->layer[layer] + (p.y << S->shift_x) + p.x;
}

static inline int
nearest(struct scene *S, int layer, struct scene_coord *p, int dist) {
	int i;
	struct scene_coord c = *p;
	int min_dist = dist;
	int stride = 1 << S->shift_x;
	for (i=0;i<8*3;i+=3) {
		struct scene_coord t = c;
		t.x += neighbor[i+0];
		t.y += neighbor[i+1];
		slot_t *s = get_dist(S, layer, t);
		if (s != NULL && *s != 0 && *s < min_dist) {
			if (neighbor[i+2] == 7) {
				// test diagonal
				slot_t *n1 = s - neighbor[i+0];
				slot_t *n2 = s - neighbor[i+1] * stride;
				if (*n1 != 0 && *n2 != 0) {
					min_dist = *s;
					*p = t;
				}
			} else {
				min_dist = *s;
				*p = t;
			}
		}
	}
	return min_dist;
}

static inline int
block(struct scene *S, int layer, struct scene_coord p0, struct scene_coord p1) {
	int x1, x2, y1, y2;
	if (p0.x > p1.x) {
		x1 = p1.x;
		x2 = p0.x;
	} else {
		x1 = p0.x;
		x2 = p1.x;
	}

	if (p0.y > p1.y) {
		y1 = p1.y;
		y2 = p0.y;
	} else {
		y1 = p0.y;
		y2 = p1.y;
	}
	
	int shift = S->shift_x;
	
	slot_t *s = S->layer[layer] + (y1 << shift);
	
	int i,j;
	for (i=y1;i<=y2;i++) {
		for (j=x1;j<=x2;j++) {
			if (s[j] == 0)
				return 1;
		}
		s += 1 << shift;
	}
	return 0;
}

// 0 : term
static int
find_next(struct scene *S, int layer, struct scene_coord *inout) {
	slot_t *s = S->layer[layer] + (inout->y << S->shift_x) + inout->x;
	int dist = *s;
	struct scene_coord c = *inout;
	nearest(S, layer, &c, dist);
	if (c.x == inout->x && c.y == inout->y)
		return 0;
	struct scene_coord last_noblock = c;
	for (;;) {
		int next_dist = nearest(S, layer, &c, dist);
		if (next_dist == dist || block(S, layer, *inout, c)) {
			break;
		} else {
			last_noblock = c;
			dist = next_dist;
		}
	}
	*inout = last_noblock;
	return 1;
}	

int
scene_path(struct scene *S, int layer, struct scene_coord target, int n, struct scene_coord p[]) {
	check_layer(S, layer);
	if (target.x < 0 || target.y < 0 || target.x >= (1 << S->shift_x) || target.y >= S->y)
		return 0;
	slot_t * slot = get_dist(S, layer, target);
	if (slot == NULL)
		return 0;
	int dist = *slot;
	if (dist == 0)
		return 0;
	
	int idx = n;
	struct scene_coord tmp = target;
	while (idx > 0 && find_next(S, layer, &tmp)) {
		--idx;
		p[idx] = tmp;
	}
	int len = n - idx;
	memmove(p, &p[idx], len * sizeof(p[0]));
	if (len < n) {
		p[len] = target;
	}
	return dist;
}

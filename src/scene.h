#ifndef SCENE_H
#define SCENE_H

typedef int slot_t;

struct scene {
	int shift_x;
	int y;
	int layer_n;
	slot_t *layer[1];
};

struct scene_coord {
	unsigned short x;
	unsigned short y;
};

void scene_build(struct scene *S, int layer, int n, struct scene_coord p[]);
void scene_pathmap(struct scene *S, int layer, struct scene_coord pos, int target_layer);
int scene_path(struct scene *S, int layer, struct scene_coord target, int n, struct scene_coord p[]);

#endif
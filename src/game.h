#ifndef GAME_H
#define GAME_H
#include <stdbool.h>
#include <stdint.h>
#define ITEM_LEVELS 11
#define MAX_ITEMS 192
#define MAX_EFFECTS 48
#define WELL_LEFT 290.0f
#define WELL_RIGHT 730.0f
#define WELL_TOP 165.0f
#define WELL_BOTTOM 735.0f
#define DANGER_Y 240.0f
#define DROP_Y 185.0f
#define FIXED_DT (1.0f / 120.0f)
typedef struct { const char *name; float radius; unsigned value; } ItemDef;
extern const ItemDef ITEM_DEFS[ITEM_LEVELS];
typedef struct { float x,y,vx,vy,age; int level; bool active; } Item;
typedef struct { float x,y,life; int level; } Effect;
typedef struct {
    Item items[MAX_ITEMS]; Effect effects[MAX_EFFECTS];
    uint32_t rng; unsigned score,merges; int current,next,highest;
    float cooldown,overflow; bool over;
} Game;
void game_init(Game *g, uint32_t seed);
bool game_drop(Game *g, float x);
void game_step(Game *g, float dt);
float game_clamp_x(float x, int level);
#endif

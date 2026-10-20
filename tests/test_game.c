#include "game.h"
#include <box2d/box2d.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <math.h>
#include <stdio.h>
static int count(const Game *g) { int n=0;for(int i=0;i<MAX_ITEMS;i++)n+=g->items[i].active;return n; }
static void tick(Game *g,int n) {for(int i=0;i<n;i++)game_step(g,FIXED_DT);}
static void check_bodies(const Game *g) {
    int n=count(g);
    b2Counters counters=b2World_GetCounters(g->world);
    assert(counters.bodyCount==n+1); /* Items plus the static stash. */
    assert(counters.shapeCount==n+3);
    for(int i=0;i<MAX_ITEMS;i++) {
        assert(b2Body_IsValid(g->bodies[i])==g->items[i].active);
        if(!g->items[i].active) continue;
        const Item *a=&g->items[i];
        assert(isfinite(a->x)&&isfinite(a->y)&&isfinite(a->angle));
        assert(isfinite(a->vx)&&isfinite(a->vy));
        assert(a->level>=0&&a->level<ITEM_LEVELS);
        b2Vec2 position=b2Body_GetPosition(g->bodies[i]);
        assert(fabsf(position.x*50-a->x)<0.01f);
        assert(fabsf(position.y*50-a->y)<0.01f);
    }
}
static void pair(Game *g,int level) {
    game_reset(g,42);
    assert(game_spawn(g,level,490,500));
    assert(game_spawn(g,level,510,500));
}
int main(void) {
    Game g; game_init(&g,42); int next=g.next;
    assert(game_drop(&g,-10000)); assert(count(&g)==1);
    assert(g.current==next); assert(!game_drop(&g,500));
    assert(g.items[0].x>=WELL_LEFT+ITEM_DEFS[g.items[0].level].radius);
    tick(&g,80);assert(game_drop(&g,10000));
    for(int l=0;l<ITEM_LEVELS-1;l++) {
        pair(&g,l);tick(&g,1);
        assert(count(&g)==1);assert(g.items[0].level==l+1);
        assert(g.score==ITEM_DEFS[l+1].value);assert(g.merges==1);
        assert(g.items[0].age==0);assert(g.highest==l+1);
        check_bodies(&g);
    }
    pair(&g,ITEM_LEVELS-1);tick(&g,1);assert(count(&g)==2);assert(g.score==0);
    game_reset(&g,42);
    assert(game_spawn(&g,0,490,500)); assert(game_spawn(&g,1,510,500));
    float mass_ratio=b2Body_GetMass(g.bodies[1])/b2Body_GetMass(g.bodies[0]);
    assert(fabsf(mass_ratio-(21.0f*21)/(16.0f*16))<0.001f);
    /* Allow the soft contact solver to resolve the deliberately deep overlap. */
    tick(&g,120);assert(count(&g)==2);assert(g.score==0);check_bodies(&g);
    assert(hypotf(g.items[0].x-g.items[1].x,g.items[0].y-g.items[1].y)>36.0f);
    game_reset(&g,42);
    assert(game_spawn(&g,0,350,500));assert(game_spawn(&g,0,670,500));
    tick(&g,30);assert(count(&g)==2&&g.merges==0);

    /* Multiple simultaneous contacts must consume each body at most once. */
    game_reset(&g,8);
    for(int i=0;i<4;i++) assert(game_spawn(&g,0,480+i*20,500));
    tick(&g,1);assert(g.merges>=1&&g.merges<=2);
    assert(count(&g)==4-(int)g.merges);check_bodies(&g);
    tick(&g,240);check_bodies(&g);

    game_reset(&g,10);g.current=0;assert(game_drop(&g,500));tick(&g,1200);
    assert(!g.over);assert(fabsf(g.items[0].y-(WELL_BOTTOM-ITEM_DEFS[0].radius))<1);
    assert(fabsf(g.items[0].vy)<0.1f);check_bodies(&g);

    /* An upgrade at the floor/right wall must fit its larger shape. */
    game_reset(&g,10);
    assert(game_spawn(&g,0,10000,WELL_BOTTOM));
    assert(game_spawn(&g,0,10000,WELL_BOTTOM));
    tick(&g,1);assert(g.merges==1);
    assert(g.items[0].x+ITEM_DEFS[1].radius<=WELL_RIGHT);
    assert(g.items[0].y+ITEM_DEFS[1].radius<=WELL_BOTTOM);
    check_bodies(&g);

    /* Continuous collisions must stop fast drops at the floor and side walls. */
    game_reset(&g,33);assert(game_spawn(&g,2,500,500));
    b2Body_SetLinearVelocity(g.bodies[0],(b2Vec2){150,150});
    for(int i=0;i<120;i++) {
        tick(&g,1);
        assert(g.items[0].x+ITEM_DEFS[2].radius<WELL_RIGHT+1);
        assert(g.items[0].x-ITEM_DEFS[2].radius>WELL_LEFT-1);
        assert(g.items[0].y+ITEM_DEFS[2].radius<WELL_BOTTOM+1);
    }
    check_bodies(&g);
    b2Body_SetAngularVelocity(g.bodies[0],4);
    tick(&g,1);assert(fabsf(g.items[0].angle)>0.001f);

    /* A supported stack crossing the line loses after the grace period. */
    game_reset(&g,12);
    for(int i=0;i<4;i++) {
        assert(game_spawn(&g,10,500,WELL_BOTTOM-97-i*194));
        g.items[i].age=3;
    }
    tick(&g,360);assert(g.over);check_bodies(&g);
    game_reset(&g,123);g.current=0;assert(game_drop(&g,500));
    game_step(&g,FIXED_DT);assert(g.overflow==0);
    g.over=true;assert(!game_drop(&g,500));

    /* Full capacity, invalid arguments, and non-positive dt cannot corrupt a world. */
    game_reset(&g,9);
    assert(!game_spawn(&g,-1,500,500)); assert(!game_spawn(&g,ITEM_LEVELS,500,500));
    assert(!game_spawn(&g,0,NAN,500)); assert(!game_drop(&g,NAN));
    for(int i=0;i<MAX_ITEMS;i++) assert(game_spawn(&g,10,500,500));
    assert(!game_spawn(&g,0,500,500));assert(!game_drop(&g,500));assert(g.over);
    check_bodies(&g);
    game_reset(&g,9);assert(game_drop(&g,500));
    float y=g.items[0].y,cooldown=g.cooldown;
    game_step(&g,0);game_step(&g,-1);game_step(&g,NAN);
    assert(g.items[0].y==y&&g.cooldown==cooldown);

    /* Seeded dense contacts cover recycled slots, merges, and sleeping bodies. */
    game_reset(&g,1234);
    for(int n=0;n<16000&&!g.over;n++) {
        if(n%75==0)game_drop(&g,WELL_LEFT+20+(n*37)%400);
        game_step(&g,FIXED_DT);
        check_bodies(&g);
    }
    assert(g.merges>0);
    /* Keep an independent world alive while restarting repeatedly. A missing
     * destroy would exhaust Box2D's world pool within these 256 resets. */
    Game other;game_init(&other,77);assert(game_drop(&other,500));
    float other_y=other.items[0].y;
    for(int n=0;n<256;n++) {
        game_reset(&g,(uint32_t)n+1);assert(game_drop(&g,510));tick(&g,2);check_bodies(&g);
        assert(other.items[0].y==other_y);check_bodies(&other);
    }
    game_destroy(&g);game_destroy(&g);game_destroy(&other);
    assert(B2_IS_NULL(g.world));
    puts("Box2D tests passed: merges, simultaneous contacts, boundaries, settling, overflow, capacity, rotation, stress, lifecycle.");
    return 0;
}

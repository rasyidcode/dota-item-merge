#include "game.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <math.h>
#include <stdio.h>
static int count(Game *g) { int n=0;for(int i=0;i<MAX_ITEMS;i++)n+=g->items[i].active;return n; }
static void tick(Game *g,int n) {for(int i=0;i<n;i++)game_step(g,FIXED_DT);}
static void pair(Game *g,int level) {
    game_init(g,42);
    g->items[0]=(Item){.x=490,.y=500,.level=level,.active=true};
    g->items[1]=(Item){.x=510,.y=500,.level=level,.active=true};
}
int main(void) {
    Game g;
    game_init(&g,42); int next=g.next;
    assert(game_drop(&g,-10000)); assert(count(&g)==1);
    assert(g.current==next); assert(!game_drop(&g,500));
    assert(g.items[0].x>=WELL_LEFT+ITEM_DEFS[g.items[0].level].radius);
    tick(&g,80);assert(game_drop(&g,10000));
    for(int l=0;l<ITEM_LEVELS-1;l++) {
        pair(&g,l);tick(&g,1);
        assert(count(&g)==1);assert(g.items[0].level==l+1);
        assert(g.score==ITEM_DEFS[l+1].value);assert(g.merges==1);
    }
    pair(&g,ITEM_LEVELS-1);tick(&g,1);assert(count(&g)==2);assert(g.score==0);
    pair(&g,0);g.items[1].level=1;tick(&g,1);assert(count(&g)==2);assert(g.score==0);
    game_init(&g,10);g.current=0;assert(game_drop(&g,500));tick(&g,1200);
    assert(!g.over);assert(fabsf(g.items[0].y-(WELL_BOTTOM-ITEM_DEFS[0].radius))<1);
    /* A supported stack crossing the line must lose after the grace period. */
    game_init(&g,12);
    for(int i=0;i<4;i++)g.items[i]=(Item){.x=500,.y=WELL_BOTTOM-97-i*194,.level=10,.age=3,.active=true};
    tick(&g,360);assert(g.over);
    game_init(&g,123);g.current=0;assert(game_drop(&g,500));
    game_step(&g,FIXED_DT);assert(g.overflow==0); /* New drops get a grace period. */
    g.over=true;assert(!game_drop(&g,500));
    /* A seeded run exercises dense collisions and verifies finite positions. */
    game_init(&g,1234);
    for(int n=0;n<16000&&!g.over;n++) {
        if(n%75==0)game_drop(&g,WELL_LEFT+20+(n*37)%400);
        game_step(&g,FIXED_DT);
        for(int i=0;i<MAX_ITEMS;i++)if(g.items[i].active) {
            Item *a=&g.items[i];assert(isfinite(a->x)&&isfinite(a->y));
            assert(isfinite(a->vx)&&isfinite(a->vy));
            assert(a->level>=0&&a->level<ITEM_LEVELS);
        }
    }
    assert(g.merges>0);
    puts("Physics tests passed: drops, all merge tiers, cap, collisions, settling, overflow, stress.");
    return 0;
}

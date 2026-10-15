#include "game.h"
#include <math.h>
#include <string.h>
const ItemDef ITEM_DEFS[ITEM_LEVELS] = {
    {"Iron Branch",16,2}, {"Tango",21,4}, {"Clarity",26,8},
    {"Healing Salve",32,16}, {"Magic Wand",39,32}, {"Boots of Speed",46,64},
    {"Blink Dagger",54,128}, {"Crystalys",63,256}, {"Black King Bar",73,512},
    {"Daedalus",84,1024}, {"Divine Rapier",97,2048}
};
static float clamp(float v,float lo,float hi) { return fminf(hi,fmaxf(lo,v)); }
static int roll(Game *g) {
    g->rng ^= g->rng << 13; g->rng ^= g->rng >> 17; g->rng ^= g->rng << 5;
    return (int)(g->rng % 4);
}
float game_clamp_x(float x,int level) {
    float r=ITEM_DEFS[level].radius;
    return clamp(x,WELL_LEFT+r+1,WELL_RIGHT-r-1);
}
void game_init(Game *g,uint32_t seed) {
    memset(g,0,sizeof(*g)); g->rng=seed?seed:1;
    g->current=roll(g); g->next=roll(g);
}
bool game_drop(Game *g,float x) {
    if(g->over || g->cooldown>0) return false;
    for(int i=0;i<MAX_ITEMS;i++) if(!g->items[i].active) {
        g->items[i]=(Item){.x=game_clamp_x(x,g->current),.y=DROP_Y,
            .level=g->current,.active=true};
        g->current=g->next; g->next=roll(g); g->cooldown=0.48f;
        return true;
    }
    g->over=true; return false;
}
static void effect(Game *g,Item *a) {
    for(int i=0;i<MAX_EFFECTS;i++) if(g->effects[i].life<=0) {
        g->effects[i]=(Effect){a->x,a->y,0.55f,a->level}; break;
    }
}
void game_step(Game *g,float dt) {
    if(g->over || dt<=0) return;
    g->cooldown=fmaxf(0,g->cooldown-dt);
    for(int i=0;i<MAX_EFFECTS;i++) g->effects[i].life=fmaxf(0,g->effects[i].life-dt);
    for(int i=0;i<MAX_ITEMS;i++) {
        Item *a=&g->items[i]; if(!a->active) continue;
        a->age+=dt; a->vy+=950*dt;
        a->vx*=powf(0.985f,dt*120); a->x+=a->vx*dt; a->y+=a->vy*dt;
    }
    for(int pass=0;pass<8;pass++) {
        for(int i=0;i<MAX_ITEMS;i++) {
            Item *a=&g->items[i]; if(!a->active) continue;
            float r=ITEM_DEFS[a->level].radius;
            if(a->x-r<WELL_LEFT) { a->x=WELL_LEFT+r; a->vx=fabsf(a->vx)*0.15f; }
            if(a->x+r>WELL_RIGHT) { a->x=WELL_RIGHT-r; a->vx=-fabsf(a->vx)*0.15f; }
            if(a->y+r>WELL_BOTTOM) {
                a->y=WELL_BOTTOM-r; a->vy=-fabsf(a->vy)*0.05f; a->vx*=0.94f;
            }
            for(int j=i+1;j<MAX_ITEMS;j++) {
                Item *b=&g->items[j]; if(!b->active) continue;
                float rb=ITEM_DEFS[b->level].radius;
                float dx=b->x-a->x,dy=b->y-a->y,d2=dx*dx+dy*dy;
                if(d2>=(r+rb)*(r+rb)) continue;
                if(a->level==b->level && a->level<ITEM_LEVELS-1) {
                    a->x=(a->x+b->x)*0.5f; a->y=(a->y+b->y)*0.5f;
                    a->vx=(a->vx+b->vx)*0.5f; a->vy=(a->vy+b->vy)*0.35f-70;
                    a->level++; a->age=0; b->active=false;
                    r=ITEM_DEFS[a->level].radius;
                    g->score+=ITEM_DEFS[a->level].value; g->merges++;
                    if(a->level>g->highest) g->highest=a->level;
                    effect(g,a); continue;
                }
                float dist=sqrtf(d2),nx=dist>0.001f?dx/dist:1,ny=dist>0.001f?dy/dist:0;
                float ia=1/(r*r),ib=1/(rb*rb),sum=ia+ib;
                float correction=fmaxf(0,r+rb-dist-0.01f)*0.85f/sum;
                a->x-=nx*correction*ia; a->y-=ny*correction*ia;
                b->x+=nx*correction*ib; b->y+=ny*correction*ib;
                float closing=(b->vx-a->vx)*nx+(b->vy-a->vy)*ny;
                if(closing<0) {
                    float impulse=-1.08f*closing/sum;
                    a->vx-=impulse*nx*ia; a->vy-=impulse*ny*ia;
                    b->vx+=impulse*nx*ib; b->vy+=impulse*ny*ib;
                }
            }
        }
    }
    bool danger=false;
    for(int i=0;i<MAX_ITEMS;i++) {
        Item *a=&g->items[i];
        if(a->active && a->age>1.5f && a->y-ITEM_DEFS[a->level].radius<DANGER_Y) danger=true;
    }
    g->overflow=danger?g->overflow+dt:0;
    if(g->overflow>=2) g->over=true;
}

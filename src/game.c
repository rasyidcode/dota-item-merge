#include "game.h"
#include <box2d/box2d.h>
#include <math.h>
#include <string.h>

/* Keep rendering in pixels and Box2D bodies in meters. */
#define PIXELS_PER_METER 50.0f
#define PHYSICS_SUBSTEPS 4
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
static void wall(b2BodyId body,float x,float y,float half_width,float half_height) {
    b2ShapeDef def=b2DefaultShapeDef();
    def.material.friction=0.5f;
    def.material.restitution=0.05f;
    b2Polygon box=b2MakeOffsetBox(half_width/PIXELS_PER_METER,half_height/PIXELS_PER_METER,
        (b2Vec2){x/PIXELS_PER_METER,y/PIXELS_PER_METER},b2Rot_identity);
    b2CreatePolygonShape(body,&def,&box);
}
void game_init(Game *g,uint32_t seed) {
    memset(g,0,sizeof(*g)); g->rng=seed?seed:1;
    g->current=roll(g); g->next=roll(g);
    b2WorldDef def=b2DefaultWorldDef();
    def.gravity=(b2Vec2){0,950.0f/PIXELS_PER_METER};
    g->world=b2CreateWorld(&def);
    if(B2_IS_NULL(g->world)) { g->over=true; return; }
    b2BodyDef body_def=b2DefaultBodyDef();
    b2BodyId boundaries=b2CreateBody(g->world,&body_def);
    float middle=(WELL_LEFT+WELL_RIGHT)*0.5f;
    wall(boundaries,middle,WELL_BOTTOM+20,(WELL_RIGHT-WELL_LEFT)*0.5f+40,20);
    /* Tall side walls leave the top open, including overflowing stacks. */
    wall(boundaries,WELL_LEFT-20,WELL_BOTTOM-1000,20,1040);
    wall(boundaries,WELL_RIGHT+20,WELL_BOTTOM-1000,20,1040);
}
void game_destroy(Game *g) {
    if(B2_IS_NON_NULL(g->world) && b2World_IsValid(g->world)) b2DestroyWorld(g->world);
    memset(g,0,sizeof(*g));
}
void game_reset(Game *g,uint32_t seed) {
    game_destroy(g);
    game_init(g,seed);
}
static void create_item_body(Game *g,int index) {
    Item *a=&g->items[index];
    b2BodyDef body=b2DefaultBodyDef();
    body.type=b2_dynamicBody;
    body.position=(b2Vec2){a->x/PIXELS_PER_METER,a->y/PIXELS_PER_METER};
    body.linearVelocity=(b2Vec2){a->vx/PIXELS_PER_METER,a->vy/PIXELS_PER_METER};
    body.rotation=b2MakeRot(a->angle);
    body.linearDamping=0.15f;
    body.angularDamping=1.2f;
    g->bodies[index]=b2CreateBody(g->world,&body);
    b2ShapeDef shape=b2DefaultShapeDef();
    shape.userData=a;
    shape.density=1.0f;
    shape.material.friction=0.5f;
    shape.material.restitution=0.08f;
    shape.material.rollingResistance=0.03f;
    shape.enableContactEvents=true;
    b2Circle circle={.center={0,0},.radius=ITEM_DEFS[a->level].radius/PIXELS_PER_METER};
    b2CreateCircleShape(g->bodies[index],&shape,&circle);
}
bool game_spawn(Game *g,int level,float x,float y) {
    if(g->over || B2_IS_NULL(g->world) || level<0 || level>=ITEM_LEVELS || !isfinite(x) || !isfinite(y)) return false;
    for(int i=0;i<MAX_ITEMS;i++) if(!g->items[i].active) {
        g->items[i]=(Item){.x=game_clamp_x(x,level),
            .y=fminf(y,WELL_BOTTOM-ITEM_DEFS[level].radius),.level=level,.active=true};
        create_item_body(g,i);
        return true;
    }
    return false;
}
bool game_drop(Game *g,float x) {
    if(g->over || g->cooldown>0 || !isfinite(x)) return false;
    if(game_spawn(g,g->current,x,DROP_Y)) {
        g->current=g->next; g->next=roll(g); g->cooldown=0.48f;
        return true;
    }
    g->over=true; return false;
}
static void effect(Game *g,const Item *a) {
    for(int i=0;i<MAX_EFFECTS;i++) if(g->effects[i].life<=0) {
        g->effects[i]=(Effect){a->x,a->y,0.55f,a->level}; break;
    }
}
static void merge(Game *g,int first,int second) {
    Item *a=&g->items[first],*b=&g->items[second];
    int level=a->level+1;
    Item upgraded={.x=game_clamp_x((a->x+b->x)*0.5f,level),
        .y=fminf((a->y+b->y)*0.5f,WELL_BOTTOM-ITEM_DEFS[level].radius),
        .vx=(a->vx+b->vx)*0.5f,.vy=(a->vy+b->vy)*0.35f-70,
        .level=level,.active=true};
    b2DestroyBody(g->bodies[first]);
    b2DestroyBody(g->bodies[second]);
    g->bodies[second]=b2_nullBodyId;
    *a=upgraded; *b=(Item){0};
    create_item_body(g,first);
    g->score+=ITEM_DEFS[level].value; g->merges++;
    if(level>g->highest) g->highest=level;
    effect(g,a);
}
void game_step(Game *g,float dt) {
    if(g->over || B2_IS_NULL(g->world) || !isfinite(dt) || dt<=0) return;
    g->cooldown=fmaxf(0,g->cooldown-dt);
    for(int i=0;i<MAX_EFFECTS;i++) g->effects[i].life=fmaxf(0,g->effects[i].life-dt);
    b2World_Step(g->world,dt,PHYSICS_SUBSTEPS);
    for(int i=0;i<MAX_ITEMS;i++) if(g->items[i].active) {
        Item *a=&g->items[i];
        b2Vec2 position=b2Body_GetPosition(g->bodies[i]);
        b2Vec2 velocity=b2Body_GetLinearVelocity(g->bodies[i]);
        a->x=position.x*PIXELS_PER_METER; a->y=position.y*PIXELS_PER_METER;
        a->vx=velocity.x*PIXELS_PER_METER; a->vy=velocity.y*PIXELS_PER_METER;
        a->angle=b2Rot_GetAngle(b2Body_GetRotation(g->bodies[i]));
        a->age+=dt;
    }
    /* Consume the entire event buffer before destroying any shapes. Claim each
     * item at most once so simultaneous contacts cannot merge stale bodies. */
    typedef struct { int first,second; } MergePair;
    MergePair pairs[MAX_ITEMS/2]; int pair_count=0;
    bool claimed[MAX_ITEMS]={false};
    b2ContactEvents events=b2World_GetContactEvents(g->world);
    for(int i=0;i<events.beginCount;i++) {
        b2ContactBeginTouchEvent *event=&events.beginEvents[i];
        if(!b2Shape_IsValid(event->shapeIdA) || !b2Shape_IsValid(event->shapeIdB)) continue;
        Item *a=b2Shape_GetUserData(event->shapeIdA),*b=b2Shape_GetUserData(event->shapeIdB);
        if(!a || !b || !a->active || !b->active || a->level!=b->level || a->level==ITEM_LEVELS-1) continue;
        int first=(int)(a-g->items),second=(int)(b-g->items);
        if(claimed[first] || claimed[second]) continue;
        claimed[first]=true; claimed[second]=true;
        pairs[pair_count++]=(MergePair){first<second?first:second,first<second?second:first};
    }
    for(int i=0;i<pair_count;i++) merge(g,pairs[i].first,pairs[i].second);
    bool danger=false;
    for(int i=0;i<MAX_ITEMS;i++) {
        const Item *a=&g->items[i];
        if(a->active && a->age>1.5f && a->y-ITEM_DEFS[a->level].radius<DANGER_Y) danger=true;
    }
    g->overflow=danger?g->overflow+dt:0;
    if(g->overflow>=2) g->over=true;
}

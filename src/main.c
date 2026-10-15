#include "raylib.h"
#include "game.h"
#include <math.h>
#include <stdio.h>
#include <time.h>
#if defined(PLATFORM_ANDROID)
#include <android_native_app_glue.h>
// Android backend extension; not declared by the cross-platform raylib header.
extern struct android_app *GetAndroidApp(void);
#endif
#define WIDTH 1040
#define HEIGHT 820
static const Color BG={12,19,25,255}, PANEL={21,32,39,255}, INK={224,232,222,255};
static const Color MUTED={130,151,153,255}, ACCENT={226,181,98,255};
static const Color COLORS[ITEM_LEVELS]={
    {137,164,83,255},{99,190,106,255},{93,174,231,255},{111,215,156,255},
    {175,129,225,255},{195,151,100,255},{90,208,221,255},{226,105,104,255},
    {238,195,93,255},{230,119,62,255},{255,218,121,255}
};
static void label(const char *s,int x,int y,int size,Color c) { DrawText(s,x,y,size,c); }
static void center(const char *s,int x,int y,int size,Color c) {
    DrawText(s,x-MeasureText(s,size)/2,y,size,c);
}
static Vector2 point(float x,float y,float u,float v,float s) { return (Vector2){x+u*s,y+v*s}; }
static void stroke(float x,float y,float s,float ax,float ay,float bx,float by,float thick,Color c) {
    DrawLineEx(point(x,y,ax,ay,s),point(x,y,bx,by,s),thick*s,c);
}
static void icon(float x,float y,int level,float r,float alpha) {
    Color c=Fade(COLORS[level],alpha),light=Fade(INK,alpha);
    float s=r/32;
    DrawCircle((int)x+2,(int)y+4,r,Fade(BLACK,0.25f*alpha));
    DrawCircleV((Vector2){x,y},r,Fade((Color){29,43,48,255},alpha));
    DrawCircleV((Vector2){x,y},r-3,Fade(COLORS[level],0.13f*alpha));
    DrawCircleLines((int)x,(int)y,r-1,c);
    if(level==0 || level==1) {
        stroke(x,y,s,-10,18,7,-17,4,c);
        for(int i=0;i<3;i++) {
            float yy=-13+i*11;
            DrawEllipse((int)(x+((i%2)?-8:8)*s),(int)(y+yy*s),9*s,5*s,c);
        }
        if(level==1) DrawCircleV(point(x,y,0,11,s),7*s,c);
    } else if(level==2 || level==3) {
        DrawRectangleRounded((Rectangle){x-11*s,y-10*s,22*s,30*s},0.5f,8,c);
        DrawRectangle((int)(x-5*s),(int)(y-21*s),10*s,15*s,light);
        stroke(x,y,s,-6,0,-6,11,2,light);
        if(level==3) {
            stroke(x,y,s,-5,5,5,5,3,light); stroke(x,y,s,0,0,0,10,3,light);
        }
    } else if(level==5) {
        DrawRectanglePro((Rectangle){x-6*s,y-4*s,15*s,28*s},(Vector2){7*s,14*s},-12,c);
        DrawEllipse((int)(x+4*s),(int)(y+15*s),17*s,8*s,c);
        stroke(x,y,s,-11,-10,2,-8,3,light); stroke(x,y,s,-10,-3,2,-1,3,light);
    } else {
        float ax=-14,ay=19,bx=13,by=-19;
        stroke(x,y,s,ax,ay,bx,by,level==8?8:5,c);
        if(level==4 || level==8) {
            DrawPoly(point(x,y,bx,by,s),level==4?5:6,9*s,0,c);
            DrawCircleV(point(x,y,bx,by,s),4*s,light);
        } else {
            stroke(x,y,s,-1,5,13,-17,5,light);
            stroke(x,y,s,-10,-2,9,12,4,c);
            if(level>=9) stroke(x,y,s,-8,3,7,-20,3,c);
            if(level==10) {
                DrawCircleLines((int)x,(int)y,r*0.72f,Fade(ACCENT,alpha*0.5f));
                stroke(x,y,s,-17,-18,-17,-9,2,light);
                stroke(x,y,s,-22,-13,-12,-13,2,light);
            }
        }
    }
}
static void card(int x,int y,int w,int h) {
    DrawRectangleRounded((Rectangle){x,y,w,h},0.08f,8,PANEL);
}
static char best_path[1024]="dota-item-merge.best";
static unsigned load_best(void) {
    unsigned best=0; FILE *f=fopen(best_path,"r");
    if(f) { if(fscanf(f,"%u",&best)!=1) best=0; fclose(f); } return best;
}
static void save_best(unsigned best) {
    FILE *f=fopen(best_path,"w");
    if(f) { fprintf(f,"%u\n",best); fclose(f); }
}
int main(int argc,char **argv) {
    (void)argc; (void)argv;
#if defined(PLATFORM_ANDROID)
    SetConfigFlags(FLAG_MSAA_4X_HINT);
#else
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
#endif
    InitWindow(WIDTH,HEIGHT,"Dota Item Merge");
#if defined(PLATFORM_ANDROID)
    struct android_app *app=(struct android_app *)GetAndroidApp();
    if(app && app->activity && app->activity->internalDataPath)
        snprintf(best_path,sizeof(best_path),"%s/dota-item-merge.best",app->activity->internalDataPath);
    bool touch_was_down=false, dragging=false;
#else
    SetWindowMinSize(780,615);
#endif
    SetTargetFPS(60);
    RenderTexture2D canvas=LoadRenderTexture(WIDTH,HEIGHT);
    SetTextureFilter(canvas.texture,TEXTURE_FILTER_BILINEAR);
    Game game; game_init(&game,(uint32_t)time(NULL));
    unsigned best=load_best(); bool paused=false; double accumulator=0; float aim=510;
    const Rectangle pause_button={40,655,96,62}, restart_button={145,655,105,62};
    while(!WindowShouldClose()) {
        float scale=fminf((float)GetScreenWidth()/WIDTH,(float)GetScreenHeight()/HEIGHT);
        float ox=(GetScreenWidth()-WIDTH*scale)/2,oy=(GetScreenHeight()-HEIGHT*scale)/2;
        Vector2 pointer=GetMousePosition();
#if defined(PLATFORM_ANDROID)
        bool touch_down=GetTouchPointCount()>0;
        if(touch_down) pointer=GetTouchPosition(0);
        bool pressed=touch_down&&!touch_was_down;
        bool released=!touch_down&&touch_was_down;
#else
        bool pressed=IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
#endif
        Vector2 mouse={(pointer.x-ox)/scale,(pointer.y-oy)/scale};
        float elapsed=fminf(GetFrameTime(),0.1f);
        bool restart=IsKeyPressed(KEY_R)||(pressed&&CheckCollisionPointRec(mouse,restart_button));
        bool toggle_pause=IsKeyPressed(KEY_P)||(pressed&&CheckCollisionPointRec(mouse,pause_button));
        if(restart) {
            if(game.score>best) {best=game.score;save_best(best);}
            game_init(&game,(uint32_t)time(NULL)); paused=false; accumulator=0;
        }
        if(toggle_pause && !game.over) paused=!paused;
        bool in_stash=mouse.x>=WELL_LEFT&&mouse.x<=WELL_RIGHT&&mouse.y>=120&&mouse.y<=WELL_BOTTOM;
#if defined(PLATFORM_ANDROID)
        if(pressed) dragging=in_stash&&!paused&&!game.over;
        if(touch_down&&dragging) aim=mouse.x;
        bool click=released&&dragging;
        if(released) dragging=false;
        touch_was_down=touch_down;
#else
        if(GetMouseDelta().x!=0) aim=mouse.x;
        bool click=pressed&&in_stash;
#endif
        if(IsKeyDown(KEY_LEFT)||IsKeyDown(KEY_A)) aim-=elapsed*340;
        if(IsKeyDown(KEY_RIGHT)||IsKeyDown(KEY_D)) aim+=elapsed*340;
        aim=game_clamp_x(aim,game.current);
        if(!paused&&!game.over&&(click||IsKeyPressed(KEY_SPACE))) game_drop(&game,aim);
        if(!paused&&!game.over) {
            accumulator+=elapsed;
            while(accumulator>=FIXED_DT) {game_step(&game,FIXED_DT);accumulator-=FIXED_DT;}
        } else accumulator=0;
        if(game.score>best) {best=game.score; save_best(best);}
        BeginTextureMode(canvas); ClearBackground(BG);
        for(int i=0;i<20;i++) DrawLine(i*60,0,i*60-200,HEIGHT,Fade(MUTED,0.025f));
        label("DOTA",40,32,18,ACCENT); label("ITEM MERGE",40,58,36,INK);
        label("A little alchemy. A lot of inventory management.",42,106,16,MUTED);
        label("THE STASH",290,131,16,ACCENT);
        label("DROP / MERGE / UPGRADE",518,133,12,MUTED);
        card(40,170,210,115); label("GOLD EARNED",60,190,14,MUTED);
        label(TextFormat("%06u",game.score),60,220,36,ACCENT);
        card(40,301,210,83); label("PERSONAL BEST",60,319,13,MUTED);
        label(TextFormat("%u",best),60,344,24,INK);
        card(40,400,210,139); label("NEXT DELIVERY",60,419,13,MUTED);
        icon(88,479,game.next,29,1); label(ITEM_DEFS[game.next].name,130,464,12,INK);
        label("Tier 01 - 04",130,484,11,MUTED);
        label("HOW TO PLAY",44,576,14,ACCENT);
#if defined(PLATFORM_ANDROID)
        label("Drag inside stash to aim",44,608,13,INK);
        label("Release to drop an item",44,632,13,INK);
#else
        label("Move mouse / A D / arrows",44,608,13,INK);
        label("Click stash / Space to drop",44,632,13,INK);
#endif
        card(40,655,96,62); card(145,655,105,62);
        center(paused?"RESUME":"PAUSE",88,679,14,ACCENT);
        center("NEW RUN",197,679,14,ACCENT);
        DrawRectangleRounded((Rectangle){282,157,456,588},0.025f,8,(Color){45,62,65,255});
        DrawRectangle(290,165,440,570,(Color){15,25,31,255});
        for(int y=270;y<735;y+=46) DrawLine(290,y,730,y,Fade(MUTED,0.035f));
        Color danger=game.overflow>0?(Color){244,108,91,255}:MUTED;
        for(int x=298;x<730;x+=18) DrawLine(x,DANGER_Y,x+9,DANGER_Y,Fade(danger,0.6f));
        if(game.overflow>0) {
            center(TextFormat("STASH FULL  %.1fs",fmaxf(0,2-game.overflow)),510,207,16,danger);
        }
        if(!game.over&&!paused) {
            float r=ITEM_DEFS[game.current].radius;
            DrawLine((int)aim,DROP_Y,(int)aim,725,Fade(COLORS[game.current],0.15f));
            icon(aim,DROP_Y,game.current,r,game.cooldown>0?0.3f:0.8f);
        }
        for(int i=0;i<MAX_ITEMS;i++) if(game.items[i].active) {
            Item *a=&game.items[i]; icon(a->x,a->y,a->level,ITEM_DEFS[a->level].radius,1);
        }
        for(int i=0;i<MAX_EFFECTS;i++) if(game.effects[i].life>0) {
            Effect *e=&game.effects[i];float t=1-e->life/0.55f;
            DrawCircleLines((int)e->x,(int)e->y,ITEM_DEFS[e->level].radius+25*t,Fade(COLORS[e->level],1-t));
            center(TextFormat("+%u",ITEM_DEFS[e->level].value),(int)e->x,(int)(e->y-35-45*t),18,Fade(ACCENT,1-t));
        }
        label("UPGRADE PATH",775,170,16,ACCENT); label("Match two of the same item",775,198,12,MUTED);
        for(int i=0;i<ITEM_LEVELS;i++) {
            int y=239+i*43; icon(793,y,i,16,1);
            label(TextFormat("%02d",i+1),818,y-7,12,MUTED);
            label(ITEM_DEFS[i].name,844,y-7,13,i<=game.highest?INK:MUTED);
        }
        label(TextFormat("%u merges",game.merges),775,737,14,MUTED);
        center("Build a Divine Rapier. Keep the stash below the line.",510,770,15,MUTED);
        if(paused||game.over) {
            DrawRectangle(290,165,440,570,Fade(BG,0.88f));
            center(game.over?"STASH OVERFLOW":"PAUSED",510,367,29,ACCENT);
            center(game.over?TextFormat("Gold earned: %u",game.score):"Take a breather, shopkeeper.",510,417,18,INK);
            center(game.over?"Tap NEW RUN to play again":"Tap RESUME to continue",510,461,16,MUTED);
        }
        EndTextureMode();
        BeginDrawing(); ClearBackground(BLACK);
        DrawTexturePro(canvas.texture,(Rectangle){0,0,WIDTH,-HEIGHT},(Rectangle){ox,oy,WIDTH*scale,HEIGHT*scale},(Vector2){0,0},0,WHITE);
        EndDrawing();
    }
    save_best(best); UnloadRenderTexture(canvas);CloseWindow(); return 0;
}

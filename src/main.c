#include "raylib.h"
#include "rlgl.h"
#include "game.h"
#include <math.h>
#include <stdio.h>
#include <time.h>
#if defined(PLATFORM_ANDROID)
#include <android_native_app_glue.h>
// Android backend extension; not declared by the cross-platform raylib header.
extern struct android_app *GetAndroidApp(void);
static int32_t (*raylib_input_handler)(struct android_app *, AInputEvent *);
static int32_t handle_android_input(struct android_app *app,AInputEvent *event) {
    if(AInputEvent_getType(event)==AINPUT_EVENT_TYPE_KEY && AKeyEvent_getKeyCode(event)==AKEYCODE_BACK) {
        // Android 13+ routes system Back through GameActivity's platform callback.
        // Older Android sends completed key presses through this native queue.
        if(app->activity->sdkVersion<33 && AKeyEvent_getAction(event)==AKEY_EVENT_ACTION_UP &&
            !(AKeyEvent_getFlags(event)&AKEY_EVENT_FLAG_CANCELED)) {
            JavaVM *vm=app->activity->vm;
            JNIEnv *env=NULL;
            bool attached=false;
            jint status=(*vm)->GetEnv(vm,(void **)&env,JNI_VERSION_1_6);
            if(status==JNI_EDETACHED) {
                attached=(*vm)->AttachCurrentThread(vm,&env,NULL)==JNI_OK;
                if(!attached) env=NULL;
            }
            if(env) {
                jclass activity=(*env)->GetObjectClass(env,app->activity->clazz);
                jmethodID on_back=activity?(*env)->GetMethodID(env,activity,"onNativeBackPressed","()V"):NULL;
                if(on_back) (*env)->CallVoidMethod(env,app->activity->clazz,on_back);
                if((*env)->ExceptionCheck(env)) {
                    (*env)->ExceptionClear(env);
                    TraceLog(LOG_WARNING,"ANDROID: Could not dispatch Back to activity");
                }
                if(activity) (*env)->DeleteLocalRef(env,activity);
            }
            if(attached) (*vm)->DetachCurrentThread(vm);
        }
        return 1; // Consume down, repeats, canceled releases, and modern duplicate keys.
    }
    return raylib_input_handler?raylib_input_handler(app,event):0;
}
#endif
#if defined(PLATFORM_ANDROID)
#define MOBILE_LAYOUT
#endif
#if defined(MOBILE_LAYOUT)
#define WIDTH 488
#define HEIGHT 960
#else
#define WIDTH 1040
#define HEIGHT 820
#endif
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
static Texture2D item_textures[ITEM_LEVELS];
static const char *ITEM_ICON_FILES[ITEM_LEVELS]={
    "branches.png","tango.png","clarity.png","flask.png","magic_wand.png","boots.png",
    "blink.png","lesser_crit.png","black_king_bar.png","greater_crit.png","rapier.png"
};
static void load_item_textures(void) {
    for(int i=0;i<ITEM_LEVELS;i++) {
        char path[1024];
#if defined(PLATFORM_ANDROID)
        // Gradle bundles the shared assets directory at the APK asset root.
        snprintf(path,sizeof(path),"items/%s",ITEM_ICON_FILES[i]);
#else
        // Prefer assets beside the executable, regardless of the launch directory.
        snprintf(path,sizeof(path),"%sassets/items/%s",GetApplicationDirectory(),ITEM_ICON_FILES[i]);
        if(!FileExists(path)) snprintf(path,sizeof(path),"assets/items/%s",ITEM_ICON_FILES[i]);
#endif
        item_textures[i]=LoadTexture(path);
        if(item_textures[i].id) SetTextureFilter(item_textures[i],TEXTURE_FILTER_BILINEAR);
        else TraceLog(LOG_WARNING,"ITEMS: Missing icon for %s; using fallback",ITEM_DEFS[i].name);
    }
}
static void unload_item_textures(void) {
    for(int i=0;i<ITEM_LEVELS;i++) {
        if(item_textures[i].id) UnloadTexture(item_textures[i]);
        item_textures[i]=(Texture2D){0};
    }
}
static Vector2 point(float x,float y,float u,float v,float s) { return (Vector2){x+u*s,y+v*s}; }
static void stroke(float x,float y,float s,float ax,float ay,float bx,float by,float thick,Color c) {
    DrawLineEx(point(x,y,ax,ay,s),point(x,y,bx,by,s),thick*s,c);
}
static void fallback_icon(float x,float y,int level,float r,float alpha) {
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
static void icon(float x,float y,int level,float r,float alpha) {
    Texture2D texture=item_textures[level];
    if(!texture.id) { fallback_icon(x,y,level,r,alpha); return; }
    DrawCircle((int)x+2,(int)y+4,r,Fade(BLACK,0.25f*alpha));
    DrawCircleV((Vector2){x,y},r,Fade((Color){29,43,48,255},alpha));
    // Crop a centered square without stretching the rectangular Dota artwork.
    // Textured triangles clip it to the physical circle, including rotation.
    float crop=(float)(texture.width<texture.height?texture.width:texture.height);
    float u=crop/(2*texture.width),v=crop/(2*texture.height),radius=fmaxf(1,r-2);
    const int segments=64;
    Color tint=Fade(WHITE,alpha);
    rlCheckRenderBatchLimit(segments*3);
    // Switching primitive mode resets the batch texture in OpenGL 3/ES2.
    rlBegin(RL_TRIANGLES);
    rlSetTexture(texture.id);
    rlColor4ub(tint.r,tint.g,tint.b,tint.a);
    for(int i=0;i<segments;i++) {
        float a=(float)i*2*PI/segments,b=(float)(i+1)*2*PI/segments;
        float ax=cosf(a),ay=sinf(a),bx=cosf(b),by=sinf(b);
        rlTexCoord2f(0.5f,0.5f); rlVertex2f(x,y);
        rlTexCoord2f(0.5f+bx*u,0.5f+by*v); rlVertex2f(x+bx*radius,y+by*radius);
        rlTexCoord2f(0.5f+ax*u,0.5f+ay*v); rlVertex2f(x+ax*radius,y+ay*radius);
    }
    rlEnd();
    rlSetTexture(0);
    DrawCircleLines((int)x,(int)y,r-1,Fade(COLORS[level],alpha));
}
static void card(int x,int y,int w,int h) {
    DrawRectangleRounded((Rectangle){x,y,w,h},0.08f,8,PANEL);
}
static void draw_stash(const Game *game,bool paused,float aim) {
    DrawRectangleRounded((Rectangle){282,157,456,588},0.025f,8,(Color){45,62,65,255});
    DrawRectangle(290,165,440,570,(Color){15,25,31,255});
    for(int y=270;y<735;y+=46) DrawLine(290,y,730,y,Fade(MUTED,0.035f));
    Color danger=game->overflow>0?(Color){244,108,91,255}:MUTED;
    for(int x=298;x<730;x+=18) DrawLine(x,DANGER_Y,x+9,DANGER_Y,Fade(danger,0.6f));
    if(game->overflow>0) {
        center(TextFormat("STASH FULL  %.1fs",fmaxf(0,2-game->overflow)),510,207,16,danger);
    }
    if(!game->over&&!paused) {
        float r=ITEM_DEFS[game->current].radius;
        DrawLine((int)aim,DROP_Y,(int)aim,725,Fade(COLORS[game->current],0.15f));
        icon(aim,DROP_Y,game->current,r,game->cooldown>0?0.3f:0.8f);
    }
    for(int i=0;i<MAX_ITEMS;i++) if(game->items[i].active) {
        const Item *a=&game->items[i];
        rlPushMatrix();
        rlTranslatef(a->x,a->y,0);
        rlRotatef(a->angle*RAD2DEG,0,0,1);
        icon(0,0,a->level,ITEM_DEFS[a->level].radius,1);
        rlPopMatrix();
    }
    for(int i=0;i<MAX_EFFECTS;i++) if(game->effects[i].life>0) {
        const Effect *e=&game->effects[i];float t=1-e->life/0.55f;
        DrawCircleLines((int)e->x,(int)e->y,ITEM_DEFS[e->level].radius+25*t,Fade(COLORS[e->level],1-t));
        center(TextFormat("+%u",ITEM_DEFS[e->level].value),(int)e->x,(int)(e->y-35-45*t),18,Fade(ACCENT,1-t));
    }
    if(paused||game->over) {
        DrawRectangle(290,165,440,570,Fade(BG,0.88f));
        center(game->over?"STASH OVERFLOW":"PAUSED",510,367,29,ACCENT);
        center(game->over?TextFormat("Gold earned: %u",game->score):"Take a breather, shopkeeper.",510,417,18,INK);
        center(game->over?"Tap NEW RUN to play again":"Tap RESUME to continue",510,461,16,MUTED);
    }
}
#if defined(MOBILE_LAYOUT)
typedef struct {
    Rectangle stash,pause,restart;
    float scale;
} MobileLayout;
static MobileLayout mobile_layout(int height) {
    MobileLayout layout;
    layout.scale=fminf(1.0f,(height-424.0f)/(WELL_BOTTOM-WELL_TOP));
    float w=(WELL_RIGHT-WELL_LEFT)*layout.scale;
    float h=(WELL_BOTTOM-WELL_TOP)*layout.scale;
    layout.stash=(Rectangle){(WIDTH-w)/2,240+(height-424-h)/2,w,h};
    layout.pause=(Rectangle){24,height-70,214,54};
    layout.restart=(Rectangle){250,height-70,214,54};
    return layout;
}
static void draw_mobile(const Game *game,unsigned best,bool paused,float aim,int height,MobileLayout layout) {
    center("DOTA ITEM MERGE",WIDTH/2,22,28,ACCENT);
    card(24,68,214,68); card(250,68,214,68);
    label("GOLD EARNED",40,80,14,MUTED);
    label(TextFormat("%06u",game->score),40,101,26,ACCENT);
    label("PERSONAL BEST",266,80,14,MUTED);
    label(TextFormat("%u",best),266,101,26,INK);
    card(24,148,440,56);
    label("NEXT DELIVERY",40,158,12,MUTED);
    label(ITEM_DEFS[game->next].name,40,178,18,INK);
    icon(432,176,game->next,22,1);
    label("THE STASH",(int)layout.stash.x,(int)layout.stash.y-28,16,ACCENT);
    const char *merges=TextFormat("%u merges",game->merges);
    label(merges,(int)(layout.stash.x+layout.stash.width)-MeasureText(merges,14),(int)layout.stash.y-27,14,MUTED);
    // Draw in simulation coordinates; input uses the inverse of this transform.
    rlPushMatrix();
    rlTranslatef(layout.stash.x,layout.stash.y,0);
    rlScalef(layout.scale,layout.scale,1);
    rlTranslatef(-WELL_LEFT,-WELL_TOP,0);
    draw_stash(game,paused,aim);
    rlPopMatrix();
    center("Drag to aim. Release to drop.",WIDTH/2,height-164,18,INK);
    label("UPGRADE PATH",24,height-134,12,MUTED);
    const char *highest=ITEM_DEFS[game->highest].name;
    label(highest,464-MeasureText(highest,12),height-134,12,ACCENT);
    for(int i=0;i<ITEM_LEVELS;i++) {
        float x=44+i*40;
        icon(x,height-100,i,15,i<=game->highest?1.0f:0.4f);
        if(i==game->highest) DrawCircleLines((int)x,height-100,19,ACCENT);
    }
    card((int)layout.pause.x,(int)layout.pause.y,(int)layout.pause.width,(int)layout.pause.height);
    card((int)layout.restart.x,(int)layout.restart.y,(int)layout.restart.width,(int)layout.restart.height);
    center(paused?"RESUME":"PAUSE",131,height-52,18,ACCENT);
    center("NEW RUN",357,height-52,18,ACCENT);
}
#endif
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
#if defined(PLATFORM_ANDROID)
    // Use the native display size to avoid a second layer of Raylib letterboxing.
    InitWindow(0,0,"Dota Item Merge");
#else
    InitWindow(WIDTH,HEIGHT,"Dota Item Merge");
#endif
#if defined(PLATFORM_ANDROID)
    struct android_app *app=(struct android_app *)GetAndroidApp();
    if(app) {
        raylib_input_handler=app->onInputEvent;
        app->onInputEvent=handle_android_input;
    }
    if(app && app->activity && app->activity->internalDataPath)
        snprintf(best_path,sizeof(best_path),"%s/dota-item-merge.best",app->activity->internalDataPath);
    bool touch_was_down=false, dragging=false;
#else
#if defined(MOBILE_LAYOUT)
    SetWindowMinSize(320,568);
#else
    SetWindowMinSize(780,615);
#endif
#endif
    load_item_textures();
    SetTargetFPS(60);
    int canvas_height=HEIGHT;
#if defined(MOBILE_LAYOUT)
    canvas_height=(int)fmaxf(820,roundf(WIDTH*(float)GetScreenHeight()/GetScreenWidth()));
#endif
    RenderTexture2D canvas=LoadRenderTexture(WIDTH,canvas_height);
    SetTextureFilter(canvas.texture,TEXTURE_FILTER_BILINEAR);
    Game game; game_init(&game,(uint32_t)time(NULL));
    unsigned best=load_best(); bool paused=false; double accumulator=0; float aim=510;
#if !defined(MOBILE_LAYOUT)
    const Rectangle pause_button={40,655,96,62}, restart_button={145,655,105,62};
#endif
    while(!WindowShouldClose()) {
#if defined(MOBILE_LAYOUT)
        int desired_height=(int)fmaxf(820,roundf(WIDTH*(float)GetScreenHeight()/GetScreenWidth()));
        if(desired_height!=canvas_height) {
            UnloadRenderTexture(canvas);
            canvas_height=desired_height;
            canvas=LoadRenderTexture(WIDTH,canvas_height);
            SetTextureFilter(canvas.texture,TEXTURE_FILTER_BILINEAR);
        }
        MobileLayout layout=mobile_layout(canvas_height);
        Rectangle pause_button=layout.pause,restart_button=layout.restart;
#endif
        float scale=fminf((float)GetScreenWidth()/WIDTH,(float)GetScreenHeight()/canvas_height);
        float ox=(GetScreenWidth()-WIDTH*scale)/2,oy=(GetScreenHeight()-canvas_height*scale)/2;
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
            game_reset(&game,(uint32_t)time(NULL)); paused=false; accumulator=0;
        }
        if(toggle_pause && !game.over) paused=!paused;
#if defined(MOBILE_LAYOUT)
        bool in_stash=CheckCollisionPointRec(mouse,layout.stash);
        float pointer_aim=WELL_LEFT+(mouse.x-layout.stash.x)/layout.scale;
#else
        bool in_stash=mouse.x>=WELL_LEFT&&mouse.x<=WELL_RIGHT&&mouse.y>=120&&mouse.y<=WELL_BOTTOM;
        float pointer_aim=mouse.x;
#endif
#if defined(PLATFORM_ANDROID)
        if(pressed) dragging=in_stash&&!paused&&!game.over;
        if(touch_down&&dragging) aim=pointer_aim;
        bool click=released&&dragging;
        if(released) dragging=false;
        touch_was_down=touch_down;
#else
        if(GetMouseDelta().x!=0) aim=pointer_aim;
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
        for(int i=0;i<20;i++) DrawLine(i*60,0,i*60-200,canvas_height,Fade(MUTED,0.025f));
#if defined(MOBILE_LAYOUT)
        draw_mobile(&game,best,paused,aim,canvas_height,layout);
#else
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
        draw_stash(&game,paused,aim);
        label("UPGRADE PATH",775,170,16,ACCENT); label("Match two of the same item",775,198,12,MUTED);
        for(int i=0;i<ITEM_LEVELS;i++) {
            int y=239+i*43; icon(793,y,i,16,1);
            label(TextFormat("%02d",i+1),818,y-7,12,MUTED);
            label(ITEM_DEFS[i].name,844,y-7,13,i<=game.highest?INK:MUTED);
        }
        label(TextFormat("%u merges",game.merges),775,737,14,MUTED);
        center("Build a Divine Rapier. Keep the stash below the line.",510,770,15,MUTED);
#endif
        EndTextureMode();
        BeginDrawing(); ClearBackground(BG);
        DrawTexturePro(canvas.texture,(Rectangle){0,0,WIDTH,-canvas_height},(Rectangle){ox,oy,WIDTH*scale,canvas_height*scale},(Vector2){0,0},0,WHITE);
        EndDrawing();
    }
#if defined(PLATFORM_ANDROID)
    if(app) app->onInputEvent=raylib_input_handler;
#endif
    save_best(best); game_destroy(&game); unload_item_textures(); UnloadRenderTexture(canvas);CloseWindow(); return 0;
}

/* Exercise the actual render.c and poly_txt.h with a vertex sink, not GL.
 * Checks routing, fractional UV retention and both transparency passes. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define FPSE_MINIGL 1
#define _W3D_H_
typedef float W3D_Float;
static struct { struct { int x,y; } DrawOffset; } PSXDisplay;
static struct { int x,y; } W3D_Offset;
static short lx0,ly0,lx1,ly1,lx2,ly2,lx3,ly3;
static int scale_gfx=2,bUsingTWin,usMirror,iGPUHeightMask=511;
static float x_scale=2,y_scale=2;
static unsigned W3D_Flags;
#ifdef FPSE_MINIGL_UI_CAPTURE
static int GlobalTextTP,GlobalTextAddrX,GlobalTextAddrY,DrawSemiTrans;
static unsigned long lGPUInfoVals[1];
#define INFO_TW 0
#endif
int texfilter=1,texred;
unsigned long mgl_sprite_uv_candidates;
#ifdef FPSE_MINIGL_QUAD_SPRITES
unsigned long mgl_quad_sprite_candidates;
int mgl_quad_sprite_enabled=1;
#endif
#define FLAG_MULTITEXT 1
#define SCALING_EXP 2
#define ARGBx2(x) (x)
#define SHADETEXBIT(x) ((x)&0x01000000)
#define CLAMP(x,a,b) ((x)<(a)?(a):(x)>(b)?(b):(x))
static unsigned long read_word(const unsigned char *p)
{
    return p[0]|((unsigned long)p[1]<<8)|((unsigned long)p[2]<<16)|((unsigned long)p[3]<<24);
}
#define gpuData(n) read_word(baseAddr+4*(n))
typedef struct { float x,y,u,v,a; } SinkVertex;
static SinkVertex vertices[32];
static unsigned used,passes;
static int second_pass;
static void OffsetSet(void) {}
static float set_blend_mode(int *opaque) { (void)opaque; return 0.5f; }
int set_texture(int x,int y,int opaque)
{
    (void)x;(void)y;(void)opaque;++passes;return second_pass;
}
static void mgl_gpu_fill(int a,int b,int c,int d,unsigned long e)
{ (void)a;(void)b;(void)c;(void)d;(void)e; }
#define SETUP(a,b) ((void)0);
#define INDEX_1(a) ((void)0);
#define INDEX_2(a,b) ((void)0);
#define INDEX_3(a,b,c) ((void)0);
#define INDEX_6(a,b,c,d,e,f) ((void)0);
static void emit(unsigned i,float x,float y,float u,float v,float alpha)
{
    assert(used+i<32);
    vertices[used+i].x=x*x_scale;vertices[used+i].y=y*y_scale;
    vertices[used+i].u=u/256.0f;vertices[used+i].v=v/256.0f;
    vertices[used+i].a=alpha;
}
#define VERTEX_TEX(i,x,y,u,v,rgb) do { (void)(rgb); emit(i,x,y,u,v,alpha); } while(0)
#define VERTEX(i,x,y,rgb) VERTEX_TEX(i,x,y,0,0,rgb)
#define DRAW(n) used+=(n);
#include "../../gpuw3d/w3d/render.c"

static unsigned char packet[40];
static void reset(void)
{
    memset(vertices,0,sizeof(vertices));memset(packet,0,sizeof(packet));
    packet[3]=0x65;packet[8]=8;packet[9]=16;packet[12]=32;packet[14]=32;
    used=passes=0;mgl_sprite_uv_candidates=0;
    texfilter=1;texred=0;bUsingTWin=0;usMirror=0;
    second_pass=0;x_scale=y_scale=2;lx0=10;ly0=20;
}
static void check(int changed,int two)
{
    unsigned p;
    float delta=changed?0.5f:0;
    assert(used==(two?8:4));assert(passes==(two?2:1));
    for(p=0;p<used;p+=4) {
        assert(vertices[p].u==(8+delta)/256.0f);
        assert(vertices[p+3].u==(40-delta)/256.0f);
        assert(vertices[p].v==(16+delta)/256.0f);
        assert(vertices[p+3].v==(48-delta)/256.0f);
        assert(vertices[p].x==10*x_scale && vertices[p+3].x==42*x_scale);
        assert(vertices[p].y==20*y_scale && vertices[p+3].y==52*y_scale);
        assert(vertices[p].a==0.5f);
    }
}
int main(void)
{
    int i;
    for(i=0;i<2;++i) {
        reset();second_pass=i;DrawSoftwareSprite(packet,32,32,8,16);
        check(FPSE_MINIGL_SPRITE_UV_AB,i);assert(mgl_sprite_uv_candidates==1);
    }
    reset();texfilter=0;DrawSoftwareSprite(packet,32,32,8,16);check(0,0);
    assert(!mgl_sprite_uv_candidates);
    reset();x_scale=y_scale=1;DrawSoftwareSprite(packet,32,32,8,16);check(0,0);
    assert(!mgl_sprite_uv_candidates);
    reset();texred=1;DrawSoftwareSprite(packet,32,32,8,16);check(0,0);
    assert(!mgl_sprite_uv_candidates);
    reset();bUsingTWin=1;DrawSoftwareSpriteTWin(packet,32,32);check(0,0);
    assert(!mgl_sprite_uv_candidates);
    /* Original packet wraps despite this fragment's safe 8..40 UVs. */
    reset();packet[12]=255;DrawSoftwareSprite(packet,32,32,8,16);check(0,0);
    assert(!mgl_sprite_uv_candidates);
    /* Ordinary textured quad/polygon entry MUST NOT get the sprite inset. */
    reset();drawPoly4FTi(10,20,10,52,42,20,42,52,8,16,8,48,40,16,40,48,0,0,0);
    check(0,0);assert(!mgl_sprite_uv_candidates);
    reset();usMirror=0x3000;DrawSoftwareSpriteMirror(packet,32,32);
    assert(used==4 && mgl_sprite_uv_candidates==1);
    assert(vertices[0].u==(FPSE_MINIGL_SPRITE_UV_AB?39.5f:40)/256.0f);
    assert(vertices[3].v==(FPSE_MINIGL_SPRITE_UV_AB?16.5f:16)/256.0f);
#ifdef FPSE_MINIGL_QUAD_SPRITES
    for(i=0;i<2;++i) {
        reset();second_pass=i;mgl_quad_sprite_candidates=0;mgl_quad_sprite_enabled=1;
        lx0=10;ly0=20;lx1=10;ly1=52;lx2=42;ly2=20;lx3=42;ly3=52;
        packet[3]=0x2c;packet[8]=8;packet[9]=16;
        packet[16]=8;packet[17]=48;packet[24]=40;packet[25]=16;packet[32]=40;packet[33]=48;
        drawPoly4FT(packet);check(1,i);assert(mgl_quad_sprite_candidates==1);
        used=passes=0;mgl_quad_sprite_enabled=0;drawPoly4FT(packet);check(0,i);
        used=passes=0;mgl_quad_sprite_enabled=1;texfilter=0;drawPoly4FT(packet);check(0,i);
        used=passes=0;texfilter=1;x_scale=y_scale=1;drawPoly4FT(packet);check(0,i);
        assert(mgl_quad_sprite_candidates==1);
    }
    puts("PASS: actual quad entry corrected in both passes; toggle/nearest/1:1 preserve old UV");
    for(i=0;i<2;++i) {
        unsigned p;
        reset();second_pass=i;mgl_quad_sprite_enabled=1;
        x_scale=y_scale=2.5f;
        lx0=117;ly0=119;lx1=53;ly1=119;lx2=117;ly2=105;lx3=53;ly3=105;
        packet[3]=0x2e;packet[8]=0;packet[9]=192;
        packet[16]=63;packet[17]=192;packet[24]=0;packet[25]=206;
        packet[32]=63;packet[33]=206;
        drawPoly4FT(packet);
        assert(used==(i?8:4));
        for(p=0;p<used;p+=4) {
            assert(vertices[p].u==0.5f/256 && vertices[p+1].u==63.5f/256);
            assert(vertices[p].v==192.5f/256 && vertices[p+2].v==205.5f/256);
            assert(vertices[p].x==117*2.5f && vertices[p+3].y==105*2.5f);
        }
        used=passes=0;mgl_quad_sprite_enabled=0;drawPoly4FT(packet);
        assert(vertices[0].u==0 && vertices[1].u==63.0f/256);
        mgl_quad_sprite_enabled=1;
    }
    /* Restore geometry used by the capture regression below. */
    lx0=10;ly0=20;lx1=10;ly1=52;lx2=42;ly2=20;lx3=42;ly3=52;
    puts("PASS: captured mirrored inclusive/exclusive quad, both passes and F7 off");
#endif
#ifdef FPSE_MINIGL_UI_CAPTURE
    {
        FILE *f;char line[512];int sprite=0,polygon=0;
        mgl_ui_reset();assert(mgl_ui_arm());mgl_ui_boundary();
        reset();DrawSoftwareSprite(packet,32,32,8,16);
        check(1,0); /* Capture must not change emitted vertices. */
        drawPoly4FTi(10,20,10,52,42,20,42,52,8,16,8,48,40,16,40,48,0,0,0);
#ifdef FPSE_MINIGL_QUAD_SPRITES
        packet[3]=0x2c;packet[16]=8;packet[17]=48;packet[24]=40;packet[25]=16;
        packet[32]=40;packet[33]=48;
        drawPoly4FT(packet);
        assert(used==12 && vertices[8].u==8.5f/256.0f);
#endif
        mgl_ui_boundary();assert(mgl_ui_ready());f=tmpfile();assert(f && mgl_ui_write(f));
        rewind(f);
        while(fgets(line,sizeof(line),f)) {
            if(strstr(line,"source=drawSprite4FTfloat")) sprite=1;
            if(strstr(line,"source=drawPoly4FTi")) polygon=1;
        }
        assert(sprite && polygon);fclose(f);mgl_ui_done(1);mgl_ui_reset();
        puts("PASS: actual primitive hooks capture sprite and polygon sources without changing geometry/UV");
    }
#endif
    printf("PASS: real render.c variant %c; sprite routing, both passes, mirror, wrap, nearest, 1:1 and polygon unchanged\n",
        FPSE_MINIGL_SPRITE_UV_AB?'B':'A');
    return 0;
}

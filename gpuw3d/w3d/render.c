/*
 * render.c
 */

#include "w3d.h"
#include "texture.h"
#ifdef FPSE_MINIGL_UI_CAPTURE
#include "../../gpuminigl/ui_capture.h"
extern int texfilter, texred;
#endif

void FillSoftwareAreaTrans(short x0,short y0,short x1,short y1,unsigned long rgb)
{
#if 1
  W3D_Float fx1, fx2, fy1, fy2, alpha;

  SETUP(W3D_PRIMITIVE_TRIANGLES, PRIM_DEFAULT)

#ifdef W3D_VERBOSE
  W3D_VERBOSE_POLY_SMP++;
#endif

  alpha = set_blend_mode(NULL);

  if (scale_gfx == SCALING_EXP) {
    OffsetSet();
    x0 += W3D_Offset.x;
    x1 += W3D_Offset.x;
    y0 += W3D_Offset.y;
    y1 += W3D_Offset.y;
  }

  fx1 = CLAMP(x0, 0, 1023);
  fx2 = CLAMP(x1, 0, 1023);
  fy1 = CLAMP(y0, 0, 511);
  fy2 = CLAMP(y1, 0, 511);

  INDEX_6(0, 1, 2, 1, 2, 3)

  VERTEX(0, fx1, fy1, rgb);
  VERTEX(1, fx2, fy1, rgb);
  VERTEX(2, fx1, fy2, rgb);
  VERTEX(3, fx2, fy2, rgb);

  DRAW(4)
#endif
}

void FillSoftwareArea(short x0,short y0,short x1,short y1,unsigned long rgb)
{
#ifdef FPSE_MINIGL
  mgl_gpu_fill(x0, y0, x1, y1, rgb);
#else
  w3d_gpu_fill(x0, y0, x1, y1, rgb);
#endif
}

static inline void drawPoly3Gi(short x1,short y1,short x2,short y2,short x3,short y3,unsigned long rgb1,unsigned long rgb2,unsigned long rgb3)
{
  #define N 3
  #define GOURAUD
  #include "poly_smp.h"
  #undef GOURAUD
  #undef N
}

static inline void drawPoly4Gi(short x1,short y1,short x2,short y2,short x3,short y3,short x4,short y4,unsigned long rgb1,unsigned long rgb2,unsigned long rgb3,unsigned long rgb4)
{
  #define N 4
  #define GOURAUD
  #include "poly_smp.h"
  #undef GOURAUD
  #undef N
}

void drawPoly3G(long rgb1, long rgb2, long rgb3)
{
  drawPoly3Gi(lx0,ly0,lx1,ly1,lx2,ly2,rgb1,rgb2,rgb3);
}

void drawPoly4G(long rgb1, long rgb2, long rgb3, long rgb4)
{
  drawPoly4Gi(lx0,ly0,lx1,ly1,lx2,ly2,lx3,ly3,rgb1,rgb2,rgb3,rgb4);
}

static inline void drawPoly3Fi(short x1,short y1,short x2,short y2,short x3,short y3,unsigned long rgb1)
{
  #define N 3
  #include "poly_smp.h"
  #undef N
}

static inline void drawPoly4Fi(short x1,short y1,short x2,short y2,short x3,short y3,short x4,short y4,unsigned long rgb1)
{
  #define N 4
  #include "poly_smp.h"
  #undef N
}

void drawPoly3F(long rgb1)
{
  drawPoly3Fi(lx0,ly0,lx1,ly1,lx2,ly2,rgb1);
}

void drawPoly4F(long rgb1)
{
  drawPoly4Fi(lx0,ly0,lx1,ly1,lx2,ly2,lx3,ly3,rgb1);
}

static inline void drawPoly3FTi(short x1,short y1,short x2,short y2,short x3,short y3,short tx1, short ty1, short tx2, short ty2, short tx3, short ty3,short clX, short clY, unsigned long rgb1)
{
  #define N 3
  #include "poly_txt.h"
  #undef N
}

static inline void drawPoly4FTi(short x1,short y1,short x2,short y2,short x3,short y3,short x4,short y4, short tx1, short ty1, short tx2, short ty2, short tx3, short ty3, short tx4, short ty4,short clX, short clY, unsigned long rgb1)
{
  #define N 4
  #include "poly_txt.h"
  #undef N
}

#ifdef FPSE_MINIGL_QUAD_SPRITES
#include "../../gpuminigl/quad_sprite_uv.h"
extern int texfilter,texred;
extern unsigned long mgl_quad_sprite_candidates;
extern int mgl_quad_sprite_enabled;
static inline void drawQuadSpriteFT(float x1,float y1,float x2,float y2,
    float x3,float y3,float x4,float y4,float tx1,float ty1,float tx2,float ty2,
    float tx3,float ty3,float tx4,float ty4,short clX,short clY,unsigned long rgb1)
{
  #define N 4
  #include "poly_txt.h"
  #undef N
}
#endif

void drawPoly3FT(unsigned char * baseAddr) {
  unsigned long *gpuData = ((unsigned long *) baseAddr);
  drawPoly3FTi(lx0,ly0,lx1,ly1,lx2,ly2,
               (gpuData(2) & 0x000000ff), ((gpuData(2)>>8) & 0x000000ff),
               (gpuData(4) & 0x000000ff), ((gpuData(4)>>8) & 0x000000ff),
               (gpuData(6) & 0x000000ff), ((gpuData(6)>>8) & 0x000000ff),
               ((gpuData(2)>>12) & 0x3f0), ((gpuData(2)>>22) & iGPUHeightMask),
               gpuData(0));
}

void drawPoly4FT(unsigned char * baseAddr) {
  unsigned long *gpuData = ((unsigned long *) baseAddr);
#ifdef FPSE_MINIGL_QUAD_SPRITES
  if(mgl_quad_sprite_enabled) {
    MGLQuadSpriteVertex q[4];unsigned i;
    q[0].x=lx0;q[0].y=ly0;q[1].x=lx1;q[1].y=ly1;
    q[2].x=lx2;q[2].y=ly2;q[3].x=lx3;q[3].y=ly3;
    for(i=0;i<4;++i) {unsigned long uv=gpuData(2+2*i);q[i].u=uv&255;q[i].v=(uv>>8)&255;}
    if(mgl_quad_sprite_inset(q,x_scale,y_scale,texfilter,texred,bUsingTWin)) {
      ++mgl_quad_sprite_candidates;
      drawQuadSpriteFT(q[0].x,q[0].y,q[1].x,q[1].y,q[2].x,q[2].y,q[3].x,q[3].y,
          q[0].u,q[0].v,q[1].u,q[1].v,q[2].u,q[2].v,q[3].u,q[3].v,
          ((gpuData(2)>>12)&0x3f0),((gpuData(2)>>22)&iGPUHeightMask),gpuData(0));
      return;
    }
  }
#endif
  drawPoly4FTi(lx0,ly0,lx1,ly1,lx2,ly2,lx3,ly3,
               (gpuData(2) & 0x000000ff), ((gpuData(2)>>8) & 0x000000ff),
               (gpuData(4) & 0x000000ff), ((gpuData(4)>>8) & 0x000000ff),
               (gpuData(6) & 0x000000ff), ((gpuData(6)>>8) & 0x000000ff),
               (gpuData(8) & 0x000000ff), ((gpuData(8)>>8) & 0x000000ff),
               ((gpuData(2)>>12) & 0x3f0), ((gpuData(2)>>22) & iGPUHeightMask),
               gpuData(0));
}

static inline void drawPoly3GTi(short x1,short y1,short x2,short y2,short x3,short y3,short tx1, short ty1, short tx2, short ty2, short tx3, short ty3,short clX, short clY, unsigned long rgb1, unsigned long rgb2, unsigned long rgb3)
{
  #define N 3
  #define GOURAUD
  #include "poly_txt.h"
  #undef GOURAUD
  #undef N
}

static inline void drawPoly4GTi(short x1,short y1,short x2,short y2,short x3,short y3,short x4,short y4, short tx1, short ty1, short tx2, short ty2, short tx3, short ty3, short tx4, short ty4,short clX, short clY, unsigned long rgb1, unsigned long rgb2, unsigned long rgb3, unsigned long rgb4)
{
  #define N 4
  #define GOURAUD
  #include "poly_txt.h"
  #undef GOURAUD
  #undef N
}

void drawPoly3GT(unsigned char * baseAddr) {
  unsigned long *gpuData = ((unsigned long *) baseAddr);
  drawPoly3GTi(lx0,ly0,lx1,ly1,lx2,ly2,
               (gpuData(2) & 0x000000ff), ((gpuData(2)>>8) & 0x000000ff),
               (gpuData(5) & 0x000000ff), ((gpuData(5)>>8) & 0x000000ff),
               (gpuData(8) & 0x000000ff), ((gpuData(8)>>8) & 0x000000ff),
               ((gpuData(2)>>12) & 0x3f0), ((gpuData(2)>>22) & iGPUHeightMask),
               gpuData(0),gpuData(3),gpuData(6));
}

void drawPoly4GT(unsigned char * baseAddr) {
  unsigned long *gpuData = ((unsigned long *) baseAddr);
  drawPoly4GTi(lx0,ly0,lx1,ly1,lx2,ly2,lx3,ly3,
               (gpuData(2) & 0x000000ff), ((gpuData(2)>>8) & 0x000000ff),
               (gpuData(5) & 0x000000ff), ((gpuData(5)>>8) & 0x000000ff),
               (gpuData(8) & 0x000000ff), ((gpuData(8)>>8) & 0x000000ff),
               (gpuData(11) & 0x000000ff), ((gpuData(11)>>8) & 0x000000ff),
               ((gpuData(2)>>12) & 0x3f0), ((gpuData(2)>>22) & iGPUHeightMask),
               gpuData(0),gpuData(3),gpuData(6),gpuData(9));
}

#if defined(FPSE_MINIGL) && defined(FPSE_MINIGL_SPRITE_UV_AB)
#include "../../gpuminigl/sprite_uv.h"
extern int texfilter, texred;
extern unsigned long mgl_sprite_uv_candidates;

#if FPSE_MINIGL_SPRITE_UV_AB
/* Separate float-UV entry: the original polygon entry keeps short UVs.
 * poly_txt.h emits the same adjusted UVs in BOTH transparency passes. */
static inline void drawSprite4FTfloat(short x1, short y1, short x2, short y2,
    short x3, short y3, short x4, short y4,
    float tx1, float ty1, float tx2, float ty2, float tx3, float ty3,
    float tx4, float ty4, short clX, short clY, unsigned long rgb1)
{
  #define N 4
  #include "poly_txt.h"
  #undef N
}
#endif

static inline void drawSprite4FTi(unsigned char *baseAddr,
    short x1, short y1, short x2, short y2, short x3, short y3, short x4, short y4,
    short tx1, short ty1, short tx2, short ty2, short tx3, short ty3,
    short tx4, short ty4, short clX, short clY, unsigned long rgb1)
{
    unsigned long *gpuData = (unsigned long *)baseAddr;
    MGLSpriteUV uv;
    int wrapped = 0;
    unsigned command = (unsigned)(gpuData(0) >> 24) & 0xfc;
    /* primSprtS splits wrapping sprites into pieces. Check the ORIGINAL
     * packet so neither side of an artificial split receives an inset. */
    if (command == 0x64) {
        unsigned wh = (unsigned)gpuData(3);
        wrapped = baseAddr[8] + (wh & 1023) > 256 ||
                  baseAddr[9] + ((wh >> 16) & 511) > 256;
    }
    uv.u0 = tx1; uv.v0 = ty1; uv.u1 = tx4; uv.v1 = ty4;
    if (mgl_sprite_uv_inset(&uv,x_scale,y_scale,texfilter,texred,bUsingTWin,wrapped)) {
        ++mgl_sprite_uv_candidates;
#if FPSE_MINIGL_SPRITE_UV_AB
        drawSprite4FTfloat(x1,y1,x2,y2,x3,y3,x4,y4,
            uv.u0,uv.v0,uv.u0,uv.v1,uv.u1,uv.v0,uv.u1,uv.v1,clX,clY,rgb1);
        return;
#endif
    }
    drawPoly4FTi(x1,y1,x2,y2,x3,y3,x4,y4,
        tx1,ty1,tx2,ty2,tx3,ty3,tx4,ty4,clX,clY,rgb1);
}
#define DRAW_SPRITE_FT(...) drawSprite4FTi(baseAddr, __VA_ARGS__)
#else
#define DRAW_SPRITE_FT(...) drawPoly4FTi(__VA_ARGS__)
#endif

void DrawSoftwareSprite(unsigned char * baseAddr,short w,short h,long tx,long ty)
{
  unsigned long *gpuData = ((unsigned long *) baseAddr);
  long sprtY0,sprtX0,sprtY1,sprtX1,textY0,textX0,textY1,textX1;

  sprtY0 = ly0 + PSXDisplay.DrawOffset.y;
  sprtX0 = lx0 + PSXDisplay.DrawOffset.x;

  if (scale_gfx == SCALING_EXP) {
    OffsetSet();
    sprtY0 += W3D_Offset.y;
    sprtX0 += W3D_Offset.x;
  }

  sprtY1 = sprtY0 + h;
  sprtX1 = sprtX0 + w;

  textY0 = ty;
  textX0 = tx;
  textY1 = textY0 + h;
  textX1 = textX0 + w;

  DRAW_SPRITE_FT(sprtX0,sprtY0, sprtX0,sprtY1, sprtX1,sprtY0, sprtX1,sprtY1,
               textX0,textY0, textX0,textY1, textX1,textY0, textX1,textY1,
               ((gpuData(2)>>12) & 0x3f0), ((gpuData(2)>>22) & iGPUHeightMask),
               gpuData(0));
}

void DrawSoftwareSpriteTWin(unsigned char * baseAddr,long w,long h) // UNTESTED
{
  unsigned long *gpuData = ((unsigned long *) baseAddr);
  long sprtY0,sprtX0,sprtY1,sprtX1,textY0,textX0,textY1,textX1;

  sprtY0 = ly0 + PSXDisplay.DrawOffset.y;
  sprtX0 = lx0 + PSXDisplay.DrawOffset.x;

  if (scale_gfx == SCALING_EXP) {
    OffsetSet();
    sprtY0 += W3D_Offset.y;
    sprtX0 += W3D_Offset.x;
  }

  sprtY1 = sprtY0 + h;
  sprtX1 = sprtX0 + w;

  textY0 = (gpuData(2)>>8)&0xff;
  textX0 = (gpuData(2)>>0)&0xff;
  textY1 = textY0 + h;
  textX1 = textX0 + w;

  drawPoly4FTi(sprtX0,sprtY0, sprtX0,sprtY1, sprtX1,sprtY0, sprtX1,sprtY1,
               textX0,textY0, textX0,textY1, textX1,textY0, textX1,textY1,
               ((gpuData(2)>>12) & 0x3f0), ((gpuData(2)>>22) & iGPUHeightMask),
               gpuData(0));
}

void DrawSoftwareSpriteMirror(unsigned char * baseAddr,long w,long h) // UNTESTED
{
  unsigned long *gpuData = ((unsigned long *) baseAddr);
  long sprtY0,sprtX0,sprtY1,sprtX1,textY0,textX0,textY1,textX1,temp;

  sprtY0 = ly0 + PSXDisplay.DrawOffset.y;
  sprtX0 = lx0 + PSXDisplay.DrawOffset.x;

  if (scale_gfx == SCALING_EXP) {
    OffsetSet();
    sprtY0 += W3D_Offset.y;
    sprtX0 += W3D_Offset.x;
  }

  sprtY1 = sprtY0 + h;
  sprtX1 = sprtX0 + w;

  textY0 = (gpuData(2)>>8)&0xff;
  textX0 = (gpuData(2)>>0)&0xff;
  textY1 = textY0 + h;
  textX1 = textX0 + w;

  #define EXG(a, b) temp = a; a = b; b = temp;

  if (usMirror&0x1000) { // X mirror
    // exchange X's
    EXG(textX0, textX1);
  }
  if (usMirror&0x2000) { // Y mirror
    // exchange Y's
    EXG(textY0, textY1);
  }

  #undef EXG

  DRAW_SPRITE_FT(sprtX0,sprtY0, sprtX0,sprtY1, sprtX1,sprtY0, sprtX1,sprtY1,
               textX0,textY0, textX0,textY1, textX1,textY0, textX1,textY1,
               ((gpuData(2)>>12) & 0x3f0), ((gpuData(2)>>22) & iGPUHeightMask),
               gpuData(0));
}

#undef DRAW_SPRITE_FT

static inline void DrawSoftwareLineShadei(int x1, int y1, int x2, int y2, unsigned long rgb1, unsigned long rgb2)
{
  #define GOURAUD
  #include "line_smp.h"
  #undef GOURAUD
}

void DrawSoftwareLineShade(long rgb0, long rgb1)
{
  DrawSoftwareLineShadei(lx0, ly0, lx1, ly1, rgb0, rgb1);
}

static inline void DrawSoftwareLineFlati(int x1, int y1, int x2, int y2, unsigned long rgb1)
{
  #include "line_smp.h"
}

void DrawSoftwareLineFlat(long rgb0)
{
  DrawSoftwareLineFlati(lx0, ly0, lx1, ly1, rgb0);
}

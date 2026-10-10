/*
 * w3d.c
 */

#include "w3d.h"
#include "texture.h"
#include "repair.h"
extern int alpha_threshold, texcache;

#define LOCK()   if (W3D_LockHardware(context) == W3D_SUCCESS) {
#define UNLOCK() W3D_UnLockHardware(context); }

/* minimum sizes */
#define MIN_W 256
#define MIN_H 240

/* default sizes */
#define DEF_W 320
#define DEF_H 240

int frame_skip = 1, scale_gfx = SCALING_EXP, speed_limit = 1, hide_pointer = 0;

static const struct {
  int w, h;
} resolutions[] = {
  {  640,  480 }, /* Actual / Mode = Max PSX */
  {  320,  240 },
  {  512,  384 },
  {  640,  480 },
  {  800,  600 },
  { 1024,  768 },
  { 1280, 1024 },
  { 1600, 1200 },
};

#define MAX_RESOLUTION (7)

#include "amidog/timer.h"
static amidog_timer_t *timer = NULL;

/* PAL by default */
static double Amiga_FrameRateHz = 50.0f;
static double pre_Amiga_FrameRateHz = 50.0f;
static double frame_rate = 50.0f;

#ifdef W3D_VERBOSE
#define TOTAL_BLENDMODES (4+4+1)
static int used_blendmodes[TOTAL_BLENDMODES];
int poly_count[3];
#endif

static unsigned long dispid = INVALID_ID;

W3D_Float x_scale = 1.0, y_scale = 1.0; // SCALING_EXP
PSXPoint_t W3D_Offset = { 0, 0 };
static int OffsetNotSet = 1;

unsigned int W3D_Flags = 0;

static W3D_Float refvalue;
struct BitMap *w3d_bitmap = NULL;
W3D_Context *context = NULL;
static W3D_Scissor scissor;
static int scissor_empty;
static unsigned int draw_errors;
MyVertex array[PRIM_BUFFER];

ULONG array_primitive, w3d_primitive, array_count = 0, prim_count = 0;
ULONG array_index[PRIM_BUFFER];
W3D_Texture *array_unit_tex[MAX_TEX_UNITS] = { 0 };
W3D_Color array_unit_color[MAX_TEX_UNITS] = { { 0 } };
static int current_blend_mode = -1; /* none */

void FLUSH_ARRAY(void)
{
  ULONG result;
  if (prim_count == 0) {
    return;
  }

  if (!context || (scissor_empty && array_primitive != PRIM_FILLAREA)) goto done;

  if ((W3D_Flags & FLAG_FASTMODE3) && (current_blend_mode == (2 + 4))) {
    goto done; // reverse subtractive not supported
  }

  /* setup */
  switch (array_primitive) {
    case PRIM_DEFAULT:
      /* Disable texture mapping */
      W3D_SetState(context, W3D_TEXMAPPING, W3D_DISABLE);
      W3D_SetState(context, W3D_ALPHATEST, W3D_DISABLE);

      if (W3D_Flags & FLAG_MULTITEXT) {
#ifdef __amigaos4__
        W3D_SetTextureBlendTags(context,
          W3D_BLEND_STAGE,   0,
          W3D_COLOR_ARG_A,   W3D_ARG_DIFFUSE_COLOR,
          W3D_ALPHA_ARG_A,   W3D_ARG_DIFFUSE_ALPHA,
          W3D_COLOR_SCALE,   1,
          W3D_COLOR_COMBINE, W3D_COMBINE_SELECT_A,
          W3D_ALPHA_COMBINE, W3D_COMBINE_SELECT_A,

          W3D_BLEND_STAGE,   1,
          W3D_COLOR_COMBINE, W3D_COMBINE_DISABLED,
          W3D_ALPHA_COMBINE, W3D_COMBINE_DISABLED,
        TAG_DONE);
#endif
      }
      break;

    case PRIM_TEXTURED:
      /* Enable texture mapping */
      W3D_SetState(context, W3D_TEXMAPPING, W3D_ENABLE);
      /* Keep the cutoff relative to the vertex alpha (0.5/0.25 in ABR0/3).
       * A fixed cutoff of 0.5 discards entire semitransparent passes. */
      refvalue = w3d_alpha_reference(alpha_threshold, array[0].primary_a);
      W3D_SetAlphaMode(context, W3D_A_GREATER, &refvalue);

      if (W3D_Flags & FLAG_MULTITEXT) {
#ifdef __amigaos4__
        if ((current_blend_mode >= 4) && (current_blend_mode < 8) && (current_blend_mode != 6)) {
          ULONG i;
          for (i=0; i<array_count; i++) {
            array[i].u1 = array[i].u;
            array[i].v1 = array[i].v;
            array[i].w1 = array[i].w;
          }

          W3D_SetState(context, W3D_ALPHATEST, W3D_DISABLE);

          W3D_BindTexture(context, 0, array_unit_tex[0]);
          W3D_BindTexture(context, 1, array_unit_tex[1]);

          if ((!(W3D_Flags & FLAG_FASTMODE3)) && (current_blend_mode == (3 + 4))) {
            W3D_SetTextureBlendTags(context,
              /* create 25% texture colors */
              W3D_BLEND_STAGE,   0,
              W3D_COLOR_ARG_A,   W3D_ARG_TEXTURE_COLOR,
              W3D_COLOR_ARG_B,   W3D_ARG_FACTOR,
              W3D_ALPHA_ARG_A,   W3D_ARG_TEXTURE_ALPHA,
              W3D_COLOR_SCALE,   1,
              W3D_BLEND_FACTOR,  &array_unit_color[0],
              W3D_COLOR_COMBINE, W3D_COMBINE_MODULATE,
              W3D_ALPHA_COMBINE, W3D_COMBINE_SELECT_A,

              /* add 100% texture color layer */
              W3D_BLEND_STAGE,   1,
              W3D_COLOR_ARG_A,   W3D_ARG_PREVIOUS_COLOR,
              W3D_COLOR_ARG_B,   W3D_ARG_TEXTURE,
              W3D_COLOR_ARG_C,   W3D_ARG_TEXTURE_ALPHA,
              W3D_ALPHA_ARG_A,   W3D_ARG_TEXTURE_ALPHA,
              W3D_COLOR_SCALE,   1,
              W3D_COLOR_COMBINE, W3D_COMBINE_INTERPOLATE,
              W3D_ALPHA_COMBINE, W3D_COMBINE_SELECT_A,

              /* modulate with poly colors and scale */
              W3D_BLEND_STAGE,   2,
              W3D_COLOR_ARG_A,   W3D_ARG_PREVIOUS_COLOR,
              W3D_COLOR_ARG_B,   W3D_ARG_DIFFUSE_COLOR,
              W3D_ALPHA_ARG_A,   W3D_ARG_PREVIOUS_ALPHA,
              W3D_ALPHA_ARG_B,   W3D_ARG_DIFFUSE_ALPHA,
              W3D_COLOR_SCALE,   2,
              W3D_COLOR_COMBINE, W3D_COMBINE_MODULATE,
              W3D_ALPHA_COMBINE, W3D_COMBINE_MODULATE,
            TAG_DONE);
          } else {
            W3D_SetTextureBlendTags(context,
              W3D_BLEND_STAGE,   0,
              W3D_COLOR_ARG_A,   W3D_ARG_TEXTURE_COLOR,
              W3D_COLOR_ARG_B,   W3D_ARG_DIFFUSE_COLOR,
              W3D_ALPHA_ARG_A,   W3D_ARG_TEXTURE_ALPHA,
              W3D_ALPHA_ARG_B,   W3D_ARG_DIFFUSE_ALPHA,
              W3D_COLOR_SCALE,   2,
              W3D_COLOR_COMBINE, W3D_COMBINE_MODULATE,
              W3D_ALPHA_COMBINE, W3D_COMBINE_MODULATE,

              W3D_BLEND_STAGE,   1,
              W3D_COLOR_ARG_A,   W3D_ARG_PREVIOUS_COLOR,
              W3D_COLOR_ARG_B,   W3D_ARG_TEXTURE,
              W3D_ALPHA_ARG_A,   W3D_ARG_PREVIOUS_ALPHA,
              W3D_ALPHA_ARG_B,   W3D_ARG_TEXTURE_ALPHA,
              W3D_ALPHA_ARG_C,   W3D_ARG_TEXTURE_ALPHA,
              W3D_COLOR_SCALE,   1,
              W3D_COLOR_COMBINE, (current_blend_mode == (3 + 4)) ? W3D_COMBINE_MODULATE :    // fixes mode 3
                                                                   W3D_COMBINE_SELECT_A,
              W3D_ALPHA_COMBINE, (current_blend_mode == (0 + 4)) ? W3D_COMBINE_INTERPOLATE : // fixes mode 0
                                                                   W3D_COMBINE_SELECT_C,     // fixes mode 1

              W3D_BLEND_STAGE,   2,
              W3D_COLOR_COMBINE, W3D_COMBINE_DISABLED,
              W3D_ALPHA_COMBINE, W3D_COMBINE_DISABLED,
            TAG_DONE);
          }
        } else {
          W3D_SetState(context, W3D_ALPHATEST, W3D_ENABLE);

          W3D_BindTexture(context, 0, array_unit_tex[(current_blend_mode == (2 + 4)) ? 1 : 0]);

          W3D_SetTextureBlendTags(context,
            W3D_BLEND_STAGE,   0,
            W3D_COLOR_ARG_A,   W3D_ARG_TEXTURE_COLOR,
            W3D_ALPHA_ARG_A,   W3D_ARG_TEXTURE_ALPHA,
            W3D_COLOR_ARG_B,   W3D_ARG_DIFFUSE_COLOR,
            W3D_ALPHA_ARG_B,   W3D_ARG_DIFFUSE_ALPHA,
            W3D_COLOR_SCALE,   2,
            W3D_COLOR_COMBINE, W3D_COMBINE_MODULATE,
            W3D_ALPHA_COMBINE, W3D_COMBINE_MODULATE,

            W3D_BLEND_STAGE,   1,
            W3D_COLOR_COMBINE, W3D_COMBINE_DISABLED,
            W3D_ALPHA_COMBINE, W3D_COMBINE_DISABLED,
          TAG_DONE);
        }
#endif
      } else {
        W3D_SetState(context, W3D_ALPHATEST, W3D_ENABLE);
        W3D_BindTexture(context, 0, array_unit_tex[0]);
      }
      break;

    case PRIM_FILLAREA:
      /* Disable texture mapping */
      W3D_SetState(context, W3D_TEXMAPPING, W3D_DISABLE);
      W3D_SetState(context, W3D_ALPHATEST, W3D_DISABLE);

      /* Disable blending */
      W3D_SetState(context, W3D_BLENDING, W3D_DISABLE);

      /* Disable scissor */
      W3D_SetState(context, W3D_SCISSOR, W3D_DISABLE);

      if (W3D_Flags & FLAG_MULTITEXT) {
#ifdef __amigaos4__
        W3D_SetTextureBlendTags(context,
          W3D_BLEND_STAGE,   0,
          W3D_COLOR_ARG_A,   W3D_ARG_DIFFUSE_COLOR,
          W3D_ALPHA_ARG_A,   W3D_ARG_DIFFUSE_ALPHA,
          W3D_COLOR_SCALE,   1,
          W3D_COLOR_COMBINE, W3D_COMBINE_SELECT_A,
          W3D_ALPHA_COMBINE, W3D_COMBINE_SELECT_A,

          W3D_BLEND_STAGE,   1,
          W3D_COLOR_COMBINE, W3D_COMBINE_DISABLED,
          W3D_ALPHA_COMBINE, W3D_COMBINE_DISABLED,
        TAG_DONE);
#endif
      }
      break;

    default:
      break;
  }

  /* render */
  result = W3D_LockHardware(context);
  if (result == W3D_SUCCESS) {
    result = W3D_DrawElements(context, w3d_primitive, W3D_INDEX_ULONG, prim_count, array_index);
    W3D_UnLockHardware(context);
  }
  if (result != W3D_SUCCESS && draw_errors++ < 12)
    printf("GPUW3D repair2: draw/lock error %lu, primitive=%lu, indices=%lu, texture=%d\n",
           result, w3d_primitive, prim_count, array_primitive == PRIM_TEXTURED);

  /* cleanup */
  switch (array_primitive) {
    case PRIM_DEFAULT:
      break;

    case PRIM_TEXTURED:
      break;

    case PRIM_FILLAREA:
      /* Enable blending */
      W3D_SetState(context, W3D_BLENDING, W3D_ENABLE);

      /* Enable scissor */
      W3D_SetState(context, W3D_SCISSOR, W3D_ENABLE);
      break;

    default:
      break;
  }

done:
  array_count = prim_count = 0;
}

/* Misc variables */
int amiga_use_screen = 0;
int plugin_is_master = 0, plugin_is_slave = 0;
static struct Window *window = NULL;
static struct Screen *screen = NULL;
int amiga_width, amiga_height;
static struct BitMap *window_bitmap = NULL;
static int cur_w = DEF_W, cur_h = DEF_H;
static int new_w = DEF_W, new_h = DEF_H;
static int show_fps = 0;
static amidog_timeval_t tvi = {0, 0}, wt = {0, 0};
static int frame_count = 0;
long nf = 0;
static int make_screenshot = 0;
static unsigned short empty_pointer[] = {0,0,0,0,0,0,0,0,0,0,0,0};
int alpha_threshold = 50;

/* CGFX variables */
unsigned long pixfmt, rect_pixfmt;
int bpp;
static int bpr;
#ifdef __amigaos4__
static struct RenderInfo ri;
static ULONG lock;
#else
static APTR baseaddress;
static APTR lock;
#endif

#define GPU_INFO_LINE (2)
#include "amiga_font.c"

static void w3d_close(void)
{
  array_count = prim_count = 0;
  if (context) W3D_WaitIdle(context);
  TCacheExit();

  if (context != NULL) {
    W3D_DestroyContext(context);
    context = NULL;
  }

  if (w3d_bitmap != NULL) {
    FreeBitMap(w3d_bitmap);
    w3d_bitmap = NULL;
  }
}

void w3d_gpu_discard(void)
{
  array_count = prim_count = 0;
  memset(array_unit_tex, 0, sizeof(array_unit_tex));
  current_blend_mode = -1;
  OffsetNotSet = 1;
}

#define KEY_F1  80
#define KEY_F2  81
#define KEY_F3  82
#define KEY_F4  83
#define KEY_F5  84
#define KEY_F6  85
#define KEY_F7  86
#define KEY_F8  87
#define KEY_F9  88
#define KEY_F10 89

#define KEY_1  1
#define KEY_2  2
#define KEY_3  3
#define KEY_4  4
#define KEY_5  5
#define KEY_6  6
#define KEY_7  7
#define KEY_8  8
#define KEY_9  9
#define KEY_0 10

#define KEY_Q 16
#define KEY_W 17
#define KEY_E 18
#define KEY_R 19
#define KEY_T 20
#define KEY_Y 21
#define KEY_U 22
#define KEY_I 23
#define KEY_O 24
#define KEY_P 25

#define KEY_A 32
#define KEY_S 33
#define KEY_D 34
#define KEY_F 35
#define KEY_G 36
#define KEY_H 37
#define KEY_J 38
#define KEY_K 39
#define KEY_L 40

#define KEY_Z 49
#define KEY_X 50
#define KEY_C 51
#define KEY_V 52
#define KEY_B 53
#define KEY_N 54
#define KEY_M 55

void input_update(void)
{
  struct IntuiMessage *imsg = NULL;
  ULONG imCode,imClass;

  if (Amiga_FrameRateHz != pre_Amiga_FrameRateHz) {
    frame_rate = Amiga_FrameRateHz;
    timer_gettime(timer, &wt);
    pre_Amiga_FrameRateHz = Amiga_FrameRateHz;
  }

  /* Check for IDCMP messages */
  if ((imsg = (struct IntuiMessage *)GetMsg(window->UserPort))) {
    imClass = imsg->Class;
    imCode = imsg->Code;

    ReplyMsg((struct Message *)imsg);

    if (imClass == IDCMP_RAWKEY) {
      if (imCode == 0xdf) {
        make_screenshot = 1;
      }

      /* Special keys */
      if (imCode >= 128) {
        switch (imCode & 0x7f) {
          case KEY_F9:
            show_fps = 1 - show_fps;
            break;

          case KEY_F10:
            TCacheInvalidate();
            break;

          default:
            break;
        }
      }
    }
  }

  frame_count++;

  if (speed_limit) {
    amidog_timeval_t ct;
    int us, sleep;

    timer_gettime(timer, &ct);

    sleep = ((signed int)wt.sec - (signed int)ct.sec) * 1000000 + ((signed int)wt.usec - (signed int)ct.usec);
    if (sleep > 0) {
      timer_usleep(timer, sleep);
    }

    us = wt.usec + (int)(1000000.0f / frame_rate);
    wt.usec = (us % 1000000);
    wt.sec += (us / 1000000);
  }
}

int resolution = 3, screenmode = 0;

void amiga_clear(void)
{
  /* Clear the screen */
#ifdef __amigaos4__
  p96RectFill(window->RPort, 0, 0, window->Width, window->Height, 0x00000000);
  if ((lock = p96LockBitMap(window_bitmap, (UBYTE *)&ri, sizeof(ri)))) {
    memset((UBYTE *)ri.Memory, 0, (bpr * amiga_height));
    p96UnlockBitMap(window_bitmap, lock);
  }
#else
  FillPixelArray(window->RPort, 0, 0, window->Width, window->Height, 0x00000000);
  if ((lock = LockBitMapTags(window_bitmap, LBMI_BASEADDRESS, (ULONG)&baseaddress, TAG_END))) {
    memset((UBYTE *)baseaddress, 0, (bpr * amiga_height));
    UnLockBitMap(lock);
  }
#endif
}

static int psx_w[8] = {256,320,512,640,368,384,512,640};
#if 1
static int psx_h[4] = {240,480,240,480};
#else
static int psx_h[4] = {240,480,256,480};
#endif

void amiga_set_size(unsigned long data)
{
  FLUSH_ARRAY(); /* pending coordinates use the previous scale */
  new_w = psx_w[(data & 0x03) | ((data & 0x40) >> 4)];
  new_h = psx_h[(data >> 2) & 0x03];

#if 1
  if (scale_gfx == SCALING_EXP) {
    x_scale = (W3D_Float)amiga_width / (W3D_Float)new_w;
    y_scale = (W3D_Float)amiga_height / (W3D_Float)new_h;
  } else {
    x_scale = y_scale = 1.0f;
  }
#endif
}

#define NOSWAP
#include "amidog/rect.h"

static void refresh_dummy(rect_t *rect, u8 *_dst, u8 *_src)
{
}

void rect_setup(rect_t *rect, u32 src_w, u32 src_h, u32 src_stride, u32 src_format, u32 dst_w, u32 dst_h, u32 dst_stride, u32 dst_format)
{
  s32 w, h, sx, sy, dx, dy, xstep, ystep;

  rect->src_w = src_w;
  rect->src_h = src_h;
  rect->src_stride = src_stride;
  rect->src_format = src_format;
  rect->dst_w = dst_w;
  rect->dst_h = dst_h;
  rect->dst_stride = dst_stride;
  rect->dst_format = dst_format;

  xstep = (1 << SCALEBITS);
  ystep = (1 << SCALEBITS);

  if ((src_w == 0) || (src_h == 0) || (dst_w == 0) || (dst_h == 0)) {
    w = h = sx = sy = dx = dy = xstep = ystep = 0;
  } else {
    if (scale_gfx) {
      src_w = dst_w;
      src_h = dst_h;
    }

    /* scaling */
    xstep = (rect->src_w * (1 << SCALEBITS)) / src_w;
    ystep = (rect->src_h * (1 << SCALEBITS)) / src_h;

    /* center output */
    if (dst_w < src_w) {
      w = dst_w;
      dx = 0;
      sx = (src_w - dst_w) / 2;
    } else {
      w = src_w;
      dx = (dst_w - src_w) / 2;
      sx = 0;
    }

    if (dst_h < src_h) {
      h = dst_h;
      dy = 0;
      sy = (src_h - dst_h) / 2;
    } else {
      h = src_h;
      dy = (dst_h - src_h) / 2;
      sy = 0;
    }
  }

  /* adjust source offsets */
  sx = (sx * xstep) >> SCALEBITS;
  sy = (sy * ystep) >> SCALEBITS;

  /* conversion adjustments */
  sx &= ~3;
  sy &= ~3;
  dx &= ~3;
  dy &= ~3;

  /* store settings */
  rect->w = w;
  rect->h = h;
  rect->sx = sx;
  rect->sy = sy;
  rect->dx = dx;
  rect->dy = dy;
  rect->xstep = xstep;
  rect->ystep = ystep;

  rect->refresh_func = rect_refresh_find(rect->src_format, rect->dst_format, (scale_gfx && ((amiga_use_screen) || (resolution > 0))) ? RECT_FLAG_SCALE : RECT_FLAG_NONE);
  if (rect->refresh_func == NULL) {
    rect->refresh_func = refresh_dummy;
  }
}

struct {
  u32 p96;
  u32 rect;
} p96torect[] = {
#ifdef __amigaos4__
  { RGBFB_R8G8B8, RECT_FORMAT_RGB24 }, /* TrueColor RGB (8 bit each) */
  { RGBFB_B8G8R8, RECT_FORMAT_BGR24 }, /* TrueColor BGR (8 bit each) */
  { RGBFB_R5G6B5PC, RECT_FORMAT_RGB565LE }, /* HiColor16 (5 bit R, 6 bit G, 5 bit B), format: gggbbbbbrrrrrggg */
  { RGBFB_R5G5B5PC, RECT_FORMAT_RGB555LE }, /* HiColor15 (5 bit each), format: gggbbbbb0rrrrrgg */
  { RGBFB_A8R8G8B8, RECT_FORMAT_ARGB32 }, /* 4 Byte TrueColor ARGB (A unused alpha channel) */
  { RGBFB_A8B8G8R8, RECT_FORMAT_ABGR32 }, /* 4 Byte TrueColor ABGR (A unused alpha channel) */
  { RGBFB_R8G8B8A8, RECT_FORMAT_RGBA32 }, /* 4 Byte TrueColor RGBA (A unused alpha channel) */
  { RGBFB_B8G8R8A8, RECT_FORMAT_BGRA32 }, /* 4 Byte TrueColor BGRA (A unused alpha channel) */
  { RGBFB_R5G6B5, RECT_FORMAT_RGB565BE }, /* HiColor16 (5 bit R, 6 bit G, 5 bit B), format: rrrrrggggggbbbbb */
  { RGBFB_R5G5B5, RECT_FORMAT_RGB555BE }, /* HiColor15 (5 bit each), format: 0rrrrrgggggbbbbb */
  { RGBFB_B5G6R5PC, RECT_FORMAT_BGR565LE }, /* HiColor16 (5 bit R, 6 bit G, 5 bit B), format: gggrrrrrbbbbbggg */
  { RGBFB_B5G5R5PC, RECT_FORMAT_BGR555LE }, /* HiColor15 (5 bit each), format: gggrrrrr0bbbbbbgg */
#else
  { PIXFMT_RGB24, RECT_FORMAT_RGB24 }, /* TrueColor RGB (8 bit each) */
  { PIXFMT_BGR24, RECT_FORMAT_BGR24 }, /* TrueColor BGR (8 bit each) */
  { PIXFMT_RGB16PC, RECT_FORMAT_RGB565LE }, /* HiColor16 (5 bit R, 6 bit G, 5 bit B), format: gggbbbbbrrrrrggg */
  { PIXFMT_RGB15PC, RECT_FORMAT_RGB555LE }, /* HiColor15 (5 bit each), format: gggbbbbb0rrrrrgg */
  { PIXFMT_ARGB32, RECT_FORMAT_ARGB32 }, /* 4 Byte TrueColor ARGB (A unused alpha channel) */
//  { PIXFMT_ABGR32, RECT_FORMAT_ABGR32 }, /* 4 Byte TrueColor ABGR (A unused alpha channel) */
  { PIXFMT_RGBA32, RECT_FORMAT_RGBA32 }, /* 4 Byte TrueColor RGBA (A unused alpha channel) */
  { PIXFMT_BGRA32, RECT_FORMAT_BGRA32 }, /* 4 Byte TrueColor BGRA (A unused alpha channel) */
  { PIXFMT_RGB16, RECT_FORMAT_RGB565BE }, /* HiColor16 (5 bit R, 6 bit G, 5 bit B), format: rrrrrggggggbbbbb */
  { PIXFMT_RGB15, RECT_FORMAT_RGB555BE }, /* HiColor15 (5 bit each), format: 0rrrrrgggggbbbbb */
  { PIXFMT_BGR16PC, RECT_FORMAT_BGR565LE }, /* HiColor16 (5 bit R, 6 bit G, 5 bit B), format: gggrrrrrbbbbbggg */
  { PIXFMT_BGR15PC, RECT_FORMAT_BGR555LE }, /* HiColor15 (5 bit each), format: gggrrrrr0bbbbbbgg */
#endif
  /* END */
  { 0, 0 },
};

unsigned long get_rect_pixfmt(unsigned long pixfmt)
{
  int i = 0;

  while (p96torect[i].p96 != 0) {
    if (p96torect[i].p96 == pixfmt) {
      return p96torect[i].rect;
    }
    i++;
  }

  return 0;
}

static rect_t rect;

void w3d_gpu_fill(int x0, int y0, int x1, int y1, unsigned long rgb)
{
  int x, y, tx, ty, w, h, width = MIN(x1 - x0, 1024), height = MIN(y1 - y0, 512);
  unsigned int pixel = ((rgb >> 3) & 31) | ((rgb >> 6) & 992) | ((rgb >> 9) & 31744);
  W3D_Float alpha = 1.0f;
  if (!context || !psxVuw || width <= 0 || height <= 0) return;
  FLUSH_ARRAY();
  InvalidateTextureArea(x0, y0, width, height);
  for (y = 0; y < height; ++y)
    for (x = 0; x < width; ++x)
      SWAP16_W(pixel, psxVuw + (((y0 + y) & 511) * 1024) + ((x0 + x) & 1023));
  if (scale_gfx == SCALING_EXP) OffsetSet();
  for (ty = 0; ty < height; ty += h) {
    int dy = (y0 + ty) & 511;
    h = MIN(height - ty, 512 - dy);
    for (tx = 0; tx < width; tx += w) {
      int dx = (x0 + tx) & 1023;
      int ox = scale_gfx == SCALING_EXP ? W3D_Offset.x : 0;
      int oy = scale_gfx == SCALING_EXP ? W3D_Offset.y : 0;
      w = MIN(width - tx, 1024 - dx);
      /* GP0(02) must ignore drawing-area scissor and semitransparency. */
      SETUP(W3D_PRIMITIVE_TRIANGLES, PRIM_FILLAREA)
      INDEX_6(0, 1, 2, 1, 2, 3)
      VERTEX(0, dx + ox,     dy + oy,     rgb);
      VERTEX(1, dx + ox + w, dy + oy,     rgb);
      VERTEX(2, dx + ox,     dy + oy + h, rgb);
      VERTEX(3, dx + ox + w, dy + oy + h, rgb);
      DRAW(4)
    }
  }
}

void OffsetSet(void)
{
  int ox, oy, left, right, top, bottom, width, height, empty;
  if (!context) return;
  /* Use the same framebuffer origin for both vertices and the scissor.
   * The old fallback moved only the scissor, leaving vertices offscreen. */
  ox = -PreviousPSXDisplay.DisplayPosition.x + PreviousPSXDisplay.Range.x0;
  oy = -PreviousPSXDisplay.DisplayPosition.y + PreviousPSXDisplay.Range.y0;
  if (drawW + ox < 0 || drawX + ox >= PSXDisplay.DisplayMode.x)
    ox = -PSXDisplay.DisplayPosition.x + PreviousPSXDisplay.Range.x0;
  if (drawH + oy < 0 || drawY + oy >= PSXDisplay.DisplayMode.y)
    oy = -PSXDisplay.DisplayPosition.y + PreviousPSXDisplay.Range.y0;
  left = CLAMP((int)((drawX + ox) * x_scale), 0, amiga_width);
  right = CLAMP((int)((drawW + ox + 1) * x_scale), 0, amiga_width);
  top = CLAMP((int)((drawY + oy) * y_scale), 0, amiga_height);
  bottom = CLAMP((int)((drawH + oy + 1) * y_scale), 0, amiga_height);
  width = MAX(0, right - left); height = MAX(0, bottom - top);
  empty = width == 0 || height == 0;
  if (W3D_Offset.x != ox || W3D_Offset.y != oy ||
      scissor.left != left || scissor.top != top ||
      scissor.width != width || scissor.height != height || scissor_empty != empty) {
    FLUSH_ARRAY();
    W3D_Offset.x = ox; W3D_Offset.y = oy;
    scissor.left = left; scissor.top = top;
    scissor.width = width; scissor.height = height;
    scissor_empty = empty;
    if (!empty) W3D_SetScissor(context, &scissor);
  }
  OffsetNotSet = 0;
}

void amiga_refresh(unsigned short *buf, int xpos, int ypos, int width, int height, int is_24bit, double rate)
{
  int disp_x, disp_y;

  FLUSH_ARRAY(); // time to display frame

  W3D_FlushFrame(context); // always flush the context
  /* The following graphics.library blits read the render target outside
   * Warp3D. Submission alone does not guarantee that it is ready. */
  W3D_WaitIdle(context);

  OffsetNotSet = 1;

  disp_x = xpos;
  disp_y = ypos;

  Amiga_FrameRateHz = rate;

  /* The visible display range, not just GP1(08), defines an Actual window.
   * Menus and gameplay may share a video mode but use different widths. */
  width = w3d_frame_extent(width, new_w, (!amiga_use_screen && resolution == 0) ? amiga_width : 1024);
  height = w3d_frame_extent(height, new_h, (!amiga_use_screen && resolution == 0) ? amiga_height : 512);

  if (((!amiga_use_screen && resolution == 0) && (width != cur_w || height != cur_h)) ||
      ((amiga_use_screen || resolution > 0) && (new_w != cur_w || new_h != cur_h))) {
    cur_w = (!amiga_use_screen && resolution == 0) ? width : new_w;
    cur_h = (!amiga_use_screen && resolution == 0) ? height : new_h;
    amiga_clear();

    if ((!amiga_use_screen) && (resolution == 0)) {
      ChangeWindowBox(window, window->LeftEdge, window->TopEdge,
                              window->BorderLeft+cur_w+window->BorderRight,
                              window->BorderTop+cur_h+window->BorderBottom);
    }
  }

  if (is_24bit) {
#ifdef __amigaos4__
    if ((lock = p96LockBitMap(window_bitmap, (UBYTE *)&ri, sizeof(ri)))) {
#else
    if ((lock = LockBitMapTags(window_bitmap, LBMI_BASEADDRESS, (ULONG)&baseaddress, TAG_END))) {
#endif
      unsigned char *src;
      src = (unsigned char *)&psxVuw[(disp_y * 1024) + disp_x];
      if ((amiga_use_screen) || (resolution > 0)) {
        rect_setup(&rect, width, height, 2048, RECT_FORMAT_BGR24, amiga_width, amiga_height, bpr, rect_pixfmt);
      } else {
        rect_setup(&rect, width, height, 2048, RECT_FORMAT_BGR24, width, height, bpr, rect_pixfmt);
      }
#ifdef __amigaos4__
      rect.refresh_func(&rect, (UBYTE *)ri.Memory, src);
      p96UnlockBitMap(window_bitmap, lock);
#else
      rect.refresh_func(&rect, (UBYTE *)baseaddress, src);
      UnLockBitMap(lock);
#endif
    }
  } else {
    if (0 /* SLOW! */ && scale_gfx && ((amiga_use_screen) || (resolution > 0))) {
      struct BitScaleArgs bsa;
      memset(&bsa, 0, sizeof(bsa));
      bsa.bsa_SrcX = disp_x;
      bsa.bsa_SrcY = disp_y;
      bsa.bsa_SrcBitMap = w3d_bitmap;
      bsa.bsa_SrcWidth = width;
      bsa.bsa_SrcHeight = height;
      bsa.bsa_XSrcFactor = width;
      bsa.bsa_YSrcFactor = height;
      bsa.bsa_DestX = 0;
      bsa.bsa_DestY = 0;
      bsa.bsa_DestWidth = amiga_width;
      bsa.bsa_DestHeight = amiga_height;
      bsa.bsa_XDestFactor = amiga_width;
      bsa.bsa_YDestFactor = amiga_height;
      bsa.bsa_DestBitMap = window_bitmap;
      BitMapScale(&bsa);
    } else {
      if (scale_gfx == SCALING_EXP) {
        BltBitMap(w3d_bitmap, 0, 0, window_bitmap, 0, 0, amiga_width, amiga_height, 0xc0, 0xff, 0);
      } else {
        int bak_scale_gfx = scale_gfx;
        scale_gfx = 0; /* temporary disable scaling */
        if ((amiga_use_screen) || (resolution > 0)) {
          rect_setup(&rect, width, height, bpr, rect_pixfmt, amiga_width, amiga_height, bpr, rect_pixfmt);
        } else {
          rect_setup(&rect, width, height, bpr, rect_pixfmt, width, height, bpr, rect_pixfmt);
        }
        BltBitMap(w3d_bitmap, rect.sx + disp_x, rect.sy + disp_y, window_bitmap, rect.dx, rect.dy, rect.w, rect.h, 0xc0, 0xff, 0);
        scale_gfx = bak_scale_gfx;
      }
    }
  }

  if (show_fps) {
    amidog_timeval_t tvf, dt;
    double titi;
    int x, fps0, fps1;
    char *cptr = text_lines[0].text;

    timer_gettime(timer, &tvf);

    dt = tvf;
    timer_subtime(timer, &dt, &tvi);
    titi = dt.sec + dt.usec / 1000000.0;

    if (titi > 2.0) {
      tvi = tvf;
      fps0 = (int)(((double)frame_count / titi) + 0.5);
      fps1 = (int)(((double)nf / titi) + 0.5);
      sprintf(cptr, "%05d %05d", fps0, fps1); cptr += (5 + 1 + 5);
      nf = frame_count = 0;
    } else {
      cptr += (5 + 1 + 5);
    }

#ifdef W3D_VERBOSE

    *cptr++ = ' ';
    sprintf(cptr, "%05d ", W3D_VERBOSE_LINE); cptr += 6;
    sprintf(cptr, "%05d ", W3D_VERBOSE_POLY_SMP); cptr += 6;
    sprintf(cptr, "%05d ", W3D_VERBOSE_POLY_TEX); cptr += 6;
    sprintf(cptr, "%05d ", W3D_VERBOSE_LINE + W3D_VERBOSE_POLY_SMP + W3D_VERBOSE_POLY_TEX); cptr += 6;

    for (x=0; x<TOTAL_BLENDMODES; x++) {
      *cptr++ = '0' + ((used_blendmodes[x] > 9) ? 9 : used_blendmodes[x]);
    }

    sprintf(cptr, " %02x", W3D_Flags & 0xff); cptr += 3;
#endif

  } else {
    text_lines[0].text[0] = 0;
    text_lines[1].text[0] = 0;
  }

  TCacheEndOfFrame(show_fps);

  if (usCursorActive) {
#ifdef __amigaos4__
    if ((lock = p96LockBitMap(window_bitmap, (UBYTE *)&ri, sizeof(ri)))) {
#else
    if ((lock = LockBitMapTags(window_bitmap, LBMI_BASEADDRESS, (ULONG)&baseaddress, TAG_END))) {
#endif
      int i,j,x,y,dx,dy,iPlayer,sx,ex,sy,ey,px,py;
      unsigned char *surf;

      if ((amiga_use_screen) || (resolution > 0)) {
        dx = amiga_width;
        dy = amiga_height;
      } else {
        dx = cur_w;
        dy = cur_h;
      }

      px = (dx / 320); px = (px < 1) ? 1 : px;
      py = (dy / 240); py = (py < 1) ? 1 : py;

#ifdef __amigaos4__
      surf = (UBYTE *)ri.Memory;
#else
      surf = (UBYTE *)baseaddress;
#endif
      for(iPlayer=0;iPlayer<8;iPlayer++) {
        if (usCursorActive&(1<<iPlayer)) {
          int ty=(ptCursorPoint[iPlayer].y*dy)/256;
          int tx=(ptCursorPoint[iPlayer].x*dx)/512;
          sx=tx-(5*px);if(sx<0) {if(sx&1) sx=1; else sx=0;}
          sy=ty-(5*py);if(sy<0) {if(sy&1) sy=1; else sy=0;}
          ex=tx+(6*px);if(ex>dx) ex=dx;
          ey=ty+(6*px);if(ey>dy) ey=dy;

          #define PIXEL() \
          for (i=0;i<py;i++) { \
            for (j=0;j<px*bpp;j++) { \
              surf[((y+i)*bpr)+(x*bpp)+j]=0xff; \
            } \
          }

          for(x=tx,y=sy;y<ey;y+=2*py) {
            PIXEL()
          }
          for(y=ty,x=sx;x<ex;x+=2*px) {
            PIXEL()
          }
        }
      }
#ifdef __amigaos4__
      p96UnlockBitMap(window_bitmap, lock);
#else
      UnLockBitMap(lock);
#endif
    }
  }

  /* Render any text */
  print_text_lines();

  if ((amiga_use_screen) || (resolution > 0)) {
    BltBitMapRastPort(window_bitmap, 0, 0, window->RPort, 0, 0, amiga_width, amiga_height, 0xc0);
  } else {
    BltBitMapRastPort(window_bitmap, 0, 0, window->RPort, 0, 0, width, height, 0xc0);
  }

#ifdef W3D_VERBOSE
  memset(used_blendmodes, 0, sizeof(used_blendmodes));
  memset(poly_count, 0, sizeof(poly_count));
#endif
}

static int w3d_open(void)
{
  ULONG flags, w3derror;
  int i;

  array_count = prim_count = 0;
  memset(array_unit_tex, 0, sizeof(array_unit_tex));
  current_blend_mode = -1;
  W3D_Flags = 0;
  scissor_empty = 0;
  draw_errors = 0;
  OffsetNotSet = 1;
  x_scale = y_scale = 1.0f;

  refvalue = ((W3D_Float)alpha_threshold) / 100.0;

  flags = W3D_CheckDriver();
  if (flags & W3D_DRIVER_3DHW) printf("Hardware driver available\n");
  if (flags & W3D_DRIVER_CPU)  printf("Software driver available\n");
  if (flags & W3D_DRIVER_UNAVAILABLE) {
     printf("PANIC: no driver available!!!\n");
     return FPSE_ERR;
  }

  if (scale_gfx == SCALING_EXP) {
    w3d_bitmap = AllocBitMap(amiga_width, amiga_height, 16,
                             BMF_CLEAR|BMF_DISPLAYABLE|BMF_INTERLEAVED|BMF_MINPLANES,
                             window->RPort->BitMap);
  } else {
    w3d_bitmap = AllocBitMap(1024, 512, 16,
                             BMF_CLEAR|BMF_DISPLAYABLE|BMF_INTERLEAVED|BMF_MINPLANES,
                             window->RPort->BitMap);
  }

  if (w3d_bitmap == NULL) {
    return FPSE_ERR;
  }

  if (dispid == (unsigned long)INVALID_ID) {
    context = W3D_CreateContextTags(&w3derror,
                    W3D_CC_BITMAP, (ULONG)w3d_bitmap,
                    W3D_CC_YOFFSET, 0,
                    W3D_CC_DRIVERTYPE, W3D_DRIVER_BEST,
                    W3D_CC_DOUBLEHEIGHT, W3D_FALSE,
                    W3D_CC_FAST, W3D_TRUE,
                    TAG_DONE);
  } else {
    context = W3D_CreateContextTags(&w3derror,
                    W3D_CC_MODEID, dispid, // Mandatory
                    W3D_CC_BITMAP, (ULONG)w3d_bitmap,
                    W3D_CC_YOFFSET, 0,
                    W3D_CC_DRIVERTYPE, W3D_DRIVER_BEST,
                    W3D_CC_DOUBLEHEIGHT, W3D_FALSE,
                    W3D_CC_FAST, W3D_TRUE,
                    TAG_DONE);
  }

  if ((!context) || (w3derror != W3D_SUCCESS)) {
       printf("Error creating context. Reason:");
       switch(w3derror) {
       case W3D_ILLEGALINPUT:
            printf("Illegal input to CreateContext function\n");
            break;
       case W3D_NOMEMORY:
            printf("Out of memory\n");
            break;
       case W3D_NODRIVER:
            printf("No suitable driver found\n");
            break;
       case W3D_UNSUPPORTEDFMT:
            printf("Supplied bitmap cannot be handled by Warp3D\n");
            break;
       case W3D_ILLEGALBITMAP:
            printf("Supplied bitmap not properly initialized\n");
            break;
       default:
            printf("An error has occured... gosh\n");
       }
       return FPSE_ERR;
  }

  for (i=0; i<MAX_TEX_UNITS; i++) {
    array_unit_color[i].a = 1.0;
    array_unit_color[i].r = 1.0;
    array_unit_color[i].g = 1.0;
    array_unit_color[i].b = 1.0;
  }

  array_unit_color[0].r = 0.25;
  array_unit_color[0].g = 0.25;
  array_unit_color[0].b = 0.25;

#ifdef __amigaos4__
  int combine = W3D_Query(context, W3D_Q_ENV_COMBINE, 0);
  int units = W3D_Query(context, W3D_Q_NUM_TMU, 0);
  int blends = W3D_Query(context, W3D_Q_NUM_BLEND, 0);

  if (combine == W3D_FULLY_SUPPORTED) {
    W3D_Flags |= FLAG_MULTITEXT;
    if ((units >= 3) && (blends >= 3)) {
      // we can do mode 3 without special texture
      W3D_Flags |= FLAG_FASTMULTI;
    } else if ((units >= 2) && (blends >= 2)) {
      // we can do mode 3 with special texture
      W3D_Flags |= FLAG_FASTMULTI;
      W3D_Flags |= FLAG_FASTMODE3;
    } else {
      // no fastmode possible
    }
  }
#endif

//  W3D_Hint(context, W3D_H_TEXMAPPING, W3D_H_NICE);
//  W3D_Hint(context, W3D_H_BILINEARFILTER, W3D_H_NICE);
//  W3D_Hint(context, W3D_H_BLENDING, W3D_H_NICE);
//  W3D_Hint(context, W3D_H_DITHERING, W3D_H_NICE);

  W3D_SetState(context, W3D_TEXMAPPING, W3D_DISABLE);
  if (W3D_Flags & FLAG_MULTITEXT) {
#ifdef __amigaos4__
    W3D_SetState(context, W3D_MULTITEXTURE, W3D_ENABLE);
#endif
  }

  W3D_SetState(context, W3D_GLOBALTEXENV, W3D_ENABLE);
  W3D_SetState(context, W3D_GOURAUD, W3D_ENABLE);
  W3D_SetState(context, W3D_ZBUFFER, W3D_DISABLE);
  W3D_SetState(context, W3D_ZBUFFERUPDATE, W3D_DISABLE);
  W3D_SetState(context, W3D_CULLFACE, W3D_DISABLE);
//  W3D_SetState(context, W3D_DITHERING, W3D_ENABLE);
  W3D_SetState(context, W3D_SCISSOR, W3D_ENABLE);
  W3D_SetState(context, W3D_AUTOTEXMANAGEMENT, W3D_ENABLE);

  if (scale_gfx == SCALING_EXP) {
    scissor.left   = 0;
    scissor.top    = 0;
    scissor.width  = amiga_width;
    scissor.height = amiga_height;
  } else {
    scissor.left   = 0;
    scissor.top    = 0;
    scissor.width  = 1024;
    scissor.height = 512;
  }
  W3D_SetScissor(context, &scissor);

  /* This causes color 0 to be transparent */
  W3D_SetAlphaMode(context, W3D_A_GREATER, &refvalue);
  W3D_SetState(context, W3D_ALPHATEST, W3D_DISABLE);

  W3D_SetState(context, W3D_BLENDING, W3D_ENABLE);

#ifdef __amigaos4__
  W3D_InterleavedArray(context, array, sizeof(MyVertex), MYVERTEX_FORMAT, W3D_TEXCOORD_NORMALIZED);
#else
  W3D_VertexPointer(context, &array[0].x, sizeof(MyVertex), W3D_VERTEX_F_F_F, 0);
  W3D_ColorPointer(context, &array[0].primary_r, sizeof(MyVertex), W3D_COLOR_FLOAT, W3D_CMODE_RGBA, 0);
  W3D_TexCoordPointer(context, &array[0].u, sizeof(MyVertex), 0, 1*sizeof(W3D_Float), 2*sizeof(W3D_Float), W3D_TEXCOORD_NORMALIZED);
#endif

#ifdef __amigaos4__
  W3D_Color color;
  color.r = 0;
  color.g = 0;
  color.b = 0;
  color.a = 0;
  LOCK();
  W3D_ClearBuffers(context, &color, NULL, NULL);
  UNLOCK();
#else
  LOCK();
  W3D_ClearDrawRegion(context, 0);
  UNLOCK();
#endif

  if (TCacheInit() != FPSE_OK) {
    return FPSE_ERR;
  }

  return FPSE_OK;
}

void amiga_close()
{
  w3d_close();

  if (window != NULL) {
    if (hide_pointer) {
      ClearPointer(window);
    }
    CloseWindow(window);
    window = NULL;
  }
  if (screen != NULL) {
    CloseScreen(screen);
    screen = NULL;
  }
  if (window_bitmap != NULL) {
    FreeBitMap(window_bitmap);
    window_bitmap = NULL;
  }
  if (timer != NULL) {
    timer_exit(timer);
    timer = NULL;
  }
}

int amiga_open(void)
{
  dispid = screenmode;
  timer = timer_init();
  if (!timer) return FPSE_ERR;
  frame_skip = CLAMP(frame_skip, 1, 10);
  scale_gfx = CLAMP(scale_gfx, SCALING_NONE, SCALING_EXP);
  new_w = DEF_W; new_h = DEF_H;
  cur_w = cur_h = 0; /* force first-frame size/clear even in the default mode */

  memset(text_lines, 0, sizeof(text_lines));

  if ((resolution < 0) || (resolution > MAX_RESOLUTION)) {
    resolution = 0;
  }

  if (amiga_use_screen) {
#ifdef __amigaos4__
    if (resolution > 0) {
      unsigned long cmodels = RGBFF_R5G5B5 | RGBFF_R5G6B5 | RGBFF_R5G5B5PC | RGBFF_R5G6B5PC;
      dispid=p96BestModeIDTags(P96BIDTAG_NominalWidth, resolutions[resolution].w, P96BIDTAG_NominalHeight, resolutions[resolution].h, P96BIDTAG_FormatsAllowed, cmodels, TAG_DONE);
      if (dispid == INVALID_ID) {
        amiga_use_screen = 0;
      }
    } else {
      if (p96GetModeIDAttr(dispid, P96IDA_DEPTH) < 15) {
        dispid = p96RequestModeIDTags(P96MA_FormatsAllowed, RGBFF_HICOLOR | RGBFF_TRUECOLOR | RGBFF_TRUEALPHA, TAG_DONE);
        if (dispid == INVALID_ID) {
          amiga_use_screen = 0;
        }
      }
    }
#else
    if (resolution > 0) {
      dispid = BestCModeIDTags(CYBRBIDTG_NominalWidth, resolutions[resolution].w,
                               CYBRBIDTG_NominalHeight, resolutions[resolution].h,
                               CYBRBIDTG_Depth, 16, TAG_DONE);
      if (dispid == 0) {
        amiga_use_screen = 0;
      }
    } else {
      if (GetCyberIDAttr(CYBRIDATTR_DEPTH, dispid) < 15) {
#ifdef __AROS__
        dispid = 0;
#else
        UWORD cmodels[] = { PIXFMT_RGB15, PIXFMT_RGB16, PIXFMT_RGB15PC, PIXFMT_RGB16PC, -1 };
        dispid = CModeRequestTags(NULL, CYBRMREQ_CModelArray, (ULONG)cmodels, TAG_DONE);
#endif
        if (dispid == 0) {
          amiga_use_screen = 0;
        }
      }
    }
#endif
  }

  if (amiga_use_screen) {
#ifdef __amigaos4__
    amiga_width = p96GetModeIDAttr(dispid, P96IDA_WIDTH);
    amiga_height = p96GetModeIDAttr(dispid, P96IDA_HEIGHT);
#else
    amiga_width = GetCyberIDAttr(CYBRIDATTR_WIDTH, dispid);
    amiga_height = GetCyberIDAttr(CYBRIDATTR_HEIGHT, dispid);
#endif

    /* Open screen */
    screen = OpenScreenTags(NULL,
             SA_Width,amiga_width,
             SA_Height,amiga_height,
             SA_Depth,8,
             SA_Quiet,TRUE,
             SA_ShowTitle,FALSE,
             SA_Type,CUSTOMSCREEN,
             SA_DisplayID,dispid,
             TAG_DONE);

    /* Could the screen be opened? */
    if (screen == NULL) {
      printf("Could NOT open screen!\n");
      return FPSE_ERR;
    }

    /* Open window */
    window = OpenWindowTags(NULL,
             WA_CustomScreen,(int)screen,
             WA_Width,screen->Width,
             WA_Height,screen->Height,
             WA_Flags,WFLG_RMBTRAP,
             WA_IDCMP,IDCMP_RAWKEY,
             WA_Backdrop,TRUE,
             WA_Borderless,TRUE,
             WA_Activate,TRUE,
             TAG_DONE);

    /* Could the window be opened? */
    if (window == NULL) {
      printf("Could NOT open window!\n");
      return FPSE_ERR;
    }
  } else {
    struct Screen *pub_screen = LockPubScreen(NULL);
    int x = 100, y = 100;
    dispid = INVALID_ID;

    /* position window */
    if ((pub_screen != NULL) && (resolution > 0))
    {
      int pub_width = pub_screen->Width, pub_height = pub_screen->Height;
      int bw = pub_screen->WBorLeft + pub_screen->WBorRight;
      int bh = pub_screen->WBorTop + pub_screen->Font->ta_YSize + 1 + pub_screen->WBorBottom;

      y = (pub_height - (resolutions[resolution].h + bh)) / 2;

      if (plugin_is_master)
      {
        x = ((pub_width / 2) - (resolutions[resolution].w + bw)) / 2;
      }
      else if (plugin_is_slave)
      {
        x = ((pub_width / 2) - (resolutions[resolution].w + bw)) / 2;
        x += (pub_width / 2);
      }
      else
      {
        x = (pub_width - (resolutions[resolution].w + bw)) / 2;
      }
    }

    window = OpenWindowTags(NULL,
                            WA_Title, (ULONG)"FPSE - *FREE* PlayStation Emulator",
                            WA_Flags, WFLG_NOCAREREFRESH|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_RMBTRAP|WFLG_GIMMEZEROZERO,
                            WA_IDCMP, IDCMP_RAWKEY|IDCMP_CHANGEWINDOW,
                            WA_Left, x,
                            WA_Top, y,
                            WA_InnerWidth, (resolution > 0) ? resolutions[resolution].w : DEF_W,
                            WA_InnerHeight, (resolution > 0) ? resolutions[resolution].h : DEF_H,
                            WA_MinWidth, 64, WA_MinHeight, 64,
                            WA_MaxWidth, -1, WA_MaxHeight, -1,
                            WA_Activate, TRUE,
                            TAG_DONE);

    if (pub_screen) UnlockPubScreen(NULL, pub_screen);

    if (window == NULL) {
      return FPSE_ERR;
    }

    amiga_width = resolutions[resolution].w;
    amiga_height = resolutions[resolution].h;
    if (resolution == 0) amiga_height = 512; /* capacity, not live window size */
  }

  if ((amiga_use_screen) || (resolution > 0)) {
  } else {
    scale_gfx = SCALING_NONE; // Scaling makes no sense here
  }

  window_bitmap = AllocBitMap(amiga_width, amiga_height, 16,
                              BMF_CLEAR|BMF_DISPLAYABLE|BMF_INTERLEAVED|BMF_MINPLANES,
                              window->RPort->BitMap);

  if (window_bitmap == NULL) {
    printf("Could NOT allocate bitmap!\n");
    return FPSE_ERR;
  }

#ifdef __amigaos4__
  pixfmt = p96GetBitMapAttr(window_bitmap, P96BMA_RGBFORMAT);
  bpr = p96GetBitMapAttr(window_bitmap, P96BMA_BYTESPERROW);
  bpp = p96GetBitMapAttr(window_bitmap, P96BMA_BYTESPERPIXEL);
#else
  UnLockBitMap(LockBitMapTags(window_bitmap,
    LBMI_PIXFMT,      (ULONG)&pixfmt,
    LBMI_BYTESPERROW, (ULONG)&bpr,
    LBMI_BYTESPERPIX, (ULONG)&bpp,
  TAG_END));
#endif
  rect_pixfmt = get_rect_pixfmt(pixfmt);

  if (hide_pointer) {
    SetPointer(window, empty_pointer, 2, 16, 0, 0);
  }

  if (w3d_open() != FPSE_OK) {
    return FPSE_ERR;
  }

  amiga_clear();

  timer_gettime(timer, &wt);
  timer_gettime(timer, &tvi);

  printf("GPUW3D repair2: Warp3D %u.%u, chip=%lu, scale=%d, output=%dx%d, cache=%d, alpha=%d\n",
         Warp3DBase->lib_Version, Warp3DBase->lib_Revision, (ULONG)context->CurrentChip,
         scale_gfx, amiga_width, amiga_height, texcache, alpha_threshold);

  return FPSE_OK;
}

void amiga_clipping(void)
{
  int left, top, right, bottom, width, height, empty;
  if (!context) return;
  if (scale_gfx == SCALING_EXP) {
    OffsetSet();
    return;
  }
  left = CLAMP(drawX, 0, 1024);
  top = CLAMP(drawY, 0, 512);
  right = CLAMP(drawW + 1, 0, 1024);
  bottom = CLAMP(drawH + 1, 0, 512);
  width = MAX(0, right - left); height = MAX(0, bottom - top);
  empty = width == 0 || height == 0;
  if (scissor.left != left || scissor.top != top || scissor.width != width ||
      scissor.height != height || scissor_empty != empty) {
    FLUSH_ARRAY();
    scissor.left = left; scissor.top = top;
    scissor.width = width; scissor.height = height;
    scissor_empty = empty;
    if (!empty) W3D_SetScissor(context, &scissor);
  }
}

/* Blending and alpha handling */

typedef struct {
  int src;
  int dst;
  W3D_Float alpha;
} ts_type;

/* Semi Transparent (DST = pixel in frame buffer, SRC = half transparent pixel):
 * 0 : DST = 0.50 x DST + 0.50 x SRC
 * 1 : DST = 1.00 x DST + 1.00 x SRC
 * 2 : DST = 1.00 x DST - 1.00 x SRC
 * 3 : DST = 1.00 x DST + 0.25 x SRC
 *
 * Not Transparent:
 * 4 : DST = 0.00 x DST + 1.00 x SRC
 *
 * 0, 1, 3, 4 = OK
 */

ts_type ts[TOTAL_BLENDMODES] = {
  {W3D_SRC_ALPHA, W3D_ONE_MINUS_SRC_ALPHA, 0.5},
  {W3D_ONE,       W3D_ONE, 1.0},
  {W3D_ZERO,      W3D_ONE_MINUS_SRC_COLOR, 1.0}, /* approximate D-S by D*(1-S) */
  {W3D_SRC_ALPHA, W3D_ONE, 0.25},

  {W3D_SRC_ALPHA, W3D_ONE_MINUS_SRC_ALPHA, 0.5},
  {W3D_ONE,       W3D_ONE_MINUS_SRC_ALPHA, 1.0},
  {W3D_ONE,       W3D_ZERO, 1.0},
  {W3D_ONE,       W3D_ONE_MINUS_SRC_ALPHA, 1.0},

  {W3D_ONE,       W3D_ZERO, 1.0},
};

W3D_Float set_blend_mode(int *opaque_pass)
{
  int blend_mode, opaque = (opaque_pass != NULL) ? *opaque_pass : 0;

  if ((DrawSemiTrans) && (opaque != 2)) { /* Semi Transparency */
    blend_mode = GlobalTextABR;

    if (opaque == 1) {
      *opaque_pass = 2;
      if (W3D_Flags & FLAG_FASTMULTI) {
        if (ts[blend_mode+4].alpha != 0.0) {
          blend_mode += 4;
          *opaque_pass = 0; /* single pass is possible */
        }
      }
    }
  } else {
    blend_mode = 4+4;
  }

#ifdef W3D_VERBOSE
  used_blendmodes[blend_mode]++;
#endif

  if (context->CurrentChip == W3D_CHIP_PERMEDIA2) {
    /* Once additive blending has been enabled, it seems more or less
     * impossible to disable for the Permedia 2, so never enable it.
     */
    switch (blend_mode) {
      case 0:
      case 2:
      case 4+4:
        /* these we can do */
        break;

      default:
        blend_mode = 0;
        break;
    }
  }

  if (blend_mode != current_blend_mode) {
    FLUSH_ARRAY(); // blend mode has changed

    if (blend_mode == 2) {
      // Set reverse subtractive blending
    } else if (current_blend_mode == 2) {
      // Unset reverse subtractive blending
    }

    W3D_SetBlendMode(context, ts[blend_mode].src, ts[blend_mode].dst);

    current_blend_mode = blend_mode;
  }

  return ts[blend_mode].alpha;
}

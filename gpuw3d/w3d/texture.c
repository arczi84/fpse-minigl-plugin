/*
 * texture.c
 */

/* Texture cache functions */

#include "w3d.h"
#include "texture.h"
#include "repair.h"

#define NOSWAP
#include "amidog/rect.h"

/* External */
extern struct BitMap *w3d_bitmap;
extern unsigned long pixfmt, rect_pixfmt;
extern int bpp;
void rect_setup(rect_t *rect, u32 src_w, u32 src_h, u32 src_stride, u32 src_format, u32 dst_w, u32 dst_h, u32 dst_stride, u32 dst_format);
extern int amiga_width, amiga_height;

/* Global */
int texred = 0, texcache = 256, texfilter = 0;

/* Verbose */
#ifdef W3D_VERBOSE
static int used_textures = 0, replaced_textures = 0;
static int opaque_passes = 0;

#define TEX_VERBOSE(_tex_) \
  if ((_tex_)->counter == 0) { /* first use */ \
    used_textures++; \
  } \
  if (DrawSemiTrans && (_tex_)->opaque_required) { \
    opaque_passes++; \
  }
#else
#define TEX_VERBOSE(_tex_)
#endif

/* Private */
#ifdef __amigaos4__
static struct RenderInfo ri;
static ULONG lock;
#else
static int bpr;
static APTR baseaddress;
static APTR lock;
#endif
static rect_t rect;

static int tex_downscale = 1; /* 1, 2 or 4 (256x256, 128x128, 64x64) */

#define TEX_S (tex_downscale)  /* scale, i.e. width / scale x height / scale */
#define TEX_W (256 / TEX_S) /* width */
#define TEX_H (256 / TEX_S) /* height */

static int tex_cache_max = 256;

#define CACHE_MAX (tex_cache_max)

/* Texture cache variables */

// User-defined types
typedef struct {
  // identifier
  int mode;
  int clY, clX;
  int GlobalTextAddrY, GlobalTextAddrX;
  int opaque;
  int bUsingTWin, x0, x1, y0, y1;

  // data
  UINT16 *Texture;
  W3D_Texture *w3d_tex;
  int counter;
  UINT16 ctbl[256];

  // flags
  int opaque_required;
} TextureCache;

static UINT16 rgbtbl_transparent[65536]; /* bgr -> rgb */
static UINT16 rgbtbl_opaque[65536];      /* bgr -> rgb */
static TextureCache *tcache = NULL;

int TCacheInit(void)
{
  static int texscale[] = { 1, 2, 4 };

  ULONG w3derror;
  int a, i, j, k;

  if ((texred < 0) || (texred > 2)) {
    texred = 0;
  }
  tex_downscale = texscale[texred];

  if ((texcache < 64) || (texcache > 256)) {
    texcache = 256;
  }
  tex_cache_max = texcache;

  /* When rendering a texture, the PSX GPU use the MSB of each pixel in the
   * texture to decide if that specific pixel should be rendered opaque (0)
   * or semitransparent (1).
   *
   *             transparency proccessing (bit 1 of command packet)
   * BGR    STP     off            on
   * 0,0,0   0   Transparent   Transparent
   * 0,0,0   1   Opaque        Semi-Transparent
   * x,x,x   0   Opaque        Opaque
   * x,x,x   1   Opaque        Semi-Transparent
   */

  for (i=0; i<65536; i++) {
    j = SWAP16(i);
    a = (j >> 15) & 1; /* alpha */

    /* BGR to RGB */
    j = ((j & 0x7c00) >> 10) | ((j & 0x001f) << 10) | (j & 0x03e0);
    j |= 0x8000; /* set alpha */

    if (W3D_Flags & FLAG_FASTMODE3) {
      /* 1.00% (31) and 0.25% (8) to scale colors for mode 3, all others only use alpha */
      k = (a == 0) ? (1<<15)|(31<<10)|(31<<5)|(31<<0) : (0<<15)|(8<<10)|(8<<5)|(8<<0);
    } else {
      k = (a == 0) ? j : 0;
    }

#ifdef LSB_FIRST
    j = ((j << 8) & 0xff00) | ((j >> 8) & 0x00ff);
    k = ((k << 8) & 0xff00) | ((k >> 8) & 0x00ff);
#endif

    rgbtbl_opaque[i] = k;
    rgbtbl_transparent[i] = j;
  }

  /* Color 0 is always transparent. */
  rgbtbl_opaque[0]      = 0;
  rgbtbl_transparent[0] = 0;

#if 0
  // Enable this to disable opaque passes.
  memset(rgbtbl_opaque, 0, sizeof(rgbtbl_opaque));
#endif

  tcache = malloc(CACHE_MAX * sizeof(TextureCache));
  if (tcache == NULL) {
    return FPSE_ERR;
  }
  memset(tcache, 0, (CACHE_MAX * sizeof(TextureCache)));

  for (i=0; i<CACHE_MAX; i++) {
    if (i==0) {
      tcache[i].Texture = (UINT16 *)malloc(TEX_W*TEX_H*CACHE_MAX*sizeof(UINT16));
      if (tcache[i].Texture == NULL) {
        return FPSE_ERR;
      }
    } else {
      tcache[i].Texture = tcache[0].Texture + TEX_W*TEX_H*i;
    }

    tcache[i].w3d_tex = W3D_AllocTexObjTags(context, &w3derror,
                            W3D_ATO_IMAGE, (ULONG)tcache[i].Texture,
                            W3D_ATO_FORMAT, W3D_A1R5G5B5,
                            W3D_ATO_WIDTH, TEX_W,
                            W3D_ATO_HEIGHT, TEX_H,
                            TAG_DONE);
    if (tcache[i].w3d_tex == NULL) {
      printf("Failed to allocate texture %d\n", i);
      return FPSE_ERR;
    }

    tcache[i].counter = 0;
  }

  TCacheInvalidate();

  return FPSE_OK;
}

void TCacheInvalidate(void)
{
  TextureCache *tc;
  int i;
  if (!tcache) return;
  FLUSH_ARRAY();

  // flush cache
  for(i=0,tc=tcache; i<CACHE_MAX; i++,tc++) {
    tc->mode = -1;
  }
}

void TCacheEndOfFrame(int verbose)
{
  TextureCache *tc;
  int i;

#ifdef W3D_VERBOSE
  if (verbose) {
    char *cptr = text_lines[1].text;

    sprintf(cptr, "%05d ", used_textures); cptr += 6;
    sprintf(cptr, "%05d ", replaced_textures); cptr += 6;
    sprintf(cptr, "%05d", opaque_passes);

    used_textures = 0;
    replaced_textures = 0;
    opaque_passes = 0;
  }
#endif

  // reset texture counter
  for(i=0, tc = tcache;i<CACHE_MAX;i++, tc++) {
    tc->counter = 0;
  }

  used_textures = 0;
  replaced_textures = 0;
}

void TCacheExit(void)
{
  int i;

  if (tcache != NULL) {
    for (i=0; i<CACHE_MAX; i++) {
      if (tcache[i].w3d_tex != NULL) {
        W3D_FreeTexObj(context, tcache[i].w3d_tex);
        tcache[i].w3d_tex = NULL;
      }
    }

    if (tcache[0].Texture != NULL) {
      free(tcache[0].Texture);
    }

    free(tcache);
    tcache = NULL;
  }
}

static void update_clut(TextureCache *tc)
{
  UINT16 *rgbtbl = (tc->opaque) ? rgbtbl_opaque : rgbtbl_transparent;
  UINT16 *ctbl_ptr;
  int y, clutP;

  clutP = (tc->clY<<10)+tc->clX;

  ctbl_ptr = &psxVuw[clutP];

  /* Create correct palette */
  switch (tc->mode) {
    case 0: // 4bit
      for(y=0;y<16;y++) {
        tc->ctbl[y] = rgbtbl[*ctbl_ptr++];
      }
      break;

    case 1: // 8bit
      for(y=0;y<256;y++) {
        tc->ctbl[y] = rgbtbl[*ctbl_ptr++];
      }
      break;

    default: // 15bit
      break;
  }
}

#ifdef MSB_FIRST
  #define AT_BITS_28 (28)
  #define AT_BITS_24 (24)
  #define AT_BITS_20 (20)
  #define AT_BITS_16 (16)
  #define AT_BITS_12 (12)
  #define AT_BITS_8  ( 8)
  #define AT_BITS_4  ( 4)
  #define AT_BITS_0  ( 0)
#else
  #define AT_BITS_28 ( 4)
  #define AT_BITS_24 ( 0)
  #define AT_BITS_20 (12)
  #define AT_BITS_16 ( 8)
  #define AT_BITS_12 (20)
  #define AT_BITS_8  (16)
  #define AT_BITS_4  (28)
  #define AT_BITS_0  (24)
#endif

#define UPDATE_01(u16) \
  opaque_required |= rgbtbl_opaque[(u16)]

#define TEX4_1 \
  UINT32 c; \
  c = ((UINT32 *)tex1)[0]; \
  UPDATE_01(ctbl_ptr[(c >> AT_BITS_24) & 15]); \
  UPDATE_01(ctbl_ptr[(c >> AT_BITS_28) & 15]); \
  UPDATE_01(ctbl_ptr[(c >> AT_BITS_16) & 15]); \
  UPDATE_01(ctbl_ptr[(c >> AT_BITS_20) & 15]); \
  UPDATE_01(ctbl_ptr[(c >> AT_BITS_8 ) & 15]); \
  UPDATE_01(ctbl_ptr[(c >> AT_BITS_12) & 15]); \
  UPDATE_01(ctbl_ptr[(c >> AT_BITS_0 ) & 15]); \
  UPDATE_01(ctbl_ptr[(c >> AT_BITS_4 ) & 15]); \
\
  texture[0] = tc->ctbl[(c >> AT_BITS_24) & 15]; \
  texture[1] = tc->ctbl[(c >> AT_BITS_28) & 15]; \
  texture[2] = tc->ctbl[(c >> AT_BITS_16) & 15]; \
  texture[3] = tc->ctbl[(c >> AT_BITS_20) & 15]; \
  texture[4] = tc->ctbl[(c >> AT_BITS_8 ) & 15]; \
  texture[5] = tc->ctbl[(c >> AT_BITS_12) & 15]; \
  texture[6] = tc->ctbl[(c >> AT_BITS_0 ) & 15]; \
  texture[7] = tc->ctbl[(c >> AT_BITS_4 ) & 15]; \
  texture += 8;

#define TEX8_1 \
  UINT32 c; \
  c = ((UINT32 *)tex1)[0]; \
  UPDATE_01(ctbl_ptr[(c >> AT_BITS_24) & 255]); \
  UPDATE_01(ctbl_ptr[(c >> AT_BITS_16) & 255]); \
  UPDATE_01(ctbl_ptr[(c >> AT_BITS_8 ) & 255]); \
  UPDATE_01(ctbl_ptr[(c >> AT_BITS_0 ) & 255]); \
\
  texture[0] = tc->ctbl[(c >> AT_BITS_24) & 255]; \
  texture[1] = tc->ctbl[(c >> AT_BITS_16) & 255]; \
  texture[2] = tc->ctbl[(c >> AT_BITS_8 ) & 255]; \
  texture[3] = tc->ctbl[(c >> AT_BITS_0 ) & 255]; \
  c = ((UINT32 *)tex1)[1]; \
  UPDATE_01(ctbl_ptr[(c >> AT_BITS_24) & 255]); \
  UPDATE_01(ctbl_ptr[(c >> AT_BITS_16) & 255]); \
  UPDATE_01(ctbl_ptr[(c >> AT_BITS_8 ) & 255]); \
  UPDATE_01(ctbl_ptr[(c >> AT_BITS_0 ) & 255]); \
\
  texture[4] = tc->ctbl[(c >> AT_BITS_24) & 255]; \
  texture[5] = tc->ctbl[(c >> AT_BITS_16) & 255]; \
  texture[6] = tc->ctbl[(c >> AT_BITS_8 ) & 255]; \
  texture[7] = tc->ctbl[(c >> AT_BITS_0 ) & 255]; \
  texture += 8;

#define TEX15_1 \
  UPDATE_01(tex1[0]); \
  UPDATE_01(tex1[1]); \
  UPDATE_01(tex1[2]); \
  UPDATE_01(tex1[3]); \
  UPDATE_01(tex1[4]); \
  UPDATE_01(tex1[5]); \
  UPDATE_01(tex1[6]); \
  UPDATE_01(tex1[7]); \
\
  texture[0] = rgbtbl[tex1[0]]; \
  texture[1] = rgbtbl[tex1[1]]; \
  texture[2] = rgbtbl[tex1[2]]; \
  texture[3] = rgbtbl[tex1[3]]; \
  texture[4] = rgbtbl[tex1[4]]; \
  texture[5] = rgbtbl[tex1[5]]; \
  texture[6] = rgbtbl[tex1[6]]; \
  texture[7] = rgbtbl[tex1[7]]; \
  texture += 8;

#define TEX4_2 \
  UINT32 c; \
  c = ((UINT32 *)tex1)[0]; \
  UPDATE_01(ctbl_ptr[(c >> AT_BITS_24) & 15]); \
  UPDATE_01(ctbl_ptr[(c >> AT_BITS_16) & 15]); \
  UPDATE_01(ctbl_ptr[(c >> AT_BITS_8 ) & 15]); \
  UPDATE_01(ctbl_ptr[(c >> AT_BITS_0 ) & 15]); \
\
  texture[0] = tc->ctbl[(c >> AT_BITS_24) & 15]; \
  texture[1] = tc->ctbl[(c >> AT_BITS_16) & 15]; \
  texture[2] = tc->ctbl[(c >> AT_BITS_8 ) & 15]; \
  texture[3] = tc->ctbl[(c >> AT_BITS_0 ) & 15]; \
  texture += 4;

#define TEX8_2 \
  UINT32 c; \
  c = ((UINT32 *)tex1)[0]; \
  UPDATE_01(ctbl_ptr[(c >> AT_BITS_24) & 255]); \
  UPDATE_01(ctbl_ptr[(c >> AT_BITS_8 ) & 255]); \
\
  texture[0] = tc->ctbl[(c >> AT_BITS_24) & 255]; \
  texture[1] = tc->ctbl[(c >> AT_BITS_8 ) & 255]; \
  c = ((UINT32 *)tex1)[1]; \
  UPDATE_01(ctbl_ptr[(c >> AT_BITS_24) & 255]); \
  UPDATE_01(ctbl_ptr[(c >> AT_BITS_8 ) & 255]); \
\
  texture[2] = tc->ctbl[(c >> AT_BITS_24) & 255]; \
  texture[3] = tc->ctbl[(c >> AT_BITS_8 ) & 255]; \
  texture += 4;

#define TEX15_2 \
  UPDATE_01(tex1[0]); \
  UPDATE_01(tex1[2]); \
  UPDATE_01(tex1[4]); \
  UPDATE_01(tex1[6]); \
\
  texture[0] = rgbtbl[tex1[0]]; \
  texture[1] = rgbtbl[tex1[2]]; \
  texture[2] = rgbtbl[tex1[4]]; \
  texture[3] = rgbtbl[tex1[6]]; \
  texture += 4;

#define TEX4_4 \
  UINT32 c; \
  c = ((UINT32 *)tex1)[0]; \
  UPDATE_01(ctbl_ptr[(c >> AT_BITS_24) & 15]); \
  UPDATE_01(ctbl_ptr[(c >> AT_BITS_8 ) & 15]); \
\
  texture[0] = tc->ctbl[(c >> AT_BITS_24) & 15]; \
  texture[1] = tc->ctbl[(c >> AT_BITS_8 ) & 15]; \
  texture += 2;

#define TEX8_4 \
  UINT32 c; \
  c = ((UINT32 *)tex1)[0]; \
  UPDATE_01(ctbl_ptr[(c >> AT_BITS_24) & 255]); \
\
  texture[0] = tc->ctbl[(c >> AT_BITS_24) & 255]; \
  c = ((UINT32 *)tex1)[1]; \
  UPDATE_01(ctbl_ptr[(c >> AT_BITS_24) & 255]); \
\
  texture[1] = tc->ctbl[(c >> AT_BITS_24) & 255]; \
  texture += 2;

#define TEX15_4 \
  UPDATE_01(tex1[0]); \
  UPDATE_01(tex1[4]); \
\
  texture[0] = rgbtbl[tex1[0]]; \
  texture[1] = rgbtbl[tex1[4]]; \
  texture += 2;

#define TEXLOOP(BITS, DOWNSCALE) \
  for (y=0; y<256; y+=DOWNSCALE) { \
    tex0 = &psxVuw[YAdjust] + (1024 * (ty + (y & tymask))) + (tx >> (8 / BITS)); \
    for(x=0; x<256; x+=8) { \
      tex1 = tex0 + ((x & txmask) >> (8 / BITS)); \
      { TEX ## BITS ## _ ## DOWNSCALE } \
    } \
  } \
  tc->opaque_required = (opaque_required != 0);

#define UPDATE(DOWNSCALE) \
static void update_texture_ ## DOWNSCALE(TextureCache *tc) \
{ \
  UINT16 *rgbtbl = (tc->opaque) ? rgbtbl_opaque : rgbtbl_transparent; \
  UINT32 tx, ty, txmask, tymask; \
  UINT16 *tex0, *tex1, *ctbl_ptr, opaque_required = 0; \
  int x, y, clutP; \
  UINT16 *texture; \
  int YAdjust; \
\
  YAdjust = ((tc->GlobalTextAddrY)<<10)+tc->GlobalTextAddrX; \
  clutP = (tc->clY<<10)+tc->clX; \
\
  ctbl_ptr = &psxVuw[clutP]; \
\
  texture = tc->Texture; \
\
  if (tc->bUsingTWin) { \
    tx = tc->x0; \
    ty = tc->y0; \
    txmask = tc->x1 - 1; \
    tymask = tc->y1 - 1; \
  } else { \
    tx = 0; \
    ty = 0; \
    txmask = 256 - 1; \
    tymask = 256 - 1; \
  } \
\
  switch (tc->mode) { \
    case 0: /* 4bit */ \
      TEXLOOP(4, DOWNSCALE) \
      break; \
\
    case 1: /* 8bit */ \
      TEXLOOP(8, DOWNSCALE) \
      break; \
\
    case 2: /* 15bit */ \
      TEXLOOP(15, DOWNSCALE) \
      break; \
\
    default: \
      break; \
  } \
}

UPDATE(1)
UPDATE(2)
UPDATE(4)

static void load_texture(TextureCache *tc)
{
  static unsigned int errors;
  ULONG result;
  update_clut(tc);

  switch (tex_downscale) {
    case 1:
      update_texture_1(tc);
      break;

    case 2:
      update_texture_2(tc);
      break;

    case 4:
      update_texture_4(tc);
      break;

    default:
      break;
  }

  result = W3D_UpdateTexImage(context, tc->w3d_tex, tc->Texture, 0, NULL);
  if (result != W3D_SUCCESS && errors++ < 12)
    printf("GPUW3D repair2: texture upload error %lu, mode=%d, page=%d,%d\n",
           result, tc->mode, tc->GlobalTextAddrX, tc->GlobalTextAddrY);
  W3D_SetTexEnv(context, tc->w3d_tex, W3D_MODULATE, NULL);

  if (texfilter) {
    W3D_SetFilter(context, tc->w3d_tex, W3D_LINEAR, W3D_LINEAR);
  } else {
    W3D_SetFilter(context, tc->w3d_tex, W3D_NEAREST, W3D_NEAREST);
  }
}

static int set_texture_internal(int clX, int clY, int opaque)
{
  TextureCache *tc;
  int i, min_count, min_index;

  // search cache
  for(i=0,tc=tcache; i<CACHE_MAX; i++,tc++) {
    if (tc->mode == GlobalTextTP &&
      tc->clY == clY &&
      tc->clX == clX &&
      tc->GlobalTextAddrY == GlobalTextAddrY &&
      tc->GlobalTextAddrX == GlobalTextAddrX &&
      tc->opaque     == opaque     &&
      tc->bUsingTWin == bUsingTWin &&
      tc->x0 == TWin.Position.x0 && tc->y0 == TWin.Position.y0 &&
      tc->x1 == TWin.Position.x1 && tc->y1 == TWin.Position.y1)
    {
      TEX_VERBOSE(tc)

      tc->counter++;

      if (tc->w3d_tex != array_unit_tex[opaque]) {
        if (opaque == 0) {
          FLUSH_ARRAY(); // texture has changed
          array_unit_tex[1] = NULL;
        }
        array_unit_tex[opaque] = tc->w3d_tex;
      }

      return tc->opaque_required;
    }
  }

  if (opaque == 0) {
    FLUSH_ARRAY(); // texture is about to change
    array_unit_tex[1] = NULL;
  }

  replaced_textures++;

  // find unused texture
  min_count = 0x7fffffff;
  min_index = -1;
  for(i=0,tc=tcache; i<CACHE_MAX; i++,tc++) {
    if (tc->counter < min_count) {
      if ((opaque == 1) && (array_unit_tex[0] == tc->w3d_tex)) {
        // we can't replace our first texture
      } else {
        min_count = tc->counter;
        min_index = i;
      }
    }
    if (tc->mode == -1) {
      break;
    }
  }

  if (i == CACHE_MAX) {
    i = min_index;
    if (i < 0) {
      i = 0;
    }
    tc = &tcache[i];
  }

  tc->mode = GlobalTextTP;
  tc->clY  = clY;
  tc->clX  = clX;
  tc->GlobalTextAddrY = GlobalTextAddrY;
  tc->GlobalTextAddrX = GlobalTextAddrX;
  tc->opaque     = opaque;
  tc->bUsingTWin = bUsingTWin;
  tc->x0 = TWin.Position.x0;
  tc->x1 = TWin.Position.x1;
  tc->y0 = TWin.Position.y0;
  tc->y1 = TWin.Position.y1;
  tc->counter = 0;

  load_texture(tc);

  TEX_VERBOSE(tc)

  tc->counter++;
  array_unit_tex[opaque] = tc->w3d_tex;

  return tc->opaque_required;
}

int set_texture(int clX, int clY, int opaque)
{
  if (opaque == 1) {
    FLUSH_ARRAY(); // texture is about to change
    array_unit_tex[0] = array_unit_tex[1];
    array_unit_tex[1] = NULL;
  } else {
    opaque = set_texture_internal(clX, clY, 0);
    if (DrawSemiTrans && (opaque == 1)) {
      set_texture_internal(clX, clY, 1);
      return 1;
    }
  }
  return 0;
}

static void UploadImageEXP(long X, long Y, long W, long H)
{
#if 1
  unsigned char *src, *dst;
  long DST_X, DST_Y, DST_W, DST_H;

  if (W==0 && H==0) return;

  if (X<0) X=0; if (X>1023) X=1023;
  if (W<0) W=0; if (W>1024) W=1024;
  if (Y<0) Y=0; if (Y>511)  Y=511;
  if (H<0) H=0; if (H>512)  H=512;

  if ((X+W)>1024) W=1024-X;
  if ((Y+H)>512)  H=512-Y;

  DST_X = X;
  DST_Y = Y;
  DST_W = W;
  DST_H = H;

  OffsetSet();
  DST_X += W3D_Offset.x;
  DST_Y += W3D_Offset.y;

  // Scale and clamp
  DST_X *= x_scale;
  DST_Y *= y_scale;
  DST_W *= x_scale;
  DST_H *= y_scale;

  // Discard images outside of visible area
  if ((DST_X < 0) || (DST_Y < 0)) return;
  if ((DST_X >= amiga_width) || (DST_Y >= amiga_height)) return;

  if (DST_X>(amiga_width-1))  DST_X=amiga_width-1;
  if (DST_W>amiga_width)      DST_W=amiga_width;
  if (DST_Y>(amiga_height-1)) DST_Y=amiga_height-1;
  if (DST_H>amiga_height)     DST_H=amiga_height;

  if ((DST_X+DST_W)>amiga_width)  DST_W=amiga_width-DST_X;
  if ((DST_Y+DST_H)>amiga_height) DST_H=amiga_height-DST_Y;

  if (DST_W==0 && DST_H==0) return;

#ifdef __amigaos4__
  if ((lock = p96LockBitMap(w3d_bitmap, (UBYTE *)&ri, sizeof(ri)))) {
    src = (unsigned char *)&psxVuw[(Y * 1024) + X];
    dst = (UBYTE *)ri.Memory + (DST_Y * ri.BytesPerRow) + (DST_X * bpp);
    rect_setup(&rect, W, H, 2048, RECT_FORMAT_BGR555LE, DST_W, DST_H, ri.BytesPerRow, rect_pixfmt);
    rect.refresh_func(&rect, dst, src);
    p96UnlockBitMap(w3d_bitmap, lock);
  }
#else
  if ((lock = LockBitMapTags(w3d_bitmap, LBMI_BASEADDRESS, (ULONG)&baseaddress, LBMI_BYTESPERROW, (ULONG)&bpr, TAG_END))) {
    src = (unsigned char *)&psxVuw[(Y * 1024) + X];
    dst = (UBYTE *)baseaddress + (DST_Y * bpr) + (DST_X * bpp);
    rect_setup(&rect, W, H, 2048, RECT_FORMAT_BGR555LE, DST_W, DST_H, bpr, rect_pixfmt);
    rect.refresh_func(&rect, dst, src);
    UnLockBitMap(lock);
  }
#endif
#endif
}

static void UploadImage(long X, long Y, long W, long H)
{
  unsigned char *src, *dst;

  if (W==0 && H==0) return;

  if (X<0) X=0; if (X>1023) X=1023;
  if (W<0) W=0; if (W>1024) W=1024;
  if (Y<0) Y=0; if (Y>511)  Y=511;
  if (H<0) H=0; if (H>512)  H=512;

  if ((X+W)>1024) W=1024-X;
  if ((Y+H)>512)  H=512-Y;

#ifdef __amigaos4__
  if ((lock = p96LockBitMap(w3d_bitmap, (UBYTE *)&ri, sizeof(ri)))) {
    src = (unsigned char *)&psxVuw[(Y * 1024) + X];
    dst = (UBYTE *)ri.Memory + (Y * ri.BytesPerRow) + (X * bpp);
    rect_setup(&rect, W, H, 2048, RECT_FORMAT_BGR555LE, W, H, ri.BytesPerRow, rect_pixfmt);
    rect.refresh_func(&rect, dst, src);
    p96UnlockBitMap(w3d_bitmap, lock);
  }
#else
  if ((lock = LockBitMapTags(w3d_bitmap, LBMI_BASEADDRESS, (ULONG)&baseaddress, LBMI_BYTESPERROW, (ULONG)&bpr, TAG_END))) {
    src = (unsigned char *)&psxVuw[(Y * 1024) + X];
    dst = (UBYTE *)baseaddress + (Y * bpr) + (X * bpp);
    rect_setup(&rect, W, H, 2048, RECT_FORMAT_BGR555LE, W, H, bpr, rect_pixfmt);
    rect.refresh_func(&rect, dst, src);
    UnLockBitMap(lock);
  }
#endif
}

void InvalidateTextureArea(long X, long Y, long W, long H)
{
  TextureCache *tc;
  int i;
  if (!tcache || W <= 0 || H <= 0) return;
  FLUSH_ARRAY();
  for (i = 0, tc = tcache; i < CACHE_MAX; ++i, ++tc) {
    if (w3d_texture_touched(tc->mode, tc->GlobalTextAddrX, tc->GlobalTextAddrY,
                           tc->clX, tc->clY, X, Y, W, H, iGPUHeight))
      tc->mode = -1;
  }
}

void w3d_gpu_vram_changed(int x, int y, int width, int height)
{
  int ty, tx, h, w;
  if (!context || width <= 0 || height <= 0) return;
  FLUSH_ARRAY();
  InvalidateTextureArea(x, y, width, height);
  if (PSXDisplay.RGB24) return;
  /* Complete queued GPU writes before a CPU bitmap upload. */
  W3D_FlushFrame(context);
  W3D_WaitIdle(context);
  width = MIN(width, 1024);
  height = MIN(height, 512);
  for (ty = 0; ty < height; ty += h) {
    int dy = (y + ty) & 511;
    h = MIN(height - ty, 512 - dy);
    for (tx = 0; tx < width; tx += w) {
      int dx = (x + tx) & 1023;
      w = MIN(width - tx, 1024 - dx);
      if (scale_gfx == SCALING_EXP) UploadImageEXP(dx, dy, w, h);
      else UploadImage(dx, dy, w, h);
    }
  }
}

void CheckWriteUpdate(void)
{
  w3d_gpu_vram_changed(VRAMWrite.x, VRAMWrite.y, VRAMWrite.Width, VRAMWrite.Height);
}

/*
 * w3d.h
 */

#ifndef _W3D_H_
#define _W3D_H_

#undef W3D_VERBOSE
#ifndef FPSE_MINIGL
#define W3D_VERBOSE
#endif

#define MIN(_a, _b) \
  (((_a) < (_b)) ? (_a) : (_b))

#define MAX(_a, _b) \
  (((_a) > (_b)) ? (_a) : (_b))

/* A *= 2; if (A > 255) A = 255, etc */
#define ARGBx2(ARGB) \
 ((((ARGB >> 7) & 0x01010101) * 0xff) | ((ARGB << 1) & 0xfefefefe))

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "plugin.h"
#include "externals.h"
#include "soft.h"

#undef BOOL /* defined in externals.h */

#ifdef __morphos__
#define USE_INLINE_STDARG
#endif

#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include <exec/memory.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>

#ifdef FPSE_MINIGL
#include "../../gpuminigl/backend.h"
#elif defined(__amigaos4__)
#include <proto/Picasso96API.h>
#include <proto/warp3d.h>
#else
#include <cybergraphx/cybergraphics.h>
#include <proto/cybergraphics.h>
#include <proto/Warp3D.h>
#endif

#ifdef MSB_FIRST

#define psxVuw(a) SWAP16_SEL_R(psxVuw, ((a)<<1))
#define gpuData(a) SWAP32_SEL_R(gpuData, ((a)<<2)) // long

#define PDESTL SWAP32_R(pdest)
#define PDESTS SWAP16_R(pdest)

#define PDESTLW(a) SWAP32_W((a), pdest)
#define PDESTSW(a) SWAP16_W((a), pdest)

#else

#define psxVuw(a) psxVuw[(a)]
#define gpuData(a) gpuData[(a)] // long

#define PDESTL *pdest
#define PDESTS *pdest

#define PDESTLW(a) *pdest = (a)
#define PDESTSW(a) *pdest = (a)

#endif

#define FLAG_MULTITEXT (1 << 0)
#define FLAG_FASTMULTI (1 << 1)
#define FLAG_FASTMODE3 (1 << 2) /* FASTMODE3 will make mode 3 fast with 2 units, but mode 2 impossible */

extern unsigned int W3D_Flags;

#ifndef FPSE_MINIGL
extern W3D_Context *context;
#endif

#define PRIM_BUFFER (1000)
#define PRIM_MAX    (PRIM_BUFFER - 6)

typedef struct {
  W3D_Float x,y,z;
  W3D_Float primary_r, primary_g, primary_b, primary_a;
  W3D_Float u,v,w;
  W3D_Float u1,v1,w1;
} MyVertex;

#define MYVERTEX_FORMAT \
  W3D_VFORMAT_COLOR|W3D_VFORMAT_TCOORD_0|W3D_VFORMAT_TCOORD_1

extern MyVertex array[PRIM_BUFFER];

#define CLAMP(val, min, max) (((val) < (min)) ? (min) : ((val) > (max)) ? (max) : (val))

W3D_Float set_blend_mode(int *opaque_pass);

extern int scale_gfx;
#define SCALING_NONE (0)
#define SCALING_MDEC (1)
#define SCALING_EXP  (2)

extern W3D_Float x_scale, y_scale;
extern PSXPoint_t W3D_Offset;
void OffsetSet(void);

#ifdef W3D_VERBOSE
#include "amidog/timer.h"
#include "amiga_font.h"
extern int poly_count[3];
#define W3D_VERBOSE_LINE     poly_count[0]
#define W3D_VERBOSE_POLY_SMP poly_count[1]
#define W3D_VERBOSE_POLY_TEX poly_count[2]
#endif

#define PRIM_DEFAULT  (0)
#define PRIM_TEXTURED (1)
#define PRIM_FILLAREA (2)

extern ULONG array_primitive, w3d_primitive, array_count, prim_count;
extern ULONG array_index[PRIM_BUFFER];
#define MAX_TEX_UNITS (3)
#ifdef FPSE_MINIGL
extern GLuint array_unit_tex[MAX_TEX_UNITS];
#else
extern W3D_Texture *array_unit_tex[MAX_TEX_UNITS];
#endif
void FLUSH_ARRAY(void);
#ifndef FPSE_MINIGL
void w3d_gpu_fill(int x0, int y0, int x1, int y1, unsigned long rgb);
#endif

#define SETUP(W3D_PRIM, ARRAY_PRIM) \
  if ((w3d_primitive != W3D_PRIM) || (array_primitive != ARRAY_PRIM)) { \
    FLUSH_ARRAY(); \
    w3d_primitive = W3D_PRIM; array_primitive = ARRAY_PRIM; \
  }

#define VERTEX(_i, _x, _y, _rgb) \
  array[array_count + (_i)].x = (_x) * x_scale; \
  array[array_count + (_i)].y = (_y) * y_scale; \
  array[array_count + (_i)].primary_r = (((_rgb) >>  0) & 0xff) / 255.0; \
  array[array_count + (_i)].primary_g = (((_rgb) >>  8) & 0xff) / 255.0; \
  array[array_count + (_i)].primary_b = (((_rgb) >> 16) & 0xff) / 255.0; \
  array[array_count + (_i)].primary_a = alpha; \
  /* unused */ \
  array[array_count + (_i)].z = 0.0; \
  array[array_count + (_i)].w = 1.0;

#define VERTEX_TEX(_i, _x, _y, _u, _v, _rgb) \
  array[array_count + (_i)].x = (_x) * x_scale; \
  array[array_count + (_i)].y = (_y) * y_scale; \
  array[array_count + (_i)].u = (_u) / 256.0; \
  array[array_count + (_i)].v = (_v) / 256.0; \
  array[array_count + (_i)].primary_r = (((_rgb) >>  0) & 0xff) / 255.0; \
  array[array_count + (_i)].primary_g = (((_rgb) >>  8) & 0xff) / 255.0; \
  array[array_count + (_i)].primary_b = (((_rgb) >> 16) & 0xff) / 255.0; \
  array[array_count + (_i)].primary_a = alpha; \
  /* unused */ \
  array[array_count + (_i)].z = 0.0; \
  array[array_count + (_i)].w = 1.0;

#define INDEX_1(idx0) \
  array_index[prim_count + 0] = array_count + idx0; \
  prim_count += 1;

#define INDEX_2(idx0,idx1) \
  array_index[prim_count + 0] = array_count + idx0; \
  array_index[prim_count + 1] = array_count + idx1; \
  prim_count += 2;

#define INDEX_3(idx0,idx1,idx2) \
  array_index[prim_count + 0] = array_count + idx0; \
  array_index[prim_count + 1] = array_count + idx1; \
  array_index[prim_count + 2] = array_count + idx2; \
  prim_count += 3;

#define INDEX_6(idx0,idx1,idx2,idx3,idx4,idx5) \
  array_index[prim_count + 0] = array_count + idx0; \
  array_index[prim_count + 1] = array_count + idx1; \
  array_index[prim_count + 2] = array_count + idx2; \
  array_index[prim_count + 3] = array_count + idx3; \
  array_index[prim_count + 4] = array_count + idx4; \
  array_index[prim_count + 5] = array_count + idx5; \
  prim_count += 6;

#define DRAW(PRIM_COUNT) \
  array_count += PRIM_COUNT; \
  if (prim_count /* should be prim_count */ >= PRIM_MAX) { \
    FLUSH_ARRAY(); \
  }

#endif /* _W3D_H_ */

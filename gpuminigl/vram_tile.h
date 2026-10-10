/* One-texel sampling gutters around opaque VRAM tiles. 256 POT storage,
 * 254 interior texels; gutters copy neighbours from the WHOLE rectangle.
 * Clamp only outer rectangle edges. UV [1/256,(1+extent)/256] preserves
 * texel mapping, unlike shrinking each tile to its first/last centres. */
#ifndef FPSE_MINIGL_VRAM_TILE_H
#define FPSE_MINIGL_VRAM_TILE_H
#include "pixels.h"
#define MGL_VRAM_TILE_SIZE 256
#define MGL_VRAM_TILE_INNER 254

static __inline__ int mgl_vram_tile_coord(int tile, int p, int extent)
{
    int v = tile + p - 1;
    return v < 0 ? 0 : v >= extent ? extent - 1 : v;
}

/* Positive dimensions, valid VRAM/dst required. x is in 16-bit VRAM words
 * even for 24-bit display; tx is in pixels. All output pixels are opaque. */
static void mgl_vram_tile_rgba(unsigned char *dst, const unsigned char *vram,
    int x, int y, int width, int height, int tx, int ty, int rgb24)
{
    int px, py;
    for (py = 0; py < MGL_VRAM_TILE_SIZE; ++py) {
        int sy = mgl_vram_tile_coord(ty, py, height);
        unsigned row = ((unsigned)(y + sy) & 511U);
        for (px = 0; px < MGL_VRAM_TILE_SIZE; ++px) {
            int sx = mgl_vram_tile_coord(tx, px, width);
            unsigned char *d = dst + (py * MGL_VRAM_TILE_SIZE + px) * 4;
            if (rgb24) {
                unsigned a = (row * 2048U + x * 2 + sx * 3) & 0xfffffU;
                d[0] = vram[a]; d[1] = vram[(a + 1) & 0xfffffU];
                d[2] = vram[(a + 2) & 0xfffffU];
            } else {
                unsigned a = row * 1024U + ((unsigned)(x + sx) & 1023U);
                psx_rgba(psx_read16(vram, a), d, 0);
            }
            d[3] = 255;
        }
    }
}
#endif

/* W3D-style lookup/palette conversion, with exact PSX address wrapping.
 * Output words are native-endian: MiniGL's packed type is A4R4G4B4.
 * RGBA lookup words are built byte-by-byte for the fallback backend. */
#ifndef FPSE_MINIGL_TEXDECODE_H
#define FPSE_MINIGL_TEXDECODE_H
#include <stdint.h>
#include "pixels.h"

static inline int psx_cache_entries(int requested)
{
    return requested < 64 || requested > 256 ? 256 : requested;
}

static inline uint16_t psx_argb4444(unsigned int p, int pass)
{
    return ((p & 0x1e) << 7) | ((p & 0x3c0) >> 2) | ((p & 0x7800) >> 11) |
           ((p && (!pass || !(p & 0x8000))) ? 0xf000 : 0);
}

static inline uint32_t psx_rgba_word(unsigned int p, int pass)
{
    uint32_t out;
    psx_rgba(p, (unsigned char *)&out, pass);
    return out;
}

/* Window bit masks are not necessarily contiguous. Precompute addresses
 * once, convert the CLUT once, and select the pixel format outside the loop. */
#define PSX_DECODE_PAGE(TYPE, SUFFIX) \
static int psx_decode_page##SUFFIX(const unsigned char *vram, int mode, \
    int px, int py, int cx, int cy, unsigned long window, int step, \
    const TYPE *lut, TYPE *out) \
{ \
    TYPE palette[256]; \
    unsigned char pal_opaque[256], shifts[256]; \
    unsigned short offsets[256]; \
    unsigned int opaque = 0, i, x, y, n = 256 / step; \
    unsigned int p, word, index, row, v; \
    if (mode < 0 || mode > 2) return 0; \
    if (mode < 2) { \
        unsigned int colors = mode == 0 ? 16 : 256; \
        for (i = 0; i < colors; ++i) { \
            p = psx_read16(vram, cy * 1024 + cx + i); \
            palette[i] = lut[p]; \
            pal_opaque[i] = p != 0 && !(p & 0x8000); \
        } \
    } \
    for (x = 0; x < n; ++x) { \
        unsigned int u = psx_window_coord(x * step, window & 31, window >> 10); \
        offsets[x] = (px + (u >> (2 - mode))) & 1023; \
        shifts[x] = mode == 0 ? (u & 3) * 4 : (u & 1) * 8; \
    } \
    for (y = 0; y < n; ++y) { \
        v = psx_window_coord(y * step, (window >> 5) & 31, window >> 15); \
        row = ((py + v) & 511) * 1024; \
        if (mode == 0 && !window && step == 1) { \
            for (x = 0; x < 256; x += 4) { \
                word = psx_read16(vram, row + ((px + x / 4) & 1023)); \
                for (i = 0; i < 4; ++i) { \
                    index = word & 15; word >>= 4; \
                    *out++ = palette[index]; opaque |= pal_opaque[index]; \
                } \
            } \
        } else if (mode == 1 && !window && step == 1) { \
            for (x = 0; x < 256; x += 2) { \
                word = psx_read16(vram, row + ((px + x / 2) & 1023)); \
                *out++ = palette[word & 255]; opaque |= pal_opaque[word & 255]; \
                *out++ = palette[word >> 8]; opaque |= pal_opaque[word >> 8]; \
            } \
        } else if (mode == 0) { \
            for (x = 0; x < n; ++x) { \
                word = psx_read16(vram, row + offsets[x]); \
                index = (word >> shifts[x]) & 15; \
                *out++ = palette[index]; opaque |= pal_opaque[index]; \
            } \
        } else if (mode == 1) { \
            for (x = 0; x < n; ++x) { \
                word = psx_read16(vram, row + offsets[x]); \
                index = (word >> shifts[x]) & 255; \
                *out++ = palette[index]; opaque |= pal_opaque[index]; \
            } \
        } else { \
            for (x = 0; x < n; ++x) { \
                p = psx_read16(vram, row + offsets[x]); \
                *out++ = lut[p]; opaque |= p != 0 && !(p & 0x8000); \
            } \
        } \
    } \
    return opaque != 0; \
}
PSX_DECODE_PAGE(uint16_t, 16)
PSX_DECODE_PAGE(uint32_t, 32)
#undef PSX_DECODE_PAGE
#endif

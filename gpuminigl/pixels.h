/* PSX VRAM is little-endian, regardless of the host CPU. GPL-2.0-or-later. */
#ifndef FPSE_MINIGL_PIXELS_H
#define FPSE_MINIGL_PIXELS_H

static inline unsigned int psx_read16(const unsigned char *vram, unsigned int address)
{
    address = (address & 0x7ffffU) * 2;
    return vram[address] | ((unsigned int)vram[address + 1] << 8);
}

/* pass: 0 = all nonzero texels, 1 = only opaque (STP=0) texels. */
static inline void psx_rgba(unsigned int pixel, unsigned char *out, int pass)
{
    unsigned int r = pixel & 31, g = (pixel >> 5) & 31, b = (pixel >> 10) & 31;
    out[0] = (r << 3) | (r >> 2);
    out[1] = (g << 3) | (g >> 2);
    out[2] = (b << 3) | (b >> 2);
    out[3] = (pixel && (!pass || !(pixel & 0x8000))) ? 255 : 0;
}

static inline unsigned int psx_texel(const unsigned char *vram, int mode,
                              int page_x, int page_y, int clut_x, int clut_y,
                              unsigned int u, unsigned int v)
{
    unsigned int word, index, row = ((page_y + v) & 511U) * 1024;
    u &= 255;
    if (mode == 0) {
        word = psx_read16(vram, row + ((page_x + (u >> 2)) & 1023));
        index = (word >> ((u & 3) * 4)) & 15;
    } else if (mode == 1) {
        word = psx_read16(vram, row + ((page_x + (u >> 1)) & 1023));
        index = (word >> ((u & 1) * 8)) & 255;
    } else {
        return psx_read16(vram, row + ((page_x + u) & 1023));
    }
    return psx_read16(vram, clut_y * 1024 + clut_x + index);
}

/* Exact GP0(E2) mask/offset operation, including non-contiguous masks. */
static inline unsigned int psx_window_coord(unsigned int coord, unsigned int mask,
                                     unsigned int offset)
{
    return ((coord & ~(mask * 8)) | ((offset & mask) * 8)) & 255;
}

#endif

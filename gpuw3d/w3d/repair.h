/* Pure helpers shared by the renderer and host regression tests.
 * GPL-2.0-or-later. */
#ifndef GPUW3D_REPAIR_H
#define GPUW3D_REPAIR_H

/* GP1 display ranges can change without a GP1(08) resolution change. */
static inline int w3d_frame_extent(int visible, int nominal, int capacity)
{
    int value = visible > 0 ? visible : nominal;
    if (value < 1) value = 1;
    return value > capacity ? capacity : value;
}

/* Alpha test sees texture alpha multiplied by the primitive blend weight. */
static inline float w3d_alpha_reference(int percent, float weight)
{
    if (percent < 0) percent = 0;
    if (percent > 99) percent = 99;
    return percent * 0.01f * weight;
}

/* Half-open intervals on a wrapping power-of-two address space. */
static inline int w3d_axis_overlap(int a, int alen, int b, int blen, int period)
{
    if (alen <= 0 || blen <= 0) return 0;
    if (alen >= period || blen >= period) return 1;
    return ((b - a) & (period - 1)) < alen ||
           ((a - b) & (period - 1)) < blen;
}

static inline int w3d_rect_overlap(int ax, int ay, int aw, int ah,
                                  int bx, int by, int bw, int bh, int rows)
{
    return w3d_axis_overlap(ax, aw, bx, bw, 1024) &&
           w3d_axis_overlap(ay, ah, by, bh, rows);
}

static inline int w3d_texture_touched(int mode, int px, int py, int cx, int cy,
                                     int x, int y, int w, int h, int rows)
{
    int palette, first;
    if (mode < 0 || mode > 2) return 0;
    if (w3d_rect_overlap(px, py, 64 << mode, 256, x, y, w, h, rows)) return 1;
    if (mode == 2) return 0;
    palette = mode == 0 ? 16 : 256;
    first = 1024 - cx;
    if (first > palette) first = palette;
    return w3d_rect_overlap(cx, cy, first, 1, x, y, w, h, rows) ||
           w3d_rect_overlap(0, cy + 1, palette - first, 1, x, y, w, h, rows);
}
#endif

/* Black-box MiniGL texture-lifetime probe. No private driver structures. */
#ifndef FPSE_MGL2D_CASES_H
#define FPSE_MGL2D_CASES_H
#include <stddef.h>

#define M2_WIDTH 640
#define M2_HEIGHT 480
#define M2_X 64
#define M2_REFERENCE_Y 16
#define M2_ACTUAL_Y 248
#define M2_PANEL_HEIGHT 160
#define M2_CAPTURE_BYTES (M2_WIDTH * M2_HEIGHT * 3)

enum { M2_IMAGE, M2_SUB, M2_DRAW, M2_FINISH, M2_FLUSH, M2_PRESENT,
       M2_ATLAS, M2_SOLID, M2_SCRATCH_IMAGE };
typedef struct { unsigned char op, object, pattern, slot; } M2Step;
typedef struct { const char *name; unsigned count; M2Step steps[8]; } M2Case;
static const M2Case m2_cases[] = {
    { "geometry + capture control", 2, {{M2_SOLID,0,0,0},{M2_SOLID,0,1,1}} },
    { "static atlas 512x256", 1, {{M2_ATLAS,2,0,0}} },
    { "distinct textures / TexImage", 4, {{M2_IMAGE,0,0,0},{M2_DRAW,0,0,0},{M2_IMAGE,1,1,0},{M2_DRAW,1,0,1}} },
    { "distinct textures / TexSubImage", 4, {{M2_SUB,0,0,0},{M2_DRAW,0,0,0},{M2_SUB,1,1,0},{M2_DRAW,1,0,1}} },
    { "reuse / TexSubImage / no barrier", 4, {{M2_SUB,0,0,0},{M2_DRAW,0,0,0},{M2_SUB,0,1,0},{M2_DRAW,0,0,1}} },
    { "reuse / TexSubImage / Finish (FPSE)", 5, {{M2_SUB,0,0,0},{M2_DRAW,0,0,0},{M2_FINISH,0,0,0},{M2_SUB,0,1,0},{M2_DRAW,0,0,1}} },
    { "reuse / TexSubImage / Flush+Finish", 6, {{M2_SUB,0,0,0},{M2_DRAW,0,0,0},{M2_FLUSH,0,0,0},{M2_FINISH,0,0,0},{M2_SUB,0,1,0},{M2_DRAW,0,0,1}} },
    { "reuse / TexImage / Finish", 5, {{M2_IMAGE,0,0,0},{M2_DRAW,0,0,0},{M2_FINISH,0,0,0},{M2_IMAGE,0,1,0},{M2_DRAW,0,0,1}} },
    { "distinct / Present+Finish control", 6, {{M2_IMAGE,0,0,0},{M2_DRAW,0,0,0},{M2_PRESENT,0,0,0},{M2_FINISH,0,0,0},{M2_IMAGE,1,1,0},{M2_DRAW,1,0,1}} },
    { "reuse / TexSubImage / Present+Finish", 6, {{M2_SUB,0,0,0},{M2_DRAW,0,0,0},{M2_PRESENT,0,0,0},{M2_FINISH,0,0,0},{M2_SUB,0,1,0},{M2_DRAW,0,0,1}} },
    { "distinct / reused CPU source buffer", 4, {{M2_SCRATCH_IMAGE,0,0,0},{M2_DRAW,0,0,0},{M2_SCRATCH_IMAGE,1,1,0},{M2_DRAW,1,0,1}} }
};
#define M2_CASE_COUNT ((unsigned)(sizeof(m2_cases) / sizeof(m2_cases[0])))

/* Asymmetric 8x8 pattern; cell centres avoid filtering and edge ambiguity.
 * A is red/yellow, B blue/cyan; corners/markers detect UV flips and shifts. */
static void m2_color(unsigned pattern, unsigned x, unsigned y, unsigned char rgb[3])
{
    rgb[0] = pattern ? 0 : 255;
    rgb[1] = ((x ^ y) & 1) ? 255 : 0;
    rgb[2] = pattern ? 255 : 0;
    if (x == 0 && y == 0) rgb[0] = rgb[1] = rgb[2] = 255;
    if (x == 7 && y == 7) rgb[0] = rgb[1] = rgb[2] = 0;
    if (x == 1 && y == 2) { rgb[0] = rgb[2] = 0; rgb[1] = 255; }
    if (x == 5 && y == 6) { rgb[0] = rgb[2] = 255; rgb[1] = 0; }
}

static void m2_pixels(unsigned char *rgba, unsigned width, unsigned pattern, int atlas)
{
    unsigned x, y;
    for (y = 0; y < 256; ++y) for (x = 0; x < width; ++x) {
        unsigned char *p = rgba + (y * width + x) * 4;
        m2_color(atlas ? x / 256 : pattern, (x & 255) / 32, y / 32, p);
        p[3] = 255;
    }
}

static unsigned m2_mismatches(const unsigned char *rgb, unsigned panel_y,
    unsigned slot, unsigned expected_pattern)
{
    unsigned x, y, c, bad = 0;
    for (y = 0; y < 8; ++y) for (x = 0; x < 8; ++x) {
        unsigned char expected[3];
        const unsigned char *p = rgb + ((panel_y + y * 20 + 10) * M2_WIDTH
            + M2_X + slot * 256 + x * 32 + 16) * 3;
        m2_color(expected_pattern, x, y, expected);
        for (c = 0; c < 3; ++c) {
            int delta = (int)p[c] - expected[c];
            if (delta < -20 || delta > 20) { ++bad; break; }
        }
    }
    return bad;
}
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../texdecode.h"

static unsigned char vram[1024 * 1024];
static uint16_t lut16[65536], out16[65536];
static uint32_t lut32[65536], out32[65536];
static const unsigned long windows[] = {
    0, 1, 0x3ff, 0xaaaaa, 0x55555, 0xfffff, 0x12345, 0x81c03
};

int main(void)
{
    unsigned int i, x, y, w, p, cases = 0;
    int pass, mode, step, n, opaque, actual16, actual32, px, py, cx, cy, wrap;
    unsigned char rgba[4];
    uint32_t rng = 7;
    for (i = 0; i < sizeof(vram); ++i) {
        rng = rng * 1664525u + 1013904223u;
        vram[i] = rng >> 24;
    }
    assert(psx_cache_entries(0) == 256 && psx_cache_entries(16) == 256);
    assert(psx_cache_entries(-1) == 256 && psx_cache_entries(257) == 256);
    assert(psx_cache_entries(64) == 64 && psx_cache_entries(128) == 128);
    assert(psx_argb4444(0, 0) == 0 && psx_argb4444(0x8000, 0) == 0xf000);
    for (pass = 0; pass < 2; ++pass) {
        for (p = 0; p < 65536; ++p) {
            psx_rgba(p, rgba, pass);
            lut16[p] = psx_argb4444(p, pass);
            /* Exactly the quantization Classic already performs on RGBA. */
            assert(lut16[p] == ((rgba[3] >> 4) << 12 | (rgba[0] >> 4) << 8 |
                                (rgba[1] >> 4) << 4 | (rgba[2] >> 4)));
            lut32[p] = psx_rgba_word(p, pass);
            assert(!memcmp(&lut32[p], rgba, 4));
        }
        for (mode = 0; mode < 3; ++mode)
        for (step = 1; step <= 4; step *= 2)
        for (wrap = 0; wrap < 2; ++wrap)
        for (w = 0; w < sizeof(windows) / sizeof(windows[0]); ++w) {
            px = wrap ? 1008 : 640; py = wrap ? 480 : 0;
            cx = wrap ? 1008 : 32; cy = wrap ? 511 : 400;
            n = 256 / step;
            actual16 = psx_decode_page16(vram, mode, px, py, cx, cy, windows[w], step, lut16, out16);
            actual32 = psx_decode_page32(vram, mode, px, py, cx, cy, windows[w], step, lut32, out32);
            opaque = 0;
            for (y = 0; y < (unsigned int)n; ++y)
            for (x = 0; x < (unsigned int)n; ++x) {
                unsigned int u = psx_window_coord(x * step, windows[w] & 31, windows[w] >> 10);
                unsigned int v = psx_window_coord(y * step, (windows[w] >> 5) & 31, windows[w] >> 15);
                p = psx_texel(vram, mode, px, py, cx, cy, u, v);
                opaque |= p != 0 && !(p & 0x8000);
                assert(out16[y * n + x] == lut16[p]);
                assert(out32[y * n + x] == lut32[p]);
            }
            assert(actual16 == opaque && actual32 == opaque);
            ++cases;
        }
    }
    memset(vram, 0, sizeof(vram));
    assert(!psx_decode_page16(vram, 2, 0, 0, 0, 0, 0, 1, lut16, out16));
    /* An opaque CLUT entry that is never sampled must not request a pass. */
    vram[(400 * 1024 + 33) * 2] = 31;
    assert(!psx_decode_page16(vram, 0, 640, 0, 32, 400, 0, 1, lut16, out16));
    vram[640 * 2] = 1;
    assert(psx_decode_page16(vram, 0, 640, 0, 32, 400, 0, 1, lut16, out16));
    printf("PASS: %u decoded pages vs reference; all colors/STP, 4/8/15-bit, 1/2/4 reduction, noncontiguous windows, VRAM/CLUT wrap\n", cases);
    return 0;
}

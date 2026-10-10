#include <assert.h>
#include <stdio.h>
#include "../w3d/repair.h"

static int reference_overlap(int a, int alen, int b, int blen, int period)
{
    int i, j;
    for (i = 0; i < alen; ++i)
        for (j = 0; j < blen; ++j)
            if (((a + i) & (period - 1)) == ((b + j) & (period - 1))) return 1;
    return 0;
}

int main(void)
{
    int a, b, alen, blen, p, i;
    const float weights[] = {1.0f, 0.5f, 0.25f};
    /* 512-wide loading screen -> 320-wide game without a video-mode change. */
    assert(w3d_frame_extent(512, 512, 640) == 512);
    assert(w3d_frame_extent(320, 512, 640) == 320);
    assert(w3d_frame_extent(256, 512, 640) == 256);
    assert(w3d_frame_extent(512, 512, 640) == 512);
    assert(w3d_frame_extent(224, 240, 512) == 224);
    assert(w3d_frame_extent(240, 240, 512) == 240);
    assert(w3d_frame_extent(0, 320, 640) == 320);
    assert(w3d_frame_extent(0, 0, 640) == 1);
    assert(w3d_frame_extent(700, 640, 640) == 640);
    assert(w3d_frame_extent(576, 480, 512) == 512);
    for (p = -10; p <= 110; ++p)
        for (i = 0; i < 3; ++i) {
            float ref = w3d_alpha_reference(p, weights[i]);
            assert(ref >= 0.0f && ref < weights[i]);
            assert(!(0.0f > ref)); /* transparent zero is always rejected */
            assert(weights[i] > ref); /* valid STP texels always survive */
        }
    /* Exhaustive comparison on a small ring exercises wrap and empty ranges. */
    for (a = 0; a < 16; ++a)
        for (b = 0; b < 16; ++b)
            for (alen = 0; alen <= 17; ++alen)
                for (blen = 0; blen <= 17; ++blen)
                    assert(w3d_axis_overlap(a, alen, b, blen, 16) ==
                           reference_overlap(a, alen, b, blen, 16));
    /* Changes in the middle of a page/CLUT must invalidate it. */
    assert(w3d_texture_touched(0, 640, 0, 704, 480, 650, 10, 1, 1, 512));
    assert(w3d_texture_touched(0, 640, 0, 704, 480, 705, 480, 1, 1, 512));
    assert(!w3d_texture_touched(0, 640, 0, 704, 480, 720, 480, 1, 1, 512));
    assert(!w3d_texture_touched(0, 640, 0, 704, 480, 704, 0, 1, 1, 512));
    assert(w3d_texture_touched(1, 640, 0, 704, 480, 704, 0, 1, 1, 512));
    assert(w3d_texture_touched(2, 640, 0, 0, 0, 850, 250, 1, 1, 512));
    assert(!w3d_texture_touched(2, 640, 0, 0, 0, 0, 0, 320, 240, 512));
    /* Wrapped source pages, writes, and palettes crossing a VRAM row/end. */
    assert(w3d_texture_touched(2, 960, 256, 0, 0, 20, 300, 1, 1, 512));
    assert(w3d_texture_touched(0, 0, 0, 0, 480, 1020, 510, 8, 4, 512));
    assert(w3d_texture_touched(1, 640, 0, 1008, 511, 0, 0, 1, 1, 512));
    assert(!w3d_texture_touched(0, 640, 0, 1008, 511, 0, 0, 1, 1, 512));
    assert(!w3d_texture_touched(-1, 0, 0, 0, 0, 0, 0, 1024, 512, 512));
    assert(!w3d_texture_touched(1, 0, 0, 0, 0, 0, 0, 0, 512, 512));
    puts("PASS: live display dimensions, alpha weights/cutoff, 82944 circular interval cases, texture/CLUT partial writes and wrap");
    return 0;
}

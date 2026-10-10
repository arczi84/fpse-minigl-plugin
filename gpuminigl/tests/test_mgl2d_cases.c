#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mgl2d_cases.h"

static void panel(unsigned char *rgb, unsigned y0, unsigned slot, unsigned pattern)
{
    unsigned x, y;
    for (y = 0; y < 160; ++y) for (x = 0; x < 256; ++x)
        m2_color(pattern, x / 32, y / 20,
            rgb + ((y0 + y) * M2_WIDTH + M2_X + slot * 256 + x) * 3);
}

/* Sensitivity check, NOT a hardware emulator: queue draws by texture ID and
 * optionally make Finish/Flush submit them. Verify that our case plans can
 * distinguish a wait-only Finish from a submit-and-wait Finish. */
static void model_submit(int content[3], int pending[2], int output[2])
{
    unsigned i;
    for (i = 0; i < 2; ++i) if (pending[i] >= 0) {
        output[i] = content[pending[i]]; pending[i] = -1;
    }
}
static int model(unsigned number, int finish_submits, int flush_submits)
{
    int content[3] = {-1,-1,-1}, pending[2] = {-1,-1}, output[2] = {-1,-1};
    unsigned i;
    for (i = 0; i < m2_cases[number].count; ++i) {
        const M2Step *s = &m2_cases[number].steps[i];
        switch (s->op) {
        case M2_IMAGE: case M2_SUB: case M2_SCRATCH_IMAGE:
            content[s->object] = s->pattern; break;
        case M2_DRAW: pending[s->slot] = s->object; break;
        case M2_PRESENT: model_submit(content, pending, output); break;
        case M2_FINISH: if (finish_submits) model_submit(content, pending, output); break;
        case M2_FLUSH: if (flush_submits) model_submit(content, pending, output); break;
        default: assert(0);
        }
    }
    model_submit(content, pending, output);
    return output[0] == 0 && output[1] == 1;
}
int main(void)
{
    unsigned char *rgb = calloc(1, M2_CAPTURE_BYTES), *atlas = malloc(512 * 256 * 4);
    unsigned i, j;
    assert(rgb && atlas);
    for (i = 0; i < M2_CASE_COUNT; ++i) {
        assert(m2_cases[i].count <= 8);
        for (j = 0; j < m2_cases[i].count; ++j) {
            assert(m2_cases[i].steps[j].op <= M2_SCRATCH_IMAGE);
            assert(m2_cases[i].steps[j].object < 3);
            assert(m2_cases[i].steps[j].slot < 2);
        }
    }
    assert(m2_cases[5].steps[2].op == M2_FINISH);
    assert(m2_cases[6].steps[2].op == M2_FLUSH && m2_cases[6].steps[3].op == M2_FINISH);
    assert(m2_cases[9].steps[2].op == M2_PRESENT && m2_cases[9].steps[3].op == M2_FINISH);
    assert(model(2, 0, 0) && model(3, 0, 0));
    assert(!model(4, 0, 0) && !model(5, 0, 0));
    assert(model(5, 1, 0));
    assert(!model(6, 0, 0) && model(6, 0, 1));
    assert(model(8, 0, 0) && model(9, 0, 0));
    for (i = 0; i < 2; ++i) {
        panel(rgb, M2_REFERENCE_Y, i, i); panel(rgb, M2_ACTUAL_Y, i, i);
        assert(!m2_mismatches(rgb, M2_REFERENCE_Y, i, i));
        assert(!m2_mismatches(rgb, M2_ACTUAL_Y, i, i));
    }
    panel(rgb, M2_ACTUAL_Y, 0, 1); /* duplicated right half */
    assert(m2_mismatches(rgb, M2_ACTUAL_Y, 0, 0) == 60);
    assert(!m2_mismatches(rgb, M2_ACTUAL_Y, 0, 1));
    assert(!m2_mismatches(rgb, M2_REFERENCE_Y, 0, 0));
    memset(rgb, 0, M2_CAPTURE_BYTES); /* unreadable/hidden window is NOT PASS */
    assert(m2_mismatches(rgb, M2_REFERENCE_Y, 0, 0) > 0);
    m2_pixels(atlas, 512, 0, 1);
    for (i = 0; i < 2; ++i) for (j = 0; j < 64; ++j) {
        unsigned char expected[3];
        const unsigned char *p = atlas + (((j / 8) * 32 + 16) * 512
            + i * 256 + (j % 8) * 32 + 16) * 4;
        m2_color(i, j % 8, j / 8, expected);
        assert(!memcmp(p, expected, 3) && p[3] == 255);
    }
    free(atlas); free(rgb);
    puts("PASS: MGL2D cases, asymmetric patterns, atlas, reference and duplicate-half classifier");
    return 0;
}

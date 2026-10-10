/* Run the actual state shadow against the old unconditional GL sequence.
 * Mock TexEnv as per-texture too: Classic/Warp3D requires this on binding.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef unsigned int GLuint;
enum {
    GL_BLEND, GL_SCISSOR_TEST, GL_TEXTURE_2D, GL_ALPHA_TEST,
    GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO,
    GL_ONE_MINUS_SRC_COLOR, GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,
    GL_MODULATE, GL_GREATER
};
typedef struct {
    int enabled[4], src, dst, x, y, w, h, alpha_func;
    float alpha_ref;
    GLuint texture;
    int env[16];
    unsigned int calls;
} FakeGL;
static FakeGL optimized, reference, *gl;
static void glEnable(int cap) { assert(cap < 4); gl->enabled[cap] = 1; ++gl->calls; }
static void glDisable(int cap) { assert(cap < 4); gl->enabled[cap] = 0; ++gl->calls; }
static void glBlendFunc(int src, int dst) { gl->src = src; gl->dst = dst; ++gl->calls; }
static void glScissor(int x, int y, int w, int h)
{ gl->x = x; gl->y = y; gl->w = w; gl->h = h; ++gl->calls; }
static void glBindTexture(int target, GLuint texture)
{ assert(target == GL_TEXTURE_2D && texture < 16); gl->texture = texture; ++gl->calls; }
static void glTexEnvi(int target, int name, int value)
{ assert(target == GL_TEXTURE_ENV && name == GL_TEXTURE_ENV_MODE); gl->env[gl->texture] = value; ++gl->calls; }
static void glAlphaFunc(int func, float ref)
{ gl->alpha_func = func; gl->alpha_ref = ref; ++gl->calls; }

#include "../draw_state.h"

static void old_apply(int mode, int textured, GLuint texture, int x, int y, int w, int h)
{
    if (mode == 4) glDisable(GL_BLEND);
    else {
        glEnable(GL_BLEND);
        switch (mode) {
        case 0: glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); break;
        case 1: glBlendFunc(GL_ONE, GL_ONE); break;
        case 2: glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR); break;
        case 3: glBlendFunc(GL_SRC_ALPHA, GL_ONE); break;
        }
    }
    glEnable(GL_SCISSOR_TEST); glScissor(x, y, w, h);
    if (textured) {
        glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, texture);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, 0.0f);
    } else {
        glDisable(GL_TEXTURE_2D); glDisable(GL_ALPHA_TEST);
    }
}

static void check(MGLDrawState *s, int mode, int textured, GLuint texture, int x, int y, int w, int h)
{
    gl = &optimized;
    mgl_draw_state_apply(s, mode, textured, texture, x, y, w, h);
    gl = &reference;
    old_apply(mode, textured, texture, x, y, w, h);
    assert(!memcmp(optimized.enabled, reference.enabled, sizeof(reference.enabled)));
    assert(optimized.x == reference.x && optimized.y == reference.y);
    assert(optimized.w == reference.w && optimized.h == reference.h);
    if (mode != 4) assert(optimized.src == reference.src && optimized.dst == reference.dst);
    if (textured) {
        assert(optimized.texture == reference.texture);
        assert(optimized.env[texture] == reference.env[texture]);
        assert(optimized.alpha_func == reference.alpha_func);
        assert(optimized.alpha_ref == reference.alpha_ref);
    }
}

int main(void)
{
    MGLDrawState s = {0};
    unsigned int before, old_before;
    int mode, tex, rect, i;
    check(&s, 0, 1, 1, 0, 0, 640, 480);
    before = optimized.calls; old_before = reference.calls;
    for (i = 0; i < 1000; ++i) check(&s, 0, 1, 1, 0, 0, 640, 480);
    assert(optimized.calls == before);
    assert(reference.calls - old_before == 9000);
    check(&s, 0, 1, 2, 0, 0, 640, 480);
    assert(optimized.calls - before == 2); /* bind plus Classic's per-object env */
    before = optimized.calls;
    check(&s, 1, 1, 2, 0, 0, 640, 480);
    assert(optimized.calls - before == 1); /* blend function, already enabled */
    before = optimized.calls;
    check(&s, 1, 1, 2, 2, 3, 320, 200);
    assert(optimized.calls - before == 1); /* scissor only */
    for (i = 0; i < 4; ++i)
        for (mode = 0; mode < 5; ++mode)
            for (tex = 0; tex < 8; ++tex)
                for (rect = 0; rect < 4; ++rect) {
                    /* Model an out-of-band upload/fill/clear/present/reset:
                     * GL state is arbitrary, including REPLACE and no alpha.
                     * Only invalidation is allowed to recover it. */
                    if ((mode + tex + rect) % 3 == 0) {
                        memset(&optimized, 0, sizeof(optimized));
                        memset(&reference, 0, sizeof(reference));
                        optimized.alpha_ref = reference.alpha_ref = 0.75f;
                        mgl_draw_state_invalidate(&s);
                    }
                    check(&s, mode, tex != 0, tex, rect, rect * 2, 320 + rect, 200 + rect);
                    before = optimized.calls;
                    check(&s, mode, tex != 0, tex, rect, rect * 2, 320 + rect, 200 + rect);
                    assert(optimized.calls == before);
                }
    puts("PASS: GL state matches old renderer; 1000 repeated batches eliminate 9000 state calls; bind/env, blend, scissor and invalidation");
    return 0;
}

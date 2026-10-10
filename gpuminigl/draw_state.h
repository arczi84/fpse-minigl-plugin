/* GL draw-state shadow. Include after GL declarations; no platform types.
 * The backend must invalidate it around out-of-band GL state changes.
 * In particular, a texture upload changes the binding, a VRAM rectangle
 * uses REPLACE without alpha/scissor, and present may touch driver state.
 */
#ifndef FPSE_MINIGL_DRAW_STATE_H
#define FPSE_MINIGL_DRAW_STATE_H
typedef struct {
    int valid, texture_defaults;
    int blend_mode, textured;
    int x, y, w, h;
    GLuint texture;
} MGLDrawState;

static __inline__ void mgl_draw_state_invalidate(MGLDrawState *s)
{
    s->valid = 0;
    s->texture_defaults = 0;
}

static __inline__ void mgl_draw_state_apply(MGLDrawState *s, int mode,
    int textured, GLuint texture, int x, int y, int w, int h)
{
    if (!s->valid || s->blend_mode != mode) {
        if (mode == 4) glDisable(GL_BLEND);
        else {
            if (!s->valid || s->blend_mode == 4) glEnable(GL_BLEND);
            switch (mode) {
            case 0: glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); break;
            case 1: glBlendFunc(GL_ONE, GL_ONE); break;
            case 2: glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR); break;
            case 3: glBlendFunc(GL_SRC_ALPHA, GL_ONE); break;
            }
        }
        s->blend_mode = mode;
    }
    if (!s->valid) glEnable(GL_SCISSOR_TEST);
    if (!s->valid || s->x != x || s->y != y || s->w != w || s->h != h) {
        glScissor(x, y, w, h);
        s->x = x; s->y = y; s->w = w; s->h = h;
    }
    if (!s->valid || s->textured != textured) {
        if (textured) {
            glEnable(GL_TEXTURE_2D);
            glEnable(GL_ALPHA_TEST);
        } else {
            glDisable(GL_TEXTURE_2D);
            glDisable(GL_ALPHA_TEST);
        }
        s->textured = textured;
    }
    if (textured) {
        if (!s->texture_defaults || s->texture != texture) {
            glBindTexture(GL_TEXTURE_2D, texture);
            /* Classic implements TexEnv per W3D texture object, so a
             * changed binding must get MODULATE even if the enum agrees. */
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            s->texture = texture;
        }
        if (!s->texture_defaults) {
            /* Alpha is also the 1/2 or 1/4 blend weight, not just a mask. */
            glAlphaFunc(GL_GREATER, 0.0f);
            s->texture_defaults = 1;
        }
    }
    s->valid = 1;
}
#endif

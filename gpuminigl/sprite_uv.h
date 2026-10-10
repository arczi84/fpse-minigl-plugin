/* Experimental sprite-only atlas-edge inset, in original PSX texel units.
 * Call ONLY for PSX sprite commands, never infer sprites from 3D polygons.
 * Geometry, alpha, blend state, texture contents and nearest are unchanged.
 * No platform types: the exact helper is exercised by host/68k tests. */
#ifndef FPSE_MINIGL_SPRITE_UV_H
#define FPSE_MINIGL_SPRITE_UV_H
typedef struct { float u0, v0, u1, v1; } MGLSpriteUV;

static __inline__ unsigned mgl_sprite_uv_inset(MGLSpriteUV *uv,
    float sx, float sy, int linear, int reduction, int window, int wrapped)
{
    unsigned axes = 0;
    float du = uv->u1 - uv->u0, dv = uv->v1 - uv->v0;
    if (!linear || reduction || window || wrapped ||
        uv->u0 < 0 || uv->u0 > 256 || uv->u1 < 0 || uv->u1 > 256 ||
        uv->v0 < 0 || uv->v0 > 256 || uv->v1 < 0 || uv->v1 > 256 ||
        (du > -1 && du < 1) || (dv > -1 && dv < 1)) return 0;
    if (sx > 1.0f) {
        float delta = du > 0 ? 0.5f : -0.5f;
        uv->u0 += delta; uv->u1 -= delta; axes |= 1;
    }
    if (sy > 1.0f) {
        float delta = dv > 0 ? 0.5f : -0.5f;
        uv->v0 += delta; uv->v1 -= delta; axes |= 2;
    }
    return axes;
}
#endif

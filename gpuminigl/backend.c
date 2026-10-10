/* Native Amiga MiniGL v23 display and renderer. GPL-2.0-or-later. */
#include "../gpuw3d/w3d/w3d.h"
#include "../gpuw3d/w3d/texture.h"
#include "pixels.h"
#ifdef FPSE_MINIGL_VRAM_GUTTERS
#include "vram_tile.h"
#endif
#include "profile.h"
#include "trace.h"
#include "draw_state.h"
#include "amidog/timer.h"
#include <proto/dos.h>
#include <math.h>
#ifdef FPSE_MINIGL_UI_CAPTURE
#include "ui_capture.h"
#ifdef FPSE_MINIGL_QUAD_SPRITES
#define UI_CAPTURE_VARIANT "E"
#else
#define UI_CAPTURE_VARIANT "D"
#endif
static void ui_capture_service(void)
{
    struct DateStamp stamp;
    char path[160];
    FILE *f;
    int attempt,ok;
    if (!mgl_ui_ready()) return;
    DateStamp(&stamp);
    for (attempt=0;attempt<100;++attempt) {
        snprintf(path,sizeof(path),"PROGDIR:fpse-ui-%s-%ld-%ld-%ld-%d.log",
            UI_CAPTURE_VARIANT,(long)stamp.ds_Days,(long)stamp.ds_Minute,(long)stamp.ds_Tick,attempt);
        f=fopen(path,"rb");
        if (!f) break;
        fclose(f);
    }
    if (attempt==100 || !(f=fopen(path,"w"))) {
        mgl_ui_done(0);puts("MiniGL UI: cannot create PROGDIR capture");return;
    }
    fprintf(f,"FPSE MiniGL %s with capture, GCC %s; MiniGL %u.%u flags=%08lx\n",UI_CAPTURE_VARIANT,__VERSION__,
        MiniGLBase->lib_Version,MiniGLBase->lib_Revision,(unsigned long)MiniGLDispatch->backendFlags);
    fprintf(f,"library_id=%s\n",MiniGLBase->lib_IdString?(char *)MiniGLBase->lib_IdString:"unknown");
    ok=mgl_ui_write(f);
    if (fflush(f)) ok=0;
    if (fclose(f)) ok=0;
    mgl_ui_done(ok);
    printf("MiniGL UI: %s %s\n",ok?"saved":"WRITE FAILED",path);
}
#endif
#ifdef FPSE_MINIGL_TITLE_FPS
#include "fps_counter.h"
#endif

extern int texfilter;
#undef PAL /* graphics/display.h collides with PSXDisplay.PAL */

typedef char fpse_requires_classic_enum_headers[
#if MINIGL_DISPATCH_ABI_VERSION >= 4
    MGL_ARRAY_TRANSFORMATIONS == 0x701D && MGL_LOCK_AUTOMATIC == 0x7010 ? 1 : -1];
#else
    MGL_ARRAY_TRANSFORMATIONS == 242 && MGL_LOCK_AUTOMATIC == 228 ? 1 : -1];
#endif

int frame_skip = 1, scale_gfx = SCALING_EXP, speed_limit = 1, hide_pointer = 0;
int amiga_use_screen = 0, resolution = 3, screenmode = 0;
int amiga_width, amiga_height, mgl_gpu_ready;
int mgl_gpu_packed_textures;
int plugin_is_master, plugin_is_slave;
long nf;
W3D_Float x_scale = 1, y_scale = 1;
PSXPoint_t W3D_Offset = {0, 0};
unsigned int W3D_Flags = 0; /* Single texture + separate opaque pass. */
MyVertex array[PRIM_BUFFER];
ULONG array_primitive, w3d_primitive, array_count, prim_count;
ULONG array_index[PRIM_BUFFER];
GLuint array_unit_tex[MAX_TEX_UNITS];

static GLcontext gpu_context;
static struct Window *window;
static amidog_timer_t *gpu_timer;
static amidog_timeval_t deadline;
static GLushort indices[PRIM_BUFFER];
static GLuint upload_texture;
static unsigned char *upload_pixels;
static int psx_width = 320, psx_height = 240, restore_display;
static int blend_mode = 4, scissor_x, scissor_y, scissor_w, scissor_h;
static int show_fps;
static unsigned short empty_pointer[12];
static MGLDrawState draw_state;
#ifdef FPSE_MINIGL_QUAD_SPRITES
unsigned long mgl_quad_sprite_candidates;
int mgl_quad_sprite_enabled = 1;
#endif
#ifdef FPSE_MINIGL_VRAM_GUTTERS
static unsigned long vram_gutter_tiles;
#endif
#ifdef FPSE_MINIGL_SPRITE_UV_AB
unsigned long mgl_sprite_uv_candidates;
#endif
#ifdef FPSE_MINIGL_TITLE_FPS
static MGLFpsCounter title_fps;
/* Intuition may retain the title pointer; never pass a stack buffer. */
static char fps_title[96];
static int fps_visible, fps_reset_pending;
#endif

/* ABI 3 native headers shift extension enums by four. ABI 4+ uses
 * explicit, shared extension values on both backends. */
static GLenum backend_enum(GLenum value)
{
#if MINIGL_DISPATCH_ABI_VERSION >= 4
    return value;
#else
    return (MiniGLDispatch->backendFlags & MINIGL_BACKEND_FLAG_PISTORM3D) ? value - 4 : value;
#endif
}

static int select_context(void)
{
    if (!gpu_context || !MiniGLDispatch) return 0;
    *MiniGLDispatch->currentContext = gpu_context;
    return 1;
}

static void set_arrays(void)
{
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(MyVertex), &array[0].x);
    glColorPointer(4, GL_FLOAT, sizeof(MyVertex), &array[0].primary_r);
    glTexCoordPointer(2, GL_FLOAT, sizeof(MyVertex), &array[0].u);
}

static void blend(int mode)
{
    if (mode == 4) {
        glDisable(GL_BLEND);
        return;
    }
    glEnable(GL_BLEND);
    switch (mode) {
    case 0: glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); break;
    case 1: glBlendFunc(GL_ONE, GL_ONE); break;
    /* No shared reverse-subtract API. Approximate D-S with D*(1-S). */
    case 2: glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR); break;
    case 3: glBlendFunc(GL_SRC_ALPHA, GL_ONE); break;
    }
}

void FLUSH_ARRAY(void)
{
    unsigned int i;
    unsigned long t;
    uint32_t measured;
    GLenum primitive;
    if (!prim_count) return;
    if (!select_context()) goto done;
    if (array_primitive == PRIM_TEXTURED && !array_unit_tex[0]) goto done;
    measured = mgl_profile_begin();
    t = mgl_trace_begin(MGL_T_STATE, array_primitive, array_unit_tex[0], blend_mode);
    mgl_draw_state_apply(&draw_state, blend_mode, array_primitive == PRIM_TEXTURED,
        array_unit_tex[0], scissor_x, scissor_y, scissor_w, scissor_h);
    mgl_trace_end(MGL_T_STATE, t, 0);
    primitive = w3d_primitive == W3D_PRIMITIVE_POINTS ? GL_POINTS :
                w3d_primitive == W3D_PRIMITIVE_LINES ? GL_LINES : GL_TRIANGLES;
    t = mgl_trace_begin(MGL_T_DRAW, primitive, prim_count, array_count);
    if (primitive == GL_TRIANGLES) {
        for (i = 0; i < prim_count; ++i) indices[i] = array_index[i];
        mgl_profile_end(MGL_P_STATE, measured);
        measured = mgl_profile_begin();
        glDrawElements(primitive, prim_count, GL_UNSIGNED_SHORT, indices);
    } else {
        /* Classic's indexed-array path implements triangles only. */
        mgl_profile_end(MGL_P_STATE, measured);
        measured = mgl_profile_begin();
        glBegin(primitive);
        for (i = 0; i < prim_count; ++i) {
            MyVertex *v = &array[array_index[i]];
            glColor4f(v->primary_r, v->primary_g, v->primary_b, v->primary_a);
            glVertex3f(v->x, v->y, 0.0f);
        }
        glEnd();
    }
    mgl_profile_end(MGL_P_DRAW, measured);
    mgl_trace_end(MGL_T_DRAW, t, 0);
done:
    array_count = prim_count = 0;
}

W3D_Float set_blend_mode(int *opaque_pass)
{
    int next = DrawSemiTrans && (!opaque_pass || *opaque_pass != 2) ? (GlobalTextABR & 3) : 4;
    if (opaque_pass && *opaque_pass == 1) *opaque_pass = 2;
    if (next != blend_mode) {
        FLUSH_ARRAY();
        blend_mode = next;
    }
    return next == 0 ? 0.5f : next == 3 ? 0.25f : 1.0f;
}

void OffsetSet(void)
{
    int ox, oy, left, right, top, bottom, sx, sy, sw, sh;
    if (!mgl_gpu_ready) return;
    ox = -PreviousPSXDisplay.DisplayPosition.x + PreviousPSXDisplay.Range.x0;
    oy = -PreviousPSXDisplay.DisplayPosition.y + PreviousPSXDisplay.Range.y0;
    left = drawX + ox; right = drawW + ox + 1;
    top = drawY + oy; bottom = drawH + oy + 1;
    if (right <= 0 || left >= psx_width) {
        ox = -PSXDisplay.DisplayPosition.x + PreviousPSXDisplay.Range.x0;
        left = drawX + ox; right = drawW + ox + 1;
    }
    if (bottom <= 0 || top >= psx_height) {
        oy = -PSXDisplay.DisplayPosition.y + PreviousPSXDisplay.Range.y0;
        top = drawY + oy; bottom = drawH + oy + 1;
    }
    sx = CLAMP((int)(left * x_scale), 0, amiga_width);
    sy = CLAMP((int)(top * y_scale), 0, amiga_height);
    sw = MAX(0, CLAMP((int)(right * x_scale), 0, amiga_width) - sx);
    sh = MAX(0, CLAMP((int)(bottom * y_scale), 0, amiga_height) - sy);
    sy = amiga_height - sy - sh;
    if (ox != W3D_Offset.x || oy != W3D_Offset.y || sx != scissor_x ||
        sy != scissor_y || sw != scissor_w || sh != scissor_h) {
        FLUSH_ARRAY();
        W3D_Offset.x = ox; W3D_Offset.y = oy;
        scissor_x = sx; scissor_y = sy; scissor_w = sw; scissor_h = sh;
    }
}

void amiga_clipping(void) { OffsetSet(); }

void amiga_set_size(unsigned long data)
{
    static const int widths[8] = {256,320,512,640,368,384,512,640};
    FLUSH_ARRAY();
    psx_width = widths[(data & 3) | ((data >> 4) & 4)];
    psx_height = (data & 4) ? 480 : 240;
    x_scale = (float)amiga_width / psx_width;
    y_scale = (float)amiga_height / psx_height;
    mgl_trace_log("PSX mode=%08lx size=%dx%d output=%dx%d", data, psx_width, psx_height, amiga_width, amiga_height);
    OffsetSet();
}

void mgl_gpu_texture_init(GLuint texture, int width, int height, const unsigned char *data)
{
    uint32_t measured = mgl_profile_begin();
    unsigned long t = mgl_trace_begin(MGL_T_TEXIMAGE, texture, width, height);
    mgl_draw_state_invalidate(&draw_state);
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texfilter ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texfilter ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    mgl_profile_end(MGL_P_TEX_UPLOAD, measured);
    mgl_trace_end(MGL_T_TEXIMAGE, t, 0);
}

void mgl_gpu_texture_wait(void)
{
    /* Classic's TexImage path updates the W3D texture in command order,
     * just like GPUW3D. WaitIdle additionally reads back the entire screen
     * on QuarkTex, which is not needed for a texture upload. Do not remove
     * the fence from native PiStorm: its deferred draws retain texels. */
    if (MiniGLDispatch->backendFlags & MINIGL_BACKEND_FLAG_PISTORM3D) {
        unsigned long t = mgl_trace_begin(MGL_T_FINISH, 1, 0, 0);
        glFinish();
        mgl_trace_end(MGL_T_FINISH, t, 0);
    }
}

int mgl_gpu_texture_upload(GLuint texture, int size, const void *data)
{
    static unsigned int errors;
    GLenum error;
    uint32_t measured;
    unsigned long t;
    if (!mgl_gpu_packed_textures) {
        mgl_gpu_texture_init(texture, size, size, data);
        goto check;
    }
    measured = mgl_profile_begin();
    t = mgl_trace_begin(MGL_T_TEXIMAGE, texture, size, size);
    mgl_draw_state_invalidate(&draw_state);
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    /* MiniGL's private packed type is ARGB4444, NOT GL's RGBA4444. */
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0,
                 GL_RGBA, MGL_UNSIGNED_SHORT_4_4_4_4, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texfilter ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texfilter ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    mgl_profile_end(MGL_P_TEX_UPLOAD, measured);
    mgl_trace_end(MGL_T_TEXIMAGE, t, 0);
check:
    error = glGetError();
    if (error != GL_NO_ERROR && errors++ < 12) {
        mgl_trace_log("texture upload error=%lu texture=%lu size=%d packed=%d", (unsigned long)error, (unsigned long)texture, size, mgl_gpu_packed_textures);
        printf("GPUMiniGL: texture upload failed, GL error %lu\n", (unsigned long)error);
    }
    return error == GL_NO_ERROR;
}

static void choose_texture_path(void)
{
    static const unsigned short probe_pixels[4] = {0xff00, 0xf0f0, 0xf00f, 0};
    GLuint probe = 0;
    GLenum error;
    mgl_draw_state_invalidate(&draw_state);
    mgl_gpu_packed_textures = 0;
    if (MiniGLDispatch->backendFlags & MINIGL_BACKEND_FLAG_PISTORM3D) return;
    /* QuarkMiniGL also advertises Classic but may lack private packed types.
     * Only select the 16-bit path after a successful, disposable upload. */
    error = glGetError();
    if (error != GL_NO_ERROR) {
        printf("GPUMiniGL: prior GL error %lu; using RGBA fallback\n", (unsigned long)error);
        return;
    }
    glGenTextures(1, &probe);
    if (!probe) return;
    glBindTexture(GL_TEXTURE_2D, probe);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0,
                 GL_RGBA, MGL_UNSIGNED_SHORT_4_4_4_4, probe_pixels);
    error = glGetError();
    glDeleteTextures(1, &probe);
    mgl_gpu_packed_textures = error == GL_NO_ERROR;
}

static void quad(float x0, float y0, float x1, float y1, float u1, float v1)
{
    unsigned long t = mgl_trace_begin(MGL_T_DRAW, GL_TRIANGLE_STRIP, 4, 4);
    if (t) mgl_trace_log("quad xy=%ld,%ld..%ld,%ld uv_end_65536=%ld,%ld", (long)x0, (long)y0, (long)x1, (long)y1, (long)(u1 * 65536), (long)(v1 * 65536));
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(0, 0); glVertex3f(x0, y0, 0);
    glTexCoord2f(0, v1); glVertex3f(x0, y1, 0);
    glTexCoord2f(u1, 0); glVertex3f(x1, y0, 0);
    glTexCoord2f(u1, v1); glVertex3f(x1, y1, 0);
    glEnd();
    mgl_trace_end(MGL_T_DRAW, t, 0);
}

#ifdef FPSE_MINIGL_VRAM_GUTTERS
static void vram_gutter_quad(float x0, float y0, float x1, float y1, int w, int h)
{
    float lo = 1.0f / MGL_VRAM_TILE_SIZE;
    float u1 = (w + 1.0f) / MGL_VRAM_TILE_SIZE;
    float v1 = (h + 1.0f) / MGL_VRAM_TILE_SIZE;
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(lo, lo); glVertex3f(x0, y0, 0);
    glTexCoord2f(lo, v1); glVertex3f(x0, y1, 0);
    glTexCoord2f(u1, lo); glVertex3f(x1, y0, 0);
    glTexCoord2f(u1, v1); glVertex3f(x1, y1, 0);
    glEnd();
}
#endif

/* Stream opaque VRAM rectangles (including zero/black pixels). The staging
 * texture is fixed-size and reused. Native texture lifetime is a separate
 * issue: gutters change sampling only, not the existing wait policy. */
static void upload_rect(int x, int y, int width, int height, int rgb24, int ox, int oy)
{
    int tx, ty, xx, yy, step = 256;
    uint32_t measured;
    unsigned long t, upload_t;
    if (!select_context() || !psxVub || !upload_pixels || width <= 0 || height <= 0) return;
    t = mgl_trace_begin(MGL_T_VRAM, width, height, rgb24);
    if (t) mgl_trace_log("VRAM src=%d,%d size=%dx%d offset=%d,%d psx=%dx%d output=%dx%d texture=%lu", x, y, width, height, ox, oy, psx_width, psx_height, amiga_width, amiga_height, (unsigned long)upload_texture);
    mgl_draw_state_invalidate(&draw_state);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_ALPHA_TEST);
    blend(4);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glColor4f(1, 1, 1, 1);
    width = MIN(width, 1024); height = MIN(height, 512);
#ifdef FPSE_MINIGL_VRAM_GUTTERS
    if (texfilter) step = MGL_VRAM_TILE_INNER;
#endif
    for (ty = 0; ty < height; ty += step) {
        for (tx = 0; tx < width; tx += step) {
            int w = MIN(step, width - tx), h = MIN(step, height - ty);
            float dx = (x + tx + ox) * x_scale, dy = (y + ty + oy) * y_scale;
            if (dx + w * x_scale <= 0 || dy + h * y_scale <= 0 ||
                dx >= amiga_width || dy >= amiga_height) continue;
            measured = mgl_profile_begin();
            mgl_gpu_texture_wait();
            mgl_profile_end(MGL_P_VRAM_WAIT, measured);
            measured = mgl_profile_begin();
#ifdef FPSE_MINIGL_VRAM_GUTTERS
            if (texfilter) {
                mgl_vram_tile_rgba(upload_pixels, psxVub, x, y, width, height, tx, ty, rgb24);
                ++vram_gutter_tiles;
            } else
#endif
            {
                memset(upload_pixels, 0, 256 * 256 * 4);
                for (yy = 0; yy < h; ++yy) {
                    for (xx = 0; xx < w; ++xx) {
                        unsigned char *d = upload_pixels + (yy * 256 + xx) * 4;
                        if (rgb24) {
                            unsigned int a = (((y + ty + yy) & 511) * 2048 + x * 2 + (tx + xx) * 3) & 0xfffff;
                            d[0] = psxVub[a]; d[1] = psxVub[(a + 1) & 0xfffff];
                            d[2] = psxVub[(a + 2) & 0xfffff];
                        } else {
                            unsigned int a = ((y + ty + yy) & 511) * 1024 + ((x + tx + xx) & 1023);
                            psx_rgba(psx_read16(psxVub, a), d, 0);
                        }
                        d[3] = 255;
                    }
                }
            }
            mgl_profile_end(MGL_P_VRAM_CONVERT, measured);
            measured = mgl_profile_begin();
            glBindTexture(GL_TEXTURE_2D, upload_texture);
            upload_t = mgl_trace_begin(MGL_T_TEXSUB, upload_texture, tx, ty);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, upload_pixels);
            mgl_trace_end(MGL_T_TEXSUB, upload_t, 0);
#ifdef FPSE_MINIGL_VRAM_GUTTERS
            if (texfilter) {
                /* Equal absolute expressions on shared edges avoid rounding
                 * differences between dx+w*scale and next_tile_x*scale. */
                vram_gutter_quad(dx, dy, (x + tx + w + ox) * x_scale,
                    (y + ty + h + oy) * y_scale, w, h);
            } else
#endif
                quad(dx, dy, dx + w * x_scale, dy + h * y_scale, w / 256.0f, h / 256.0f);
            mgl_profile_end(MGL_P_VRAM_UPLOAD, measured);
        }
    }
    mgl_trace_end(MGL_T_VRAM, t, 0);
}

void mgl_gpu_upload(int x, int y, int width, int height)
{
    if (!mgl_gpu_ready) return;
    FLUSH_ARRAY();
    OffsetSet();
    upload_rect(x, y, width, height, 0, W3D_Offset.x, W3D_Offset.y);
}

void mgl_gpu_fill(int x0, int y0, int x1, int y1, unsigned long rgb)
{
    int x, y;
    uint32_t measured;
    unsigned int p = ((rgb >> 3) & 31) | ((rgb >> 6) & 992) | ((rgb >> 9) & 31744);
    if (!mgl_gpu_ready) return;
    FLUSH_ARRAY();
    mgl_gpu_invalidate_area(x0, y0, x1 - x0, y1 - y0);
    OffsetSet();
    measured = mgl_profile_begin();
    /* GP0(02) ignores the drawing area and preserves a CPU copy for textures. */
    for (y = y0; y < y1 && y < y0 + 512; ++y)
        for (x = x0; x < x1 && x < x0 + 1024; ++x)
            SWAP16_W(p, psxVuw + ((y & 511) * 1024) + (x & 1023));
    select_context();
    mgl_draw_state_invalidate(&draw_state);
    glDisable(GL_SCISSOR_TEST); glDisable(GL_TEXTURE_2D); glDisable(GL_ALPHA_TEST);
    blend(4);
    glColor4f((rgb & 255) / 255.0f, ((rgb >> 8) & 255) / 255.0f, ((rgb >> 16) & 255) / 255.0f, 1);
    quad((x0 + W3D_Offset.x) * x_scale, (y0 + W3D_Offset.y) * y_scale,
         (x1 + W3D_Offset.x) * x_scale, (y1 + W3D_Offset.y) * y_scale, 0, 0);
    mgl_profile_end(MGL_P_FILL, measured);
}

void mgl_gpu_discard(void)
{
    mgl_draw_state_invalidate(&draw_state);
    array_count = prim_count = 0;
    blend_mode = 4;
    memset(array_unit_tex, 0, sizeof(array_unit_tex));
}

void mgl_gpu_restore(void)
{
    mgl_gpu_discard();
    TCacheInvalidate();
    restore_display = 1;
}

void amiga_clear(void)
{
    unsigned long t;
    if (!select_context()) return;
    FLUSH_ARRAY();
    mgl_draw_state_invalidate(&draw_state);
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    t = mgl_trace_begin(MGL_T_PRESENT, 1, 0, 0);
    mglSwitchDisplay();
    mgl_trace_end(MGL_T_PRESENT, t, 0);
}

void amiga_refresh(unsigned short *buf, int x, int y, int width, int height, int rgb24, double rate)
{
    uint32_t measured;
    unsigned long t, present_t;
    (void)buf; (void)rate;
    if (!select_context()) return;
    t = mgl_trace_begin(MGL_T_REFRESH, width, height, rgb24);
    if (t) mgl_trace_log("refresh src=%d,%d range=%d,%d display=%d,%d previous=%d,%d restore=%d", x, y, PreviousPSXDisplay.Range.x0, PreviousPSXDisplay.Range.y0, PSXDisplay.DisplayPosition.x, PSXDisplay.DisplayPosition.y, PreviousPSXDisplay.DisplayPosition.x, PreviousPSXDisplay.DisplayPosition.y, restore_display);
    FLUSH_ARRAY();
    if (width <= 0) width = psx_width;
    if (height <= 0) height = psx_height;
    if (rgb24 || restore_display) {
        mgl_draw_state_invalidate(&draw_state);
        glDisable(GL_SCISSOR_TEST);
        glClear(GL_COLOR_BUFFER_BIT);
        upload_rect(x, y, width, height, rgb24,
                    -x + PreviousPSXDisplay.Range.x0, -y + PreviousPSXDisplay.Range.y0);
        restore_display = 0;
    }
    measured = mgl_profile_begin();
    present_t = mgl_trace_begin(MGL_T_PRESENT, 2, width, height);
    mglSwitchDisplay();
    mgl_trace_end(MGL_T_PRESENT, present_t, 0);
#ifdef FPSE_MINIGL_UI_CAPTURE
    mgl_ui_boundary();
#endif
#ifdef FPSE_MINIGL_TITLE_FPS
    mgl_fps_present(&title_fps);
#endif
    /* Do not assume that the driver preserves GL state across present. */
    mgl_draw_state_invalidate(&draw_state);
    mgl_profile_end(MGL_P_PRESENT, measured);
    mgl_profile_frame();
    TCacheEndOfFrame(show_fps);
    OffsetSet();
    mgl_trace_end(MGL_T_REFRESH, t, 0);
}

void input_update(void)
{
    amidog_timeval_t now;
    int remaining, period;
    struct IntuiMessage *msg;
    if (!mgl_gpu_ready) return;
    if (window && window->UserPort) {
        while ((msg = (struct IntuiMessage *)GetMsg(window->UserPort))) {
            ULONG cls = msg->Class;
            UWORD code = msg->Code;
            ReplyMsg((struct Message *)msg);
            if (cls == IDCMP_RAWKEY && code == (0x80 | 0x59)) TCacheInvalidate();
#ifdef FPSE_MINIGL_QUAD_SPRITES
            if (cls == IDCMP_RAWKEY && code == (0x80 | 0x56)) {
                FLUSH_ARRAY();
                mgl_quad_sprite_enabled = !mgl_quad_sprite_enabled;
            }
#endif
#ifdef FPSE_MINIGL_UI_CAPTURE
            if (cls == IDCMP_RAWKEY && code == (0x80 | 0x57)) mgl_ui_arm();
#endif
#ifdef FPSE_MINIGL_TITLE_FPS
            if (cls == IDCMP_RAWKEY && code == (0x80 | 0x58)) {
                fps_visible = !fps_visible;
                fps_reset_pending = 1;
                SetWindowTitles(window, (UBYTE *)(fps_visible ?
                    "FPSE - MiniGL | FPS -- | emu --" : "FPSE - MiniGL"), (UBYTE *)-1);
            }
#elif !defined(FPSE_MINIGL_NO_PROFILE)
            if (cls == IDCMP_RAWKEY && code == (0x80 | 0x58)) {
                show_fps = !show_fps;
                mgl_profile_toggle();
                printf("GPUMiniGL: rendered frames=%ld, PSX mode=%dx%d\n", nf, psx_width, psx_height);
            }
#endif
        }
    }
#ifdef FPSE_MINIGL_UI_CAPTURE
    ui_capture_service();
#endif
    timer_gettime(gpu_timer, &now);
#ifdef FPSE_MINIGL_TITLE_FPS
    if (fps_reset_pending) {
        mgl_fps_reset(&title_fps, (uint32_t)now.sec, (uint32_t)now.usec, (uint32_t)nf);
        fps_reset_pending = 0;
    }
    if (mgl_fps_sample(&title_fps, (uint32_t)now.sec, (uint32_t)now.usec, (uint32_t)nf)
            && fps_visible && window) {
        mgl_fps_title(&title_fps, fps_title, sizeof(fps_title));
#ifdef FPSE_MINIGL_SPRITE_UV_AB
        {
            size_t len = strlen(fps_title);
#ifdef FPSE_MINIGL_VRAM_GUTTERS
#ifdef FPSE_MINIGL_UI_CAPTURE
#ifdef FPSE_MINIGL_QUAD_SPRITES
            snprintf(fps_title + len, sizeof(fps_title) - len, " | E:%s Q:%lu LOG:%s",
                mgl_quad_sprite_enabled?"ON":"OFF",mgl_quad_sprite_candidates,mgl_ui_status());
#else
            snprintf(fps_title + len, sizeof(fps_title) - len, " | UI-D:%s",mgl_ui_status());
#endif
#elif defined(FPSE_MINIGL_QUAD_SPRITES)
            snprintf(fps_title + len, sizeof(fps_title) - len, " | E:%s Q:%lu",
                mgl_quad_sprite_enabled?"ON":"OFF",mgl_quad_sprite_candidates);
#else
            snprintf(fps_title + len, sizeof(fps_title) - len, " | UV-C:%lu BG:%lu",
                mgl_sprite_uv_candidates, vram_gutter_tiles);
#endif
#else
            snprintf(fps_title + len, sizeof(fps_title) - len, " | UV-%c:%lu",
                FPSE_MINIGL_SPRITE_UV_AB ? 'B' : 'A', mgl_sprite_uv_candidates);
#endif
        }
#endif
        SetWindowTitles(window, (UBYTE *)fps_title, (UBYTE *)-1);
    }
#endif
    period = PSXDisplay.PAL ? 20000 : 16683;
    remaining = (int)(deadline.sec - now.sec) * 1000000 + (int)deadline.usec - (int)now.usec;
    if (!speed_limit || remaining < -period * 4 || remaining > period * 4) deadline = now;
    else if (remaining > 0) {
        uint32_t measured = mgl_profile_begin();
        unsigned long t = mgl_trace_begin(MGL_T_LIMIT, remaining, period, 0);
        timer_usleep(gpu_timer, remaining);
        mgl_trace_end(MGL_T_LIMIT, t, 0);
        mgl_profile_end(MGL_P_LIMIT, measured);
    }
    deadline.usec += period;
    deadline.sec += deadline.usec / 1000000;
    deadline.usec %= 1000000;
}

int amiga_open(void)
{
    static const int sizes[][2] = {{640,480},{320,240},{512,384},{640,480},{800,600},{1024,768},{1280,1024},{1600,1200}};
#ifdef FPSE_MINIGL_QUAD_SPRITES
    mgl_quad_sprite_candidates = 0; mgl_quad_sprite_enabled = 1;
#endif
#ifdef FPSE_MINIGL_UI_CAPTURE
    mgl_ui_reset();
#endif
#ifdef FPSE_MINIGL_SPRITE_UV_AB
    mgl_sprite_uv_candidates = 0;
#endif
#ifdef FPSE_MINIGL_VRAM_GUTTERS
    vram_gutter_tiles = 0;
#endif
    mgl_trace_log("amiga_open ENTER");
    if (!MiniGLOpen()) {
        puts("GPUMiniGL: cannot open compatible MiniGL dispatch ABI 3 (PiStorm3D v14+ / Classic v23).");
        return FPSE_ERR;
    }
    if (*MiniGLDispatch->currentContext) {
        mgl_trace_log("FAIL existing context=%08lx", (unsigned long)*MiniGLDispatch->currentContext);
        puts("GPUMiniGL: another MiniGL context is active; close that application first.");
        goto fail;
    }
    resolution = CLAMP(resolution, 0, 7);
    frame_skip = CLAMP(frame_skip, 1, 10);
    scale_gfx = SCALING_EXP;
    amiga_width = sizes[resolution][0]; amiga_height = sizes[resolution][1];
    mgl_trace_log("context setup ENTER size=%dx%d fullscreen=%d mode=%08lx", amiga_width, amiga_height, amiga_use_screen, (unsigned long)screenmode);
    mglChooseWindowMode(!amiga_use_screen);
    mglChoosePixelDepth(16);
    /* PSX may update only a portion of a frame; keep the rendering surface. */
    mglChooseNumberOfBuffers(1);
    mglChooseVertexBufferSize(PRIM_BUFFER + 16);
    mglProhibitMipMapping(GL_TRUE);
    if (amiga_use_screen && resolution == 0 && screenmode) {
        GLint w = amiga_width, h = amiga_height;
        gpu_context = mglCreateContextFromID(screenmode, &w, &h);
        amiga_width = w; amiga_height = h;
    } else gpu_context = mglCreateContext(0, 0, amiga_width, amiga_height);
    mgl_trace_log("context setup LEAVE context=%08lx size=%dx%d", (unsigned long)gpu_context, amiga_width, amiga_height);
    if (!gpu_context) { puts("GPUMiniGL: context creation failed."); goto fail; }
    window = (struct Window *)mglGetInputWindowHandle();
    if (!window) window = (struct Window *)mglGetWindowHandle();
    if (window) {
        ModifyIDCMP(window, window->IDCMPFlags | IDCMP_RAWKEY);
        SetWindowTitles(window, (UBYTE *)"FPSE - MiniGL", (UBYTE *)-1);
        if (hide_pointer) SetPointer(window, empty_pointer, 1, 16, 0, 0);
    }
    mgl_trace_log("timer_init ENTER");
    gpu_timer = timer_init();
    mgl_trace_log("timer_init LEAVE timer=%08lx", (unsigned long)gpu_timer);
    if (!gpu_timer) { puts("GPUMiniGL: timer.device initialization failed."); goto fail; }
    mgl_profile_init(gpu_timer);
    mgl_profile_identify(MiniGLBase->lib_Version, MiniGLBase->lib_Revision,
                         MiniGLDispatch->backendFlags, amiga_width, amiga_height);
    mgl_trace_log("choose_texture_path ENTER");
    choose_texture_path();
    mgl_trace_log("choose_texture_path LEAVE packed=%d native_fence=%d", mgl_gpu_packed_textures, !!(MiniGLDispatch->backendFlags & MINIGL_BACKEND_FLAG_PISTORM3D));
    mgl_profile_texture_path(mgl_gpu_packed_textures,
        !!(MiniGLDispatch->backendFlags & MINIGL_BACKEND_FLAG_PISTORM3D));
    upload_pixels = calloc(256 * 256, 4);
    mgl_trace_log("upload_pixels=%08lx bytes=262144", (unsigned long)upload_pixels);
    if (!upload_pixels) goto fail;
    mgl_trace_log("glGenTextures ENTER staging");
    glGenTextures(1, &upload_texture);
    mgl_trace_log("glGenTextures LEAVE staging=%lu", (unsigned long)upload_texture);
    if (!upload_texture) goto fail;
    mgl_gpu_texture_init(upload_texture, 256, 256, upload_pixels);
    if (TCacheInit() != FPSE_OK) goto fail;
    mgl_trace_log("GL setup ENTER lock_token=%lu transform_token=%lu", (unsigned long)backend_enum(MGL_LOCK_AUTOMATIC), (unsigned long)backend_enum(MGL_ARRAY_TRANSFORMATIONS));
    mglEnableSync(GL_FALSE);
    mglLockMode(backend_enum(MGL_LOCK_AUTOMATIC));
    glViewport(0, 0, amiga_width, amiga_height);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glOrtho(0, amiga_width, amiga_height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE); glDisable(GL_FOG);
    glEnable(backend_enum(MGL_ARRAY_TRANSFORMATIONS));
    /* Client array addresses never change. Like GPUW3D's pointer setup,
     * configure once per context rather than six calls for every batch. */
    set_arrays();
    glShadeModel(GL_SMOOTH);
    mgl_trace_log("GL setup LEAVE");
    psx_width = 320; psx_height = 240;
    x_scale = amiga_width / 320.0f; y_scale = amiga_height / 240.0f;
    scissor_x = scissor_y = 0; scissor_w = amiga_width; scissor_h = amiga_height;
    W3D_Offset.x = W3D_Offset.y = 0;
    mgl_gpu_discard();
    mgl_gpu_ready = 1;
    restore_display = show_fps = 0; nf = 0;
    timer_gettime(gpu_timer, &deadline);
#ifdef FPSE_MINIGL_TITLE_FPS
    mgl_fps_reset(&title_fps, (uint32_t)deadline.sec, (uint32_t)deadline.usec, 0);
    fps_visible = 1; fps_reset_pending = 0;
    if (window) SetWindowTitles(window,
        (UBYTE *)"FPSE - MiniGL | FPS -- | emu --", (UBYTE *)-1);
#endif
    mgl_trace_log("initial clear ENTER");
    amiga_clear();
    mgl_trace_log("initial clear LEAVE; amiga_open ready");
#ifndef FPSE_MINIGL_NO_PROFILE
    /* Only the separate diagnostic DLL starts timing and PROGDIR logging. */
    mgl_profile_toggle();
    show_fps = 1;
#endif
    printf("GPUMiniGL: library %u, %s, %dx%d\n", MiniGLBase->lib_Version,
           (MiniGLDispatch->backendFlags & MINIGL_BACKEND_FLAG_PISTORM3D) ? "PiStorm3D" : "Classic", amiga_width, amiga_height);
    return FPSE_OK;
fail:
    mgl_trace_log("amiga_open FAIL");
    amiga_close();
    return FPSE_ERR;
}

void amiga_close(void)
{
#ifdef FPSE_MINIGL_UI_CAPTURE
    ui_capture_service();
    mgl_ui_reset();
#endif
    mgl_trace_log("amiga_close ENTER");
    mgl_profile_close();
    mgl_gpu_discard();
    mgl_gpu_ready = 0;
    if (select_context()) {
        unsigned long t = mgl_trace_begin(MGL_T_FINISH, 2, 0, 0);
        glFinish();
        mgl_trace_end(MGL_T_FINISH, t, 0);
        mgl_trace_log("TCacheExit ENTER");
        TCacheExit();
        mgl_trace_log("TCacheExit LEAVE");
        if (upload_texture) glDeleteTextures(1, &upload_texture);
        if (window && hide_pointer) ClearPointer(window);
        mgl_trace_log("mglDeleteContext ENTER");
        mglDeleteContext();
        mgl_trace_log("mglDeleteContext LEAVE");
    }
    gpu_context = NULL; window = NULL; upload_texture = 0;
    free(upload_pixels); upload_pixels = NULL;
    timer_exit(gpu_timer); gpu_timer = NULL;
    MiniGLClose();
    mgl_trace_log("amiga_close LEAVE");
}

void GPU_InfoDisplay(GPU_InfoDisplayType *info)
{
    if (info && info->text) printf("FPSE: %s\n", info->text);
}

void GPU_ScreenShot(char *path)
{
    if (path && select_context()) {
        FLUSH_ARRAY(); glFinish(); mglWriteShotPPM(path);
        mgl_draw_state_invalidate(&draw_state);
    }
}

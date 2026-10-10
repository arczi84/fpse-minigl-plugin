/* Included by mgl2d.c: shares context, PROGDIR log and RTG capture helpers.
 * Never overwrite a texture after submitting a draw that uses it. */
#include "mglalpha_pattern.h"

typedef struct { const char *name; int kind, blend, inset; float cutoff; } MAExample;
static const MAExample ma_examples[] = {
    {"black RGB / alpha GT 0 / no blend", 0, 0, 0, 0.0f},
    {"black RGB / alpha GT 0.5 / no blend", 0, 0, 0, 0.5f},
    {"gold RGB in transparent texels / no blend", 1, 0, 0, 0.0f},
    {"black RGB / straight alpha blending", 0, 1, 0, 0.0f},
    {"gold RGB / straight alpha blending", 1, 1, 0, 0.0f},
    {"atlas / black neighbour / edge UV", 2, 0, 0, 0.0f},
    {"atlas / black neighbour / inset UV", 2, 0, 1, 0.0f}
};
#define MA_COUNT ((unsigned)(sizeof(ma_examples) / sizeof(ma_examples[0])))

static void ma_label(const char *s, float x, float y)
{
    unsigned row, col;
    glDisable(GL_TEXTURE_2D); glDisable(GL_ALPHA_TEST); glDisable(GL_BLEND);
    glColor4f(1, 1, 1, 1);
    for (; *s; ++s, x += 12) for (row = 0; row < 7; ++row)
        for (col = 0; col < 5; ++col) if (ma_glyph(*s, row) & (16 >> col))
            rectangle(x + col*2, y + row*2, x + col*2 + 2, y + row*2 + 2, 0);
}

static void ma_quad(float x, float lo, float hi)
{
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(lo,lo); glVertex3f(x,80,0);
    glTexCoord2f(lo,hi); glVertex3f(x,400,0);
    glTexCoord2f(hi,lo); glVertex3f(x+288,80,0);
    glTexCoord2f(hi,hi); glVertex3f(x+288,400,0);
    glEnd();
}

static int ma_run(void)
{
    unsigned n, slot, errors = 0, captures = 0;
    report("ALPHA examples: LEFT NEAREST, RIGHT LINEAR; synthetic TOMB TEST III logo.");
    report("Visual demonstrations, NOT driver PASS/FAIL tests. Halos may be valid filtering results.");
    report("00/01 alpha cutoff; 00/02 transparent RGB; 03/04 blended RGB; 05/06 atlas UV.");
    report("Both texture objects and both CPU sources remain unchanged until presentation.");
    for (n = 0; n < MA_COUNT; ++n) {
        const MAExample *e = &ma_examples[n];
        GLenum error;
        float lo = e->kind == 2 ? (e->inset ? 8.5f : 8.0f)/MA_SIZE : 0;
        float hi = e->kind == 2 ? (e->inset ? 119.5f : 120.0f)/MA_SIZE : 1;
        report("EXAMPLE %02u: %s; new context; kind=%d blend=%d cutoff=%ld/100 UV=%ld..%ld/65536",
            n, e->name, e->kind, e->blend, (long)(e->cutoff*100),
            (long)(lo*65536), (long)(hi*65536));
        if (!open_context()) { report("CONTEXT INIT FAIL"); return 20; }
        ma_pattern(source[0], e->kind); ma_pattern(source[1], e->kind);
        /* Upload BOTH objects before any textured draw. No lifetime test here. */
        for (slot = 0; slot < 2; ++slot) {
            glBindTexture(GL_TEXTURE_2D, textures[slot]);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, MA_SIZE, MA_SIZE, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, source[slot]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, slot ? GL_LINEAR : GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, slot ? GL_LINEAR : GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }
        glClearColor(0.04f,0.06f,0.12f,1); glClear(GL_COLOR_BUFFER_BIT);
        ma_label("NEAREST", 118, 24); ma_label("LINEAR", 444, 24);
        ma_label(e->kind == 2 ? "ATLAS EDGES" : "ALPHA EDGES", 254, 440);
        glColor4f(48/255.0f,96/255.0f,176/255.0f,1);
        rectangle(16,80,304,400,0); rectangle(336,80,624,400,0);
        for (slot = 0; slot < 2; ++slot) {
            glEnable(GL_TEXTURE_2D); glEnable(GL_ALPHA_TEST);
            glAlphaFunc(GL_GREATER, e->cutoff);
            if (e->blend) {
                glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
            } else glDisable(GL_BLEND);
            glBindTexture(GL_TEXTURE_2D,textures[slot]);
            glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
            glColor4f(1,1,1,1);
            ma_quad(slot ? 336 : 16,lo,hi);
        }
        mglSwitchDisplay(); glFinish();
        error = glGetError();
        report("EXAMPLE %02u GL error=%lu (image is VISUAL ONLY)",n,(unsigned long)error);
        if (error != GL_NO_ERROR) ++errors;
        snprintf(title,sizeof(title),"MGL2D ALPHA %02u | %s | Space=next Esc=stop",n,e->name);
        SetWindowTitles(test_window,(UBYTE *)title,(UBYTE *)-1);
        Delay(10);
        if (CyberGfxBase) {
            ULONG read;
            memset(capture,0x71,M2_CAPTURE_BYTES);
            read = ReadPixelArray(capture,0,0,M2_WIDTH*3,test_window->RPort,
                test_window->BorderLeft,test_window->BorderTop,M2_WIDTH,M2_HEIGHT,RECTFMT_RGB);
            report("RTG capture returned %lu; keep window unobscured",(unsigned long)read);
            if (read == M2_WIDTH*M2_HEIGHT) {
                if (no_ppm || save_ppm(n)) ++captures;
            } else report("CAPTURE INCOMPLETE: no screenshot saved");
        } else report("NO CAPTURE: cybergraphics.library unavailable");
        if (!wait_case()) { report("STOPPED by user"); break; }
        close_context();
    }
    report("ALPHA SUMMARY examples=%u/%u GL_errors=%u captures=%u; no automatic image verdict",
        n < MA_COUNT ? n+1 : n,MA_COUNT,errors,captures);
    return errors || captures != MA_COUNT || n != MA_COUNT ? 5 : 0;
}

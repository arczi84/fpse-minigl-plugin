/* Standalone black-box test: public MiniGL ABI only, no FPSE or AHI code.
 * Capture the visible RTG window through CyberGraphX, not glReadPixels. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <exec/libraries.h>
#include <intuition/intuitionbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h>
#include <proto/minigl.h>
#include "mgl2d_cases.h"

struct IntuitionBase *IntuitionBase;
struct Library *CyberGfxBase;
static struct Window *test_window;
static GLcontext test_context;
static GLuint textures[3];
static unsigned char *source[2], *atlas, *scratch, *blank, *capture;
static char stem[128], log_path[160], title[160];
static int linear_filter, manual, no_ppm, log_failed;
static const char version[] = "$VER: MGL2DTest 0.2 (20.09.2026)";

static void report(const char *format, ...)
{
    char text[768];
    FILE *f;
    va_list ap;
    va_start(ap, format); vsnprintf(text, sizeof(text), format, ap); va_end(ap);
    puts(text); fflush(stdout);
    f = fopen(log_path, "a");
    if (!f) log_failed = 1;
    else {
        if (fprintf(f, "%s\n", text) < 0 || fflush(f)) log_failed = 1;
        if (fclose(f)) log_failed = 1;
    }
}

static GLenum native_enum(GLenum e)
{
    return MiniGLDispatch->backendFlags & MINIGL_BACKEND_FLAG_PISTORM3D ? e - 4 : e;
}

static void rectangle(float x0, float y0, float x1, float y1, int textured)
{
    glBegin(GL_TRIANGLE_STRIP);
    if (textured) glTexCoord2f(0, 0);
    glVertex3f(x0, y0, 0);
    if (textured) glTexCoord2f(0, 1);
    glVertex3f(x0, y1, 0);
    if (textured) glTexCoord2f(1, 0);
    glVertex3f(x1, y0, 0);
    if (textured) glTexCoord2f(1, 1);
    glVertex3f(x1, y1, 0);
    glEnd();
}

static void reference_tile(unsigned slot, unsigned pattern, unsigned y0)
{
    unsigned x, y;
    glDisable(GL_TEXTURE_2D);
    for (y = 0; y < 8; ++y) for (x = 0; x < 8; ++x) {
        unsigned char rgb[3];
        float dx = M2_X + slot * 256 + x * 32, dy = y0 + y * 20;
        m2_color(pattern, x, y, rgb);
        glColor4f(rgb[0] / 255.0f, rgb[1] / 255.0f, rgb[2] / 255.0f, 1);
        rectangle(dx, dy, dx + 32, dy + 20, 0);
    }
}

static void texture_parameters(void)
{
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, linear_filter ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, linear_filter ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

static void image(unsigned object, const unsigned char *data, int width, int sub)
{
    glBindTexture(GL_TEXTURE_2D, textures[object]);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if (sub) glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, 256, GL_RGBA, GL_UNSIGNED_BYTE, data);
    else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        texture_parameters();
    }
}

static void textured_tile(unsigned object, unsigned slot, int width)
{
    float x = M2_X + slot * 256;
    glDisable(GL_SCISSOR_TEST); glDisable(GL_ALPHA_TEST); glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textures[object]);
    /* Set AFTER binding: do not depend on per-texture versus per-unit TexEnv. */
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glColor4f(1, 1, 1, 1);
    rectangle(x, M2_ACTUAL_Y, x + width, M2_ACTUAL_Y + M2_PANEL_HEIGHT, 1);
}

static int open_context(void)
{
    unsigned i;
    memset(textures, 0, sizeof(textures));
    mglChooseWindowMode(GL_TRUE); mglChoosePixelDepth(16);
    mglChooseNumberOfBuffers(1); mglChooseVertexBufferSize(2048);
    mglProhibitMipMapping(GL_TRUE);
    test_context = mglCreateContext(0, 0, M2_WIDTH, M2_HEIGHT);
    if (!test_context) return 0;
    *MiniGLDispatch->currentContext = test_context;
    test_window = (struct Window *)mglGetWindowHandle();
    if (!test_window) return 0;
    if (test_window->Width - test_window->BorderLeft - test_window->BorderRight != M2_WIDTH ||
        test_window->Height - test_window->BorderTop - test_window->BorderBottom != M2_HEIGHT) {
        report("INVALID: window inner size is not 640x480"); return 0;
    }
    ModifyIDCMP(test_window, test_window->IDCMPFlags | IDCMP_RAWKEY | IDCMP_CLOSEWINDOW);
    WindowToFront(test_window); ActivateWindow(test_window);
    mglEnableSync(GL_FALSE); mglLockMode(native_enum(MGL_LOCK_AUTOMATIC));
    glViewport(0, 0, M2_WIDTH, M2_HEIGHT);
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, M2_WIDTH, M2_HEIGHT, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glEnable(native_enum(MGL_ARRAY_TRANSFORMATIONS));
    glDisable(GL_DEPTH_TEST); glDepthMask(GL_FALSE); glDisable(GL_CULL_FACE);
    glDisable(GL_FOG); glDisable(GL_BLEND); glDisable(GL_ALPHA_TEST); glDisable(GL_SCISSOR_TEST);
    glShadeModel(GL_SMOOTH);
    glGenTextures(3, textures);
    for (i = 0; i < 3; ++i) {
        if (!textures[i]) return 0;
        /* A 512-wide atlas is exercised ONLY by case 01. Its failure must
         * not prevent the 256-wide controls from running. */
        image(i, blank, 256, 0);
    }
    glClearColor(0.08f, 0.08f, 0.08f, 1);
    return 1;
}

static void close_context(void)
{
    if (test_context) {
        report("CLOSE context ENTER");
        glFinish(); glDeleteTextures(3, textures); mglDeleteContext();
        *MiniGLDispatch->currentContext = NULL;
        test_context = NULL; test_window = NULL;
        report("CLOSE context LEAVE");
    }
}

static void sequence(unsigned number)
{
    unsigned i;
    const M2Case *test = &m2_cases[number];
    glClear(GL_COLOR_BUFFER_BIT);
    reference_tile(0, 0, M2_REFERENCE_Y); reference_tile(1, 1, M2_REFERENCE_Y);
    /* No logging, disk I/O, delays or GetError between these operations. */
    for (i = 0; i < test->count; ++i) {
        const M2Step *s = &test->steps[i];
        switch (s->op) {
        case M2_IMAGE: image(s->object, source[s->pattern], 256, 0); break;
        case M2_SUB: image(s->object, source[s->pattern], 256, 1); break;
        case M2_DRAW: textured_tile(s->object, s->slot, 256); break;
        case M2_FINISH: glFinish(); break;
        case M2_FLUSH: glFlush(); break;
        case M2_PRESENT: mglSwitchDisplay(); break;
        case M2_ATLAS: image(2, atlas, 512, 0); textured_tile(2, 0, 512); break;
        case M2_SOLID: reference_tile(s->slot, s->pattern, M2_ACTUAL_Y); break;
        case M2_SCRATCH_IMAGE:
            memcpy(scratch, source[s->pattern], 256 * 256 * 4);
            image(s->object, scratch, 256, 0); break;
        }
    }
    mglSwitchDisplay(); glFinish();
}

static int save_ppm(unsigned number)
{
    char path[160];
    FILE *f;
    int ok;
    snprintf(path, sizeof(path), "%s-%02u.ppm", stem, number);
    f = fopen(path, "wb");
    if (!f) { report("PPM open failed: %s", path); return 0; }
    ok = fprintf(f, "P6\n%d %d\n255\n", M2_WIDTH, M2_HEIGHT) > 0;
    if (fwrite(capture, 1, M2_CAPTURE_BYTES, f) != M2_CAPTURE_BYTES) ok = 0;
    if (fclose(f)) ok = 0;
    report("PPM %s: %s", ok ? "saved" : "write failed", path);
    return ok;
}

/* Capture validation is independent of GL readback. Never mark an unreadable
 * or obscured reference as a texture failure. Return -1 for inconclusive. */
static int inspect(unsigned number, int geometry_ok, int present_control_ok)
{
    unsigned ref0, ref1, left, right, left_as_b, right_as_a;
    ULONG read;
    if (!CyberGfxBase) { report("UNVERIFIED: cybergraphics.library unavailable"); return -1; }
    memset(capture, 0x71, M2_CAPTURE_BYTES);
    read = ReadPixelArray(capture, 0, 0, M2_WIDTH * 3, test_window->RPort,
        test_window->BorderLeft, test_window->BorderTop, M2_WIDTH, M2_HEIGHT, RECTFMT_RGB);
    report("RTG capture returned %lu", (unsigned long)read);
    if (!read) { report("UNVERIFIED: RTG capture failed"); return -1; }
    if (!no_ppm) save_ppm(number);
    ref0 = m2_mismatches(capture, M2_REFERENCE_Y, 0, 0);
    ref1 = m2_mismatches(capture, M2_REFERENCE_Y, 1, 1);
    left = m2_mismatches(capture, M2_ACTUAL_Y, 0, 0);
    right = m2_mismatches(capture, M2_ACTUAL_Y, 1, 1);
    left_as_b = m2_mismatches(capture, M2_ACTUAL_Y, 0, 1);
    right_as_a = m2_mismatches(capture, M2_ACTUAL_Y, 1, 0);
    report("SAMPLES bad/64: reference A=%u B=%u; actual A=%u B=%u; left_as_B=%u right_as_A=%u",
        ref0, ref1, left, right, left_as_b, right_as_a);
    if (ref0 || ref1 || (!geometry_ok && number != 0)) {
        report("INVALID: reference/geometry/capture failed; keep window visible, no texture conclusion");
        return -1;
    }
    if (number == 9 && !present_control_ok) {
        report("INVALID: case 08 did not verify surface preservation across Present"); return -1;
    }
    if (left || right) {
        report("IMAGE FAIL%s%s", left && !left_as_b ? ": LEFT replaced by B" : "",
            right && !right_as_a ? ": RIGHT replaced by A" : "");
        return 0;
    }
    report("IMAGE PASS: all 128 actual samples match the CPU reference");
    return 1;
}

static int wait_case(void)
{
    unsigned ticks = 0;
    for (;;) {
        struct IntuiMessage *msg;
        if (SetSignal(0, 0) & SIGBREAKF_CTRL_C) return 0;
        if (test_window->UserPort) while ((msg = (struct IntuiMessage *)GetMsg(test_window->UserPort))) {
            ULONG cls = msg->Class; UWORD code = msg->Code;
            ReplyMsg((struct Message *)msg);
            if (cls == IDCMP_CLOSEWINDOW || (cls == IDCMP_RAWKEY && code == 0xc5)) return 0;
            if (cls == IDCMP_RAWKEY && (code == 0xc0 || code == 0xc4)) return 1;
        }
        if (!manual && ticks++ >= 100) return 1;
        Delay(1);
    }
}

#include "mglalpha_scene.h"

int main(void)
{
    LONG options[4] = {0,0,0,0};
    struct RDArgs *args;
    struct DateStamp date;
    unsigned n, pass = 0, fail = 0, unverified = 0;
    int result = 20, opened = 0, geometry_ok = 0, present_control_ok = 0;
    args = ReadArgs((CONST_STRPTR)"MANUAL/S,LINEAR/S,NOPPM/S,ALPHA/S", options, NULL);
    if (!args) { PrintFault(IoErr(), (CONST_STRPTR)"MGL2DTest"); return 20; }
    manual = options[0] != 0; linear_filter = options[1] != 0; no_ppm = options[2] != 0;
    DateStamp(&date);
    snprintf(stem, sizeof(stem), "PROGDIR:MGL2DTest%s-%ld-%ld-%ld", options[3] ? "-alpha" : "",
        (long)date.ds_Days, (long)date.ds_Minute, (long)date.ds_Tick);
    snprintf(log_path, sizeof(log_path), "%s.log", stem);
    report("%s; GCC %s; RC10 build; log=%s", version, __VERSION__, log_path);
    if (options[3]) report("ALPHA mode: LEFT NEAREST / RIGHT LINEAR. LINEAR switch ignored.");
    else report("Filter=%s. TOP=untextured CPU reference A/B, BOTTOM=test A/B. Keep window unobscured.",
        linear_filter ? "LINEAR" : "NEAREST");
    report("Default AUTO 2s/case; MANUAL waits for Space/Enter. Esc/close/Ctrl-C stops. No FPSE/AHI.");
    if (log_failed) { puts("Cannot create PROGDIR log; stopped before opening graphics."); goto done; }
    IntuitionBase = (struct IntuitionBase *)OpenLibrary((CONST_STRPTR)"intuition.library", 39);
    CyberGfxBase = OpenLibrary((CONST_STRPTR)"cybergraphics.library", 41);
    if (!IntuitionBase || !MiniGLOpen()) { report("OPEN FAIL: Intuition/MiniGL"); goto done; }
    opened = 1;
    report("MiniGL %u.%u id=%s; ABI=%lu size=%lu flags=%08lx",
        MiniGLBase->lib_Version, MiniGLBase->lib_Revision,
        MiniGLBase->lib_IdString ? (char *)MiniGLBase->lib_IdString : "(none)",
        (unsigned long)MiniGLDispatch->abiVersion, (unsigned long)MiniGLDispatch->structSize,
        (unsigned long)MiniGLDispatch->backendFlags);
    if (*MiniGLDispatch->currentContext) { report("STOP: another MiniGL context is active; close its application"); goto done; }
    source[0] = malloc(256*256*4); source[1] = malloc(256*256*4);
    atlas = malloc(512*256*4); scratch = malloc(256*256*4);
    blank = calloc(256*256, 4); capture = malloc(M2_CAPTURE_BYTES);
    if (!source[0] || !source[1] || !atlas || !scratch || !blank || !capture) {
        report("FAIL: test memory allocation"); goto done;
    }
    if (options[3]) { result = ma_run(); goto done; }
    m2_pixels(source[0], 256, 0, 0); m2_pixels(source[1], 256, 1, 0);
    m2_pixels(atlas, 512, 0, 1);
    for (n = 0; n < M2_CASE_COUNT; ++n) {
        GLenum error;
        int status;
        report("CASE %02u BEGIN: %s; fresh context", n, m2_cases[n].name);
        if (!open_context()) { report("CONTEXT INIT FAIL"); goto done; }
        error = glGetError();
        if (error != GL_NO_ERROR) { report("CONTEXT GL error=%lu", (unsigned long)error); goto done; }
        snprintf(title, sizeof(title), "MGL2D %02u | %s | TOP expected / BOTTOM actual", n, m2_cases[n].name);
        SetWindowTitles(test_window, (UBYTE *)title, (UBYTE *)-1);
        report("SEQUENCE ENTER (no I/O inside upload/draw/barrier sequence)");
        sequence(n);
        error = glGetError();
        report("SEQUENCE LEAVE GL error=%lu", (unsigned long)error);
        Delay(10);
        status = inspect(n, geometry_ok, present_control_ok);
        if (error != GL_NO_ERROR) status = 0;
        if (!n) geometry_ok = status == 1;
        if (n == 8) present_control_ok = status == 1;
        if (status > 0) ++pass; else if (!status) ++fail; else ++unverified;
        report("CASE %02u RESULT=%s", n, status > 0 ? "PASS" : status == 0 ? "FAIL" : "UNVERIFIED");
        snprintf(title, sizeof(title), "MGL2D %02u %s | %s | Space=next Esc=stop", n,
            status > 0 ? "PASS" : status == 0 ? "FAIL" : "UNVERIFIED", m2_cases[n].name);
        SetWindowTitles(test_window, (UBYTE *)title, (UBYTE *)-1);
        if (!wait_case()) { report("STOPPED by user"); break; }
        close_context();
    }
    report("SUMMARY pass=%u fail=%u unverified=%u complete=%s", pass, fail, unverified,
        n == M2_CASE_COUNT ? "yes" : "no");
    report("No cause is proved by a single failed case. Compare 02/03, 03/05, 05/06/07, 08/09 and 02/10; compare Classic with native.");
    result = (fail || unverified || n != M2_CASE_COUNT) ? 5 : 0;
done:
    close_context();
    free(source[0]); free(source[1]); free(atlas); free(scratch); free(blank); free(capture);
    if (opened) MiniGLClose();
    if (CyberGfxBase) CloseLibrary(CyberGfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    FreeArgs(args);
    if (log_failed) { puts("WARNING: one or more PROGDIR log writes failed"); result = 20; }
    return result;
}

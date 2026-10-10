/* V23 package contains incompatible extensions to dispatch ABI 3:
 * PiStorm3D library 14 and Classic library 23 share only the prefix ending
 * in GLPolygonOffset. Do not require or call either backend's tail. */
#include <stddef.h>
#include <proto/exec.h>
#include <proto/minigl.h>
#include "trace.h"

extern const MGLDispatchTable *MiniGLGetDispatchTableLVO(void);
#if MINIGL_DISPATCH_ABI_VERSION >= 5
#define FPSE_MGL_MIN_VERSION MINIGL_VERSION
#define FPSE_MGL_CLASSIC_MIN_VERSION MINIGL_VERSION
#else
#define FPSE_MGL_MIN_VERSION 14
#define FPSE_MGL_CLASSIC_MIN_VERSION 23
#endif

/* Catch accidental use of an incompatible SDK or target pointer size. */
typedef char fpse_minigl_common_prefix_must_be_536[
    offsetof(MGLDispatchTable, GLClientActiveTextureARB) == 536 ? 1 : -1];

BOOL MiniGLOpen(void)
{
    const MGLDispatchTable *d;
    if (MiniGLBase && MiniGLDispatch) return TRUE;
    mgl_trace_log("MiniGLOpen ENTER");
    #if defined(FPSE_MINIGL_CLASSIC_ORTHO)
    MiniGLBase = OpenLibrary("PROGDIR:minigl-ortho.library", FPSE_MGL_CLASSIC_MIN_VERSION);
    #elif defined(FPSE_MINIGL_LIBRARY_PROFILE)
    MiniGLBase = OpenLibrary("PROGDIR:minigl-profile.library", FPSE_MGL_CLASSIC_MIN_VERSION);
    #else
    mgl_trace_log("OpenLibrary %s version=%u ENTER", MINIGLNAME, (unsigned)FPSE_MGL_MIN_VERSION);
    MiniGLBase = OpenLibrary(MINIGLNAME, FPSE_MGL_MIN_VERSION);
    #endif
    if (!MiniGLBase) { mgl_trace_log("OpenLibrary FAIL"); return FALSE; }
    mgl_trace_log("OpenLibrary LEAVE base=%08lx version=%u.%u id=%s", (unsigned long)MiniGLBase, MiniGLBase->lib_Version, MiniGLBase->lib_Revision, MiniGLBase->lib_IdString ? (const char *)MiniGLBase->lib_IdString : "(none)");
    mgl_trace_log("MiniGLGetDispatchTable ENTER");
    d = MiniGLGetDispatchTableLVO();
    mgl_trace_log("MiniGLGetDispatchTable LEAVE table=%08lx", (unsigned long)d);
    if (d) mgl_trace_log("dispatch abi=%lu size=%lu flags=%08lx currentContext_ptr=%08lx", (unsigned long)d->abiVersion, (unsigned long)d->structSize, (unsigned long)d->backendFlags, (unsigned long)d->currentContext);
    if (!d || d->abiVersion != MINIGL_DISPATCH_ABI_VERSION ||
        d->structSize <
#if MINIGL_DISPATCH_ABI_VERSION >= 5
        sizeof(MGLDispatchTable) ||
#else
        offsetof(MGLDispatchTable, GLClientActiveTextureARB) ||
#endif
        !d->currentContext || (d->backendFlags & MINIGL_BACKEND_FLAG_STUB) ||
        !(d->backendFlags & (MINIGL_BACKEND_FLAG_PISTORM3D | MINIGL_BACKEND_FLAG_CLASSIC))) {
        mgl_trace_log("FAIL incompatible MiniGL dispatch");
        CloseLibrary(MiniGLBase);
        MiniGLBase = NULL;
        return FALSE;
    }
    MiniGLDispatch = d;
    mgl_trace_log("MiniGLOpen LEAVE ok");
    return TRUE;
}

void MiniGLClose(void)
{
    mgl_trace_log("MiniGLClose ENTER");
    MiniGLDispatch = NULL;
    if (MiniGLBase) CloseLibrary(MiniGLBase);
    MiniGLBase = NULL;
    mgl_trace_log("MiniGLClose LEAVE");
}

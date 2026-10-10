/* Synchronous breadcrumbs, not a CPU profiler. GPL-2.0-or-later. */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "trace.h"
#include "trace_udp.h"
#include "version.h"
#ifndef FPSE_MINIGL_TRACE
#error trace.c belongs only in the separate diagnostic build
#endif
#ifndef MGL_TRACE_PATH
#define MGL_TRACE_PATH "PROGDIR:fpse-minigl-debug.log"
#endif
#ifndef MGL_TRACE_DEEP_PATH
#define MGL_TRACE_DEEP_PATH "PROGDIR:fpse-minigl-deep"
#endif
#ifndef MGL_TRACE_LIMIT
#define MGL_TRACE_LIMIT (16UL * 1024UL * 1024UL)
#endif
static int active, udp;
static unsigned long bytes, sequence, entered[MGL_T_COUNT], left[MGL_T_COUNT];
static int deep, limited;
static const char *names[MGL_T_COUNT] = {
    "GPU_Update", "GP0_Read", "GP1_Read", "GP0_Write", "GP1_Write",
    "DMA_chain", "DMA_in", "DMA_out", "DMA_exec", "GL_state",
    "GL_draw", "glFinish", "glTexImage2D", "glTexSubImage2D",
    "mglSwitchDisplay", "limiter", "refresh", "VRAM_rect",
    "input_update", "GPUupdateLace", "DMA_node", "GPUwriteDataMem",
    "GP0_command", "texture_cache", "texture_decode", "glGenTextures"
};
static void trace_emit(const char *line, unsigned long length)
{
    FILE *output;
    int failed;
    if (udp) {
        mgl_trace_udp_send(line, length);
        return;
    }
    /* No live writer is left open while we call the GPU/driver. Closing also
     * commits file length; fflush alone only drains the C runtime buffer. */
    output = fopen(MGL_TRACE_PATH, "a");
    if (!output) failed = 1;
    else {
        failed = fwrite(line, 1, length, output) != length;
        if (fflush(output) != 0) failed = 1;
        if (fclose(output) != 0) failed = 1;
    }
    if (failed) {
        active = 0;
        puts("GPUMiniGL debug: log write/close failed; tracing stopped.");
    }
}
void mgl_trace_log(const char *format, ...)
{
    char line[768], packet[832];
    int n;
    va_list ap;
    if (!active || limited) return;
    if (bytes >= MGL_TRACE_LIMIT) {
        static const char limit[] = "TRACE LIMIT reached; no later events recorded.\n";
        trace_emit(limit, sizeof(limit) - 1);
        limited = 1;
        return;
    }
    va_start(ap, format);
    n = vsnprintf(line, sizeof(line), format, ap);
    va_end(ap);
    if (n < 0) return;
    n = snprintf(packet, sizeof(packet), "%lu %s\n", ++sequence, line);
    if (n < 0) return;
    if ((unsigned int)n >= sizeof(packet)) n = sizeof(packet) - 1;
    trace_emit(packet, (unsigned long)n);
    bytes += (unsigned long)n;
}
void mgl_trace_open(void)
{
    FILE *flag, *output;
    if (active) mgl_trace_close();
    bytes = sequence = 0; limited = 0;
    memset(entered, 0, sizeof(entered)); memset(left, 0, sizeof(left));
    flag = fopen(MGL_TRACE_DEEP_PATH, "rb");
    deep = flag != NULL;
    if (flag) fclose(flag);
    udp = mgl_trace_udp_open();
    if (!udp) {
        output = fopen(MGL_TRACE_PATH, "w");
        if (!output) { puts("GPUMiniGL debug: cannot create " MGL_TRACE_PATH); return; }
        if (fclose(output) != 0) { puts("GPUMiniGL debug: cannot close " MGL_TRACE_PATH); return; }
    }
    active = 1;
    mgl_trace_log("START %s GCC=%s deep=%d limit=%lu transport=%s", VERSION, __VERSION__, deep, (unsigned long)MGL_TRACE_LIMIT, udp ? "UDP-only" : "file-close-per-line");
    mgl_trace_log("Full boundaries except GP1_Read and empty DMA nodes (sampled). Read-only DMA_RAM probe enabled. deep=1 logs all. GPU-only, not CD/CPU/audio. UDP can lose packets.");
}
unsigned long mgl_trace_begin(int event, unsigned long a, unsigned long b, unsigned long c)
{
    unsigned long n;
    int i, emit;
    if (!active || limited || event < 0 || event >= MGL_T_COUNT) return 0;
    n = ++entered[event];
    /* GP1 status polling exceeded one million calls in seven seconds in TR3.
     * Trace every other instrumented operation, not just its first calls.
     * Periodic status samples continue even if the emulator spins forever. */
    emit = event != MGL_T_GP1_READ || deep || n <= 4 || !(n & (n - 1)) || !(n & 65535UL);
    /* Long empty DMA tables need sparse progress, not thousands of packets.
     * b is the node number WITHIN this chain; retain the previous stop address. */
    if (event == MGL_T_DMA_NODE && !deep)
        emit = b <= 8 || !(b & 63UL) || a == 0x6eb88;
    if (event == MGL_T_UPDATE && !(n % 60)) {
        emit = 1;
        for (i = 0; i < MGL_T_COUNT; ++i)
            mgl_trace_log("COUNTERS %s entered=%lu left=%lu", names[i], entered[i], left[i]);
        if (udp) mgl_trace_log("UDP_HEALTH local_send_failures=%lu", mgl_trace_udp_dropped());
    }
    if (!emit) return 0;
    mgl_trace_log("ENTER %s call=%lu a=%08lx b=%08lx c=%08lx", names[event], n, a, b, c);
    return n;
}
void mgl_trace_end(int event, unsigned long token, unsigned long result)
{
    if (!active || limited || event < 0 || event >= MGL_T_COUNT) return;
    ++left[event];
    if (token) mgl_trace_log("LEAVE %s call=%lu result=%08lx", names[event], token, result);
    if (event == MGL_T_GP1_READ && token && !(token & 65535UL))
        mgl_trace_log("GP1_POLL status=%08lx updates=%lu/%lu gp0_write=%lu/%lu gp1_write=%lu/%lu dma_chain=%lu/%lu dma_out=%lu/%lu", result,
            entered[MGL_T_UPDATE], left[MGL_T_UPDATE], entered[MGL_T_GP0_WRITE], left[MGL_T_GP0_WRITE],
            entered[MGL_T_GP1_WRITE], left[MGL_T_GP1_WRITE], entered[MGL_T_DMA_CHAIN], left[MGL_T_DMA_CHAIN],
            entered[MGL_T_DMA_OUT], left[MGL_T_DMA_OUT]);
}
void mgl_trace_close(void)
{
    int i;
    if (!active) { mgl_trace_udp_close(); return; }
    for (i = 0; i < MGL_T_COUNT; ++i)
        mgl_trace_log("FINAL %s entered=%lu left=%lu", names[i], entered[i], left[i]);
    mgl_trace_log("END GPU session");
    if (udp) mgl_trace_log("UDP local send failures=%lu (remote delivery is not acknowledged)", mgl_trace_udp_dropped());
    mgl_trace_udp_close();
    active = udp = 0;
}

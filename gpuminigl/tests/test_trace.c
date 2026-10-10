/* Runs both on the build host and on m68k under vamos. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define FPSE_MINIGL_TRACE
#ifdef __AMIGA__
#define MGL_TRACE_PATH "PROGDIR:trace-selftest.log"
#else
#define MGL_TRACE_PATH "obj/trace-selftest.log"
#endif
#define MGL_TRACE_DEEP_PATH "test-deep-flag"
#define MGL_TRACE_LIMIT 32768UL
static int enable_deep, fail_flush;
static unsigned long flush_calls;
static int use_udp, udp_opened;
static unsigned long udp_packets, file_write_opens;
static char last_packet[832];
/* The real transport lives in a separate object, with no cross-TU inlining.
 * Keep that boundary in the m68k selftest too. */
__attribute__((noinline)) int mgl_trace_udp_open(void) { udp_opened = use_udp; return use_udp; }
__attribute__((noinline)) void mgl_trace_udp_close(void) { udp_opened = 0; }
unsigned long mgl_trace_udp_dropped(void) { return 0; }
void mgl_trace_udp_send(const char *data, unsigned long length)
{
    if (!udp_opened || length >= sizeof(last_packet))
        printf("UDP mock mismatch: opened=%d length=%lu\n", udp_opened, length);
    assert(udp_opened && length < sizeof(last_packet));
    memcpy(last_packet, data, length); last_packet[length] = 0;
    ++udp_packets;
}
static FILE *test_open(const char *name, const char *mode)
{
    if (!strcmp(name, MGL_TRACE_DEEP_PATH))
        return enable_deep ? fopen(MGL_TRACE_PATH, "rb") : NULL;
    if (!strcmp(name, MGL_TRACE_PATH) && (mode[0] == 'a' || mode[0] == 'w'))
        ++file_write_opens;
    return fopen(name, mode);
}
static int test_flush(FILE *file)
{
    ++flush_calls;
    return fail_flush ? EOF : fflush(file);
}
#define fopen test_open
#define fflush test_flush
#include "../trace.c"
#undef fopen
#undef fflush
static char contents[16384];
static void read_log(void)
{
    FILE *file = fopen(MGL_TRACE_PATH, "rb");
    size_t n;
    assert(file);
    n = fread(contents, 1, sizeof(contents) - 1, file);
    contents[n] = 0;
    fclose(file);
}
int main(void)
{
    unsigned long t;
    int i;
    mgl_trace_open();
    assert(active && !deep);
    mgl_trace_log("FORMAT int=%d long=%lu hex=%08lx", 256, 305419896UL, 0x12345678UL);
    assert(flush_calls == sequence);
    /* Must be visible BEFORE GPU_Close, including under vamos now. */
    read_log();
    assert(strstr(contents, "int=256 long=305419896 hex=12345678"));
    for (i = 1; i <= 9; ++i) {
        t = mgl_trace_begin(MGL_T_DRAW, 10, 20, 30);
        assert(t == (unsigned long)i);
        mgl_trace_end(MGL_T_DRAW, t, 0x12345678UL);
    }
    assert(entered[MGL_T_DRAW] == 9 && left[MGL_T_DRAW] == 9);
    t = mgl_trace_begin(MGL_T_DMA_NODE, 0x60000, 9, 512);
    assert(t == 0); mgl_trace_end(MGL_T_DMA_NODE, t, 0);
    t = mgl_trace_begin(MGL_T_DMA_NODE, 0x60000, 64, 512);
    assert(t != 0); mgl_trace_end(MGL_T_DMA_NODE, t, 0);
    t = mgl_trace_begin(MGL_T_DMA_NODE, 0x6eb88, 170, 512);
    assert(t != 0); mgl_trace_end(MGL_T_DMA_NODE, t, 0);
    for (i = 1; i <= 9; ++i) {
        t = mgl_trace_begin(MGL_T_GP1_READ, 0, 0, 0);
        assert(!!t == (i <= 4 || i == 8));
        mgl_trace_end(MGL_T_GP1_READ, t, 0x54042200UL);
    }
    entered[MGL_T_GP1_READ] = left[MGL_T_GP1_READ] = 65535;
    t = mgl_trace_begin(MGL_T_GP1_READ, 0, 0, 0);
    assert(t == 65536UL);
    mgl_trace_end(MGL_T_GP1_READ, t, 0x54042200UL);
    for (i = 0; i < 60; ++i) {
        t = mgl_trace_begin(MGL_T_UPDATE, 0, 0, 0);
        mgl_trace_end(MGL_T_UPDATE, t, 0);
    }
    mgl_trace_close();
    mgl_trace_close();
    read_log();
    assert(strstr(contents, "int=256 long=305419896 hex=12345678"));
    assert(strstr(contents, "COUNTERS GL_draw entered=9 left=9"));
    assert(strstr(contents, "GP1_POLL status=54042200"));
    assert(strstr(contents, "FINAL GPU_Update entered=60 left=60"));
    assert(strstr(contents, "END GPU session"));
    enable_deep = 1;
    mgl_trace_open();
    assert(active && deep && entered[MGL_T_DRAW] == 0);
    for (i = 1; i <= 9; ++i) {
        t = mgl_trace_begin(MGL_T_GP1_READ, 0, 0, 0);
        assert(t == (unsigned long)i);
        mgl_trace_end(MGL_T_GP1_READ, t, 0);
    }
    bytes = MGL_TRACE_LIMIT;
    mgl_trace_log("must not appear");
    assert(limited);
    mgl_trace_close();
    read_log();
    assert(strstr(contents, "deep=1"));
    assert(strstr(contents, "TRACE LIMIT reached"));
    assert(!strstr(contents, "must not appear"));
    mgl_trace_open();
    fail_flush = 1;
    mgl_trace_close(); /* A write failure during close must not fclose(NULL). */
    assert(!active);
    use_udp = 1;
    file_write_opens = 0;
    mgl_trace_open();
    assert(active && udp_opened && udp_packets == 2);
    mgl_trace_log("UDP BEFORE HANG");
    assert(strstr(last_packet, "UDP BEFORE HANG"));
    assert(file_write_opens == 0); /* A broken disk must not break UDP. */
    mgl_trace_close();
    assert(!active && !udp_opened && file_write_opens == 0);
    puts("PASS: trace printf ABI, close-per-line, full boundaries, GP1 sampling/periodic progress, counters, deep, cap, failure, UDP-only");
    return 0;
}

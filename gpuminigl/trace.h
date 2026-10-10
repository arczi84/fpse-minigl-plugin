/* Opt-in hang diagnostics. Ordinary plugins compile all hooks out. */
#ifndef FPSE_MINIGL_TRACE_H
#define FPSE_MINIGL_TRACE_H
enum {
    MGL_T_UPDATE, MGL_T_GP0_READ, MGL_T_GP1_READ, MGL_T_GP0_WRITE,
    MGL_T_GP1_WRITE, MGL_T_DMA_CHAIN, MGL_T_DMA_IN, MGL_T_DMA_OUT,
    MGL_T_DMA_EXEC, MGL_T_STATE, MGL_T_DRAW, MGL_T_FINISH,
    MGL_T_TEXIMAGE, MGL_T_TEXSUB, MGL_T_PRESENT, MGL_T_LIMIT,
    MGL_T_REFRESH, MGL_T_VRAM, MGL_T_INPUT, MGL_T_LACE,
    MGL_T_DMA_NODE, MGL_T_DECODE, MGL_T_COMMAND, MGL_T_CACHE,
    MGL_T_TEXDECODE, MGL_T_TEXGEN, MGL_T_COUNT
};
#ifdef FPSE_MINIGL_TRACE
void mgl_trace_open(void);
void mgl_trace_close(void);
void mgl_trace_log(const char *format, ...);
void mgl_trace_dma_probe(const void *ram, unsigned long start, int height);
unsigned long mgl_trace_begin(int event, unsigned long a, unsigned long b, unsigned long c);
void mgl_trace_end(int event, unsigned long token, unsigned long result);
#else
#define mgl_trace_open() ((void)0)
#define mgl_trace_close() ((void)0)
#define mgl_trace_log(...) ((void)0)
#define mgl_trace_begin(event, a, b, c) 0UL
#define mgl_trace_end(event, token, result) ((void)(token))
#endif
#endif

#include "trace.h"
#include "dma_probe.h"

static void probe_visit(int before, const DMAProbe *state)
{
    unsigned long node = (unsigned long)state->nodes + (before ? 1 : 0);
    /* The last received node in the 0.4.9 TR3 run was 0x6eb88. Also sample
     * progress across long, valid, empty ordering tables without UDP floods. */
    int emit = node <= 4 || !(node & 255) || state->address == 0x6eb88;
    if (before) {
        if (emit) mgl_trace_log("DMA_RAM READ_ENTER node=%lu addr=%08lx", node, (unsigned long)state->address);
    } else if (emit || (state->header >> 24))
        mgl_trace_log("DMA_RAM READ_LEAVE node=%lu addr=%08lx header=%08lx words=%lu next=%08lx", node,
            (unsigned long)state->address, (unsigned long)state->header,
            (unsigned long)(state->header >> 24), (unsigned long)state->next);
}

void mgl_trace_dma_probe(const void *ram, unsigned long start, int height)
{
    DMAProbe result;
    static const char *reasons[] = {"end", "probe-limit", "header-out-of-range", "self-loop", "payload-out-of-range"};
    if (height != 512) { mgl_trace_log("DMA_RAM SKIP non-PSX height=%d", height); return; }
    mgl_trace_log("DMA_RAM BEGIN ram=%08lx start=%08lx (headers only; original DMA unchanged)", (unsigned long)ram, start);
    result = dma_probe_walk(ram, 2UL * 1024 * 1024, start, 65536, probe_visit);
    mgl_trace_log("DMA_RAM END reason=%s nodes=%lu nonempty=%lu words=%lu hash=%08lx last=%08lx next=%08lx",
        reasons[result.reason], (unsigned long)result.nodes, (unsigned long)result.nonempty,
        (unsigned long)result.words, (unsigned long)result.hash,
        (unsigned long)result.address, (unsigned long)result.next);
}

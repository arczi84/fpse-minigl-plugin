/* Read-only PSX DMA header walk. No command execution or renderer calls. */
#ifndef FPSE_DMA_PROBE_H
#define FPSE_DMA_PROBE_H
#include <stdint.h>
enum { DMA_PROBE_END, DMA_PROBE_LIMIT, DMA_PROBE_RANGE, DMA_PROBE_SELF_LOOP, DMA_PROBE_PAYLOAD_RANGE };
typedef struct {
    uint32_t nodes, nonempty, words, hash, address, next, header;
    int reason;
} DMAProbe;
typedef void (*DMAProbeVisit)(int before, const DMAProbe *state);
static DMAProbe dma_probe_walk(const volatile unsigned char *ram, uint32_t size,
    uint32_t start, uint32_t limit, DMAProbeVisit visit)
{
    DMAProbe r = {0, 0, 0, UINT32_C(2166136261), 0, start, 0, DMA_PROBE_LIMIT};
    uint32_t n;
    while (r.nodes < limit) {
        r.address = r.next & UINT32_C(0x1ffffc);
        if (!ram || size < 4 || r.address > size - 4) { r.reason = DMA_PROBE_RANGE; return r; }
        if (visit) visit(1, &r);
        r.header = (uint32_t)ram[r.address] | ((uint32_t)ram[r.address + 1] << 8) |
            ((uint32_t)ram[r.address + 2] << 16) | ((uint32_t)ram[r.address + 3] << 24);
        ++r.nodes;
        n = r.header >> 24;
        if (n) ++r.nonempty;
        r.words += n;
        r.hash = (r.hash ^ r.header ^ r.address) * UINT32_C(16777619);
        r.next = r.header & UINT32_C(0xffffff);
        if (visit) visit(0, &r);
        if (n > (size - r.address - 4) / 4) { r.reason = DMA_PROBE_PAYLOAD_RANGE; return r; }
        if (r.next == UINT32_C(0xffffff)) { r.reason = DMA_PROBE_END; return r; }
        if ((r.next & UINT32_C(0x1ffffc)) == r.address) { r.reason = DMA_PROBE_SELF_LOOP; return r; }
    }
    return r;
}
#endif

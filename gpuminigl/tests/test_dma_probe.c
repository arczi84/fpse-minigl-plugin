#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../dma_probe.h"
static unsigned char ram[512 * 1024];
static unsigned long before_count, after_count, watched;
static void put_header(unsigned long address, unsigned long header)
{
    int i;
    for (i = 0; i < 4; ++i) ram[address+i] = (unsigned char)(header >> (i*8));
}
static void visit(int before, const DMAProbe *r)
{
    if (before) ++before_count;
    else { ++after_count; if (r->address == 0x6eb88) ++watched; }
}
int main(void)
{
    DMAProbe r;
    unsigned long i, start = 0x6ee2c, end = start - 4095 * 4;
    unsigned char unchanged[64];
    for (i = start; i > end; i -= 4) put_header(i, i - 4);
    put_header(end, 0xffffff);
    memcpy(unchanged, ram + 0x6eb80, sizeof(unchanged));
    r = dma_probe_walk(ram, sizeof(ram), start | 0x80000000UL, 65536, visit);
    assert(r.reason == DMA_PROBE_END && r.nodes == 4096 && r.words == 0);
    assert(before_count == 4096 && after_count == 4096 && watched == 1);
    assert(!memcmp(unchanged, ram + 0x6eb80, sizeof(unchanged)));
    put_header(start, 0x03000000UL | (start - 4));
    r = dma_probe_walk(ram, sizeof(ram), start, 65536, NULL);
    assert(r.reason == DMA_PROBE_END && r.nonempty == 1 && r.words == 3);
    r = dma_probe_walk(ram, sizeof(ram), start, 5, NULL);
    assert(r.reason == DMA_PROBE_LIMIT && r.nodes == 5);
    put_header(start, start);
    r = dma_probe_walk(ram, sizeof(ram), start, 65536, NULL);
    assert(r.reason == DMA_PROBE_SELF_LOOP && r.nodes == 1);
    put_header(start, start - 4); put_header(start - 4, start);
    r = dma_probe_walk(ram, sizeof(ram), start, 32, NULL);
    assert(r.reason == DMA_PROBE_LIMIT && r.nodes == 32);
    r = dma_probe_walk(ram, sizeof(ram), 0xffff0, 32, NULL);
    assert(r.reason == DMA_PROBE_RANGE && !r.nodes);
    put_header(sizeof(ram) - 4, 0x01ffffffUL);
    r = dma_probe_walk(ram, sizeof(ram), sizeof(ram) - 4, 32, NULL);
    assert(r.reason == DMA_PROBE_PAYLOAD_RANGE && r.nodes == 1);
    r = dma_probe_walk(NULL, sizeof(ram), 0, 32, NULL);
    assert(r.reason == DMA_PROBE_RANGE);
    puts("PASS: DMA probe 4096-node descending empty table, watched RAM address, no writes, payload count, cycles/limit/range");
    return 0;
}

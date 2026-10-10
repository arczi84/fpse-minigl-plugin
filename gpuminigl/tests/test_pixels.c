#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../pixels.h"

static unsigned char vram[1024 * 1024];
static void put(unsigned int word, unsigned int value)
{
    word = (word & 0x7ffff) * 2;
    vram[word] = value;
    vram[word + 1] = value >> 8;
}

int main(void)
{
    unsigned char rgba[4];
    unsigned int i, u, mask, offset;
    put(0, 0x1234);
    assert(psx_read16(vram, 0) == 0x1234);
    assert(psx_read16(vram, 0x80000) == 0x1234);
    put(0x7ffff, 0xabcd);
    assert(psx_read16(vram, 0x7ffff) == 0xabcd);
    for (i = 0; i < 65536; ++i) {
        psx_rgba(i, rgba, 0);
        assert(rgba[0] == ((i & 31) * 8 + (i & 31) / 4));
        assert(rgba[1] == (((i >> 5) & 31) * 8 + ((i >> 5) & 31) / 4));
        assert(rgba[2] == (((i >> 10) & 31) * 8 + ((i >> 10) & 31) / 4));
        assert(rgba[3] == (i ? 255 : 0));
        psx_rgba(i, rgba, 1);
        assert(rgba[3] == (i && !(i & 0x8000) ? 255 : 0));
    }
    memset(vram, 0, sizeof(vram));
    for (i = 0; i < 256; ++i) put(400 * 1024 + 32 + i, 0x8000 | i);
    put(256 * 1024 + 64, 0x4321);
    for (i = 0; i < 4; ++i)
        assert(psx_texel(vram, 0, 64, 256, 32, 400, i, 0) == (0x8001 + i));
    put(256 * 1024 + 64, 0xfe12);
    assert(psx_texel(vram, 1, 64, 256, 32, 400, 0, 0) == 0x8012);
    assert(psx_texel(vram, 1, 64, 256, 32, 400, 1, 0) == 0x80fe);
    assert(psx_texel(vram, 2, 64, 256, 0, 0, 0, 0) == 0xfe12);
    put(0, 0x7c00);
    assert(psx_texel(vram, 2, 960, 256, 0, 0, 64, 256) == 0x7c00);
    /* Palette lookup may cross a VRAM row, including the end of VRAM. */
    put(64, 0x0010);
    assert(psx_texel(vram, 1, 64, 0, 1008, 511, 0, 0) == 0x7c00);
    for (mask = 0; mask < 32; ++mask)
        for (offset = 0; offset < 32; ++offset)
            for (u = 0; u < 512; ++u) {
                unsigned int expected = u & 255;
                for (i = 0; i < 5; ++i)
                    if (mask & (1U << i)) {
                        expected &= ~(1U << (i + 3));
                        expected |= ((offset >> i) & 1U) << (i + 3);
                    }
                assert(psx_window_coord(u, mask, offset) == expected);
            }
    puts("PASS: PSX little-endian VRAM, all 65536 colors/STP, 4/8/15-bit textures, wrapping, texture windows");
    return 0;
}

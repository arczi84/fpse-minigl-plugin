/* Run on Amiga, not the build host. Exercises the actual FPSE command decoder
 * and MiniGL backend without requiring a PSX BIOS or game. */
#include "plugin.h"
#include "dll/dll.h"
#include <proto/dos.h>
#include <proto/minigl.h>

static int read_cfg(char *section, char *key, char *value)
{
    (void)section;
    if (!strcmp(key, "Resolution")) { strcpy(value, "1"); return FPSE_OK; }
    if (!strcmp(key, "TextureCache")) { strcpy(value, "0"); return FPSE_OK; }
    return FPSE_ERR;
}
static int write_cfg(char *section, char *key, char *value)
{
    (void)section; (void)key; (void)value;
    return FPSE_OK;
}
static void flush_rec(UINT32 first, UINT32 last) { (void)first; (void)last; }
static UINT32 xy(int x, int y) { return (UINT32)x | ((UINT32)y << 16); }

int main(void)
{
    FPSEAmiga host;
    int cycle, i, x, y, result = 20;
    memset(&host, 0, sizeof(host));
    host.SystemRam = calloc(1, 2 * 1024 * 1024);
    if (!host.SystemRam) return 20;
    host.ReadCfg = read_cfg; host.WriteCfg = write_cfg; host.FlushRec = flush_rec;
    if (dll_setup() != DLL_RESULT_OK) { free(host.SystemRam); return 20; }
    for (cycle = 0; cycle < 2; ++cycle) {
        if (GPU_Open((UINT32 *)&host) != FPSE_OK) goto done;
        GP1_Write(0x00000000); /* reset */
        GP1_Write(0x08000001); /* 320x240 NTSC */
        GP1_Write(0x05000000); /* display origin */
        GP1_Write(0x06c60260); /* horizontal range */
        GP1_Write(0x0703fc10); /* vertical range: 16..255 */
        GP1_Write(0x03000000); /* display on */
        GP0_Write(0xe3000000);
        GP0_Write(0xe4000000 | 319 | (239 << 10));
        GP0_Write(0xe5000000);
        GPU_Update(); /* establish display geometry before drawing */

        GP0_Write(0x02202020); GP0_Write(xy(0, 0)); GP0_Write(xy(320, 240));
        /* A Gouraud triangle: red, green, blue corners. */
        GP0_Write(0x300000ff); GP0_Write(xy(20, 20));
        GP0_Write(0x0000ff00); GP0_Write(xy(145, 20));
        GP0_Write(0x00ff0000); GP0_Write(xy(20, 130));

        /* Upload a 16x16 RGB555 checkerboard to texture page x=640. */
        GP0_Write(0xa0000000); GP0_Write(xy(640, 0)); GP0_Write(xy(16, 16));
        for (y = 0; y < 16; ++y)
            for (x = 0; x < 16; x += 2) {
                UINT32 p = ((x / 4 + y / 4) & 1) ? 0x7fff : 0x001f;
                GP0_Write(p | (p << 16));
            }
        /* Raw texture sprite, without vertex modulation. */
        GP0_Write(0xe100010a);
        GP0_Write(0x65000000); GP0_Write(xy(175, 25)); GP0_Write(0); GP0_Write(xy(16, 16));
        /* 4-bit RGB / transparent / STP-black palette strips, then mutate
         * only one CLUT entry later to exercise selective invalidation. */
        GP0_Write(0xa0000000); GP0_Write(xy(32, 480)); GP0_Write(xy(16, 1));
        GP0_Write(0x001f0000); GP0_Write(0x7c0003e0); GP0_Write(0x00008000);
        for (i = 0; i < 5; ++i) GP0_Write(0);
        GP0_Write(0xa0000000); GP0_Write(xy(704, 0)); GP0_Write(xy(4, 16));
        for (i = 0; i < 32; ++i) GP0_Write(0x43214321);
        GP0_Write(0xe100000b);
        GP0_Write(0x65000000); GP0_Write(xy(205, 25));
        GP0_Write((UINT32)((480 << 6) | 2) << 16); GP0_Write(xy(16, 16));
        /* All four PSX semitransparency modes over grey, ABR2 approximate. */
        for (i = 0; i < 4; ++i) {
            GP0_Write(0xe1000000 | (i << 5));
            GP0_Write(0x62808080); GP0_Write(xy(20 + i * 72, 170)); GP0_Write(xy(56, 40));
        }
        /* VRAM transfer readback checks the little-endian ABI, not GL readback. */
        GP0_Write(0xc0000000); GP0_Write(xy(640, 0)); GP0_Write(xy(2, 1));
        if (GP0_Read() != 0x001f001f) {
            puts("FAIL: VRAM transfer readback");
            GPU_Close(); goto done;
        }
        puts("Inspect: RGB triangle, red/white checkerboard, RGB/black palette stripes, four blend boxes (5 seconds).");
        for (i = 0; i < 300; ++i) GPU_Update();
        GP0_Write(0xa0000000); GP0_Write(xy(33, 480)); GP0_Write(xy(2, 1));
        GP0_Write(0x03e07fff); /* red entry becomes white, green stays green */
        GP0_Write(0xe100000b);
        GP0_Write(0x65000000); GP0_Write(xy(205, 25));
        GP0_Write((UINT32)((480 << 6) | 2) << 16); GP0_Write(xy(16, 16));
        puts("Palette-only update: red stripes should now be white; green/blue/black remain.");
        for (i = 0; i < 120; ++i) GPU_Update();
        puts("Cache test: checkerboard must survive 120 framebuffer clears, then turn green after a texture-page fill.");
        for (i = 0; i < 120; ++i) {
            GP0_Write(0x02202020); GP0_Write(xy(0, 0)); GP0_Write(xy(320, 240));
            GP0_Write(0xe100010a);
            GP0_Write(0x65000000); GP0_Write(xy(175, 25)); GP0_Write(0); GP0_Write(xy(16, 16));
            GPU_Update();
        }
        GP0_Write(0x0200ff00); GP0_Write(xy(640, 0)); GP0_Write(xy(16, 16));
        GP0_Write(0x65000000); GP0_Write(xy(175, 25)); GP0_Write(0); GP0_Write(xy(16, 16));
        for (i = 0; i < 120; ++i) GPU_Update();
        glFinish();
        if (glGetError() != GL_NO_ERROR) {
            puts("FAIL: MiniGL reported a rendering error");
            GPU_Close(); goto done;
        }
        GPU_Close();
        GPU_Close(); /* must be harmless */
    }
    puts("PASS: VRAM readback and two open/draw/close cycles; verify the image visually.");
    result = 0;
done:
    dll_cleanup();
    free(host.SystemRam);
    return result;
}

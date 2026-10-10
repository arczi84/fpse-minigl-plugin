/* On-device regression for the real GPUW3D backend; no BIOS or game needed. */
#include "plugin.h"
#include "dll/dll.h"

static int read_cfg(char *section, char *key, char *value)
{
    (void)section;
    if (!strcmp(key, "Scale")) strcpy(value, "0");
    else if (!strcmp(key, "Resolution")) strcpy(value, "0");
    else if (!strcmp(key, "TextureCache")) strcpy(value, "64");
    else if (!strcmp(key, "AlphaThreshold")) strcpy(value, "50");
    else return FPSE_ERR;
    return FPSE_OK;
}
static int write_cfg(char *s, char *k, char *v) { (void)s; (void)k; (void)v; return FPSE_OK; }
static void flush_rec(UINT32 a, UINT32 b) { (void)a; (void)b; }
static UINT32 xy(int x, int y) { return x | ((UINT32)y << 16); }
static void hold(void) { int i; for (i = 0; i < 180; ++i) GPU_Update(); }

static void draw_area(int top)
{
    GP0_Write(0xe3000000 | (top << 10));
    GP0_Write(0xe4000000 | 319 | ((top + 239) << 10));
    GP0_Write(0xe5000000 | (top << 11));
}
static void fill(int x, int y, int w, int h, UINT32 color)
{
    GP0_Write(0x02000000 | color); GP0_Write(xy(x, y)); GP0_Write(xy(w, h));
}
static void sprite(int x, int y, UINT32 uv, int semi)
{
    GP0_Write(semi ? 0x67000000 : 0x65000000);
    GP0_Write(xy(x, y)); GP0_Write(uv); GP0_Write(xy(16, 16));
}
static void upload_word(int x, int y, UINT32 word)
{
    GP0_Write(0xa0000000); GP0_Write(xy(x, y)); GP0_Write(xy(1, 1)); GP0_Write(word);
}

int main(void)
{
    FPSEAmiga host;
    int pass, i, result = 20;
    UINT32 palette_uv = (480UL << 22) | (44UL << 16) | (64 << 8);
    memset(&host, 0, sizeof(host));
    host.SystemRam = calloc(1, 2 * 1024 * 1024);
    if (!host.SystemRam) return 20;
    host.ReadCfg = read_cfg; host.WriteCfg = write_cfg; host.FlushRec = flush_rec;
    if (dll_setup() != DLL_RESULT_OK) { free(host.SystemRam); return 20; }
    for (pass = 0; pass < 2; ++pass) {
        if (GPU_Open((UINT32 *)&host) != FPSE_OK) goto done;
        GP1_Write(0);
        GP1_Write(0x08000001);
        GP1_Write(0x05000000);
        GP1_Write(0x06c60260); GP1_Write(0x0703fc10); GP1_Write(0x03000000);
        draw_area(0); GPU_Update();
        fill(0, 0, 320, 240, 0x606060);
        /* Fill outside drawing area must still update GPU and CPU VRAM. */
        fill(640, 0, 16, 16, 0x0000ff);
        GP0_Write(0xc0000000); GP0_Write(xy(640, 0)); GP0_Write(xy(2, 1));
        if (GP0_Read() != 0x001f001f) { puts("FAIL: fill did not update CPU VRAM"); GPU_Close(); goto done; }
        GP0_Write(0xe100010a); sprite(16, 16, 0, 0);
        /* Upload STP red. ABR0/3 must not vanish at alpha threshold 50. */
        GP0_Write(0xa0000000); GP0_Write(xy(640, 0)); GP0_Write(xy(16, 16));
        for (i = 0; i < 128; ++i) GP0_Write(0x801f801f);
        for (i = 0; i < 4; ++i) {
            GP0_Write(0xe100010a | (i << 5)); sprite(16 + i * 64, 64, 0, 1);
        }
        /* 4-bit texture, followed by a partial palette update excluding CLUT origin. */
        GP0_Write(0xa0000000); GP0_Write(xy(640, 64)); GP0_Write(xy(4, 16));
        for (i = 0; i < 32; ++i) GP0_Write(0x11111111);
        upload_word(705, 480, 0x001f);
        GP0_Write(0xe100000a); sprite(16, 112, palette_uv, 0);
        upload_word(705, 480, 0x03e0); sprite(80, 112, palette_uv, 0);
        puts("Expect red square; four blend squares; red then green palette squares.");
        hold();
        /* Render to the other PSX framebuffer and then present it. */
        draw_area(256);
        fill(0, 256, 320, 240, 0x202020);
        GP0_Write(0x300000ff); GP0_Write(xy(20, 20));
        GP0_Write(0x0000ff00); GP0_Write(xy(250, 20));
        GP0_Write(0x00ff0000); GP0_Write(xy(20, 180));
        GP1_Write(0x05040000);
        puts("Expect RGB triangle after switching to framebuffer y=256.");
        hold();
        /* A menu and game can change GP1(06) alone, keeping mode 512. */
        GP1_Write(0x05000000);
        GP1_Write(0x08000002);
        GP1_Write(0x06c60260);
        draw_area(0);
        fill(0, 0, 512, 240, 0x303090);
        puts("Window test: wide 512-pixel menu.");
        hold();
        GP1_Write(0x068a0260); /* 1600 display clocks -> visible width 320 */
        fill(0, 0, 320, 240, 0x309030);
        puts("Window test: must shrink to 320 pixels; no red menu strip on the right.");
        hold();
        GP1_Write(0x06c60260);
        puts("Window test: back to 512 pixels.");
        hold();
        GPU_Close(); GPU_Close();
    }
    puts("PASS: CPU readback and two lifecycles; inspect graphics and console errors manually.");
    result = 0;
done:
    dll_cleanup(); free(host.SystemRam); return result;
}

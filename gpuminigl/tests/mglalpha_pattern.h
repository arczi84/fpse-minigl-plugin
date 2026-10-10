/* Synthetic, freely generated logo; no game assets or driver dependencies. */
#ifndef MGL_ALPHA_PATTERN_H
#define MGL_ALPHA_PATTERN_H
#define MA_SIZE 128
#define MA_BYTES (MA_SIZE * MA_SIZE * 4)

static unsigned ma_glyph(char c, unsigned row)
{
    static const unsigned char letters[26][7] = {
        {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30},
        {14,17,16,16,16,17,14}, {30,17,17,17,17,17,30},
        {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
        {14,17,16,23,17,17,15}, {17,17,17,31,17,17,17},
        {31,4,4,4,4,4,31}, {7,2,2,2,18,18,12},
        {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
        {17,27,21,21,17,17,17}, {17,25,25,21,19,19,17},
        {14,17,17,17,17,17,14}, {30,17,17,30,16,16,16},
        {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
        {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4},
        {17,17,17,17,17,17,14}, {17,17,17,17,17,10,4},
        {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
        {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31}
    };
    return c >= 'A' && c <= 'Z' && row < 7 ? letters[c-'A'][row] : 0;
}

static int ma_word_pixel(const char *s, int x, int y, int ox, int oy, int scale)
{
    unsigned cell, i;
    if (x < ox || y < oy || y >= oy + 7 * scale) return 0;
    cell = (unsigned)(x - ox) / (6 * scale);
    for (i = 0; i <= cell; ++i) if (!s[i]) return 0;
    x = ((x - ox) / scale) % 6;
    return x < 5 && (ma_glyph(s[cell], (y - oy) / scale) & (16 >> x));
}

/* kind 0: transparent black around gold letters;
 * kind 1: SAME alpha mask, gold RGB even in transparent texels;
 * kind 2: opaque blue atlas region [8,120), opaque black outside.
 * The atlas deliberately has an unsafe black neighbour, not a GL error. */
static void ma_pattern(unsigned char *rgba, int kind)
{
    int x, y;
    for (y = 0; y < MA_SIZE; ++y) for (x = 0; x < MA_SIZE; ++x) {
        unsigned char *p = rgba + (y * MA_SIZE + x) * 4;
        int ink = ma_word_pixel("TOMB", x, y, 18, 19, 4) ||
                  ma_word_pixel("TEST", x, y, 18, 57, 4) ||
                  ma_word_pixel("III", x, y, 39, 95, 3);
        p[0] = p[1] = p[2] = 0;
        p[3] = kind == 2 ? 255 : ink ? 255 : 0;
        if (kind == 2 && x >= 8 && x < 120 && y >= 8 && y < 120) {
            p[0] = 48; p[1] = 96; p[2] = 176;
        }
        if (ink || kind == 1) { p[0] = 255; p[1] = 192; p[2] = 48; }
    }
}
#endif

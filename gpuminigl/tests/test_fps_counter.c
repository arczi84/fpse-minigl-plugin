#include <assert.h>
#include <string.h>
#include "../fps_counter.h"

int main(void)
{
    MGLFpsCounter s;
    char title[96], tiny[8];
    int i;
    mgl_fps_reset(&s, 10, 900000, 100);
    for (i = 0; i < 25; ++i) mgl_fps_present(&s);
    assert(!mgl_fps_sample(&s, 11, 899999, 150));
    assert(s.presents == 25);
    assert(mgl_fps_sample(&s, 11, 900000, 150));
    assert(s.shown10 == 250 && s.emulated10 == 500 && !s.presents);
    mgl_fps_title(&s, title, sizeof(title));
    assert(!strcmp(title, "FPSE - MiniGL | FPS 25.0 | emu 50.0"));
    memset(tiny, 'x', sizeof(tiny));
    mgl_fps_title(&s, tiny, sizeof(tiny));
    assert(tiny[sizeof(tiny) - 1] == 0);

    /* NTSC interval, actual elapsed time, not an assumed one second. */
    for (i = 0; i < 60; ++i) mgl_fps_present(&s);
    assert(mgl_fps_sample(&s, 12, 901000, 210));
    assert(s.shown10 == 599 && s.emulated10 == 599);
    assert(mgl_fps_sample(&s, 13, 901000, 210));
    assert(!s.shown10 && !s.emulated10);

    mgl_fps_reset(&s, UINT32_MAX, 900000, UINT32_MAX - 9);
    for (i = 0; i < 25; ++i) mgl_fps_present(&s);
    assert(mgl_fps_sample(&s, 0, 900000, 40));
    assert(s.shown10 == 250 && s.emulated10 == 500);
    assert(!mgl_fps_sample(&s, 0, 899999, 41)); /* clock moved backwards */
    assert(!s.shown10 && !s.emulated10 && !s.presents);
    mgl_fps_reset(&s, 100, 0, 0);
    mgl_fps_present(&s);
    assert(!mgl_fps_sample(&s, 99, 900000, 1));
    assert(!s.presents);
    assert(mgl_fps_sample(&s, 101, 900000, 31));
    assert(s.emulated10 == 150);
    puts("PASS: FPS/emu counts, skip, NTSC rounding, elapsed time, idle, wrap, clock reset, title bounds");
    return 0;
}

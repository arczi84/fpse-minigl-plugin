#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define _AMIDOG_TIMER_H_
typedef struct amidog_timer_s { int unused; } amidog_timer_t;
typedef struct { int sec, usec; } amidog_timeval_t;
static uint64_t fake_time;
static unsigned int reads;
static void timer_gettime(amidog_timer_t *timer, amidog_timeval_t *t)
{
    assert(timer);
    ++reads;
    t->sec = fake_time / 1000000;
    t->usec = fake_time % 1000000;
}
static FILE *no_file(const char *path, const char *mode)
{
    assert(!strcmp(path, "PROGDIR:fpse-minigl-profile.log") && !strcmp(mode, "a"));
    return NULL; /* never create an Amiga PROGDIR: path on the build host */
}
#define fopen no_file
#include "../profile.c"
#undef fopen

int main(void)
{
    amidog_timer_t timer;
    uint32_t start;
    unsigned int before;
    mgl_profile_init(&timer);
    mgl_profile_identify(23, 0, 2, 640, 480);
    mgl_profile_cache_settings(256, 256, 256);
    assert(strstr(cache_identity, "configured=256 effective=256 texture-size=256x256"));
    mgl_profile_texture_path(1, 0);
    assert(strstr(texture_identity, "ARGB4444 lookup+CLUT, upload-fence=ordered-update"));
    assert(!mgl_profile_enabled && reads == 0);
    start = mgl_profile_begin();
    mgl_profile_end(MGL_P_DRAW, start);
    mgl_profile_frame();
    assert(reads == 0 && calls[MGL_P_DRAW] == 0);

    mgl_profile_toggle();
    assert(mgl_profile_enabled && reads == 1);
    fake_time = 100;
    start = mgl_profile_begin();
    fake_time += 250;
    mgl_profile_end(MGL_P_DRAW, start);
    assert(time_us[MGL_P_DRAW] == 250 && calls[MGL_P_DRAW] == 1);
    fake_time = 1999999;
    mgl_profile_frame();
    assert(frames == 1 && calls[MGL_P_DRAW] == 1);
    fake_time = 2000000;
    mgl_profile_frame();
    assert(frames == 0 && calls[MGL_P_DRAW] == 0 && started == 2000000);

    fake_time = UINT64_C(0x100000000) - 50;
    start = mgl_profile_begin();
    fake_time += 200;
    mgl_profile_end(MGL_P_TEX_WAIT, start);
    assert(time_us[MGL_P_TEX_WAIT] == 200 && calls[MGL_P_TEX_WAIT] == 1);
    start = mgl_profile_begin();
    fake_time -= 100;
    mgl_profile_end(MGL_P_TEX_WAIT, start);
    assert(time_us[MGL_P_TEX_WAIT] == 200 && calls[MGL_P_TEX_WAIT] == 1);
    before = reads;
    mgl_profile_end(-1, 0);
    mgl_profile_end(MGL_P_COUNT, 0);
    assert(reads == before);

    mgl_profile_toggle();
    before = reads;
    mgl_profile_begin();
    mgl_profile_frame();
    assert(!mgl_profile_enabled && reads == before);
    mgl_profile_close();
    mgl_profile_close();
    mgl_profile_toggle();
    assert(!mgl_profile_enabled);
    puts("PASS: profile disabled clock-free, categories, time-based reporting, wrap, clock adjustment, lifecycle");
    return 0;
}

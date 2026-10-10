/* Must link without profile.c, an Amiga timer or any logging implementation. */
#include <assert.h>
#include <stdio.h>
#include "../profile.h"

int main(void)
{
    uint32_t start;
    mgl_profile_init(NULL);
    mgl_profile_identify(23, 1, 1, 640, 480);
    mgl_profile_cache_settings(256, 256, 256);
    mgl_profile_texture_path(0, 1);
    mgl_profile_toggle();
    assert(!mgl_profile_enabled);
    start = mgl_profile_begin();
    assert(start == 0);
    mgl_profile_end(MGL_P_DRAW, start);
    mgl_profile_frame();
    mgl_profile_toggle();
    assert(!mgl_profile_enabled);
    mgl_profile_close();
    puts("PASS: release profiling compiled out; no timer/logger linkage, toggle stays disabled");
    return 0;
}

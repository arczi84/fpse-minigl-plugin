#include "profile.h"
#include "version.h"
#include "amidog/timer.h"
#include <stdio.h>
#include <string.h>

int mgl_profile_enabled;
static amidog_timer_t *profile_timer;
static FILE *profile_log;
static uint32_t time_us[MGL_P_COUNT], calls[MGL_P_COUNT], started, frames;
static char identity[160];
static char cache_identity[128];
static char texture_identity[128];
static const char *labels[MGL_P_COUNT] = {
    "state", "draw", "tex-wait", "tex-convert", "tex-upload",
    "vram-wait", "vram-convert", "vram-upload", "present", "fill", "limiter"
};

uint32_t mgl_profile_begin(void)
{
    amidog_timeval_t t;
    if (!mgl_profile_enabled || !profile_timer) return 0;
    timer_gettime(profile_timer, &t);
    return (uint32_t)t.sec * UINT32_C(1000000) + (uint32_t)t.usec;
}

static void reset_sample(void)
{
    memset(time_us, 0, sizeof(time_us));
    memset(calls, 0, sizeof(calls));
    frames = 0;
    started = mgl_profile_begin();
}

void mgl_profile_end(int category, uint32_t start)
{
    uint32_t elapsed;
    if (!mgl_profile_enabled || category < 0 || category >= MGL_P_COUNT) return;
    elapsed = mgl_profile_begin() - start;
    /* Ignore a backwards system-clock adjustment, not the uint32 wrap. */
    if (elapsed > UINT32_C(0x7fffffff)) return;
    time_us[category] += elapsed;
    ++calls[category];
}

static void report(FILE *out, uint32_t elapsed)
{
    int i;
    fprintf(out, "GPUMiniGL profile: frames=%lu elapsed-ms=%lu (timed diagnostic)\n",
            (unsigned long)frames, (unsigned long)(elapsed / 1000));
    for (i = 0; i < MGL_P_COUNT; ++i) {
        if (i && i % 4 == 0) fputc('\n', out);
        fprintf(out, " %s=%luus/%lu", labels[i],
                (unsigned long)time_us[i], (unsigned long)calls[i]);
    }
    fputc('\n', out);
    fflush(out);
}

void mgl_profile_frame(void)
{
    uint32_t elapsed;
    if (!mgl_profile_enabled) return;
    ++frames;
    elapsed = mgl_profile_begin() - started;
    if (elapsed > UINT32_C(0x7fffffff)) { reset_sample(); return; }
    if (elapsed < UINT32_C(2000000)) return;
    report(stdout, elapsed);
    if (profile_log) report(profile_log, elapsed);
    reset_sample();
}

void mgl_profile_close(void)
{
    mgl_profile_enabled = 0;
    if (profile_log) fclose(profile_log);
    profile_log = NULL;
    profile_timer = NULL;
}

void mgl_profile_init(amidog_timer_t *timer)
{
    mgl_profile_close();
    profile_timer = timer;
    identity[0] = 0;
    cache_identity[0] = 0;
    texture_identity[0] = 0;
}

void mgl_profile_texture_path(int packed, int fence)
{
    snprintf(texture_identity, sizeof(texture_identity),
             "GPUMiniGL textures: %s lookup+CLUT, upload-fence=%s\n",
             packed ? "ARGB4444" : "RGBA8888", fence ? "native-finish" : "ordered-update");
}

void mgl_profile_cache_settings(int configured, int entries, int texture_size)
{
    snprintf(cache_identity, sizeof(cache_identity),
             "GPUMiniGL cache: configured=%d effective=%d texture-size=%dx%d\n",
             configured, entries, texture_size, texture_size);
}

void mgl_profile_identify(unsigned int version, unsigned int revision,
                          unsigned long flags, int width, int height)
{
    snprintf(identity, sizeof(identity),
             "GPUMiniGL " VERSION ": library %u.%u flags=0x%lx output=%dx%d\n",
             version, revision, flags, width, height);
}

void mgl_profile_toggle(void)
{
    if (!profile_timer) return;
    mgl_profile_enabled = !mgl_profile_enabled;
    if (mgl_profile_enabled) {
        /* Append: do not erase an earlier diagnostic run. */
        profile_log = fopen("PROGDIR:fpse-minigl-profile.log", "a");
        fputs(identity, stdout);
        fputs(cache_identity, stdout);
        fputs(texture_identity, stdout);
        if (profile_log) {
            fputs(identity, profile_log);
            fputs(cache_identity, profile_log);
            fputs(texture_identity, profile_log);
            fputs("GPUMiniGL profile enabled; values are total microseconds/calls per sample.\n", profile_log);
            fflush(profile_log);
        }
        puts(profile_log ? "GPUMiniGL profile ON: PROGDIR:fpse-minigl-profile.log" :
                           "GPUMiniGL profile ON: console only (cannot open PROGDIR: log)");
        reset_sample();
    } else {
        if (profile_log) fclose(profile_log);
        profile_log = NULL;
        puts("GPUMiniGL profile OFF");
    }
}

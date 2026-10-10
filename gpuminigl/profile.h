/* Optional diagnostic timing. Disabled path never reads the clock. */
#ifndef FPSE_MINIGL_PROFILE_H
#define FPSE_MINIGL_PROFILE_H
#include <stdint.h>

enum {
    MGL_P_STATE, MGL_P_DRAW, MGL_P_TEX_WAIT, MGL_P_TEX_CONVERT,
    MGL_P_TEX_UPLOAD, MGL_P_VRAM_WAIT, MGL_P_VRAM_CONVERT,
    MGL_P_VRAM_UPLOAD, MGL_P_PRESENT, MGL_P_FILL, MGL_P_LIMIT,
    MGL_P_COUNT
};
struct amidog_timer_s;
#ifdef FPSE_MINIGL_NO_PROFILE
/* Compile out instrumentation, not merely its runtime enable flag. */
#define mgl_profile_enabled 0
#define mgl_profile_init(timer) ((void)0)
#define mgl_profile_identify(version, revision, flags, width, height) ((void)0)
#define mgl_profile_cache_settings(configured, entries, texture_size) ((void)0)
#define mgl_profile_texture_path(packed, fence) ((void)0)
#define mgl_profile_toggle() ((void)0)
#define mgl_profile_close() ((void)0)
#define mgl_profile_begin() UINT32_C(0)
#define mgl_profile_end(category, start) ((void)(category), (void)(start))
#define mgl_profile_frame() ((void)0)
#else
extern int mgl_profile_enabled;
void mgl_profile_init(struct amidog_timer_s *timer);
void mgl_profile_identify(unsigned int version, unsigned int revision,
                          unsigned long flags, int width, int height);
void mgl_profile_cache_settings(int configured, int entries, int texture_size);
void mgl_profile_texture_path(int packed, int fence);
void mgl_profile_toggle(void);
void mgl_profile_close(void);
uint32_t mgl_profile_begin(void);
void mgl_profile_end(int category, uint32_t start);
void mgl_profile_frame(void);
#endif
#endif

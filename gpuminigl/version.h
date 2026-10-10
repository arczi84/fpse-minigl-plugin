#if defined(FPSE_MINIGL_V291)
#define VERSION "0.4.4-v29.1"
#define PLUGIN_NAME "gpuminigl-v29.1"
#define PLUGIN_DESCRIPTION "MiniGL V29 ABI 5; quad sprites, heap cache, title FPS; GCC " __VERSION__
#elif defined(FPSE_MINIGL_SPRITE_UV_AB)
#if !defined(FPSE_MINIGL_HEAP_HASH) || !defined(FPSE_MINIGL_TITLE_FPS) || !defined(FPSE_MINIGL_NO_PROFILE) || defined(FPSE_MINIGL_TRACE)
#error Sprite UV A/B requires heap hash, title FPS, no profiler and no UDP trace
#endif
#if defined(FPSE_MINIGL_VRAM_GUTTERS)
#if !FPSE_MINIGL_SPRITE_UV_AB
#error VRAM gutter C requires the tested B sprite inset
#endif
#if defined(FPSE_MINIGL_UI_CAPTURE) && defined(FPSE_MINIGL_QUAD_SPRITES)
#define VERSION "0.4.4-rc10-quad-E-log2"
#define PLUGIN_NAME "gpuminigl-quad-E"
#define PLUGIN_DESCRIPTION "E with F7 correction toggle and F8 one-frame capture to PROGDIR; no profiler"
#elif defined(FPSE_MINIGL_UI_CAPTURE)
#define VERSION "0.4.4-rc10-ui-D"
#define PLUGIN_NAME "gpuminigl-ui-D"
#define PLUGIN_DESCRIPTION "C rendering unchanged; F8 captures one frame of textured primitives to PROGDIR"
#elif defined(FPSE_MINIGL_QUAD_SPRITES)
#define VERSION "0.4.4-rc10-quad-E"
#define PLUGIN_NAME "gpuminigl-quad-E"
#define PLUGIN_DESCRIPTION "C plus experimental 1:1 rectangle-quad UV inset; F7 toggles; no profiler"
#else
#define VERSION "0.4.4-rc10-spriteuv-C"
#define PLUGIN_NAME "gpuminigl-spriteuv-C"
#define PLUGIN_DESCRIPTION "RC10 heap/FPS; TEST C: B sprite inset plus neighbour gutters on linear VRAM tiles"
#endif
#elif FPSE_MINIGL_SPRITE_UV_AB
#define VERSION "0.4.4-rc10-spriteuv-B"
#define PLUGIN_NAME "gpuminigl-spriteuv-B"
#define PLUGIN_DESCRIPTION "RC10 heap/FPS; TEST B: half-texel inset for magnified linear sprites only"
#else
#define VERSION "0.4.4-rc10-spriteuv-A"
#define PLUGIN_NAME "gpuminigl-spriteuv-A"
#define PLUGIN_DESCRIPTION "RC10 heap/FPS; CONTROL A: original sprite UV; eligible sprite counter"
#endif
#elif defined(FPSE_MINIGL_TITLE_FPS)
#if !defined(FPSE_MINIGL_HEAP_HASH) || !defined(FPSE_MINIGL_NO_PROFILE) || defined(FPSE_MINIGL_TRACE)
#error FPS release requires heap hashcache, no profiler and no UDP trace
#endif
#define VERSION "0.4.4-rc10-heap-fps"
#define PLUGIN_NAME "gpuminigl-rc10-heap-fps"
#define PLUGIN_DESCRIPTION "RC10 heap hashcache; window title FPS/emu, F9 toggles; no profiling/UDP"
#elif defined(FPSE_MINIGL_HASH_AB_TEST)
#ifdef FPSE_MINIGL_HEAP_HASH
#define VERSION "0.4.4-rc10-heap"
#define PLUGIN_NAME "gpuminigl-rc10-heap"
#define PLUGIN_DESCRIPTION "RC10 hashcache A/B: heap buckets, no tracing/profiling"
#else
#define VERSION "0.4.4-rc10-static"
#define PLUGIN_NAME "gpuminigl-rc10-static"
#define PLUGIN_DESCRIPTION "RC10 hashcache A/B: static buckets, no tracing/profiling"
#endif
#elif defined(FPSE_MINIGL_TRACE)
#define VERSION "0.4.10-udp-ram"
#if __GNUC__ == 6 && __GNUC_MINOR__ == 5
#define PLUGIN_NAME "gpuminigl-debug-gcc65"
#else
#define PLUGIN_NAME "gpuminigl-debug"
#endif
#define PLUGIN_DESCRIPTION "GPU hang diagnostics: PROGDIR:fpse-minigl-udp.cfg or close-per-line file log"
#elif defined(FPSE_MINIGL_CLASSIC_ORTHO)
#define VERSION "0.4.5-ortho-test"
#define PLUGIN_NAME "gpuminigl-ortho"
#define PLUGIN_DESCRIPTION "Classic ortho experiment, no profiling; PROGDIR:minigl-ortho.library"
#elif defined(FPSE_MINIGL_LIBRARY_PROFILE)
#define VERSION "0.4.4-libprofile"
#define PLUGIN_NAME "gpuminigl-libprofile"
#define PLUGIN_DESCRIPTION "Classic library -pg diagnostic; opens PROGDIR:minigl-profile.library"
#elif defined(FPSE_MINIGL_GPROF)
#include "gprof_build.h"
#define VERSION "0.4.4-gprof"
#define PLUGIN_NAME FPSE_GPROF_NAME
#define PLUGIN_DESCRIPTION "MiniGL -pg diagnostic: automatic PROGDIR gmon on GPU_Close"
#elif defined(FPSE_MINIGL_NO_PROFILE)
#define VERSION "0.4.4-hashcache"
#if __GNUC__ == 6 && __GNUC_MINOR__ == 5
#define PLUGIN_NAME "gpuminigl-gcc65"
#else
#define PLUGIN_NAME "gpuminigl"
#endif
#define PLUGIN_DESCRIPTION "MiniGL renderer: profiling compiled out; GCC " __VERSION__
#else
#define VERSION "0.4.4-profile"
#define PLUGIN_NAME "gpuminigl-profile"
#define PLUGIN_DESCRIPTION "MiniGL timing diagnostic: automatic PROGDIR:fpse-minigl-profile.log"
#endif
#if defined(FPSE_MINIGL_V291)
#define VERSTAG "\0$VER: " PLUGIN_NAME " " VERSION " (07.10.2026)"
#elif defined(FPSE_MINIGL_QUAD_SPRITES) && defined(FPSE_MINIGL_UI_CAPTURE)
#define VERSTAG "\0$VER: " PLUGIN_NAME " " VERSION " (26.09.2026)"
#elif defined(FPSE_MINIGL_QUAD_SPRITES)
#define VERSTAG "\0$VER: " PLUGIN_NAME " " VERSION " (21.09.2026)"
#elif defined(FPSE_MINIGL_SPRITE_UV_AB)
#define VERSTAG "\0$VER: " PLUGIN_NAME " " VERSION " (20.09.2026)"
#elif defined(FPSE_MINIGL_TRACE) || defined(FPSE_MINIGL_HASH_AB_TEST) || defined(FPSE_MINIGL_TITLE_FPS)
#define VERSTAG "\0$VER: " PLUGIN_NAME " " VERSION " (18.09.2026)"
#else
#define VERSTAG "\0$VER: " PLUGIN_NAME " " VERSION " (13.09.2026)"
#endif

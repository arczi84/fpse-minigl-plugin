/* MiniGL texture cache; reuses the GPUW3D primitive decoder. GPL-2.0-or-later. */
#include "../gpuw3d/w3d/w3d.h"
#include "../gpuw3d/w3d/texture.h"
#include "pixels.h"
#include "profile.h"
#include "trace.h"
#include "texdecode.h"
#include "../gpuw3d/w3d/repair.h"

int texred = 0, texcache = 256, texfilter = 0;

typedef struct Texture {
    GLuint name;
    unsigned long window, age;
    int valid, mode, page_x, page_y, clut_x, clut_y, pass, opaque;
    struct Texture *hash_next;
    unsigned int hash_bucket;
} Texture;

static Texture *cache;
#define TEXTURE_BUCKETS 1024
#ifdef FPSE_MINIGL_HEAP_HASH
/* Diagnostic: keep the same index and lookup algorithm, but remove its
 * 4 KiB table from the m68k BSS. Do not confuse this with a cache rollback. */
static Texture **texture_buckets;
#else
static Texture *texture_buckets[TEXTURE_BUCKETS];
#endif
#define TEXTURE_BUCKET_BYTES (TEXTURE_BUCKETS * sizeof(*texture_buckets))
/* Keep separate MRU entries: semi-transparent primitives alternate passes. */
static Texture *last_texture[2];
#ifndef FPSE_MINIGL_CACHE_PROBE
#define FPSE_MINIGL_CACHE_PROBE() ((void)0)
#endif
#ifndef FPSE_MINIGL_EVICTION_PROBE
#define FPSE_MINIGL_EVICTION_PROBE() ((void)0)
#endif
static unsigned char *pixels;
static void *lookup;
static int count, size;
static unsigned long age;
#ifndef FPSE_MINIGL_NO_PROFILE
static unsigned long hits, uploads, invalidations, report_frames;
#define COUNT_STAT(name) (++(name))
#else
#define COUNT_STAT(name) ((void)0)
#endif

/* Hash only selects a chain; texture_matches always checks the full key.
 * Fixed-width arithmetic keeps bucket selection identical on host and 68k. */
static __inline__ unsigned int texture_hash(int mode, int page_x, int page_y,
    int clut_x, int clut_y, int pass, unsigned long window)
{
    uint32_t h = (uint32_t)window ^ ((uint32_t)page_x << 2) ^
        ((uint32_t)page_y << 12) ^ ((uint32_t)clut_x << 19) ^
        ((uint32_t)clut_y << 7) ^ ((uint32_t)mode << 27) ^ ((uint32_t)pass << 29);
    h ^= h >> 16;
    h *= UINT32_C(2654435761);
    return h >> 22;
}

static void texture_unlink(Texture *t)
{
    Texture **link;
    if (!t->valid) return;
    link = &texture_buckets[t->hash_bucket];
    while (*link && *link != t) link = &(*link)->hash_next;
    if (*link) *link = t->hash_next;
    t->hash_next = NULL;
    t->valid = 0;
}

int TCacheInit(void)
{
    int i, bytes = mgl_gpu_packed_textures ? 2 : 4;
    count = psx_cache_entries(texcache);
    size = 256 >> CLAMP(texred, 0, 2);
    mgl_trace_log("TCacheInit ENTER entries=%d texture_size=%d bytes_per_pixel=%d", count, size, bytes);
    age = 0;
    last_texture[0] = last_texture[1] = NULL;
#ifdef FPSE_MINIGL_HEAP_HASH
    texture_buckets = calloc(TEXTURE_BUCKETS, sizeof(*texture_buckets));
    if (!texture_buckets) return FPSE_ERR;
#else
    memset(texture_buckets, 0, TEXTURE_BUCKET_BYTES);
#endif
#ifndef FPSE_MINIGL_NO_PROFILE
    hits = uploads = invalidations = report_frames = 0;
#endif
    cache = calloc(count, sizeof(*cache));
    pixels = malloc(size * size * bytes);
    lookup = malloc(2 * 65536 * bytes);
    if (!cache || !pixels || !lookup) {
        mgl_trace_log("TCacheInit allocation FAIL cache=%08lx pixels=%08lx lookup=%08lx", (unsigned long)cache, (unsigned long)pixels, (unsigned long)lookup);
        TCacheExit();
        return FPSE_ERR;
    }
    for (i = 0; i < 65536; ++i) {
        if (mgl_gpu_packed_textures) {
            ((uint16_t *)lookup)[i] = psx_argb4444(i, 0);
            ((uint16_t *)lookup)[65536 + i] = psx_argb4444(i, 1);
        } else {
            ((uint32_t *)lookup)[i] = psx_rgba_word(i, 0);
            ((uint32_t *)lookup)[65536 + i] = psx_rgba_word(i, 1);
        }
    }
    mgl_profile_cache_settings(texcache, count, size);
    mgl_trace_log("TCacheInit LEAVE ok");
    return FPSE_OK;
}

void TCacheExit(void)
{
    int i;
    last_texture[0] = last_texture[1] = NULL;
#ifdef FPSE_MINIGL_HEAP_HASH
    free(texture_buckets);
    texture_buckets = NULL;
#else
    memset(texture_buckets, 0, TEXTURE_BUCKET_BYTES);
#endif
    if (cache) {
        for (i = 0; i < count; ++i)
            if (cache[i].name) glDeleteTextures(1, &cache[i].name);
    }
    free(cache); cache = NULL;
    free(pixels); pixels = NULL;
    free(lookup); lookup = NULL;
    memset(array_unit_tex, 0, sizeof(GLuint) * MAX_TEX_UNITS);
}

void TCacheInvalidate(void)
{
    int i;
    if (!cache) return;
    FLUSH_ARRAY();
    memset(texture_buckets, 0, TEXTURE_BUCKET_BYTES);
    last_texture[0] = last_texture[1] = NULL;
    for (i = 0; i < count; ++i) {
        if (cache[i].valid) COUNT_STAT(invalidations);
        cache[i].valid = 0;
        cache[i].hash_next = NULL;
    }
}

/* The framebuffer and texture pages share PSX VRAM. A frame clear must
 * invalidate overlapping pages/CLUTs, not every cached texture. Keep GL
 * objects alive: already submitted primitives may still reference them. */
void mgl_gpu_invalidate_area(int x, int y, int width, int height)
{
    int i;
    if (!cache || width <= 0 || height <= 0) return;
    FLUSH_ARRAY();
    for (i = 0; i < count; ++i) {
        Texture *t = &cache[i];
        if (t->valid && w3d_texture_touched(t->mode, t->page_x, t->page_y,
                t->clut_x, t->clut_y, x, y, width, height, 512)) {
            texture_unlink(t);
            COUNT_STAT(invalidations);
        }
    }
}

void mgl_gpu_invalidate(void) { TCacheInvalidate(); }
void TCacheEndOfFrame(int verbose)
{
#ifndef FPSE_MINIGL_NO_PROFILE
    if (++report_frames < 120) return;
    if (verbose)
        printf("GPUMiniGL cache/120 frames: hits=%lu uploads=%lu invalidated=%lu\n",
               hits, uploads, invalidations);
    hits = uploads = invalidations = report_frames = 0;
#else
    (void)verbose;
#endif
}

static __inline__ int texture_matches(const Texture *c, int clut_x, int clut_y,
    int pass, unsigned long window)
{
    FPSE_MINIGL_CACHE_PROBE();
    return c && c->valid && c->mode == GlobalTextTP && c->page_x == GlobalTextAddrX &&
        c->page_y == GlobalTextAddrY && c->clut_x == clut_x &&
        c->clut_y == clut_y && c->window == window && c->pass == pass;
}

static Texture *get_texture(int clut_x, int clut_y, int pass)
{
    unsigned long window = lGPUInfoVals[INFO_TW];
    Texture *t = NULL;
    Texture *c;
    unsigned int bucket;
    int i, step;
    uint32_t measured;
    unsigned long trace_cache = mgl_trace_begin(MGL_T_CACHE, clut_x, clut_y, pass);
    unsigned long trace_step;
    Texture *recent = last_texture[pass];
    /* A pointer alone is not a hit: invalidation and LRU reuse can change
     * this slot. Check the full key and validity, exactly as in the scan. */
    if (texture_matches(recent, clut_x, clut_y, pass, window)) {
        recent->age = ++age;
        COUNT_STAT(hits);
        mgl_trace_end(MGL_T_CACHE, trace_cache, recent->name);
        return recent;
    }
    bucket = texture_hash(GlobalTextTP, GlobalTextAddrX, GlobalTextAddrY,
        clut_x, clut_y, pass, window);
    for (c = texture_buckets[bucket]; c; c = c->hash_next) {
        if (texture_matches(c, clut_x, clut_y, pass, window)) {
            c->age = ++age;
            COUNT_STAT(hits);
            last_texture[pass] = c;
            mgl_trace_end(MGL_T_CACHE, trace_cache, c->name);
            return c;
        }
    }
    /* Cache miss only: preserve the old invalid-first/LRU victim policy. */
    for (i = 0; i < count; ++i) {
        c = &cache[i];
        FPSE_MINIGL_EVICTION_PROBE();
        /* Never overwrite the first pass while obtaining the opaque texture. */
        if (pass && c->name && c->name == array_unit_tex[0]) continue;
        if (!t || (!c->valid && t->valid) ||
            (c->valid == t->valid && c->age < t->age)) t = c;
    }
    if (!t) { mgl_trace_end(MGL_T_CACHE, trace_cache, 0); return NULL; }
    FLUSH_ARRAY();
    /* Classic uses the same ordered texture update as GPUW3D. Native
     * PiStorm still requires a fence before mutating referenced textures. */
    measured = mgl_profile_begin();
    mgl_gpu_texture_wait();
    mgl_profile_end(MGL_P_TEX_WAIT, measured);
    measured = mgl_profile_begin();
    texture_unlink(t);
    step = 256 / size;
    t->mode = GlobalTextTP;
    t->page_x = GlobalTextAddrX; t->page_y = GlobalTextAddrY;
    t->clut_x = clut_x; t->clut_y = clut_y;
    t->window = window; t->pass = pass; t->opaque = 0;
    trace_step = mgl_trace_begin(MGL_T_TEXDECODE, t->mode, size, mgl_gpu_packed_textures);
    if (mgl_gpu_packed_textures)
        t->opaque = psx_decode_page16(psxVub, t->mode, t->page_x, t->page_y,
            clut_x, clut_y, window, step, (uint16_t *)lookup + pass * 65536,
            (uint16_t *)pixels);
    else
        t->opaque = psx_decode_page32(psxVub, t->mode, t->page_x, t->page_y,
            clut_x, clut_y, window, step, (uint32_t *)lookup + pass * 65536,
            (uint32_t *)pixels);
    mgl_profile_end(MGL_P_TEX_CONVERT, measured);
    mgl_trace_end(MGL_T_TEXDECODE, trace_step, t->opaque);
    if (!t->name) {
        trace_step = mgl_trace_begin(MGL_T_TEXGEN, 1, 0, 0);
        glGenTextures(1, &t->name);
        mgl_trace_end(MGL_T_TEXGEN, trace_step, t->name);
    }
    if (!t->name) { t->valid = 0; mgl_trace_end(MGL_T_CACHE, trace_cache, 0); return NULL; }
    if (!mgl_gpu_texture_upload(t->name, size, pixels)) {
        t->valid = 0;
        mgl_trace_end(MGL_T_CACHE, trace_cache, 0);
        return NULL;
    }
    COUNT_STAT(uploads);
    t->valid = 1;
    t->hash_bucket = bucket;
    t->hash_next = texture_buckets[bucket];
    texture_buckets[bucket] = t;
    t->age = ++age;
    last_texture[pass] = t;
    mgl_trace_end(MGL_T_CACHE, trace_cache, t->name);
    return t;
}

int set_texture(int clX, int clY, int opaque)
{
    Texture *t;
    if (!cache) return 0;
    if (opaque) {
        FLUSH_ARRAY();
        array_unit_tex[0] = array_unit_tex[1];
        array_unit_tex[1] = 0;
        return 0;
    }
    t = get_texture(clX, clY, 0);
    if (!t) { FLUSH_ARRAY(); array_unit_tex[0] = 0; return 0; }
    if (array_unit_tex[0] != t->name) FLUSH_ARRAY();
    array_unit_tex[0] = t->name;
    array_unit_tex[1] = 0;
    if (DrawSemiTrans && t->opaque) {
        t = get_texture(clX, clY, 1);
        if (t) {
            array_unit_tex[1] = t->name;
            return 1;
        }
    }
    return 0;
}

void mgl_gpu_vram_changed(int x, int y, int width, int height)
{
    mgl_gpu_invalidate_area(x, y, width, height);
    if (!PSXDisplay.RGB24)
        mgl_gpu_upload(x, y, width, height);
}

void CheckWriteUpdate(void)
{
    mgl_gpu_vram_changed(VRAMWrite.x, VRAMWrite.y, VRAMWrite.Width, VRAMWrite.Height);
}

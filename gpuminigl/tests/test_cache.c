/* Exercise the real cache with host-side GL stubs (no Amiga headers). */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../profile.h"

#ifndef FPSE_MINIGL_NO_PROFILE
uint32_t mgl_profile_begin(void) { return 0; }
void mgl_profile_end(int category, uint32_t start) { (void)category; (void)start; }
static int logged_configured, logged_entries, logged_texture_size;
void mgl_profile_cache_settings(int configured, int entries, int texture_size)
{
    logged_configured = configured;
    logged_entries = entries;
    logged_texture_size = texture_size;
}
#endif

#define _W3D_H_
#define CLAMP(v, lo, hi) ((v) < (lo) ? (lo) : (v) > (hi) ? (hi) : (v))
#define FPSE_OK 0
#define FPSE_ERR -1
#define INFO_TW 0
#define MAX_TEX_UNITS 4
typedef unsigned int GLuint;
static unsigned char vram[1024 * 1024];
static unsigned char *psxVub = vram;
static unsigned long lGPUInfoVals[1];
static int GlobalTextTP, GlobalTextAddrX, GlobalTextAddrY, DrawSemiTrans;
static struct { int RGB24; } PSXDisplay;
static struct { int x, y, Width, Height; } VRAMWrite;
static GLuint array_unit_tex[MAX_TEX_UNITS];
static unsigned int next_name, upload_calls, finish_calls, screen_uploads;
static int mgl_gpu_packed_textures = 1, native_backend;
static int fail_upload;
static unsigned int cache_probes, eviction_probes;
#define FPSE_MINIGL_CACHE_PROBE() (++cache_probes)
#define FPSE_MINIGL_EVICTION_PROBE() (++eviction_probes)

static void FLUSH_ARRAY(void) {}
static void mgl_gpu_texture_wait(void) { if (native_backend) ++finish_calls; }
static void glGenTextures(int n, GLuint *names)
{
    while (n--) *names++ = ++next_name;
}
static void glDeleteTextures(int n, const GLuint *names) { (void)n; (void)names; }
static int mgl_gpu_texture_upload(GLuint name, int size, const void *data)
{
    assert(name && size > 0 && data);
    ++upload_calls;
    return !fail_upload;
}
static void mgl_gpu_upload(int x, int y, int w, int h)
{
    (void)x; (void)y; (void)w; (void)h;
    ++screen_uploads;
}

#ifdef FPSE_MINIGL_HEAP_HASH
static int fail_calloc_at, calloc_calls;
static void *cache_test_calloc(size_t n, size_t size)
{
    if (++calloc_calls == fail_calloc_at) return NULL;
    return calloc(n, size);
}
#define calloc cache_test_calloc
#endif
#include "../texture.c"
#ifdef FPSE_MINIGL_HEAP_HASH
#undef calloc
#endif

static void check_index(void)
{
    unsigned int bucket;
    int i, seen[256] = {0};
    Texture *t;
    for (bucket = 0; bucket < TEXTURE_BUCKETS; ++bucket) {
        for (t = texture_buckets[bucket]; t; t = t->hash_next) {
            assert(t >= cache && t < cache + count);
            assert(t->valid && t->hash_bucket == bucket);
            assert(texture_hash(t->mode, t->page_x, t->page_y,
                t->clut_x, t->clut_y, t->pass, t->window) == bucket);
            assert(++seen[t - cache] == 1); /* also catches cycles */
        }
    }
    for (i = 0; i < count; ++i) assert(seen[i] == !!cache[i].valid);
}

/* Independent old linear lookup/victim selection, checked before mutation. */
static Texture *reference_lookup(int x, int y, int pass, int *hit)
{
    int i;
    Texture *victim = NULL;
    *hit = 0;
    for (i = 0; i < count; ++i) {
        Texture *c = &cache[i];
        if (c->valid && c->mode == GlobalTextTP && c->page_x == GlobalTextAddrX &&
            c->page_y == GlobalTextAddrY && c->clut_x == x && c->clut_y == y &&
            c->window == lGPUInfoVals[INFO_TW] && c->pass == pass) {
            *hit = 1;
            return c;
        }
        if (pass && c->name && c->name == array_unit_tex[0]) continue;
        if (!victim || (!c->valid && victim->valid) ||
            (c->valid == victim->valid && c->age < victim->age)) victim = c;
    }
    return victim;
}

static void test_hash_index(void)
{
    Texture *entries[167], *collisions[4], *expected, *actual;
    unsigned long windows[4];
    unsigned int before, old_probes = 0, key, bucket, found = 0;
    uint32_t rng = 12345;
    int i, hit, x, y, pass;
    texcache = 256; texred = 2; DrawSemiTrans = 0;
    GlobalTextTP = 2; GlobalTextAddrX = 640; GlobalTextAddrY = 0;
    mgl_gpu_packed_textures = 1; native_backend = 0;
    assert(TCacheInit() == FPSE_OK);

    /* Rotating textures defeat a one-entry MRU, like the measured workload.
     * Compare lookup work, not host execution time or target FPS. */
    for (i = 0; i < 167; ++i) {
        lGPUInfoVals[INFO_TW] = i;
        entries[i] = get_texture(0, 480, 0);
        assert(entries[i]);
    }
    before = upload_calls;
    cache_probes = eviction_probes = 0;
    for (i = 0; i < 10000; ++i) {
        key = (unsigned)i % 167;
        lGPUInfoVals[INFO_TW] = key;
        assert(get_texture(0, 480, 0) == entries[key]);
        old_probes += 1 + (unsigned)(entries[key] - cache) + 1;
    }
    assert(upload_calls == before && eviction_probes == 0);
    assert(cache_probes < old_probes / 10);
    printf("PASS: rotating 167 textures / 10000 hits: %u key probes vs %u linear; zero victim scans/uploads\n",
           cache_probes, old_probes);
    check_index();
    TCacheInvalidate();
    check_index();

    /* Deliberate collisions: removing middle/head/tail must not lose siblings. */
    bucket = texture_hash(2, 640, 0, 0, 480, 0, 0);
    for (key = 0; key < 1048576 && found < 4; ++key) {
        if (texture_hash(2, 640, 0, 0, 480, 0, key) != bucket) continue;
        windows[found] = key;
        lGPUInfoVals[INFO_TW] = key;
        collisions[found++] = get_texture(0, 480, 0);
    }
    assert(found == 4);
    check_index();
    texture_unlink(collisions[1]); check_index();
    texture_unlink(collisions[3]); check_index();
    texture_unlink(collisions[0]); check_index();
    before = upload_calls;
    lGPUInfoVals[INFO_TW] = windows[2];
    assert(get_texture(0, 480, 0) == collisions[2]);
    assert(upload_calls == before);
    for (i = 0; i < 4; ++i) {
        lGPUInfoVals[INFO_TW] = windows[i];
        assert(get_texture(0, 480, 0));
        check_index();
    }
    mgl_gpu_invalidate_area(640, 0, 1, 1);
    check_index();
    TCacheExit();

    /* Full-cache churn, every key field, both passes and failed replacement.
     * Compare the exact chosen slot with the old policy for every request. */
    texcache = 64;
    assert(TCacheInit() == FPSE_OK);
    for (i = 0; i < 1500; ++i) {
        rng = rng * UINT32_C(1664525) + UINT32_C(1013904223);
        key = (rng >> 16) % 100;
        GlobalTextTP = key % 3;
        GlobalTextAddrX = (key % 16) * 64;
        GlobalTextAddrY = ((key >> 1) & 1) * 256;
        x = (key % 64) * 16; y = 480 + key % 32;
        pass = key & 1;
        lGPUInfoVals[INFO_TW] = key * 1025;
        if (!(i % 31)) mgl_gpu_invalidate_area(0, 0, 320, 240);
        if (!(i % 233)) TCacheInvalidate();
        expected = reference_lookup(x, y, pass, &hit);
        before = upload_calls;
        fail_upload = !(i % 19);
        actual = get_texture(x, y, pass);
        assert(upload_calls == before + !hit);
        if (fail_upload && !hit) {
            assert(!actual && !expected->valid);
        } else {
            assert(actual == expected && actual->valid);
            array_unit_tex[0] = actual->name;
        }
        fail_upload = 0;
        check_index();
    }
    TCacheExit();
#ifdef FPSE_MINIGL_HEAP_HASH
    assert(!texture_buckets);
#else
    for (i = 0; i < TEXTURE_BUCKETS; ++i) assert(!texture_buckets[i]);
#endif
}

int main(void)
{
    unsigned int before, first, second;
    int i;
    texcache = 64;
    texred = 2;
#ifdef FPSE_MINIGL_HEAP_HASH
    /* Table allocation failure and failure after table allocation must both
     * permit cleanup, repeated cleanup and a later successful open. */
    for (i = 1; i <= 2; ++i) {
        calloc_calls = 0; fail_calloc_at = i;
        assert(TCacheInit() == FPSE_ERR);
        TCacheExit(); TCacheExit();
        assert(!texture_buckets && !cache && !pixels && !lookup);
    }
    fail_calloc_at = 0;
#endif
    assert(TCacheInit() == FPSE_OK);
#ifndef FPSE_MINIGL_NO_PROFILE
    assert(logged_configured == 64 && logged_entries == 64 && logged_texture_size == 64);
#endif
    GlobalTextTP = 2; GlobalTextAddrX = 640; GlobalTextAddrY = 0;
    assert(!set_texture(0, 0, 0));
    first = array_unit_tex[0];
    assert(upload_calls == 1 && finish_calls == 0);

    /* 120 framebuffer writes must not rebuild an unrelated texture page. */
    for (i = 0; i < 120; ++i) {
        mgl_gpu_vram_changed(0, 0, 320, 240);
        assert(!set_texture(0, 0, 0));
        assert(array_unit_tex[0] == first);
    }
    assert(upload_calls == 1 && finish_calls == 0 && screen_uploads == 120);
    /* Last texel, not just the page origin, invalidates; edges are exclusive. */
    mgl_gpu_invalidate_area(896, 0, 1, 256);
    set_texture(0, 0, 0);
    assert(upload_calls == 1);
    mgl_gpu_invalidate_area(895, 255, 1, 1);
    set_texture(0, 0, 0);
    assert(upload_calls == 2);

    /* A fill invalidates both semi-transparent and opaque cached passes. */
    vram[640 * 2] = 0x1f;
    mgl_gpu_invalidate_area(640, 0, 1, 1);
    DrawSemiTrans = 1;
    assert(set_texture(0, 0, 0) == 1);
    first = array_unit_tex[0]; second = array_unit_tex[1];
    assert(first && second && first != second);
    before = upload_calls;
    mgl_gpu_invalidate_area(0, 0, 320, 240);
    assert(set_texture(0, 0, 0) == 1);
    assert(upload_calls == before);
    mgl_gpu_invalidate_area(640, 0, 1, 1);
    assert(set_texture(0, 0, 0) == 1);
    assert(upload_calls == before + 2);
    assert(array_unit_tex[0] != array_unit_tex[1]);
    DrawSemiTrans = 0;

    /* 4-bit palette-only writes, including its last entry. */
    GlobalTextTP = 0; GlobalTextAddrY = 256;
    set_texture(32, 480, 0);
    before = upload_calls;
    mgl_gpu_invalidate_area(48, 480, 1, 1);
    set_texture(32, 480, 0);
    assert(upload_calls == before);
    mgl_gpu_invalidate_area(47, 480, 1, 1);
    set_texture(32, 480, 0);
    assert(upload_calls == before + 1);

    /* 8-bit CLUT crossing a row AND the end of VRAM. */
    GlobalTextTP = 1;
    set_texture(1008, 511, 0);
    before = upload_calls;
    mgl_gpu_invalidate_area(0, 0, 1, 1);
    set_texture(1008, 511, 0);
    assert(upload_calls == before + 1);

    /* A direct-color page wraps horizontally; CLUT writes do not affect it. */
    GlobalTextTP = 2; GlobalTextAddrX = 960;
    set_texture(32, 480, 0);
    before = upload_calls;
    mgl_gpu_invalidate_area(32, 480, 1, 1); /* inside the wrapped texture page */
    set_texture(32, 480, 0);
    assert(upload_calls == before + 1);
    set_texture(32, 100, 0);
    before = upload_calls;
    mgl_gpu_invalidate_area(32, 100, 1, 1);
    mgl_gpu_invalidate_area(0, 256, 0, 1);
    mgl_gpu_invalidate_area(0, 256, 1, 0);
    set_texture(32, 100, 0);
    assert(upload_calls == before);
    mgl_gpu_invalidate_area(0, 256, 1, 1);
    set_texture(32, 100, 0);
    assert(upload_calls == before + 1);

    /* Evict entries repeatedly; the currently selected first pass survives
     * allocation of the corresponding opaque pass even with a full cache. */
    memset(vram, 0x1f, sizeof(vram));
    TCacheInvalidate();
    DrawSemiTrans = 1;
    for (i = 0; i < count * 3; ++i) {
        lGPUInfoVals[INFO_TW] = i;
        assert(set_texture(0, 0, 0) == 1);
        assert(array_unit_tex[0] && array_unit_tex[1]);
        assert(array_unit_tex[0] != array_unit_tex[1]);
        before = upload_calls;
        cache_probes = 0;
        assert(set_texture(0, 0, 0) == 1);
        assert(upload_calls == before);
        assert(cache_probes == 2); /* one MRU check per pass, no table scan */
    }
    DrawSemiTrans = 0;

    /* Reset/LoadState still invalidate everything. RGB24 skips display upload. */
    before = upload_calls;
    TCacheInvalidate();
    set_texture(32, 100, 0);
    assert(upload_calls == before + 1);
    PSXDisplay.RGB24 = 1;
    before = screen_uploads;
    mgl_gpu_vram_changed(0, 0, 1024, 512);
    assert(screen_uploads == before);
    TCacheExit();
    mgl_gpu_invalidate_area(0, 0, 1024, 512);
    TCacheInvalidate();
    texcache = 256;
    assert(TCacheInit() == FPSE_OK);
#ifndef FPSE_MINIGL_NO_PROFILE
    assert(logged_configured == 256 && logged_entries == 256);
#endif
    TCacheExit();
    texcache = 0;
    assert(TCacheInit() == FPSE_OK);
#ifndef FPSE_MINIGL_NO_PROFILE
    assert(logged_configured == 0 && logged_entries == 256);
#endif
    TCacheExit();
    mgl_gpu_packed_textures = 0;
    native_backend = 1;
    assert(TCacheInit() == FPSE_OK);
    before = finish_calls;
    set_texture(0, 0, 0);
    assert(finish_calls == before + 1);
    set_texture(0, 0, 0);
    assert(finish_calls == before + 1);
    mgl_gpu_invalidate_area(0, 0, 1024, 512);
    set_texture(0, 0, 0);
    assert(finish_calls == before + 2);
    TCacheInvalidate();
    fail_upload = 1;
    set_texture(0, 0, 0);
    assert(!array_unit_tex[0]);
    fail_upload = 0;
    before = upload_calls;
    set_texture(0, 0, 0);
    assert(array_unit_tex[0] && upload_calls == before + 1);
    /* Ordinary repeated draws are also constant-time after a late-slot hit. */
    DrawSemiTrans = 0;
    for (i = 0; i < 32; ++i) {
        lGPUInfoVals[INFO_TW] = i + 512;
        set_texture(0, 0, 0);
    }
    before = upload_calls;
    cache_probes = 0;
    for (i = 0; i < 120; ++i) set_texture(0, 0, 0);
    assert(upload_calls == before && cache_probes == 120);
    /* Changing every key component must reject the previous MRU entry. */
    GlobalTextTP = 1; set_texture(0, 0, 0);
    assert(upload_calls == ++before);
    GlobalTextAddrX = 640; set_texture(0, 0, 0);
    assert(upload_calls == ++before);
    GlobalTextAddrY = 0; set_texture(0, 0, 0);
    assert(upload_calls == ++before);
    set_texture(16, 0, 0); assert(upload_calls == ++before);
    set_texture(16, 1, 0); assert(upload_calls == ++before);
    lGPUInfoVals[INFO_TW] = 1024;
    set_texture(16, 1, 0); assert(upload_calls == ++before);
    TCacheExit();
    assert(!last_texture[0] && !last_texture[1]);
    test_hash_index();
    puts("PASS: real MiniGL cache; hash collisions/removal, linear-policy equivalence, MRU, invalidation, CLUT, reset, native fence");
    return 0;
}

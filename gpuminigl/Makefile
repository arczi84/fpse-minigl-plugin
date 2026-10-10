# Standalone AmigaOS 3 / 68060 build; no legacy SDK object archives needed.
.DEFAULT_GOAL := all
CROSS ?= /opt/amiga-gcc16-rc9-fix/bin/m68k-amigaos-
CC = $(CROSS)gcc
STRIP = $(CROSS)strip
HOSTCC ?= cc
OBJDIR ?= obj
BINDIR ?= bin
MINIGL_SRC ?= ../../Pistorm3D/PiStorm3D_V23_Classic/dev/MiniGL_Library_Source_Code
CPPFLAGS += -I. -Iinclude -I$(MINIGL_SRC)/include -I../gpuw3d -I.. \
            -DAMIGA -D__AMIGA__ -DFPSE_MINIGL -DMSB_FIRST $(SDK_EXTRA_CPPFLAGS)
CFLAGS = -std=gnu89 -O2 -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare \
         -mcpu=68060 -mhard-float -mcrt=nix20 -fno-strict-aliasing -fcommon -fomit-frame-pointer
LDFLAGS = -mcpu=68060 -mhard-float -mcrt=nix20
LOCAL = backend texture plugin dll minigl_open profile
CORE = gpu prim draw amiga
OBJECTS = $(addprefix $(OBJDIR)/,$(addsuffix .o,$(LOCAL))) \
          $(addprefix $(OBJDIR)/core_,$(addsuffix .o,$(CORE))) \
          $(OBJDIR)/soft.o $(OBJDIR)/render.o $(OBJDIR)/timer.o $(OBJDIR)/dll_main.o \
          $(OBJDIR)/minigl_base.o $(OBJDIR)/minigl_dispatch_stub_gcc.o
RELEASE_OBJECTS = $(filter-out $(OBJDIR)/backend.o $(OBJDIR)/texture.o $(OBJDIR)/plugin.o $(OBJDIR)/profile.o,$(OBJECTS)) \
                  $(OBJDIR)/release/backend.o $(OBJDIR)/release/texture.o $(OBJDIR)/release/plugin.o
ifeq ($(TRACE_BUILD),1)
RELEASE_OBJECTS += $(OBJDIR)/trace.o $(OBJDIR)/trace_udp.o $(OBJDIR)/trace_dma.o
endif
ifeq ($(UI_CAPTURE_BUILD),1)
RELEASE_OBJECTS += $(OBJDIR)/ui_capture.o
endif

.PHONY: debug-trace debug-trace-gcc65
debug-trace:
	$(MAKE) OBJDIR=obj/debug-trace BINDIR=bin/debug-trace TRACE_BUILD=1 \
	 SDK_EXTRA_CPPFLAGS='$(SDK_EXTRA_CPPFLAGS) -DFPSE_MINIGL_TRACE -DFPSE_MINIGL_NO_PROFILE' \
	 bin/debug-trace/gpuminigl.dll

# Separate objects also for the GCC 6.5 comparison; never reuse RC9 objects.
debug-trace-gcc65:
	$(MAKE) CROSS=/opt/amiga/bin/m68k-amigaos- \
	 OBJDIR=obj/debug-trace-gcc65 BINDIR=bin/debug-trace-gcc65 TRACE_BUILD=1 \
	 LDLIBS=-lmui \
	 SDK_EXTRA_CPPFLAGS='$(SDK_EXTRA_CPPFLAGS) -DFPSE_MINIGL_TRACE -DFPSE_MINIGL_NO_PROFILE' \
	 bin/debug-trace-gcc65/gpuminigl.dll bin/debug-trace-gcc65/fpse-udp-test

$(BINDIR)/fpse-udp-test: tests/udp_smoke.c $(OBJDIR)/trace_udp.o trace_udp.h Makefile
	@mkdir -p $(BINDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/udp_smoke.c $(OBJDIR)/trace_udp.o

.PHONY: all profile test smoke config-test clean
all: $(BINDIR)/gpuminigl.dll $(BINDIR)/gpuminigl-profile.dll
profile: $(BINDIR)/gpuminigl-profile.dll

# Controlled regression pair: same sources, flags and RC10; no old objects.
.PHONY: hash-ab-rc10
hash-ab-rc10:
	$(MAKE) CROSS=/opt/amiga-gcc16-rc10/bin/m68k-amigaos- \
	 OBJDIR=obj/hash-ab-rc10/static BINDIR=bin/hash-ab-rc10/static \
	 SDK_EXTRA_CPPFLAGS='-DFPSE_MINIGL_HASH_AB_TEST' \
	 bin/hash-ab-rc10/static/gpuminigl.dll
	$(MAKE) CROSS=/opt/amiga-gcc16-rc10/bin/m68k-amigaos- \
	 OBJDIR=obj/hash-ab-rc10/heap BINDIR=bin/hash-ab-rc10/heap \
	 SDK_EXTRA_CPPFLAGS='-DFPSE_MINIGL_HASH_AB_TEST -DFPSE_MINIGL_HEAP_HASH' \
	 bin/hash-ab-rc10/heap/gpuminigl.dll

.PHONY: heap-fps-rc10
heap-fps-rc10:
	$(MAKE) CROSS=/opt/amiga-gcc16-rc10/bin/m68k-amigaos- \
	 OBJDIR=obj/heap-fps-rc10 BINDIR=bin/heap-fps-rc10 \
	 SDK_EXTRA_CPPFLAGS='-DFPSE_MINIGL_HEAP_HASH -DFPSE_MINIGL_TITLE_FPS -DFPSE_MINIGL_NO_PROFILE' \
	 bin/heap-fps-rc10/gpuminigl.dll

.PHONY: sprite-uv-ab-rc10 sprite-uv-test vram-gutters-rc10 vram-tile-test ui-capture-rc10 ui-capture-test
.PHONY: quad-sprites-rc10 quad-sprites-test
quad-sprites-rc10:
	$(MAKE) CROSS=/opt/amiga-gcc16-rc10/bin/m68k-amigaos- \
	 OBJDIR=obj/quad-sprites-log-rc10 BINDIR=bin/quad-sprites-log-rc10 UI_CAPTURE_BUILD=1 \
	 SDK_EXTRA_CPPFLAGS='-DFPSE_MINIGL_HEAP_HASH -DFPSE_MINIGL_TITLE_FPS -DFPSE_MINIGL_NO_PROFILE -DFPSE_MINIGL_SPRITE_UV_AB=1 -DFPSE_MINIGL_VRAM_GUTTERS -DFPSE_MINIGL_QUAD_SPRITES -DFPSE_MINIGL_UI_CAPTURE' \
	 bin/quad-sprites-log-rc10/gpuminigl.dll
quad-sprites-test: $(OBJDIR)/test_quad_sprite_uv $(OBJDIR)/test_quad_render
	./$(OBJDIR)/test_quad_sprite_uv
	./$(OBJDIR)/test_quad_render
$(OBJDIR)/test_quad_sprite_uv: tests/test_quad_sprite_uv.c quad_sprite_uv.h
	@mkdir -p $(OBJDIR)
	$(HOSTCC) -std=c99 -O2 -Wall -Wextra -Werror $< -o $@
$(OBJDIR)/test_quad_render: tests/test_sprite_render.c quad_sprite_uv.h ../gpuw3d/w3d/render.c ../gpuw3d/w3d/poly_txt.h sprite_uv.h
	@mkdir -p $(OBJDIR)
	$(HOSTCC) -DFPSE_MINIGL_QUAD_SPRITES -DFPSE_MINIGL_SPRITE_UV_AB=1 -std=c99 -O2 -Wall -Wextra -Werror -Wno-unused-variable $< -o $@
test: quad-sprites-test
ui-capture-rc10:
	$(MAKE) CROSS=/opt/amiga-gcc16-rc10/bin/m68k-amigaos- \
	 OBJDIR=obj/ui-capture-rc10 BINDIR=bin/ui-capture-rc10 UI_CAPTURE_BUILD=1 \
	 SDK_EXTRA_CPPFLAGS='-DFPSE_MINIGL_HEAP_HASH -DFPSE_MINIGL_TITLE_FPS -DFPSE_MINIGL_NO_PROFILE -DFPSE_MINIGL_SPRITE_UV_AB=1 -DFPSE_MINIGL_VRAM_GUTTERS -DFPSE_MINIGL_UI_CAPTURE' \
	 bin/ui-capture-rc10/gpuminigl.dll
ui-capture-test: $(OBJDIR)/test_ui_capture $(OBJDIR)/test_ui_render $(OBJDIR)/test_ui_quad_render
	./$(OBJDIR)/test_ui_capture
	./$(OBJDIR)/test_ui_render
	./$(OBJDIR)/test_ui_quad_render
$(OBJDIR)/test_ui_quad_render: tests/test_sprite_render.c ui_capture.c ui_capture.h ../gpuw3d/w3d/render.c ../gpuw3d/w3d/poly_txt.h sprite_uv.h quad_sprite_uv.h
	@mkdir -p $(OBJDIR)
	$(HOSTCC) -DFPSE_MINIGL_QUAD_SPRITES -DFPSE_MINIGL_UI_CAPTURE -DFPSE_MINIGL_SPRITE_UV_AB=1 -std=c99 -O2 -Wall -Wextra -Werror -Wno-unused-variable tests/test_sprite_render.c ui_capture.c -o $@
$(OBJDIR)/test_ui_render: tests/test_sprite_render.c ui_capture.c ui_capture.h ../gpuw3d/w3d/render.c ../gpuw3d/w3d/poly_txt.h sprite_uv.h
	@mkdir -p $(OBJDIR)
	$(HOSTCC) -DFPSE_MINIGL_UI_CAPTURE -DFPSE_MINIGL_SPRITE_UV_AB=1 -std=c99 -O2 -Wall -Wextra -Werror -Wno-unused-variable tests/test_sprite_render.c ui_capture.c -o $@
$(OBJDIR)/test_ui_capture: tests/test_ui_capture.c ui_capture.c ui_capture.h
	@mkdir -p $(OBJDIR)
	$(HOSTCC) -std=c99 -O2 -Wall -Wextra -Werror tests/test_ui_capture.c ui_capture.c -o $@
test: ui-capture-test
vram-gutters-rc10:
	$(MAKE) CROSS=/opt/amiga-gcc16-rc10/bin/m68k-amigaos- \
	 OBJDIR=obj/sprite-uv-rc10/C BINDIR=bin/sprite-uv-rc10/C \
	 SDK_EXTRA_CPPFLAGS='-DFPSE_MINIGL_HEAP_HASH -DFPSE_MINIGL_TITLE_FPS -DFPSE_MINIGL_NO_PROFILE -DFPSE_MINIGL_SPRITE_UV_AB=1 -DFPSE_MINIGL_VRAM_GUTTERS' \
	 bin/sprite-uv-rc10/C/gpuminigl.dll
test: vram-tile-test
vram-tile-test: $(OBJDIR)/test_vram_tile
	./$(OBJDIR)/test_vram_tile
$(OBJDIR)/test_vram_tile: tests/test_vram_tile.c vram_tile.h pixels.h
	@mkdir -p $(OBJDIR)
	$(HOSTCC) -std=c99 -O2 -Wall -Wextra -Werror $< -o $@ -lm
sprite-uv-ab-rc10:
	$(MAKE) CROSS=/opt/amiga-gcc16-rc10/bin/m68k-amigaos- \
	 OBJDIR=obj/sprite-uv-rc10/A BINDIR=bin/sprite-uv-rc10/A \
	 SDK_EXTRA_CPPFLAGS='-DFPSE_MINIGL_HEAP_HASH -DFPSE_MINIGL_TITLE_FPS -DFPSE_MINIGL_NO_PROFILE -DFPSE_MINIGL_SPRITE_UV_AB=0' \
	 bin/sprite-uv-rc10/A/gpuminigl.dll
	$(MAKE) CROSS=/opt/amiga-gcc16-rc10/bin/m68k-amigaos- \
	 OBJDIR=obj/sprite-uv-rc10/B BINDIR=bin/sprite-uv-rc10/B \
	 SDK_EXTRA_CPPFLAGS='-DFPSE_MINIGL_HEAP_HASH -DFPSE_MINIGL_TITLE_FPS -DFPSE_MINIGL_NO_PROFILE -DFPSE_MINIGL_SPRITE_UV_AB=1' \
	 bin/sprite-uv-rc10/B/gpuminigl.dll
sprite-uv-test: $(OBJDIR)/test_sprite_uv $(OBJDIR)/test_sprite_render_A $(OBJDIR)/test_sprite_render_B
	./$(OBJDIR)/test_sprite_uv
	./$(OBJDIR)/test_sprite_render_A
	./$(OBJDIR)/test_sprite_render_B
$(OBJDIR)/test_sprite_render_A: tests/test_sprite_render.c sprite_uv.h ../gpuw3d/w3d/render.c ../gpuw3d/w3d/poly_txt.h
	@mkdir -p $(OBJDIR)
	$(HOSTCC) -DFPSE_MINIGL_SPRITE_UV_AB=0 -std=c99 -O2 -Wall -Wextra -Werror -Wno-unused-variable $< -o $@
$(OBJDIR)/test_sprite_render_B: tests/test_sprite_render.c sprite_uv.h ../gpuw3d/w3d/render.c ../gpuw3d/w3d/poly_txt.h
	@mkdir -p $(OBJDIR)
	$(HOSTCC) -DFPSE_MINIGL_SPRITE_UV_AB=1 -std=c99 -O2 -Wall -Wextra -Werror -Wno-unused-variable $< -o $@
$(OBJDIR)/test_sprite_uv: tests/test_sprite_uv.c sprite_uv.h
	@mkdir -p $(OBJDIR)
	$(HOSTCC) -std=c99 -O2 -Wall -Wextra -Werror $< -o $@

# Standalone black-box repro: only the MiniGL import client, no FPSE/renderer.
.PHONY: mgl2d-rc10
mgl2d-rc10:
	$(MAKE) CROSS=/opt/amiga-gcc16-rc10/bin/m68k-amigaos- \
	 OBJDIR=obj/mgl2d-rc10 BINDIR=bin/mgl2d-rc10 bin/mgl2d-rc10/MGL2DTest
$(BINDIR)/MGL2DTest: tests/mgl2d.c tests/mgl2d_cases.h tests/mglalpha_scene.h tests/mglalpha_pattern.h $(OBJDIR)/minigl_open.o $(OBJDIR)/minigl_base.o $(OBJDIR)/minigl_dispatch_stub_gcc.o Makefile
	@mkdir -p $(BINDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@.debug tests/mgl2d.c $(filter %.o,$^) -lm
	$(STRIP) -o $@ $@.debug

.PHONY: mgl2d-test
mgl2d-test: $(OBJDIR)/test_mgl2d_cases $(OBJDIR)/test_mglalpha
	./$(OBJDIR)/test_mgl2d_cases
	./$(OBJDIR)/test_mglalpha
$(OBJDIR)/test_mglalpha: tests/test_mglalpha.c tests/mglalpha_pattern.h
	@mkdir -p $(OBJDIR)
	$(HOSTCC) -std=c99 -O2 -Wall -Wextra -Werror $< -o $@
$(OBJDIR)/test_mgl2d_cases: tests/test_mgl2d_cases.c tests/mgl2d_cases.h
	@mkdir -p $(OBJDIR)
	$(HOSTCC) -std=c99 -O2 -Wall -Wextra -Werror $< -o $@

# Uninstrumented 0.4.4 client of an isolated, instrumented Classic library.
.PHONY: classic-ortho
classic-ortho:
	$(MAKE) OBJDIR=$(OBJDIR)/ortho BINDIR=$(BINDIR)/ortho \
	 SDK_EXTRA_CPPFLAGS='$(SDK_EXTRA_CPPFLAGS) -DFPSE_MINIGL_CLASSIC_ORTHO' \
	 $(BINDIR)/ortho/gpuminigl.dll

.PHONY: library-profile
library-profile:
	$(MAKE) OBJDIR=$(OBJDIR)/libprofile BINDIR=$(BINDIR)/libprofile \
	 SDK_EXTRA_CPPFLAGS='$(SDK_EXTRA_CPPFLAGS) -DFPSE_MINIGL_LIBRARY_PROFILE' \
	 $(BINDIR)/libprofile/gpuminigl.dll

# Isolated -pg build: never mix instrumented and release objects. The runtime
# itself must NOT be instrumented, and owns start/stop plus PROGDIR output.
.PHONY: gprof gprof-build
gprof:
	$(MAKE) OBJDIR=$(OBJDIR)/gprof BINDIR=$(BINDIR)/gprof \
	  CFLAGS='$(filter-out -fomit-frame-pointer,$(CFLAGS)) -pg -g -fno-omit-frame-pointer' \
	  CPPFLAGS='$(CPPFLAGS) -DFPSE_MINIGL_GPROF -DFPSE_MINIGL_NO_PROFILE' \
	  LDFLAGS='$(LDFLAGS) -pg' gprof-build
gprof-build: $(BINDIR)/gpuminigl-gprof.dll
$(BINDIR)/gpuminigl-gprof.dll: $(RELEASE_OBJECTS) $(OBJDIR)/gprof_runtime.o Makefile
	@mkdir -p $(BINDIR)
	$(CC) $(LDFLAGS) -Wl,-Map,$@.map -o $@.debug $(RELEASE_OBJECTS) $(OBJDIR)/gprof_runtime.o -lm
	$(STRIP) -o $@ $@.debug
$(OBJDIR)/gprof_runtime.o: gprof_runtime.c gmon.h gprof_build.h Makefile
	@mkdir -p $(OBJDIR)
	$(CC) $(CPPFLAGS) $(filter-out -pg -fno-omit-frame-pointer,$(CFLAGS)) -fomit-frame-pointer -MMD -MP -c $< -o $@

.PHONY: gprof-test
gprof-test: $(BINDIR)/gprof-selftest
$(OBJDIR)/gprof-selftest-runtime.o: gprof_runtime.c gmon.h gprof_build.h Makefile
	@mkdir -p $(OBJDIR)
	$(CC) $(CPPFLAGS) -DFPSE_GPROF_SELFTEST $(filter-out -pg -fno-omit-frame-pointer,$(CFLAGS)) -fomit-frame-pointer -c $< -o $@
$(OBJDIR)/gprof-selftest.o: tests/test_gprof.c gprof_build.h Makefile
	@mkdir -p $(OBJDIR)
	$(CC) $(CPPFLAGS) -DFPSE_GPROF_SELFTEST $(filter-out -fomit-frame-pointer,$(CFLAGS)) -pg -g -fno-omit-frame-pointer -c $< -o $@
$(BINDIR)/gprof-selftest: $(OBJDIR)/gprof-selftest.o $(OBJDIR)/gprof-selftest-runtime.o
	@mkdir -p $(BINDIR)
	$(CC) $(LDFLAGS) -pg -o $@ $^

# nix20 already adds libamiga AFTER libc. An explicit -lamiga here steals
# sprintf from libc: RawDoFmt reads %d as 16 bits and saves our options as 0.
$(BINDIR)/gpuminigl.dll: $(RELEASE_OBJECTS) Makefile
	@mkdir -p $(BINDIR)
	$(CC) $(LDFLAGS) -o $@.debug $(RELEASE_OBJECTS) $(LDLIBS) -lm
	$(STRIP) -o $@ $@.debug
$(BINDIR)/gpuminigl-profile.dll: $(OBJECTS) Makefile
	@mkdir -p $(BINDIR)
	$(CC) $(LDFLAGS) -o $@.debug $(OBJECTS) -lm
	$(STRIP) -o $@ $@.debug

$(OBJDIR)/release/%.o: %.c Makefile
	@mkdir -p $(OBJDIR)/release
	$(CC) $(CPPFLAGS) -DFPSE_MINIGL_NO_PROFILE $(CFLAGS) -MMD -MP -c $< -o $@
$(OBJDIR)/%.o: %.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@
$(OBJDIR)/core_%.o: ../gpuw3d/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@
$(OBJDIR)/soft.o: ../gpuw3d/w3d/soft.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@
$(OBJDIR)/render.o: ../gpuw3d/w3d/render.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@
$(OBJDIR)/timer.o: ../amidog/timer.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@
$(OBJDIR)/dll_main.o: ../dll/dll_main.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -include clib/alib_protos.h -MMD -MP -c $< -o $@
$(OBJDIR)/minigl_open.o: minigl_open.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c minigl_open.c -o $@
$(OBJDIR)/minigl_%.o: $(MINIGL_SRC)/client/minigl_%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@
$(OBJDIR)/minigl_dispatch_stub_gcc.o: $(MINIGL_SRC)/client/minigl_dispatch_stub_gcc.s
	@mkdir -p $(OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

test: sprite-uv-test $(OBJDIR)/test_pixels $(OBJDIR)/test_cache $(OBJDIR)/test_cache_release $(OBJDIR)/test_cache_heap $(OBJDIR)/test_cache_heap_release $(OBJDIR)/test_fps_counter $(OBJDIR)/test_profile $(OBJDIR)/test_no_profile $(OBJDIR)/test_texdecode $(OBJDIR)/test_draw_state $(OBJDIR)/test_trace $(OBJDIR)/test_no_trace $(OBJDIR)/test_trace_endpoint $(OBJDIR)/test_dma_probe
	./$(OBJDIR)/test_pixels
	./$(OBJDIR)/test_cache
	./$(OBJDIR)/test_cache_release
	./$(OBJDIR)/test_cache_heap
	./$(OBJDIR)/test_cache_heap_release
	./$(OBJDIR)/test_fps_counter
	./$(OBJDIR)/test_profile
	./$(OBJDIR)/test_no_profile
	./$(OBJDIR)/test_texdecode
	./$(OBJDIR)/test_draw_state
	./$(OBJDIR)/test_trace
	./$(OBJDIR)/test_no_trace
	./$(OBJDIR)/test_trace_endpoint
	./$(OBJDIR)/test_dma_probe
$(OBJDIR)/test_dma_probe: tests/test_dma_probe.c dma_probe.h
	@mkdir -p $(OBJDIR)
	$(HOSTCC) -std=c99 -O2 -Wall -Wextra -Werror $< -o $@
$(OBJDIR)/test_fps_counter: tests/test_fps_counter.c fps_counter.h
	@mkdir -p $(OBJDIR)
	$(HOSTCC) -std=c99 -O2 -Wall -Wextra -Werror $< -o $@
$(OBJDIR)/test_trace: tests/test_trace.c trace.c trace.h trace_udp.h version.h
	@mkdir -p $(OBJDIR)
	$(HOSTCC) -std=c99 -O2 -Wall -Wextra -Werror $< -o $@
$(OBJDIR)/test_no_trace: tests/test_no_trace.c trace.h
	@mkdir -p $(OBJDIR)
	$(HOSTCC) -std=c99 -O2 -Wall -Wextra -Werror $< -o $@
$(OBJDIR)/test_trace_endpoint: tests/test_trace_endpoint.c trace_endpoint.h
	@mkdir -p $(OBJDIR)
	$(HOSTCC) -std=c99 -O2 -Wall -Wextra -Werror $< -o $@

.PHONY: trace-test
trace-test: $(BINDIR)/trace-selftest $(BINDIR)/dma-probe-selftest
$(BINDIR)/dma-probe-selftest: tests/test_dma_probe.c dma_probe.h Makefile
	@mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $<
$(BINDIR)/trace-selftest: tests/test_trace.c trace.c trace.h trace_udp.h version.h Makefile
	@mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) -D__AMIGA__ -o $@ $<
$(OBJDIR)/test_draw_state: tests/test_draw_state.c draw_state.h
	@mkdir -p $(OBJDIR)
	$(HOSTCC) -std=c99 -O2 -Wall -Wextra -Werror $< -o $@
$(OBJDIR)/test_cache: tests/test_cache.c texture.c pixels.h texdecode.h profile.h ../gpuw3d/w3d/repair.h
	@mkdir -p $(OBJDIR)
	$(HOSTCC) -std=c99 -O2 -Wall -Wextra -Werror $< -o $@
$(OBJDIR)/test_profile: tests/test_profile.c profile.c profile.h version.h
	@mkdir -p $(OBJDIR)
	$(HOSTCC) -I.. -std=c99 -O2 -Wall -Wextra -Werror $< -o $@
$(OBJDIR)/test_cache_release: tests/test_cache.c texture.c pixels.h texdecode.h profile.h ../gpuw3d/w3d/repair.h
	@mkdir -p $(OBJDIR)
	$(HOSTCC) -DFPSE_MINIGL_NO_PROFILE -std=c99 -O2 -Wall -Wextra -Werror $< -o $@
$(OBJDIR)/test_cache_heap: tests/test_cache.c texture.c pixels.h texdecode.h profile.h ../gpuw3d/w3d/repair.h
	@mkdir -p $(OBJDIR)
	$(HOSTCC) -DFPSE_MINIGL_HEAP_HASH -std=c99 -O2 -Wall -Wextra -Werror $< -o $@
$(OBJDIR)/test_cache_heap_release: tests/test_cache.c texture.c pixels.h texdecode.h profile.h ../gpuw3d/w3d/repair.h
	@mkdir -p $(OBJDIR)
	$(HOSTCC) -DFPSE_MINIGL_HEAP_HASH -DFPSE_MINIGL_NO_PROFILE -std=c99 -O2 -Wall -Wextra -Werror $< -o $@
$(OBJDIR)/test_no_profile: tests/test_no_profile.c profile.h
	@mkdir -p $(OBJDIR)
	$(HOSTCC) -DFPSE_MINIGL_NO_PROFILE -std=c99 -O2 -Wall -Wextra -Werror $< -o $@
$(OBJDIR)/test_pixels: tests/test_pixels.c pixels.h
	@mkdir -p $(OBJDIR)
	$(HOSTCC) -std=c99 -O2 -Wall -Wextra -Werror $< -o $@
$(OBJDIR)/test_texdecode: tests/test_texdecode.c texdecode.h pixels.h
	@mkdir -p $(OBJDIR)
	$(HOSTCC) -std=c99 -O2 -Wall -Wextra -Werror $< -o $@

smoke: $(BINDIR)/minigl-smoke
$(OBJDIR)/smoke.o: tests/smoke.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@
$(BINDIR)/minigl-smoke: $(OBJDIR)/smoke.o $(filter-out $(OBJDIR)/dll_main.o,$(RELEASE_OBJECTS)) Makefile
	@mkdir -p $(BINDIR)
	$(CC) $(LDFLAGS) -o $@ $(filter %.o,$^) -lm

# Run on Amiga or under vamos; host printf cannot reproduce this link bug.
config-test: $(BINDIR)/config-test
$(BINDIR)/config-test: ../amidog/tests/test_printf.c Makefile
	@mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) -fno-builtin -o $@ $< -lm

clean:
	$(RM) $(OBJECTS) $(OBJECTS:.o=.d) $(OBJDIR)/test_pixels $(OBJDIR)/test_cache $(OBJDIR)/test_profile $(OBJDIR)/test_texdecode $(OBJDIR)/smoke.o $(OBJDIR)/smoke.d $(BINDIR)/gpuminigl.dll $(BINDIR)/gpuminigl.dll.debug $(BINDIR)/minigl-smoke $(BINDIR)/config-test
	$(RM) $(RELEASE_OBJECTS) $(RELEASE_OBJECTS:.o=.d) $(OBJDIR)/test_cache_release $(OBJDIR)/test_no_profile $(BINDIR)/gpuminigl-profile.dll $(BINDIR)/gpuminigl-profile.dll.debug
	$(RM) $(OBJDIR)/test_draw_state

-include $(OBJECTS:.o=.d) $(RELEASE_OBJECTS:.o=.d)

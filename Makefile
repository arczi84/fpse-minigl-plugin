# Supply an Amiga GCC toolchain and an unpacked MiniGL SDK.
CROSS ?= m68k-amigaos-
MINIGL_SDK ?=
COMMON_FLAGS = -DFPSE_MINIGL_HEAP_HASH -DFPSE_MINIGL_TITLE_FPS -DFPSE_MINIGL_NO_PROFILE -DFPSE_MINIGL_SPRITE_UV_AB=1 -DFPSE_MINIGL_VRAM_GUTTERS -DFPSE_MINIGL_QUAD_SPRITES
SDK_EXTRA_CPPFLAGS ?=
LDLIBS ?=
.DEFAULT_GOAL := release
.PHONY: release v29 test
release v29:
	@test -n "$(MINIGL_SDK)" || { echo 'Set MINIGL_SDK to the MiniGL SDK directory containing include/ and client/.'; exit 1; }
	$(MAKE) -C gpuminigl CROSS='$(CROSS)' MINIGL_SRC='$(abspath $(MINIGL_SDK))' OBJDIR=obj/$(if $(filter v29,$@),v29,release) BINDIR=bin/$(if $(filter v29,$@),v29,release) SDK_EXTRA_CPPFLAGS='$(COMMON_FLAGS) $(if $(filter v29,$@),-DFPSE_MINIGL_V291,) $(SDK_EXTRA_CPPFLAGS)' LDLIBS='$(LDLIBS)' bin/$(if $(filter v29,$@),v29,release)/gpuminigl.dll
test:
	$(MAKE) -C gpuminigl test

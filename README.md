# FPSE MiniGL GPU plugin

A MiniGL renderer plugin for FPSE on AmigaOS 3 / m68k, targeting Classic MiniGL and native PiStorm3D MiniGL. It reuses the GPUW3D / P.E.Op.S. PlayStation primitive decoder and provides a MiniGL rendering backend.

## Status

The E renderer was reported working with MiniGL v27 on PiStorm3D and on Classic. The newer ABI 5 build for MiniGL v29.1 works on Classic but currently hangs the user's PiStorm3D Amiga at the first image. The obsolete native enum offset has been corrected; that change did not resolve the hang. Native v29 support remains experimental.

Linear filtering still shows rectangular outlines around some 2D sprites. The half-texel and inclusive-UV fixes are experimental and do not identify every 2D primitive reliably. Do not treat passing CPU tests as verification of rendering on an Amiga.

## Build

Requires an AmigaOS m68k GCC toolchain with libnix/nix20, AmigaOS headers including MUI and Warp3D types, GNU Make, and a separate MiniGL dispatch SDK with `include/` and `client/`. MiniGL libraries and SDK sources are not bundled. Warp3D types are inherited from the decoder; the renderer calls MiniGL.

GCC 6.5 is preferred by the author for performance. GCC16 RC14 is the current GCC16 comparison toolchain. The previously working E binary was compiled with GCC16 RC10. Check the actual compiler with `m68k-amigaos-gcc -v` before building; older installations may put GCC4 first in PATH.

Build the legacy renderer with the compatible ABI 3 SDK from the MiniGL V23 Classic package:

```sh
make -j4 release \
  CROSS=/path/to/amiga-gcc65/bin/m68k-amigaos- \
  MINIGL_SDK=/path/to/MiniGL_Library_Source_Code
```

Build the experimental ABI 5 renderer using the v29.1 SDK:

```sh
make -j4 v29 \
  CROSS=/path/to/amiga-gcc16-rc14/bin/m68k-amigaos- \
  MINIGL_SDK=/path/to/MiniGL_Classic_v29.1/libraryroot
```

Output: `gpuminigl/bin/release/gpuminigl.dll` or `gpuminigl/bin/v29/gpuminigl.dll`. Flags include `-O2 -mcpu=68060 -mhard-float -mcrt=nix20 -fno-strict-aliasing`; ordinary builds have profiling compiled out. Each target uses separate object directories. Use separate `OBJDIR`/`BINDIR` when comparing compilers from the lower-level `gpuminigl/Makefile`.

Some GCC6 installations need extra MUI/Warp3D headers. Pass their directory through `SDK_EXTRA_CPPFLAGS='-idirafter /path/to/headers'`. Linker libraries, if needed, can be supplied through `LDLIBS`. Never add an explicit `-lamiga` before libc: its `sprintf` breaks configuration values.

## Install and controls

Copy the DLL into FPSE's `plugin/` directory, select it in FPSE, save settings and restart FPSE. Install a compatible `minigl.library` separately. The ABI 5 build requires library version 29 or newer and dispatch ABI exactly 5.

- F7 toggles the experimental rectangle-quad UV correction.
- F9 toggles the FPS/emu display in the window title.
- Texture Reduction `1/1 (256x256)` disables reduction; Texture Cache controls the number of cached textures.
- FrameSkip 1 renders every frame; 2 renders every other frame.

F8 capture and UDP tracing exist as separate diagnostic build options in the lower-level Makefile. Ordinary builds do not include them. Diagnostic file output uses `PROGDIR:`.

## Tests and layout

```sh
make test
```

Host tests cover texture decoding, cache invalidation, draw state, sprite UV mapping, mirrored and inclusive UV endpoints, VRAM gutters, FPS accounting, capture and trace helpers. They require a host C compiler and do not execute the GPU driver.

`gpuminigl/` contains the backend and tests; `gpuw3d/` contains the inherited decoder and renderer helpers. `dll/`, `amidog/` and the root headers supply the FPSE interface and Amiga support needed to build without an existing FPSE-SDK checkout.

## License and attribution

The inherited P.E.Op.S. GPU source carries Pete Bernert's GPL-2.0-or-later notices, which are preserved. See `COPYING` and the original file headers. FPSE/Amiga support retains its existing authorship and notices; this repository does not claim ownership of third-party code or change its terms. The profiler's additional notices are in `gpuminigl/GPROF-LICENSE.txt`.

This repository contains no PlayStation BIOS, game images, private machine configuration, runtime logs or prebuilt binaries.

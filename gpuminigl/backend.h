/* MiniGL backend for the FPSE Amiga GPU plugin. GPL-2.0-or-later. */
#ifndef FPSE_MINIGL_BACKEND_H
#define FPSE_MINIGL_BACKEND_H

#include <proto/minigl.h>
/* The decoder still uses Warp3D scalar/primitive types. V29's public GL
 * headers no longer include them implicitly; no Warp3D calls are linked. */
#include <Warp3D/Warp3D.h>

int amiga_open(void);
void amiga_close(void);
void input_update(void);
void amiga_clear(void);
void amiga_clipping(void);
void mgl_gpu_fill(int x0, int y0, int x1, int y1, unsigned long rgb);
void mgl_gpu_upload(int x, int y, int width, int height);
void mgl_gpu_vram_changed(int x, int y, int width, int height);
void mgl_gpu_invalidate(void);
void mgl_gpu_invalidate_area(int x, int y, int width, int height);
void mgl_gpu_restore(void);
void mgl_gpu_discard(void);
void mgl_gpu_texture_init(GLuint texture, int width, int height,
                          const unsigned char *pixels);
extern int mgl_gpu_ready;
extern int amiga_width, amiga_height;
extern int mgl_gpu_packed_textures;
void mgl_gpu_texture_wait(void);
int mgl_gpu_texture_upload(GLuint texture, int size, const void *pixels);

#endif

/*
 * texture.h
 */

#ifndef _TEXTURE_H_
#define _TEXTURE_H_

int TCacheInit(void);
void TCacheInvalidate(void);
void TCacheEndOfFrame(int verbose);
void TCacheExit(void);
void InvalidateTextureArea(long x, long y, long width, long height);
void w3d_gpu_vram_changed(int x, int y, int width, int height);

/* Returns non-zero if opaque pass is required. */
int set_texture(int clX, int clY, int opaque);

#endif /* _TEXTURE_H_ */

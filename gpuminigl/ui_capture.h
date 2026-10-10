#ifndef FPSE_MINIGL_UI_CAPTURE_H
#define FPSE_MINIGL_UI_CAPTURE_H
#include <stdio.h>
#define MGL_UI_CAPACITY 2048
typedef struct {
    const char *source;
    unsigned n;
    float vertex[4][4]; /* PSX x,y and pre-normalization u,v */
    unsigned long rgb[4], window;
    int mode, page_x, page_y, clut_x, clut_y, semitrans, twin;
    int linear, reduction;
    float sx,sy;
} MGLUIRecord;
extern int mgl_ui_collecting;
int mgl_ui_arm(void);
void mgl_ui_boundary(void);
void mgl_ui_record(const MGLUIRecord *record);
int mgl_ui_ready(void);
int mgl_ui_write(FILE *f);
void mgl_ui_done(int ok);
void mgl_ui_reset(void);
const char *mgl_ui_status(void);
#endif

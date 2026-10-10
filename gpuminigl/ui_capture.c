/* One frame, opt-in, no allocation or I/O inside drawing hooks. */
#include <stdlib.h>
#include "ui_capture.h"
static MGLUIRecord *records;
static unsigned count, dropped;
static int state; /* idle, armed, collecting, ready, saved, error */
int mgl_ui_collecting;
void mgl_ui_reset(void)
{
    free(records); records=NULL;
    count=dropped=0;state=0;mgl_ui_collecting=0;
}
int mgl_ui_arm(void)
{
    if(state>=1 && state<=3) return 0;
    mgl_ui_reset();
    records=calloc(MGL_UI_CAPACITY,sizeof(*records));
    if(!records) {state=5;return 0;}
    state=1;return 1;
}
void mgl_ui_boundary(void)
{
    if(state==1) {state=2;mgl_ui_collecting=1;}
    else if(state==2) {state=3;mgl_ui_collecting=0;}
}
void mgl_ui_record(const MGLUIRecord *r)
{
    if(!mgl_ui_collecting) return;
    if(count<MGL_UI_CAPACITY) records[count++]=*r;
    else ++dropped;
}
int mgl_ui_ready(void) {return state==3;}
int mgl_ui_write(FILE *f)
{
    unsigned i,j;
    if(!f || state!=3) return 0;
    fprintf(f,"UI capture: one complete presentation interval; textured primitives only.\n");
    fprintf(f,"count=%u dropped=%u complete=%s\n",count,dropped,dropped?"NO (capacity limit)":"yes");
    fprintf(f,"xyuv256 are PSX x,y,u,v multiplied by 256 (not screen pixels). sx256/sy256 are scale*256.\n");
    fprintf(f,"Records are taken once before texture lookup/blend setup; both transparency passes use these UVs.\n");
    fprintf(f,"drawSprite4FTfloat = inset applied; drawPoly4FTi may be a polygon OR an unchanged sprite.\n");
    for(i=0;i<count;++i) {
        const MGLUIRecord *r=records+i;
        fprintf(f,"P %u source=%s n=%u mode=%d page=%d,%d clut=%d,%d window=%08lx twin=%d semi=%d linear=%d reduction=%d sx256=%ld sy256=%ld\n",
            i,r->source,r->n,r->mode,r->page_x,r->page_y,r->clut_x,r->clut_y,
            r->window,r->twin,r->semitrans,r->linear,r->reduction,
            (long)(r->sx*256),(long)(r->sy*256));
        for(j=0;j<r->n && j<4;++j) fprintf(f," V%u xyuv256=%ld,%ld,%ld,%ld rgb=%08lx\n",j,
            (long)(r->vertex[j][0]*256),(long)(r->vertex[j][1]*256),
            (long)(r->vertex[j][2]*256),(long)(r->vertex[j][3]*256),r->rgb[j]);
    }
    return !ferror(f);
}
void mgl_ui_done(int ok)
{
    free(records);records=NULL;mgl_ui_collecting=0;state=ok?4:5;
}
const char *mgl_ui_status(void)
{
    static const char *names[]={"F8","armed","recording","ready","saved","ERROR"};
    return names[state];
}

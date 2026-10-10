#include <assert.h>
#include <string.h>
#include "../ui_capture.h"
int main(void)
{
    MGLUIRecord r={0};FILE *f;char line[512];unsigned i;int header=0,source=0,vertex=0;
    r.source="drawSprite4FTfloat";r.n=4;r.vertex[0][2]=8.5f;r.vertex[0][3]=16.5f;
    r.sx=r.sy=2;r.linear=1;
    mgl_ui_reset();assert(!mgl_ui_collecting && !mgl_ui_ready());
    mgl_ui_record(&r);assert(mgl_ui_arm());assert(!mgl_ui_arm());
    mgl_ui_record(&r);mgl_ui_boundary();assert(mgl_ui_collecting);
    mgl_ui_record(&r);mgl_ui_boundary();assert(!mgl_ui_collecting && mgl_ui_ready());
    mgl_ui_record(&r);f=tmpfile();assert(f && mgl_ui_write(f));rewind(f);
    while(fgets(line,sizeof(line),f)) {
        if(strstr(line,"count=1 dropped=0 complete=yes")) header=1;
        if(strstr(line,"source=drawSprite4FTfloat n=4")) source=1;
        if(strstr(line,"V0 xyuv256=0,0,2176,4224")) vertex=1;
    }
    assert(header && source && vertex);fclose(f);mgl_ui_done(1);
    assert(!strcmp(mgl_ui_status(),"saved"));assert(mgl_ui_arm());mgl_ui_boundary();
    for(i=0;i<MGL_UI_CAPACITY+7;++i) mgl_ui_record(&r);
    mgl_ui_boundary();f=tmpfile();assert(f && mgl_ui_write(f));rewind(f);header=0;
    while(fgets(line,sizeof(line),f)) if(strstr(line,"dropped=7 complete=NO")) header=1;
    assert(header);fclose(f);mgl_ui_done(0);assert(!strcmp(mgl_ui_status(),"ERROR"));
    assert(mgl_ui_arm());mgl_ui_reset();mgl_ui_reset();
    assert(!strcmp(mgl_ui_status(),"F8"));
    puts("PASS: capture arms at boundary, exactly one interval, bounded records, fractional UV, rearm, reset");
    return 0;
}

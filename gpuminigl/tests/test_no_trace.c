#include <assert.h>
#include <stdio.h>
#include "../trace.h"
int main(void)
{
    int side_effects = 0;
    unsigned long t;
    mgl_trace_open();
    mgl_trace_log("unused=%d", ++side_effects);
    t = mgl_trace_begin(MGL_T_DRAW, ++side_effects, ++side_effects, ++side_effects);
    mgl_trace_end(MGL_T_DRAW, t, ++side_effects);
    mgl_trace_close();
    assert(t == 0 && side_effects == 0);
    puts("PASS: ordinary plugin trace hooks compile out without logger linkage or argument evaluation");
    return 0;
}

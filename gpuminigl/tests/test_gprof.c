/* Target-side test of -pg hooks, relative addresses, save and reopen.
 * Samples are injected: this does not test the real VBlank sampler. */
#include <stdio.h>
#include "gprof_build.h"
extern void fpse_gprof_test_sample(void *pc);
static volatile unsigned int result;
__attribute__((noinline)) void gprof_test_leaf(unsigned int x)
{
    result += x;
}
__attribute__((noinline)) void gprof_test_parent(void)
{
    unsigned int i;
    for (i = 0; i < 1000; ++i) gprof_test_leaf(i);
}
int main(void)
{
    int pass;
    FILE *f;
    fpse_gprof_end(); /* must be safe before begin and after end */
    for (pass = 0; pass < 2; ++pass) {
        result = 0;
        fpse_gprof_begin();
        gprof_test_parent();
        fpse_gprof_test_sample((void *)gprof_test_leaf);
        fpse_gprof_end();
        fpse_gprof_end();
        if (result != 499500) return 1;
        f = fopen(FPSE_GPROF_DATA, "rb");
        if (!f) { puts("FAIL: cannot read saved profile"); return 2; }
        fclose(f);
    }
    puts("PASS: -pg execution and reopen; verify 1000 leaf calls in gprof output.");
    return 0;
}

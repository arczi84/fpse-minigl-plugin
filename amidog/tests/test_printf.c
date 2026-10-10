/* Target-side regression for the GUI configuration corruption in both GPUs.
 * Use the same link flags as the plugins, with -fno-builtin: libamiga's
 * sprintf accepts RawDoFmt formats, NOT C formats (%d consumes 16 bits).
 * With the old explicit -lamiga this test fails even though host tests pass.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    static const struct {
        int value;
        const char *text;
    } cases[] = {
        { 0, "0" }, { 1, "1" }, { 3, "3" }, { 10, "10" },
        { 64, "64" }, { 128, "128" }, { 256, "256" },
        { 1375736066, "1375736066" }, { -1, "-1" }
    };
    char buffer[256];
    unsigned int i;
    int failed = 0;
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        sprintf(buffer, "%d", cases[i].value);
        if (strcmp(buffer, cases[i].text) || atoi(buffer) != cases[i].value) {
            fprintf(stderr, "FAIL: expected %s, saved %s\n", cases[i].text, buffer);
            failed = 1;
        }
    }
    sprintf(buffer, "%d %d %d", 1, 256, 3);
    if (strcmp(buffer, "1 256 3")) {
        fprintf(stderr, "FAIL: multiple 32-bit arguments: %s\n", buffer);
        failed = 1;
    }
    if (failed) return 20;
    puts("PASS: C sprintf / configuration values retain all 32 bits");
    return 0;
}

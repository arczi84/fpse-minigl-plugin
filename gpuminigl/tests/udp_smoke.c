/* Optional Amiga connectivity check, independent of FPSE/MiniGL. */
#include <stdio.h>
#include <string.h>
#include "../trace_udp.h"
int main(void)
{
    static const char *messages[] = {
        "UDP-TEST 1 bsdsocket.library open; this is NOT a GPU log\n",
        "UDP-TEST 2 nonblocking send from Amiga\n",
        "UDP-TEST 3 end; check all three lines on the PC receiver\n"
    };
    unsigned long failed;
    int i;
    if (!mgl_trace_udp_open()) {
        puts("UDP test: no UDP transport. Put fpse-minigl-udp.cfg beside this executable.");
        return 10;
    }
    for (i = 0; i < 3; ++i) mgl_trace_udp_send(messages[i], strlen(messages[i]));
    failed = mgl_trace_udp_dropped();
    mgl_trace_udp_close();
    printf("UDP test: local send failures=%lu. Confirm reception on the PC; no acknowledgements are used.\n", failed);
    return failed ? 10 : 0;
}

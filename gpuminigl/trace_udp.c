/* Optional nonblocking, one-way breadcrumbs. No receive, ACK, DNS or retry. */
#include <stdio.h>
#include <string.h>
#include <proto/exec.h>
#define __NOLIBBASE__
#define BSDSOCKET_BASE_NAME trace_socket_base
#include <proto/bsdsocket.h>
#undef __NOLIBBASE__
#include <sys/ioctl.h>
#include "trace_udp.h"
#include "trace_endpoint.h"

static struct Library *trace_socket_base;
static struct Task *owner;
static int trace_socket = -1;
static struct sockaddr_in destination;
static unsigned long dropped;

void mgl_trace_udp_close(void)
{
    if (trace_socket_base) {
        if (trace_socket >= 0) CloseSocket(trace_socket);
        CloseLibrary(trace_socket_base);
    }
    trace_socket = -1; trace_socket_base = NULL; owner = NULL;
}

int mgl_trace_udp_open(void)
{
    FILE *config;
    char text[128];
    size_t n;
    unsigned long address;
    unsigned short port;
    long nonblocking = 1;
    mgl_trace_udp_close(); dropped = 0;
    config = fopen("PROGDIR:fpse-minigl-udp.cfg", "rb");
    if (!config) return 0;
    n = fread(text, 1, sizeof(text) - 1, config);
    if (ferror(config) || n == sizeof(text) - 1) {
        fclose(config);
        puts("GPUMiniGL UDP: cannot read config or config too long; using file log.");
        return 0;
    }
    fclose(config); text[n] = 0;
    if (memchr(text, 0, n) || !trace_endpoint(text, &address, &port)) {
        puts("GPUMiniGL UDP: invalid config; expected numeric IPv4 and optional port; using file log.");
        return 0;
    }
    trace_socket_base = OpenLibrary((CONST_STRPTR)"bsdsocket.library", 4);
    if (!trace_socket_base) {
        puts("GPUMiniGL UDP: cannot open bsdsocket.library; using file log.");
        return 0;
    }
    trace_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (trace_socket < 0 || IoctlSocket(trace_socket, FIONBIO, (APTR)&nonblocking) < 0) {
        puts("GPUMiniGL UDP: cannot create nonblocking socket; using file log.");
        mgl_trace_udp_close();
        return 0;
    }
    memset(&destination, 0, sizeof(destination));
    destination.sin_family = AF_INET;
    destination.sin_port = htons(port);
    destination.sin_addr.s_addr = htonl(address);
    owner = FindTask(NULL);
    printf("GPUMiniGL UDP: %lu.%lu.%lu.%lu:%u; UDP-only, no Amiga log writes.\n",
        (address >> 24) & 255, (address >> 16) & 255,
        (address >> 8) & 255, address & 255, (unsigned int)port);
    return 1;
}

void mgl_trace_udp_send(const char *data, unsigned long length)
{
    /* A bsdsocket base belongs to its opening task; never borrow it elsewhere. */
    if (trace_socket < 0 || FindTask(NULL) != owner ||
        sendto(trace_socket, (APTR)data, length, 0,
            (struct sockaddr *)&destination, sizeof(destination)) != (long)length)
        ++dropped;
}

unsigned long mgl_trace_udp_dropped(void) { return dropped; }

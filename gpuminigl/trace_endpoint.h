/* Numeric IPv4 only: no DNS, broadcast or multicast in the hang logger. */
#ifndef FPSE_TRACE_ENDPOINT_H
#define FPSE_TRACE_ENDPOINT_H
static int trace_space(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}
static int trace_endpoint(const char *p, unsigned long *address, unsigned short *port)
{
    unsigned long ip = 0, n;
    int i, digits;
    while (trace_space(*p)) ++p;
    for (i = 0; i < 4; ++i) {
        n = 0; digits = 0;
        while (*p >= '0' && *p <= '9') {
            n = n * 10 + (*p++ - '0');
            if (++digits > 3 || n > 255) return 0;
        }
        if (!digits || (i == 0 && (n == 0 || n >= 224))) return 0;
        ip = (ip << 8) | n;
        if (i < 3 && *p++ != '.') return 0;
    }
    if (*p && !trace_space(*p)) return 0;
    while (trace_space(*p)) ++p;
    n = 5514; digits = 0;
    if (*p) {
        n = 0;
        while (*p >= '0' && *p <= '9') {
            n = n * 10 + (*p++ - '0');
            if (++digits > 5 || n > 65535) return 0;
        }
        if (!digits || !n) return 0;
        while (trace_space(*p)) ++p;
        if (*p) return 0;
    }
    *address = ip; *port = (unsigned short)n;
    return 1;
}
#endif

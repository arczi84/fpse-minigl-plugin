#ifndef FPSE_TRACE_UDP_H
#define FPSE_TRACE_UDP_H
int mgl_trace_udp_open(void);
void mgl_trace_udp_send(const char *data, unsigned long length);
unsigned long mgl_trace_udp_dropped(void);
void mgl_trace_udp_close(void);
#endif

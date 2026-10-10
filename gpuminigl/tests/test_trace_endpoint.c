#include <assert.h>
#include <stdio.h>
#include "../trace_endpoint.h"
int main(void)
{
    unsigned long ip;
    unsigned short port;
    assert(trace_endpoint("192.168.0.108 5514\r\n", &ip, &port));
    assert(ip == 0xc0a8006cUL && port == 5514);
    assert(trace_endpoint(" 127.0.0.1 ", &ip, &port) && port == 5514);
    assert(trace_endpoint("10.0.0.1 65535", &ip, &port) && port == 65535);
    assert(!trace_endpoint("10.0.0.1 65536", &ip, &port));
    assert(!trace_endpoint("10.0.0.1 0", &ip, &port));
    assert(!trace_endpoint("10.0.0.1 5514 garbage", &ip, &port));
    assert(!trace_endpoint("256.0.0.1", &ip, &port));
    assert(!trace_endpoint("255.255.255.255", &ip, &port));
    assert(!trace_endpoint("224.0.0.1", &ip, &port));
    assert(!trace_endpoint("0.0.0.0", &ip, &port));
    assert(!trace_endpoint("192.168.0", &ip, &port));
    assert(!trace_endpoint("localhost", &ip, &port));
    assert(!trace_endpoint("192.168.0.108:5514", &ip, &port));
    assert(!trace_endpoint("", &ip, &port));
    puts("PASS: UDP endpoint numeric IPv4, default/explicit port, malformed/range/broadcast rejection");
    return 0;
}

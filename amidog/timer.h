/*
 * timer.h
 */

#ifndef _AMIDOG_TIMER_H_
#define _AMIDOG_TIMER_H_

#ifdef __AROS__
#include <devices/timer.h>
#endif

#include <proto/timer.h>

typedef struct amidog_timer_s amidog_timer_t;
typedef struct amidog_timeval_s amidog_timeval_t;

struct amidog_timeval_s {
  int sec;
  int usec;
};

amidog_timer_t *timer_init(void);
void timer_gettime(amidog_timer_t *timer, amidog_timeval_t *tv);
void timer_subtime(amidog_timer_t *timer, amidog_timeval_t *dt, amidog_timeval_t *st);
void timer_usleep(amidog_timer_t *timer, int us);
void timer_exit(amidog_timer_t *timer);

#endif /* _AMIDOG_TIMER_H_ */

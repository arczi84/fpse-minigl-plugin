/*
 * timer.c
 */

#ifndef __amigaos4__

#include <proto/exec.h>
#include <proto/timer.h>

#include "timer.h"

struct amidog_timer_s {
  struct MsgPort *TimerMP;
  struct timerequest *TimerIO;
  struct Device *TimerBase;
};

amidog_timer_t *timer_init(void)
{
  amidog_timer_t *timer = AllocVec(sizeof(amidog_timer_t), MEMF_PUBLIC | MEMF_CLEAR);
  if (timer == NULL) {
    return NULL;
  }

  if ((timer->TimerMP = CreateMsgPort())) {
    if ((timer->TimerIO = CreateIORequest(timer->TimerMP, sizeof(struct timerequest)))) {
      if (OpenDevice(TIMERNAME, UNIT_MICROHZ, (struct IORequest *)timer->TimerIO, 0) == 0) {
        timer->TimerBase = timer->TimerIO->tr_node.io_Device;
        if (timer->TimerBase != NULL) {
          return timer;
        }
      }
    }
  }

  timer_exit(timer);

  return NULL;
}

void timer_exit(amidog_timer_t *timer)
{
  if (timer != NULL) {
    if (timer->TimerBase != NULL) {
      CloseDevice((struct IORequest *)timer->TimerIO);
    }
    if (timer->TimerIO != NULL) {
      DeleteIORequest(timer->TimerIO);
    }
    if (timer->TimerMP != NULL) {
      DeleteMsgPort(timer->TimerMP);
    }
    FreeVec(timer);
  }
}

void timer_gettime(amidog_timer_t *timer, amidog_timeval_t *tv)
{
  if (timer != NULL) {
    struct Library *TimerBase = (struct Library *)timer->TimerBase;
    struct timeval t;
    GetSysTime(&t);
    tv->sec = t.tv_secs;
    tv->usec = t.tv_micro;
  }
}

void timer_subtime(amidog_timer_t *timer, amidog_timeval_t *dt, amidog_timeval_t *st)
{
  if (timer != NULL) {
    struct Library *TimerBase = (struct Library *)timer->TimerBase;
    struct timeval d, s;
    d.tv_secs = dt->sec;
    d.tv_micro = dt->usec;
    s.tv_secs = st->sec;
    s.tv_micro = st->usec;
    SubTime(&d, &s);
    dt->sec = d.tv_secs;
    dt->usec = d.tv_micro;
  }
}

void timer_usleep(amidog_timer_t *timer, int us)
{
  if (timer != NULL) {
    /* setup */
    timer->TimerIO->tr_node.io_Command = TR_ADDREQUEST;
    timer->TimerIO->tr_time.tv_secs = us / 1000000;
    timer->TimerIO->tr_time.tv_micro = us % 1000000;

    /* send request */
    SetSignal(0, (1L << timer->TimerMP->mp_SigBit));
    SendIO((struct IORequest *)timer->TimerIO);
    Wait((1L << timer->TimerMP->mp_SigBit));
    WaitIO((struct IORequest *)timer->TimerIO);
  }
}

#else

#undef __USE_INLINE__

#include <proto/exec.h>
#include <proto/timer.h>

#include "timer.h"

struct amidog_timer_s {
  struct MsgPort *TimerMP;
  struct TimeRequest *TimerIO;
  struct Device *TimerBase;
  struct TimerIFace *ITimer;
};

amidog_timer_t *timer_init(void)
{
  amidog_timer_t *timer = IExec->AllocVec(sizeof(amidog_timer_t), MEMF_PUBLIC | MEMF_CLEAR);
  if (timer == NULL) {
    return NULL;
  }

  if ((timer->TimerMP = IExec->AllocSysObject(ASOT_PORT, NULL))) {
    if ((timer->TimerIO = IExec->AllocSysObjectTags(ASOT_IOREQUEST, ASOIOR_Size, sizeof(struct TimeRequest), ASOIOR_ReplyPort, timer->TimerMP, TAG_DONE))) {
      if (IExec->OpenDevice(TIMERNAME, UNIT_MICROHZ, (struct IORequest *)timer->TimerIO, 0) == 0) {
        timer->TimerBase = timer->TimerIO->Request.io_Device;
        timer->ITimer = (struct TimerIFace *)IExec->GetInterface((struct Library *)timer->TimerBase, "main", 1, NULL);
        if (timer->ITimer != NULL) {
          return timer;
        }
      }
    }
  }

  timer_exit(timer);

  return NULL;
}

void timer_exit(amidog_timer_t *timer)
{
  if (timer != NULL) {
    if (timer->ITimer != NULL) {
      IExec->DropInterface((struct Interface *)timer->ITimer);
    }
    if (timer->TimerBase != NULL) {
      IExec->CloseDevice((struct IORequest *)timer->TimerIO);
    }
    if (timer->TimerIO != NULL) {
      IExec->FreeSysObject(ASOT_IOREQUEST, timer->TimerIO);
    }
    if (timer->TimerMP != NULL) {
      IExec->FreeSysObject(ASOT_PORT, timer->TimerMP);
    }
    IExec->FreeVec(timer);
  }
}

void timer_gettime(amidog_timer_t *timer, amidog_timeval_t *tv)
{
  if (timer != NULL) {
    struct TimeVal t;
    timer->ITimer->GetUpTime(&t);
    tv->sec = t.Seconds;
    tv->usec = t.Microseconds;
  }
}

void timer_subtime(amidog_timer_t *timer, amidog_timeval_t *dt, amidog_timeval_t *st)
{
  if (timer != NULL) {
    struct TimeVal d = { dt->sec, dt->usec };
    struct TimeVal s = { st->sec, st->usec };
    timer->ITimer->SubTime(&d, &s);
    dt->sec = d.Seconds;
    dt->usec = d.Microseconds;
  }
}

void timer_usleep(amidog_timer_t *timer, int us)
{
  if (timer != NULL) {
    /* setup */
    timer->TimerIO->Request.io_Command = TR_ADDREQUEST;
    timer->TimerIO->Time.Seconds = us / 1000000;
    timer->TimerIO->Time.Microseconds = us % 1000000;

    /* send request */
    IExec->SetSignal(0, (1L << timer->TimerMP->mp_SigBit));
    IExec->SendIO((struct IORequest *)timer->TimerIO);
    IExec->Wait((1L << timer->TimerMP->mp_SigBit));
    IExec->WaitIO((struct IORequest *)timer->TimerIO);
  }
}

#endif

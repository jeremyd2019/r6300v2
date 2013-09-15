/*
 * delay.c
 *
 * implementation of delay API
 */

#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "delay.h"

int delay_is_running = 0;

sig_atomic_t delay_timeout;

extern int verbose;

void delay_setup_fixed (struct itimerval *delay, int delay_secs)
{

  memset(delay, 0, sizeof(struct itimerval));

  if (verbose)
    fprintf(stderr, "delay fixed for %d\n", delay_secs);

  delay->it_value.tv_sec = delay_secs;

}

void delay_setup_random(struct itimerval *delay,
			int delay_min_secs,
			int delay_max_secs)
{

  long delay_usecs;
  long delay_rand;
  long delay_usec_range;

  memset(delay, 0, sizeof(struct itimerval));

  /* compute the delay time in usecs */
  delay_rand = abs(random());

  delay_usec_range = (delay_max_secs - delay_min_secs) * 100;

  delay_usecs = (delay_rand % delay_usec_range) + (delay_min_secs * 100);

  delay->it_value.tv_usec = delay_usecs % 100;
  delay->it_value.tv_sec  = delay_usecs - delay->it_value.tv_usec;

  if (delay->it_value.tv_sec)
    delay->it_value.tv_sec = delay->it_value.tv_sec / 100;

  if (verbose)
    fprintf(stderr, "delay random for %ld.%ld\n",
	    delay->it_value.tv_sec,
	    delay->it_value.tv_usec);

}

void delay_setup_immed(struct itimerval *delay)
{

  memset(delay, 0, sizeof(struct itimerval));

  delay->it_value.tv_usec = 1;
}

void delay_run         (struct itimerval *delay)
{
  if (!delay_is_running) {
    setitimer(ITIMER_REAL, delay, NULL);
    delay_timeout = 0;
    delay_is_running = 1;
  }
}

/* delay_wait defined in delay.h */
#if 0
void delay_wait        (void)
{
  if (!delay_timeout)
    continue;

  delay_is_running = 0;
}
#endif

int  delay_is_waiting  (void)
{
  if ((delay_timeout == 0) && (delay_is_running))
    return 1;

  return 0;
}

void delay_cancel      (void)
{
  if (setitimer(ITIMER_REAL, NULL, NULL) < 0) {
    fprintf(stderr, "failed to cancel timer: %s\n", strerror(errno));
  }

  delay_timeout = 0;
  delay_is_running = 0;

}

/*
 * our delay API
 *
 * zeroconf requires us to have things outstanding on the network
 * and to wait until a certain amount of time has passed before
 * proceeding.
 *
 * This API tries to be somewhat sane to use but it has limitations,
 * like it needs to have SIGALARM handled specially. It can only
 * have a single delay outstanding at once. Delays are always against
 * observed real-time
 */

#ifndef DELAY_H
#define DELAY_H 1

#include <signal.h>

extern sig_atomic_t delay_timeout;
extern int delay_is_running;

void delay_setup_fixed (struct itimerval *delay, int delay_secs);

void delay_setup_random(struct itimerval *delay,
			int delay_min_secs,
			int delay_max_secs);

void delay_setup_immed (struct itimerval *delay);

void delay_run         (struct itimerval *delay);

int  delay_is_waiting  (void);

void delay_cancel      (void);

#define delay_wait()  { if (!delay_timeout) continue; }

#endif /* DELAY_H */

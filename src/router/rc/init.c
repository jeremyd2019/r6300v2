/*
 * System V init functionality
 *
 * Copyright (C) 2011, Broadcom Corporation. All Rights Reserved.
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * $Id: init.c 241182 2011-02-17 21:50:03Z $
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <paths.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <limits.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/poll.h>

#include <shutils.h>

#include <rc.h>
#include <bcmparams.h>
#include <wlutils.h>
#include <pmon.h>

#define loop_forever() do { sleep(1); } while (1)
#define SHELL "/bin/sh"

/* Set terminal settings to reasonable defaults */
static void set_term(int fd)
{
	struct termios tty;

	tcgetattr(fd, &tty);

	/* set control chars */
	tty.c_cc[VINTR]  = 3;	/* C-c */
	tty.c_cc[VQUIT]  = 28;	/* C-\ */
	tty.c_cc[VERASE] = 127; /* C-? */
	tty.c_cc[VKILL]  = 21;	/* C-u */
	tty.c_cc[VEOF]   = 4;	/* C-d */
	tty.c_cc[VSTART] = 17;	/* C-q */
	tty.c_cc[VSTOP]  = 19;	/* C-s */
	tty.c_cc[VSUSP]  = 26;	/* C-z */

	/* use line dicipline 0 */
	tty.c_line = 0;

	/* Make it be sane */
	tty.c_cflag &= CBAUD|CBAUDEX|CSIZE|CSTOPB|PARENB|PARODD;
	tty.c_cflag |= CREAD|HUPCL|CLOCAL;


	/* input modes */
	tty.c_iflag = ICRNL | IXON | IXOFF;

	/* output modes */
	tty.c_oflag = OPOST | ONLCR;

	/* local modes */
	tty.c_lflag =
		ISIG | ICANON | ECHO | ECHOE | ECHOK | ECHOCTL | ECHOKE | IEXTEN;

	tcsetattr(fd, TCSANOW, &tty);
}

int
console_init()
{
	int fd;

	/* Clean up */
	ioctl(0, TIOCNOTTY, 0);
	close(0);
	close(1);
	close(2);
	setsid();

	/* Reopen console */
	if ((fd = open(_PATH_CONSOLE, O_RDWR)) < 0) {
		perror(_PATH_CONSOLE);
		return errno;
	}
	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);

	ioctl(0, TIOCSCTTY, 1);
	tcsetpgrp(0, getpgrp());
	set_term(0);

	return 0;
}

#if defined(BB_SH)
#undef CONFIG_FEATURE_SH_IS_NONE
#endif

#if !defined(__CONFIG_BUSYBOX__) || !defined(CONFIG_FEATURE_SH_IS_NONE)
pid_t
run_shell(int timeout, int nowait)
{
	pid_t pid;
	char tz[1000];
	char *envp[] = {
		"TERM=vt100",
		"HOME=/",
		"PATH=/usr/bin:/bin:/usr/sbin:/sbin",
		"SHELL=" SHELL,
		"USER=root",
		tz,
		NULL
	};
	int sig;

	/* Wait for user input */
	cprintf("Hit enter to continue...");
	if (waitfor(STDIN_FILENO, timeout) <= 0)
		return 0;

	switch ((pid = fork())) {
	case -1:
		perror("fork");
		return 0;
	case 0:
		/* Reset signal handlers set for parent process */
		for (sig = 0; sig < (_NSIG-1); sig++)
			signal(sig, SIG_DFL);

		/* Reopen console */
		console_init();

		/* Pass on TZ */
		snprintf(tz, sizeof(tz), "TZ=%s", getenv("TZ"));

		/* Now run it.  The new program will take over this PID, 
		 * so nothing further in init.c should be run.
		 */
		execve(SHELL, (char *[]) { "/bin/sh", NULL }, envp);

		/* We're still here?  Some error happened. */
		perror(SHELL);
		exit(errno);
	default:
		if (nowait)
			return pid;
		else {
			waitpid(pid, NULL, 0);
			return 0;
		}
	}
}
#else /* Busybox configured w/o a shell */
pid_t
run_shell(int timeout, int nowait)
{
	/* Wait for usr input to say we're ignoring it */
	if (waitfor(STDIN_FILENO, timeout) > 0) {
		char c;
		struct pollfd pfd = { STDIN_FILENO, POLLIN, 0 };

		/* Read while available (flush input) */
		while ((poll(&pfd, 1, 0) > 0) && (pfd.revents & POLLIN))
			read(STDIN_FILENO, &c, 1);

		cprintf("Busybox configured w/o a shell\n");
	}
	return 0;
}
#endif /* !__CONFIG_BUSYBOX__ || !CONFIG_FEATURE_SH_IS_NONE */

static void
shutdown_system(void)
{
	int sig;

#ifdef __CONFIG_USBAP__
	{
		int ret = 0, i = 0;
		char	ifname[16] = {0};

		for (i = 1; i <= DEV_NUMIFS; i++) {
			sprintf(ifname, "eth%d", i);
			if (wl_probe(ifname))
				break;
		}

		sprintf(ifname, "eth%d", i - 1);
		if ((ret = wl_ioctl(ifname, WLC_UP, NULL, 0)))
			fprintf(stderr, "%s:%d:(%s): cannot bringup, err = %d\n",
				__FUNCTION__, __LINE__, ifname, ret);

		if ((ret = wl_iovar_setint(ifname, "dngl_wd", 1)))
			fprintf(stderr, "%s:%d:(%s): setting iovar \"%s\" to 0x%x failed,"
				" err = %d\n", __FUNCTION__, __LINE__, ifname, "dngl_wd",
				(unsigned int)1, ret);
		sleep(1);
	}
#endif /* __CONFIG_USBAP__ */

	/* Disable signal handlers */
	for (sig = 0; sig < (_NSIG-1); sig++)
		signal(sig, SIG_DFL);

	cprintf("Sending SIGTERM to all processes\n");
	kill(-1, SIGTERM);
	sleep(1);

	cprintf("Sending SIGKILL to all processes\n");
	kill(-1, SIGKILL);
	sleep(1);

	sync();
}

static int fatal_signals[] = {
	SIGQUIT,
	SIGILL,
	SIGABRT,
	SIGFPE,
	SIGPIPE,
	SIGBUS,
	SIGSEGV,
	SIGSYS,
	SIGTRAP,
	SIGPWR,
	SIGTERM,	/* reboot */
	SIGUSR1,	/* halt */
};

void
fatal_signal(int sig)
{
	char *message = NULL;

	switch (sig) {
	case SIGQUIT: message = "Quit"; break;
	case SIGILL: message = "Illegal instruction"; break;
	case SIGABRT: message = "Abort"; break;
	case SIGFPE: message = "Floating exception"; break;
	case SIGPIPE: message = "Broken pipe"; break;
	case SIGBUS: message = "Bus error"; break;
	case SIGSEGV: message  = "Segmentation fault"; break;
	case SIGSYS: message = "Bad system call"; break;
	case SIGTRAP: message = "Trace trap"; break;
	case SIGPWR: message = "Power failure"; break;
	case SIGTERM: message = "Terminated"; break;
	case SIGUSR1: message = "User-defined signal 1"; break;
	}

	if (message)
		cprintf("%s\n", message);
	else
		cprintf("Caught signal %d\n", sig);

	shutdown_system();
	sleep(2);

    /* wklin modified start, 01/17/2007 */
#if 1
    /* NEVER HALT */
    reboot(RB_AUTOBOOT);
#else
	/* Halt on SIGUSR1 */
	reboot(sig == SIGUSR1 ? RB_HALT_SYSTEM : RB_AUTOBOOT);
#endif
    /* wklin modified end, 01/17/2007 */
	loop_forever();
}

static void
reap(int sig)
{
	pid_t pid;
	pmon_func_t func;

	while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
		dprintf("Reaped %d\n", pid);
		if ((func = pmon_get_func(pid)) != NULL) {
			pmon_unregister(pid);
			(*func)();
		}
	}
}


void
signal_init(void)
{
	int i;

	for (i = 0; i < sizeof(fatal_signals)/sizeof(fatal_signals[0]); i++)
		signal(fatal_signals[i], fatal_signal);

	signal(SIGCHLD, reap);
}

/*  Copyright 1996,1997,2001,2002,2007,2009 Alain Knaff.
 *  This file is part of mtools.
 *
 *  Mtools is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Mtools is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Mtools.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "sysincludes.h"
#include "mtools.h"

#undef got_signal

int got_signal = 0;

static void signal_handler(int dummy)
{
	got_signal = 1;
#if 0
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
#endif
}

#if 0
int do_gotsignal(char *f, int n)
{
	if(got_signal)
		fprintf(stderr, "file=%s line=%d\n", f, n);
	return got_signal;
}
#endif

void setup_signal(void)
{
	/* catch signals */
#ifdef SIGHUP
	signal(SIGHUP, signal_handler);
#endif
#ifdef SIGINT
	signal(SIGINT, signal_handler);
#endif
#ifdef SIGTERM
	signal(SIGTERM, signal_handler);
#endif
#ifdef SIGQUIT
	signal(SIGQUIT, signal_handler);
#endif
}

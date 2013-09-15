/*  Copyright 1997,2000-2002,2009 Alain Knaff.
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
 *
 * mcopy.c
 * Copy an MSDOS files to and from Unix
 *
 */


#define LOWERCASE

#include "sysincludes.h"
#include "msdos.h"
#include "mtools.h"
#include "vfat.h"
#include "mainloop.h"
#include "plain_io.h"
#include "nameclash.h"
#include "file.h"
#include "fs.h"



typedef struct Arg_t {
	char *target;
	MainParam_t mp;
	ClashHandling_t ch;
	Stream_t *sourcefile;
} Arg_t;

static int dos_showfat(direntry_t *entry, MainParam_t *mp)
{
	Stream_t *File=mp->File;

	fprintPwd(stdout, entry,0);
	putchar(' ');
	printFat(File);
	printf("\n");
	return GOT_ONE;
}

static int unix_showfat(MainParam_t *mp)
{
	fprintf(stderr,"File does not reside on a Dos fs\n");
	return ERROR_ONE;
}


static void usage(int ret) NORETURN;
static void usage(int ret)
{
	fprintf(stderr,
		"Mtools version %s, dated %s\n", mversion, mdate);
	fprintf(stderr,
		"Usage: %s files\n", progname);
	exit(ret);
}

void mshowfat(int argc, char **argv, int mtype)
{
	Arg_t arg;
	int c, ret;
	
	/* get command line options */

	init_clash_handling(& arg.ch);

	/* get command line options */
	if(helpFlag(argc, argv))
		usage(0);
	while ((c = getopt(argc, argv, "i:h")) != EOF) {
		switch (c) {
			case 'i':
				set_cmd_line_image(optarg, 0);
				break;
			case 'h':
				usage(0);
			case '?':
				usage(1);
				break;
		}
	}

	if (argc - optind < 1)
		usage(1);

	/* only 1 file to copy... */
	init_mp(&arg.mp);
	arg.mp.arg = (void *) &arg;

	arg.mp.callback = dos_showfat;
	arg.mp.unixcallback = unix_showfat;

	arg.mp.lookupflags = ACCEPT_PLAIN | ACCEPT_DIR | DO_OPEN;
	ret=main_loop(&arg.mp, argv + optind, argc - optind);
	exit(ret);
}

/*  Copyright 1995-1999,2001-2003,2007,2009 Alain Knaff.
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
 * mbadblocks.c
 * Mark bad blocks on disk
 *
 */

#include "sysincludes.h"
#include "msdos.h"
#include "mtools.h"
#include "mainloop.h"
#include "fsP.h"

void mbadblocks(int argc, char **argv, int type)
{
	unsigned int i;
	char *in_buf;
	int in_len;
	off_t start;
	struct MainParam_t mp;
	Fs_t *Fs;
	Stream_t *Dir;
	int ret;

	if (argc != 2 || !argv[1][0] || argv[1][1] != ':' || argv[1][2]) {
		fprintf(stderr, "Mtools version %s, dated %s\n", 
			mversion, mdate);
		fprintf(stderr, "Usage: %s [-V] device\n", argv[0]);
		exit(1);
	}

	init_mp(&mp);

	Dir = open_root_dir(argv[1][0], O_RDWR, NULL);
	if (!Dir) {
		fprintf(stderr,"%s: Cannot initialize drive\n", argv[0]);
		exit(1);
	}

	Fs = (Fs_t *)GetFs(Dir);
	in_len = Fs->cluster_size * Fs->sector_size;
	in_buf = malloc(in_len);
	if(!in_buf) {
		FREE(&Dir);
		printOom();
		exit(1);
	}
	for(i=0; i < Fs->clus_start; i++ ){
		ret = READS(Fs->Next, in_buf, 
			    sectorsToBytes((Stream_t*)Fs, i), Fs->sector_size);
		if( ret < 0 ){
			perror("early error");
			exit(1);
		}
		if(ret < (signed int) Fs->sector_size){
			fprintf(stderr,"end of file in file_read\n");
			exit(1);
		}
	}
		
	in_len = Fs->cluster_size * Fs->sector_size;
	for(i=2; i< Fs->num_clus + 2; i++){
		if(got_signal)
			break;
		if(Fs->fat_decode((Fs_t*)Fs,i))
			continue;
		start = (i - 2) * Fs->cluster_size + Fs->clus_start;
		ret = force_read(Fs->Next, in_buf, 
						 sectorsToBytes((Stream_t*)Fs, start), in_len);
		if(ret < in_len ){
			printf("Bad cluster %d found\n", i);
			fatEncode((Fs_t*)Fs, i, 0xfff7);
			continue;
		}
	}
	FREE(&Dir);
	exit(0);
}

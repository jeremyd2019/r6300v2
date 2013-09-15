/*  Copyright 1986-1992 Emmet P. Gray.
 *  Copyright 1996-1998,2000-2002,2005,2007-2009 Alain Knaff.
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
 * mlabel.c
 * Make an MSDOS volume label
 */

#include "sysincludes.h"
#include "msdos.h"
#include "mainloop.h"
#include "vfat.h"
#include "mtools.h"
#include "nameclash.h"
#include "file_name.h"

#define USB_VOL_NAME_FILE   "/tmp/usb_vol_name/%s"  /*  added pling 05/06/2009 */

void label_name(doscp_t *cp, const char *filename, int verbose,
		int *mangled, dos_name_t *ans)
{
	int len;
	int i;
	int have_lower, have_upper;
	wchar_t wbuffer[12];

	strcpy(ans->base,"           ");
	len = native_to_wchar(filename, wbuffer, 11, 0, 0);
	if(len > 11){
		*mangled = 1;
		len = 11;
	} else
		*mangled = 0;

	have_lower = have_upper = 0;
	for(i=0; i<len; i++){
		if(islower(wbuffer[i]))
			have_lower = 1;
		if(isupper(wbuffer[i]))
			have_upper = 1;
		wbuffer[i] = towupper(wbuffer[i]);
		if(
#ifdef HAVE_WCHAR_H
		   wcschr(L"^+=/[]:,?*\\<>|\".", wbuffer[i])
#else
		   strchr("^+=/[]:,?*\\<>|\".", wbuffer[i])
#endif
		   ){
			*mangled = 1;
			wbuffer[i] = '~';
		}
	}
	if (have_lower && have_upper)
		*mangled = 1;
	wchar_to_dos(cp, wbuffer, ans->base, len, mangled);
}

int labelit(struct dos_name_t *dosname,
	    char *longname,
	    void *arg0,
	    direntry_t *entry)
{
	time_t now;

	/* find out current time */
	getTimeNow(&now);
	mk_entry(dosname, 0x8, 0, 0, now, &entry->dir);
	return 0;
}

static void usage(int ret) NORETURN;
static void usage(int ret)
{
	fprintf(stderr, "Mtools version %s, dated %s\n",
		mversion, mdate);
	fprintf(stderr, "Usage: %s [-vscVn] [-N serial] drive:\n", progname);
	exit(ret);
}


void mlabel(int argc, char **argv, int type)
{

	char *newLabel;
	int verbose, clear, interactive, show;
	direntry_t entry;
	int result=0;
	char longname[VBUFSIZE];
	char shortname[45];
	ClashHandling_t ch;
	struct MainParam_t mp;
	Stream_t *RootDir;
	int c;
	int mangled;
	enum { SER_NONE, SER_RANDOM, SER_SET }  set_serial = SER_NONE;
	long serial = 0;
	int need_write_boot = 0;
	int have_boot = 0;
	char *eptr;
	struct bootsector boot;
	Stream_t *Fs=0;
	int r;
	struct label_blk_t *labelBlock;
	int isRo=0;
	int *isRop=NULL;

    /*  added start pling 05/06/2009 */
    char sd_name[32];
    char vol_name[64];

    memset(sd_name, 0, sizeof(sd_name));
    memset(vol_name, 0, sizeof(vol_name));
    /*  added end pling 05/06/2009 */

	init_clash_handling(&ch);
	ch.name_converter = label_name;
	ch.ignore_entry = -2;

	verbose = 0;
	clear = 0;
	show = 0;

	if(helpFlag(argc, argv))
		usage(0);
	while ((c = getopt(argc, argv, "i:vcsnN:h")) != EOF) {
		switch (c) {
			case 'i':
				set_cmd_line_image(optarg, 0);
                strcpy(sd_name, optarg);    //  added pling 05/06/2009
				break;
			case 'v':
				verbose = 1;
				break;
			case 'c':
				clear = 1;
				break;
			case 's':
				show = 1;
				break;
			case 'n':
				set_serial = SER_RANDOM;
				srandom((long)time (0));
				serial=random();
				break;
			case 'N':
				set_serial = SER_SET;
				serial = strtol(optarg, &eptr, 16);
				if(*eptr) {
					fprintf(stderr,
						"%s not a valid serial number\n",
						optarg);
					exit(1);
				}
				break;
			case 'h':
				usage(0);
			default:
				usage(1);
			}
	}

	if (argc - optind != 1 || !argv[optind][0] || argv[optind][1] != ':')
		usage(1);

	init_mp(&mp);
	newLabel = argv[optind]+2;
	if(strlen(newLabel) > VBUFSIZE) {
		fprintf(stderr, "Label too long\n");
		FREE(&RootDir);
		exit(1);
	}

	interactive = !show && !clear &&!newLabel[0] &&
		(set_serial == SER_NONE);
	if(!clear && !newLabel[0]) {
		isRop = &isRo;
	}
	RootDir = open_root_dir(argv[optind][0], isRop ? 0 : O_RDWR, isRop);
	if(isRo) {
		show = 1;
		interactive = 0;
	}	
	if(!RootDir) {
		fprintf(stderr, "%s: Cannot initialize drive\n", argv[0]);
		exit(1);
	}

	initializeDirentry(&entry, RootDir);
	r=vfat_lookup(&entry, 0, 0, ACCEPT_LABEL | MATCH_ANY,
		      shortname, longname);
	if (r == -2) {
		FREE(&RootDir);
		exit(1);
	}

	if(show || interactive){
		if(isNotFound(&entry)) {
			printf(" Volume has no label\n");
            strcpy(vol_name, "");
        }
		else if (*longname) {
			printf(" Volume label is %s (abbr=%s)\n",
			       longname, shortname);
            strcpy(vol_name, longname);
        }
		else {
			printf(" Volume label is %s\n",  shortname);
            strcpy(vol_name, shortname);
        }

        /*  added start pling 05/06/2009 */
        /* Store the volume label in a file under /tmp */
        FILE *fp = NULL;
        char *devname;
        char filename[64];

        if (strlen(sd_name)) {
            devname = strstr(sd_name, "sd");
            if (devname) {
                sprintf(filename, USB_VOL_NAME_FILE, devname);
                fp = fopen(filename, "w");
                if (fp != NULL) {
                    fprintf(fp, "%s\n", vol_name);
                    fclose(fp);
                }
            }
        }   
        /*  added end pling 05/06/2009 */
	}

	/* ask for new label */
	if(interactive){
		newLabel = longname;
		fprintf(stderr,"Enter the new volume label : ");
		fgets(newLabel, VBUFSIZE, stdin);
		if(newLabel[0])
			newLabel[strlen(newLabel)-1] = '\0';
	}

	if((!show || newLabel[0]) && !isNotFound(&entry)){
		/* if we have a label, wipe it out before putting new one */
		if(interactive && newLabel[0] == '\0')
			if(ask_confirmation("Delete volume label (y/n): ")){
				FREE(&RootDir);
				exit(0);
			}
		entry.dir.attr = 0; /* for old mlabel */
		wipeEntry(&entry);
	}

	if (newLabel[0] != '\0') {
		ch.ignore_entry = 1;
		result = mwrite_one(RootDir,newLabel,0,labelit,NULL,&ch) ?
		  0 : 1;
	}

	have_boot = 0;
	if( (!show || newLabel[0]) || set_serial != SER_NONE) {
		Fs = GetFs(RootDir);
		have_boot = (force_read(Fs,(char *)&boot,0,sizeof(boot)) ==
			     sizeof(boot));
	}

	if(_WORD(boot.fatlen)) {
	    labelBlock = &boot.ext.old.labelBlock;
	} else {
	    labelBlock = &boot.ext.fat32.labelBlock;
	}

	if(!show || newLabel[0]){
		dos_name_t dosname;
		const char *shrtLabel;
		doscp_t *cp;
		if(!newLabel[0])
			shrtLabel = "NO NAME    ";
		else
			shrtLabel = newLabel;
		cp = GET_DOSCONVERT(Fs);
		label_name(cp, shrtLabel, verbose, &mangled, &dosname);

		if(have_boot && boot.descr >= 0xf0 &&
		   labelBlock->dos4 == 0x29) {
			strncpy(labelBlock->label, dosname.base, 11);
			need_write_boot = 1;

		}
	}

	if((set_serial != SER_NONE) & have_boot) {
		if(have_boot && boot.descr >= 0xf0 &&
		   labelBlock->dos4 == 0x29) {
			set_dword(labelBlock->serial, serial);	
			need_write_boot = 1;
		}
	}

	if(need_write_boot) {
		force_write(Fs, (char *)&boot, 0, sizeof(boot));
	}

	FREE(&RootDir);
	exit(result);
}

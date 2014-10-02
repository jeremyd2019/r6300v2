#include <config.h>
#include <stdio.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_DIRENT_H
#  include <dirent.h>
#else
#    define dirent direct
#    define NAMLEN(dirent) (dirent)->d_namlen
#  ifdef HAVE_SYS_NDIR_H
#    include <sys/ndir.h>
#  endif
#  ifdef HAVE_SYS_DIR_H
#    include <sys/dir.h>
#  endif
#  ifdef HAVE_NDIR_H
#    include <ndir.h>
#  endif
#endif

#include <unistd.h>
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <mystring.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <glob.h>

#include "cwd.h"
#include "options.h"
#include "main.h"
#include "login.h"
#include "dirlist.h"

/* pling added start 06/24/2009 */
/* Calculcate the real file size based on stat info */
static unsigned long long get_real_file_size(struct stat *st)
{
    unsigned long long file_size_64;
    unsigned long long order;

    /* Check whether file size > 4GB which exceeds 32-bit st->st_size 
     *  st->sb_blocks in units of 512 bytes.
     *  If 'st->sb_blocks' x 512 > 4GB, then this file > 4GB.
     */
    if (st->st_blocks >= 8388608L)
    {
        order = st->st_blocks / 8388608L;  // order = # of 4GB
        file_size_64 = 0x100000000 * order + (unsigned long)(st->st_size);
    }
    else
        file_size_64 = st->st_size & 0xFFFFFFFF;

    //printf("file_size_64 = %llu\n", file_size_64);

    return file_size_64;
}
/* pling added end 06/24/2009 */

struct hidegroup *hidegroups = NULL;

void add_to_hidegroups(int gid)
{
    static struct hidegroup *tmp = NULL;
    if (tmp)
        tmp = tmp->next = malloc(sizeof(struct hidegroup));
    else
        hidegroups = tmp = malloc(sizeof(struct hidegroup));

    if (tmp)
    {
      tmp->next = NULL;
      tmp->data = gid;
    }
}

void hidegroups_init()
{
    char *foo = strdup(config_getoption("HIDE_GROUP"));
    char *foo_save = foo;
    char *bar;
    struct group *tmpgrp;

    while ((bar = strtok(foo, ","))) {
        foo = NULL; /* strtok requirement */
        if ((strcmp(bar, "0")) && (!strtoul(bar, NULL, 10))) {
            /* bar is not numeric */
            if ((tmpgrp = getgrnam(bar)))
                add_to_hidegroups(tmpgrp->gr_gid);
        } else
            if (strtoul(bar, NULL, 10))
                add_to_hidegroups(strtoul(bar, NULL, 10));
    }
	free(foo_save);
}

void hidegroups_end()
{
    struct hidegroup *tmp = hidegroups;
    if (hidegroups)
        while (hidegroups->next) {
            tmp = hidegroups->next;
            free(hidegroups);
            hidegroups = tmp;
        }
}

/*  added start pling 06/20/2009 */
#include "logging.h"
int is_dir_allow_read(char *dir_name)
{
    char curr_dir[1024];
    int allow_read = 0;

    /* Change to the specified dir, and see if the
     * "ALLOWCOMMAND_LIST" is enabled or not.
     */
    memset(curr_dir, 0, sizeof(curr_dir));
    getcwd(curr_dir, sizeof(curr_dir)-1);
    
    bftpd_log("Cwd='%s', checking '%s' for admin-read\n", curr_dir, dir_name);

    if (chdir(dir_name))
        bftpd_log("Unable to chdir '%s', errno=%d\n", dir_name, errno);
    else
    {
        if (!strcasecmp(getoption_user("ALLOWCOMMAND_LIST"), "yes"))
            allow_read = 1;
    	    chdir(curr_dir);
    }

    return allow_read;
}
/*  added end pling 06/20/2009 */

void bftpd_stat(char *name, FILE * client)
{
    struct stat statbuf;
    char temp[MAXCMD + 3], linktarget[MAXCMD + 5], perm[11], timestr[17], uid[USERLEN + 1], gid[USERLEN + 1];
    struct tm filetime;
    time_t t;
    static int first_time = 1;  /*  added pling 06/28/2010 */


    if (lstat(name, (struct stat *) &statbuf) == -1) { // used for command_stat
        fprintf(client, "213-Error: %s.\n", strerror(errno));
        return;
    }

#if 0    
    /*  added start pling 06/20/2009 */
    /* Don't let 'guest' user see "Admin-read" folders */
    if (S_ISDIR(statbuf.st_mode) || S_ISLNK(statbuf.st_mode))
    {
        if (!is_dir_allow_read(name))
            return;
    }
    /*  added end pling 06/20/2009 */
#endif

    //if ( strcmp(user, "admin") != 0 ) {
    //    if ( !is_dir_allow_read(name) )
    //        return;	       
    //}	    

#ifdef S_ISLNK
	if (S_ISLNK(statbuf.st_mode)) {
        /*  modified start pling 03/09/2010 */
        /* Show links as directories, other Safari can't 
         * handle links properly.
         * Note: in FAT32/NTFS, there are no links.
         * This link is either "USB_Storage", "T_Drive"
         * or other share names that user defines.
         */
		//strcpy(perm, "lrwxrwxrwx");
		strcpy(perm, "drwxrwxrwx");
        /*  modified end pling 03/09/2010 */
        /*  modified start pling 06/20/2009 */
        /* Don't show symlink */
#if 0
		temp[readlink(name, temp, sizeof(temp) - 1)] = '\0';
		sprintf(linktarget, " -> %s", temp);
#endif
        linktarget[0] = '\0';
        /*  modified end pling 06/20/2009 */
	} else {
#endif
		strcpy(perm, "----------");
		if (S_ISDIR(statbuf.st_mode))
			perm[0] = 'd';
		if (statbuf.st_mode & S_IRUSR)
			perm[1] = 'r';
		if (statbuf.st_mode & S_IWUSR)
			perm[2] = 'w';
		if (statbuf.st_mode & S_IXUSR)
			perm[3] = 'x';
		if (statbuf.st_mode & S_IRGRP)
			perm[4] = 'r';
		if (statbuf.st_mode & S_IWGRP)
			perm[5] = 'w';
		if (statbuf.st_mode & S_IXGRP)
			perm[6] = 'x';
		if (statbuf.st_mode & S_IROTH)
			perm[7] = 'r';
		if (statbuf.st_mode & S_IWOTH)
			perm[8] = 'w';
		if (statbuf.st_mode & S_IXOTH)
			perm[9] = 'x';
		linktarget[0] = '\0';
#ifdef S_ISLNK
	}
#endif
    memcpy(&filetime, localtime(&(statbuf.st_mtime)), sizeof(struct tm));
    time(&t);
    if (filetime.tm_year == localtime(&t)->tm_year)
    	mystrncpy(timestr, ctime(&(statbuf.st_mtime)) + 4, 12);
    else
        strftime(timestr, sizeof(timestr), "%b %d  %G", &filetime);
    mygetpwuid(statbuf.st_uid, passwdfile, uid)[8] = 0;
    mygetpwuid(statbuf.st_gid, groupfile, gid)[8] = 0;

    /*  added start pling 06/28/2010 */
    /* Fix Chrome V4.0.249.78 FTP issue;
     *  need to provide a "." directory entry for Chrome to
     *  show properly.
     */
    if (first_time)
    {
    	fprintf(client, "%s %3i %-8s %-8s 0 %s .\r\n", "drwxrwxrwx",
	    		(int) 1, uid, gid,
			    timestr);
        first_time = 0;
    }
    /*  added end pling 06/28/2010 */

    /*  modified start pling 06/29/2009 */
    /* Show big file size (>4GB) correctly */
#if 0
	fprintf(client, "%s %3i %-8s %-8s %8lu %s %s%s\r\n", perm,
			(int) statbuf.st_nlink, uid, gid,
			(unsigned long) statbuf.st_size,
			timestr, name, linktarget);
#endif
    unsigned long long real_file_size = get_real_file_size(&statbuf);
	fprintf(client, "%s %3i %-8s %-8s %llu %s %s%s\r\n", perm,
			(int) statbuf.st_nlink, uid, gid,
			(unsigned long long) real_file_size,
			timestr, name, linktarget);
    /*  modified end pling 06/29/2009 */
}

void dirlist_one_file(char *name, FILE *client, char verbose)
{
    struct stat statbuf;
    struct hidegroup *tmp = hidegroups;
    char *filename_index;      /* place where filename starts in path */

    if (!stat(name, (struct stat *) &statbuf)) {
        if (tmp)
            do {
                if (statbuf.st_gid == tmp->data)
                    return;
            } while ((tmp = tmp->next));
    }

    /* find start of filename after path */
    filename_index = strrchr(name, '/');
    if (filename_index)
       filename_index++;   /* goto first character after '/' */
    else
       filename_index = name;    

    if (lstat(name, (struct stat *) &statbuf) == -1) { // used for command_stat
       fprintf(client, "213-Error: %s.\n", strerror(errno));
       return;
    }

    char *cwd = bftpd_cwd_getcwd();
    if (cwd != NULL)
    {
        if (strcmp(cwd, "/") == 0 && strcmp(name, "shares") != 0)
        {
            free(cwd);
            return;
        }
        free(cwd);
    }
    /*  added start pling 06/20/2009 */
    /* Don't let 'guest' user see "Admin-read" folders */
    if (S_ISDIR(statbuf.st_mode) || S_ISLNK(statbuf.st_mode))
    {
          if (!is_dir_allow_read(name))
          return;
    }
    /*  added end pling 06/20/2009 */

    if (verbose)
        bftpd_stat(name, client);
    else
        fprintf(client, "%s\r\n", filename_index);
}

void dirlist(char *name, FILE * client, char verbose)
{
	DIR *directory;
    char *cwd = NULL;
    int i;
	glob_t globbuf;
    if ((strstr(name, "/.")) && strchr(name, '*'))
        return; /* DoS protection */
	if ((directory = opendir(name))) {
	   closedir(directory);
           cwd = bftpd_cwd_getcwd();
           chdir(name);
           glob("*", 0, NULL, &globbuf);
	} else
    	   glob(name, 0, NULL, &globbuf);

	for (i = 0; i < globbuf.gl_pathc; i++)
            dirlist_one_file(globbuf.gl_pathv[i], client, verbose);

	globfree(&globbuf);
	if (cwd) {
  	   chdir(cwd);
		free(cwd);
	}
}

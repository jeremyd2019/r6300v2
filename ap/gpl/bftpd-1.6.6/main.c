/*

bftpd Copyright (C) 1999-2003 Max-Wilhelm Bruker

This program is is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License, version 2 of the
License as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/

#include <config.h>
#include <stdio.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_ASM_SOCKET_H
#include <asm/socket.h>
#endif
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#ifdef HAVE_WAIT_H
# include <wait.h>
#else
# ifdef HAVE_SYS_WAIT_H
#  include <sys/wait.h>
# endif
#endif

#include "main.h"
#include "cwd.h"
#include "mystring.h"
#include "logging.h"
#include "dirlist.h"
#include "bftpdutmp.h"
#include "options.h"
#include "login.h"
#include "list.h"

#ifdef MAX_USB_ACCESS
#ifndef LINUX26
#include <linux/spinlock.h>
#endif
#include <sys/shm.h>
#include <sys/stat.h>
#define MAX_CON_NUM 15
typedef struct
{
    int sem_id;             //ID of Semaphore, for data sync.
    int num;    //The number of total connections.
    int ftp_num; //The number of ftp connections without pure WAN ftp..
    int wan_ftp_num; //The number of pure WAN ftp connections.
}CON_STATISTIC;

static int segment_id;
static CON_STATISTIC *con_st;

char client_ip[32];
int  g_isLanIp;



#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>

/* We must define union semun ourselves for using Semaphore. */
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short int *array;
    struct seminfo *__buf;
};

/* Obtain a binary semaphore¡¦s ID, allocating if necessary. */
int binary_semaphore_allocation (key_t key, int sem_flags)
{
    return semget (key, 1, sem_flags);
}
/* Deallocate a binary semaphore. All users must have finished their
use. Returns -1 on failure. */
int binary_semaphore_deallocate (int semid)
{
    union semun ignored_argument;
    return semctl (semid, 1, IPC_RMID, ignored_argument);
}

/* Initialize a binary semaphore with a value of 1. */
int binary_semaphore_initialize (int semid)
{
    union semun argument;
    unsigned short values[1];
    values[0] = 1;
    argument.array = values;
    return semctl (semid, 0, SETALL, argument);
}
/* Wait on a binary semaphore. Block until the semaphore value is positive, then
decrement it by 1. */
int binary_semaphore_wait (int semid)
{
    struct sembuf operations[1];
    /* Use the first (and only) semaphore. */
    operations[0].sem_num = 0;
    /* Decrement by 1. */
    operations[0].sem_op = -1;
    /* Permit undo¡¦ing. */
    operations[0].sem_flg = SEM_UNDO;
    return semop (semid, operations, 1);
}

/* Post to a binary semaphore: increment its value by 1.
This returns immediately. */
int binary_semaphore_post (int semid)
{
    struct sembuf operations[1];
    /* Use the first (and only) semaphore. */
    operations[0].sem_num = 0;
    /* Increment by 1. */
    operations[0].sem_op = 1;
    /* Permit undo¡¦ing. */
    operations[0].sem_flg = SEM_UNDO;
    return semop (semid, operations, 1);
}
#else
char client_ip[32];
int  g_isLanIp;
#endif  /* End of MAX_USB_ACCESS */

int global_argc;
char **global_argv;
char **my_argv_list;            // jesse
struct sockaddr_in name;
int isparent = 1;
int listensocket, sock;
FILE *passwdfile = NULL, *groupfile = NULL, *devnull;
struct sockaddr_in remotename;
char *remotehostname;
int control_timeout, data_timeout;
int alarm_type = 0;

struct bftpd_list_element *child_list;

/* Command line parameters */
char *configpath = PATH_BFTPD_CONF;
int daemonmode = 0;

int all_no_password = 0;

int inc_conn_num();
int dec_conn_num();

void print_file (int number, char *filename)
{
    FILE *phile;
    char foo[256];

    phile = fopen (filename, "r");
    if (phile)
    {
        while (fgets (foo, sizeof (foo), phile))
        {
            foo[strlen (foo) - 1] = '\0';
            control_printf (SL_SUCCESS, "%i-%s", number, foo);
        }
        fclose (phile);
    }
}

void end_child ()
{
    printf("end_child invoked.\n");
/*    if(con_st != NULL)
    {
        binary_semaphore_wait(con_st->sem_id);
        --(con_st->num);
        binary_semaphore_post(con_st->sem_id);
#ifdef CON_DEBUG
        printf("->total con num: %d, by bftpd\n",
                con_st->num);
#endif
    }*/

    if (passwdfile)
        fclose (passwdfile);
    if (groupfile)
        fclose (groupfile);
    config_end ();
    bftpd_log ("Quitting.\n");
    bftpd_statuslog (1, 0, "quit");
    bftpdutmp_end ();
    log_end ();
    login_end ();
    bftpd_cwd_end ();
    if (daemonmode)
    {
        close (sock);
        close (0);
        close (1);
        close (2);
    }
}



/*
This function causes the program to 
re-read parts of the config file.

-- Jesse
*/
void handler_sighup (int sig)
{
    bftpd_log("%s.\n", __FUNCTION__);
    /*if(con_st != NULL)
    {
        binary_semaphore_wait(con_st->sem_id);
        --(con_st->num);
        binary_semaphore_post(con_st->sem_id);
#ifdef CON_DEBUG
        printf("->total con num: %d, by bftpd\n",
                con_st->num);
#endif
    }*/
    bftpd_log ("Caught HUP signal. Re-reading config file.\n");
    Reread_Config_File ();
    signal (sig, handler_sighup);
}




void handler_sigchld (int sig)
{
    pid_t pid;
    int i;
    struct bftpd_childpid *childpid;

    bftpd_log("%s\n", __FUNCTION__);

#ifdef MAX_USB_ACCESS_OLD
    if(con_st != NULL)
    {
        FILE *pfp;
        int bftpd_pid;
        int wan_ftp = 0;
        if ((pfp = fopen ("/var/run/bftpd_wan.pid", "r")))
        {
            char tmp[32];
            fgets(tmp, 80, pfp);
            bftpd_pid = atoi(tmp);
            if((getpid() == bftpd_pid))
                wan_ftp = 1;
            fclose(pfp);
        }
        binary_semaphore_wait(con_st->sem_id);
	if(con_st->num > 0)
	{
            --(con_st->num);
            if(wan_ftp)
	    	--(con_st->wan_ftp_num);
            else
		--(con_st->ftp_num);
	}
        binary_semaphore_post(con_st->sem_id);
#ifdef CON_DEBUG
        printf("->total con num: %d, by bftpd\n", con_st->num);
#endif
    }
#endif // End of MAX_USB_ACCESS


    /* Get the child's return code so that the zombie dies */
    pid = wait (NULL);
    for (i = 0; i < bftpd_list_count (child_list); i++)
    {
        childpid = bftpd_list_get (child_list, i);
        if (childpid->pid == pid)
        {
            close (childpid->sock);
            bftpd_list_del (&child_list, i);
            free (childpid);
            /* make sure the child is removed from the log */
            bftpdutmp_remove_pid (pid);
#ifdef MAX_USB_ACCESS	    
	    dec_conn_num();
#endif	    
        }
    }
}

void handler_sigterm (int signum)
{
    bftpd_log ("%s.\n", __FUNCTION__);
/*    if(con_st != NULL)
    {
	binary_semaphore_wait(con_st->sem_id);
	//if(con_st->ftp_num != 0)
        (con_st->num) = (con_st->num) -100;
        binary_semaphore_post(con_st->sem_id);
#ifdef CON_DEBUG
        printf("->total con num: %d, by bftpd\n",
               con_st->num);
#endif
    }*/


    bftpdutmp_end ();
    exit (0);                   /* Force normal termination so that end_child() is called */
}

void handler_sigalrm (int signum)
{
    printf("%s:\n", __FUNCTION__);
    /* Log user out. -- Jesse <slicer69@hotmail.com> */
    bftpdutmp_end ();

/*    if(con_st != NULL)
    {
        binary_semaphore_wait(con_st->sem_id);
        --(con_st->num);
        binary_semaphore_post(con_st->sem_id);
#ifdef CON_DEBUG
        printf("->total con num: %d, by bftpd\n", con_st->num);
#endif
    }*/

    if (alarm_type)
    {
        close (alarm_type);
 
        bftpd_log
            ("Kicked from the server due to data connection timeout.\n");
        control_printf (SL_FAILURE,
                        "421 Kicked from the server due to data connection timeout.");
        exit (0);
    }
    else
    {
        bftpd_log
            ("Kicked from the server due to control connection timeout.\n");
        control_printf (SL_FAILURE,
                        "421 Kicked from the server due to control connection timeout.");
        exit (0);
    }
}

void init_everything ()
{
    if (!daemonmode)
    {
        config_init ();
        hidegroups_init ();
    }
    log_init ();
    bftpdutmp_init ();
    login_init ();
}

#ifdef MAX_USB_ACCESS
int dec_conn_num()
{
    if(con_st != NULL)
    {
        FILE *pfp;
        int bftpd_pid;
        int wan_ftp = 0;
        if ((pfp = fopen ("/var/run/bftpd_wan.pid", "r")))
	{
	    char tmp[32];
	    fgets(tmp, 80, pfp);
	    bftpd_pid = atoi(tmp);
	    if((getpid() == bftpd_pid))
	          wan_ftp = 1;
	    fclose(pfp);
        }
        binary_semaphore_wait(con_st->sem_id);
        if(con_st->num > 0)
	{
	     --(con_st->num);
	     if(wan_ftp)
	         --(con_st->wan_ftp_num);
	     else
	         --(con_st->ftp_num);
	}
        binary_semaphore_post(con_st->sem_id);
//#ifdef CON_DEBUG
        printf("->total con num: %d, by bftpd\n", con_st->num);
//#endif
    }
}	

int inc_conn_num ()
{
    if(con_st != NULL)
    {
	FILE *pfp;
	int bftpd_pid;
	int wan_ftp = 0;

	if ((pfp = fopen ("/var/run/bftpd_wan.pid", "r")))
	{
	    char tmp[32];
	    fgets(tmp, 80, pfp);
	    bftpd_pid = atoi(tmp);
	    if((getppid() == bftpd_pid))
	        wan_ftp = 1;		
	    fclose(pfp);
	}
        binary_semaphore_wait(con_st->sem_id);
        if(con_st->num >= MAX_CON_NUM){
            ++(con_st->num);
	    if(wan_ftp) 
		++(con_st->wan_ftp_num);
	    else
	        ++(con_st->ftp_num);
            printf("The Connections are over 15.\n");
            binary_semaphore_post(con_st->sem_id);
            return 0;
        }

        ++(con_st->num);
        if(wan_ftp)
	    ++(con_st->wan_ftp_num);
	else
	    ++(con_st->ftp_num);
        binary_semaphore_post(con_st->sem_id);
//#ifdef CON_DEBUG
        printf("->total con num: %d, by bftpd.\n", con_st->num);
//#endif
        return 1;
    }
    return 0;
}
#endif

int main (int argc, char **argv)
{
    char str[MAXCMD + 1];
    static struct hostent *he;
    int i = 1, port;
    int retval;
    int over_limit = 0;
    socklen_t my_length;
    FILE *pid_fp;
    extern int cur_conn;

    my_argv_list = argv;
    signal (SIGHUP, handler_sighup);

#ifdef MAX_USB_ACCESS
    FILE *shmid_fp = fopen("/tmp/shm_id", "r");
    if(shmid_fp != NULL)
    {
        fscanf(shmid_fp, "%d", &segment_id);
        printf("bftpd: segment_id:%d\n", segment_id);
        fclose(shmid_fp);
    }
    else{
        printf("/tmp/shm_id open failed.\n");
        segment_id = -1;
        //exit(1);
    }
    /* attach the shared memory segment, at a different address. */
    if(segment_id != -1){
        con_st = (CON_STATISTIC*) shmat (segment_id, (void*) 0x5000000, 0);
        printf ("shared memory reattached at address %p\n", con_st);
    }    else {
        con_st = NULL;
	printf ("fail to get shared memory reattached at address \n");
    }	
#endif // End of MAX_USB_ACCESS




    while (((retval = getopt (argc, argv, "c:hdDin"))) > -1)
    {
        switch (retval)
        {
        case 'h':
            printf ("Usage: %s [-h] [-i|-d|-D] [-c <filename>|-n]\n"
                    "-h print this help\n" "-i (default) run from inetd\n"
                    "-d daemon mode: fork() and run in TCP listen mode\n"
                    "-D run in TCP listen mode, but don't pre-fork()\n"
                    "-c read the config file named \"filename\" instead of "
                    PATH_BFTPD_CONF "\n" "-n no config file, use defaults\n",
                    argv[0]);
            return 0;
        case 'i':
            daemonmode = 0;
            break;
        case 'd':
            daemonmode = 1;
            break;
        case 'D':
            daemonmode = 2;
            break;
        case 'c':
            configpath = strdup (optarg);
            break;
        case 'n':
            configpath = NULL;
            break;
        }
    }
    if (daemonmode)
    {
        struct sockaddr_in myaddr, new;

        if (daemonmode == 1)
        {
            if (fork ())
                exit (0);       /* Exit from parent process */
            setsid ();
            if (fork ())
                return 0;
            if (!(pid_fp = fopen ("/var/run/bftpd.pid", "w")))
            {
                perror ("/var/run/bftpd.pid");
                return 0;
            }
            fprintf (pid_fp, "%d", getpid ());
            fclose (pid_fp);
        }
        signal (SIGCHLD, handler_sigchld);
        
//#ifndef QUICK_FIX_ISSUES //@ftpRW : we need this
//FIXME
        config_init ();
//#endif
        chdir ("/");
        hidegroups_init ();
        listensocket = socket (AF_INET, SOCK_STREAM, 0);
#ifdef SO_REUSEADDR
        setsockopt (listensocket, SOL_SOCKET, SO_REUSEADDR, (void *) &i,
                    sizeof (i));
#endif
#ifdef SO_REUSEPORT
        setsockopt (listensocket, SOL_SOCKET, SO_REUSEPORT, (void *) &i,
                    sizeof (i));
#endif
        memset ((void *) &myaddr, 0, sizeof (myaddr));
        if (!((port = strtoul (config_getoption ("PORT"), NULL, 10))))
            port = DEFAULT_PORT;
        myaddr.sin_port = htons (port);
        if (!strcasecmp (config_getoption ("BIND_TO_ADDR"), "any")
            || !config_getoption ("BIND_TO_ADDR")[0])
            myaddr.sin_addr.s_addr = INADDR_ANY;
        else{
            char file_path[256];
            FILE *pfp;

            myaddr.sin_addr.s_addr =
                inet_addr (config_getoption ("BIND_TO_ADDR"));
            
            printf("write pid:%d tp /var/run/bftpd_wan.pid.\n", getpid());

            if (!(pfp = fopen ("/var/run/bftpd_wan.pid", "w")))
            {
                perror ("/var/run/bftpd_wan.pid");
                return 0;
            }
            fprintf (pfp, "%d", getpid ());
            fclose (pfp);
        }
        if (bind (listensocket, (struct sockaddr *) &myaddr, sizeof (myaddr))
            < 0)
        {
            fprintf (stderr, "Bind failed: %s\n", strerror (errno));
            exit (1);
        }
        else
            printf("bftpd: socket bound in %s:%d\n", inet_ntoa(myaddr.sin_addr), port);
	
        if (listen (listensocket, 1))
        {
            fprintf (stderr, "Listen failed: %s\n", strerror (errno));
            exit (1);
        }

        for (i = 0; i < 3; i++)
        {
            close (i);          /* Remove fd pointing to the console */
            open ("/dev/null", O_RDWR); /* Create fd pointing nowhere */
        }

        my_length = sizeof (new);
        while ((sock =
                accept (listensocket, (struct sockaddr *) &new, &my_length)))
        {
            pid_t pid;

	    over_limit = 0;

            strcpy(client_ip, inet_ntoa(new.sin_addr));
            g_isLanIp = isLanSubnet(client_ip);

#ifdef MAX_USB_ACCESS
            /* printf("cur conn:%d %d\n", cur_conn, bftpd_list_count( &child_list) ); */
            if ( cur_conn > MAX_CON_NUM ) {
                 close(sock);
                 continue;
            }

            if(con_st != NULL)
            {
                 printf("curconn:%d\n", con_st->num);
                 if ( con_st->num >= MAX_CON_NUM ) {
		 //if ( con_st->num >= 3 ) {
	             close(sock);		 
                     /* printf("over 15 connection"); */
                     //sleep(1);
		     over_limit = 1;
                 }

		 if( over_limit ) {
                    over_limit = 0;
  		    continue;
                 }		    
            }		 
#endif
            /* If accept() becomes interrupted by SIGCHLD, it will return -1.
             * So in order not to create a child process when that happens,
             * we have to check if accept() returned an error.
             */
            if (sock > 0)
            {
                pid = fork ();
                if (!pid)
                {               /* child */
                    close (0);
                    close (1);
                    close (2);
                    isparent = 0;
                    dup2 (sock, fileno (stdin));
                    dup2 (sock, fileno (stderr));
                    break;
                }
                else
                {               /* parent */
                    struct bftpd_childpid *tmp_pid;

		    tmp_pid = malloc (sizeof (struct bftpd_childpid));
                    tmp_pid->pid = pid;
                    tmp_pid->sock = sock;
                    bftpd_list_add (&child_list, tmp_pid);
#ifdef MAX_USB_ACCESS		    
		    inc_conn_num();
#endif
	         }		
             }
        }
    }

    /* Child only. From here on... */

    //devnull = fopen ("/dev/null", "w");
    devnull = fopen ("/dev/console", "w");
    global_argc = argc;
    global_argv = argv;
    init_everything ();

    atexit (end_child);

#ifdef MAX_USB_ACCESS_OLD
    if(con_st != NULL)
    {
	FILE *pfp;
	int bftpd_pid;
	int wan_ftp = 0;

	if ((pfp = fopen ("/var/run/bftpd_wan.pid", "r")))
	{
	    char tmp[32];
	    fgets(tmp, 80, pfp);
	    bftpd_pid = atoi(tmp);
	    if((getppid() == bftpd_pid))
	        wan_ftp = 1;		
	    fclose(pfp);
	}
        binary_semaphore_wait(con_st->sem_id);
        if(con_st->num >= MAX_CON_NUM){
            ++(con_st->num);
	    if(wan_ftp) 
		++(con_st->wan_ftp_num);
	    else
	        ++(con_st->ftp_num);
            printf("The Connections are over 15.\n");
            binary_semaphore_post(con_st->sem_id);
            goto exit;
        }

        ++(con_st->num);
        if(wan_ftp)
	    ++(con_st->wan_ftp_num);
	else
	    ++(con_st->ftp_num);
        binary_semaphore_post(con_st->sem_id);
#ifdef CON_DEBUG
        printf("->total con num: %d, by bftpd.\n", con_st->num);
#endif
    }
#endif // End of MAX_USB_ACCESS

    //atexit (end_child);
    signal (SIGTERM, handler_sigterm);
    signal (SIGALRM, handler_sigalrm);


    /* If we do not have getpwnam() for some reason, then
       we must use FILE_AUTH or exit. */
#ifdef NO_GETPWNAM
    {
        char *file_auth_option;

        file_auth_option = config_getoption ("FILE_AUTH");
        if (!file_auth_option[0])
        {
            bftpd_log
                ("Exiting, becasue we have no way to authorize clients.\n");
            exit (0);
        }
    }
#endif

    control_timeout = strtoul (config_getoption ("CONTROL_TIMEOUT"), NULL, 0);
    if (!control_timeout)
        control_timeout = CONTROL_TIMEOUT;
    data_timeout = strtoul (config_getoption ("DATA_TIMEOUT"), NULL, 0);
    if (!data_timeout)
        data_timeout = DATA_TIMEOUT;
    xfer_bufsize = strtoul (config_getoption ("XFER_BUFSIZE"), NULL, 0);
    if (!xfer_bufsize)
        xfer_bufsize = XFER_BUFSIZE;

    /* get scripts to run pre and post write */
    pre_write_script = config_getoption ("PRE_WRITE_SCRIPT");
    if (!pre_write_script[0])
        pre_write_script = NULL;
    post_write_script = config_getoption ("POST_WRITE_SCRIPT");
    if (!post_write_script[0])
        post_write_script = NULL;


    my_length = sizeof (remotename);
    if (getpeername
        (fileno (stderr), (struct sockaddr *) &remotename, &my_length))
    {
        control_printf (SL_FAILURE,
                        "421-Could not get peer IP address.\r\n421 %s.",
                        strerror (errno));
        return 0;
    }
    i = 1;
    setsockopt (fileno (stdin), SOL_SOCKET, SO_OOBINLINE, (void *) &i,
                sizeof (i));
    setsockopt (fileno (stdin), SOL_SOCKET, SO_KEEPALIVE, (void *) &i,
                sizeof (i));
    /* If option is set, determine the client FQDN */
    if (!strcasecmp ((char *) config_getoption ("RESOLVE_CLIENT_IP"), "yes"))
    {
        if ((he =
             gethostbyaddr ((char *) &remotename.sin_addr,
                            sizeof (struct in_addr), AF_INET)))
            remotehostname = strdup (he->h_name);
        else
            remotehostname = strdup (inet_ntoa (remotename.sin_addr));
    }
    else
        remotehostname = strdup (inet_ntoa (remotename.sin_addr));
    bftpd_log ("Incoming connection from %s.\n", remotehostname);
    
    /*if(con_st != NULL)
    {
        binary_semaphore_wait(con_st->sem_id);
        if(con_st->num >= MAX_CON_NUM){
	    ++(con_st->num);	
            printf("The Connections are over 15.\n");
            binary_semaphore_post(con_st->sem_id);
            goto exit;
        }

        ++(con_st->num);
        binary_semaphore_post(con_st->sem_id);    
#ifdef CON_DEBUG
        printf("->total con num: %d, by bftpd.\n", con_st->num);
#endif
    }*/


    bftpd_statuslog (1, 0, "connect %s", remotehostname);
    my_length = sizeof (name);
    getsockname (fileno (stdin), (struct sockaddr *) &name, &my_length);
    print_file (220, config_getoption ("MOTD_GLOBAL"));
    /* Parse hello message */
    strcpy (str, (char *) config_getoption ("HELLO_STRING"));
    replace (str, "%v", VERSION);
    if (strstr (str, "%h"))
    {
        if ((he =
             gethostbyaddr ((char *) &name.sin_addr, sizeof (struct in_addr),
                            AF_INET)))
            replace (str, "%h", he->h_name);
        else
            replace (str, "%h", (char *) inet_ntoa (name.sin_addr));
    }
    replace (str, "%i", (char *) inet_ntoa (name.sin_addr));

	/* If all shared folders are 'All - no password',
	 * then no need to login for "FTP",
	 * by auto-login as user 'guest'.
	 */
	FILE* fp = NULL;
	fp = fopen("/tmp/all_no_password","r");
	if (fp != NULL)
	{
		fclose(fp);
		command_user("guest");
		all_no_password = 1;
	}
	else
		control_printf (SL_SUCCESS, "220 %s", str);

    /* We might not get any data, so let's set an alarm before the
       first read. -- Jesse <slicer69@hotmail.com> */
    alarm (control_timeout);

    /* Read lines from client and execute appropriate commands */
    while (fgets (str, sizeof (str), stdin))
    {
        alarm (control_timeout);
        str[strlen (str) - 2] = 0;
        bftpd_statuslog (2, 0, "%s", str);
#ifdef DEBUG
        bftpd_log ("Processing command: %s\n", str);
#endif
        parsecmd (str);
    }

    if (remotehostname)
        free (remotehostname);

    //if(con_st != NULL)
    //{
    //    binary_semaphore_wait(con_st->sem_id);
    //    --(con_st->num);
    //    binary_semaphore_post(con_st->sem_id);
exit:
    //    printf("->total con num: %d, by bftpd\n", con_st->num);
    //}

    return 0;
}

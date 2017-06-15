#include <config.h>
#include <stdio.h>
#include <pwd.h>
#ifdef HAVE_SHADOW_H
#include <shadow.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif
#ifdef WANT_PAM
#include <security/pam_appl.h>
#endif
#ifdef HAVE_UTMP_H
# include <utmp.h>
# ifdef HAVE_PATHS_H
#  include <paths.h>
#  ifndef _PATH_WTMP
#   define _PATH_WTMP "/dev/null"
#   warning "<paths.h> doesn't set _PATH_WTMP. You can not use wtmp logging"
#   warning "with bftpd."
#  endif
# else
#  define _PATH_WTMP "/dev/null"
#  warning "<paths.h> was not found. You can not use wtmp logging with bftpd."
# endif
#endif
#include <errno.h>
#include <grp.h>
#include <stdlib.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif

#include "cwd.h"
#include "dirlist.h"
#include "mystring.h"
#include "options.h"
#include "login.h"
#include "logging.h"
#include "bftpdutmp.h"
#include "main.h"

/* Foxconn add start, Ken Chen, 02/07/2017, no plain text in /etc/passwd */
#include <openssl/sha.h>
/* Foxconn add end, Ken Chen, 02/07/2017, no plain text in /etc/passwd */

#ifdef WANT_PAM
char usepam = 0;
pam_handle_t *pamh = NULL;
#endif

#ifdef HAVE_UTMP_H
FILE *wtmp;
#endif

struct passwd userinfo;
char userinfo_set = 0;

/*zzz add for geting first partition, 12/11/2007 */
#define NDEBUG //turn off to set assert and P1MSG
#ifdef NDEBUG
  #define P1MSG(args...)
#else
  #define P1MSG(args...) fprintf(stderr, "%s-%04d: ", __FILE__, __LINE__) ; fprintf(stderr, ## args)
#endif // NDEBUG

/* Foxconn added start by Jenny Zhao, 06/10/2011 @USB log */
#include <acosNvramConfig.h>
#include <errno.h>
extern int g_isLanIp;
extern char client_ip[32];

/*
 ** check whether an IP address is in the LAN subnet
 **/
int isLanSubnet(char *ipAddr)
{
    long netAddr, netMask, netIp;
    netAddr = inet_addr(acosNvramConfig_get("lan_ipaddr"));
    netMask = inet_addr(acosNvramConfig_get("lan_netmask"));
    netIp   = inet_addr(ipAddr);
    if ((netAddr & netMask) != (netIp & netMask))
    {
        return FALSE;
    }
    return TRUE;
}

void write_usb_access_log(void)
{
    if (!g_isLanIp)
    {
        FILE *fp;
        char logBuffer[96];
        if ((fp = fopen("/dev/aglog", "r+")) != NULL)
        {
            sprintf(logBuffer, "[USB remote access] from %s through FTP,", client_ip);
            fwrite(logBuffer, sizeof(char), strlen(logBuffer)+1, fp);
            fclose(fp);
        }
    }
}

void write_usb_fail_log(void)
{
    if (!g_isLanIp)
    {
        FILE *fp;
        char logBuffer[96];
        if ((fp = fopen("/dev/aglog", "r+")) != NULL)
        {
            sprintf(logBuffer, "[USB remote access rejected] from %s through FTP,", client_ip);
            fwrite(logBuffer, sizeof(char), strlen(logBuffer)+1, fp);
            fclose(fp);
        }
    }
}
/*  added end by Jenny Zhao, 06/10/2011 */


static char mount_path[128];
int readOnlyOnPart1 = 0; //@ftpRW

static void scanPartitons()
{
    FILE *fp = 0;
    char line[256];

//===================================
// parse /proc/mounts
// format:
// /dev/sdb1 /mnt/share/usb1/part1 vfat rw,sync 0 0 
//===================================


    fp = fopen("/proc/mounts", "r");
    mount_path[0] = 0;
    readOnlyOnPart1 = 0;
    if (fp)
    {
        while (1)
        {
            memset(line, 0x00, 256);
            if (feof(fp))
            {
                break;
            }
            
            fgets(line, 256, fp);
            if (strncmp(line, "/dev/sd", 7) == 0)
            {
                char *saveptr1;
                char *token = strtok_r(line, " \t\n", &saveptr1);

                if (token)
                {
                    token = strtok_r(NULL, " \t,\n", &saveptr1);

                    P1MSG("save path %s \n", token);
                    strcpy(mount_path, token);

                    /*
                    ** @ftpRW
                    **  update the r/w mode from mount message
                    */
                    token = strtok_r(NULL, " \t,\n", &saveptr1);//fs type
                    token = strtok_r(NULL, " \t,\n", &saveptr1);//rw
                    if(!strstr(token,"w"))
                        readOnlyOnPart1 = 1;

                    
                    break;
                } //if token
                
            }
        } //while 1 -> read lines in files
        fclose(fp);
    }
    else 
    {
        fprintf(stderr, "open /porc/mount failed\n");
    }   
}
/* szzz added end */





char *mygetpwuid (int uid, FILE * file, char *name)
{
    int _uid;
    char foo[256];
    int i;

    if (file)
    {
        rewind (file);
        while (fscanf (file, "%255s%*[^\n]\n", foo) != EOF)
        {
            if ((foo[0] == '#') || (!strchr (foo, ':'))
                || (strchr (foo, ':') > foo + USERLEN - 1))
                continue;
            i = strchr (foo, ':') - foo;
            strncpy (name, foo, i);
            name[i] = 0;
            sscanf (strchr (foo + i + 1, ':') + 1, "%i", &_uid);
            if (_uid == uid)
            {
                if (name[0] == '\n')
                    cutto (name, 1);
                return name;
            }
        }
    }
    sprintf (name, "%i", uid);
    return name;
}

int mygetpwnam (char *name, FILE * file)
{
    char _name[USERLEN + 1];
    char foo[256];
    int uid, i;

    if (file)
    {
        rewind (file);
        while (fscanf (file, "%255s%*[^\n]\n", foo) != EOF)
        {
            if ((foo[0] == '#') || (!strchr (foo, ':'))
                || (strchr (foo, ':') > foo + USERLEN - 1))
                continue;
            i = strchr (foo, ':') - foo;
            strncpy (_name, foo, i);
            _name[i] = 0;
            sscanf (strchr (foo + i + 1, ':') + 1, "%i", &uid);
            if (_name[0] == '\n')
                cutto (_name, 1);
            if (!strcmp (name, _name))
                return uid;
        }
    }
    return -1;
}

#ifdef HAVE_UTMP_H
void wtmp_init ()
{
    if (strcasecmp (config_getoption ("LOG_WTMP"), "no"))
    {
        if (!((wtmp = fopen (_PATH_WTMP, "a"))))
            bftpd_log ("Warning: Unable to open %s.\n", _PATH_WTMP);
    }
}

void bftpd_logwtmp (char type)
{
    struct utmp ut;

    if (!wtmp)
        return;
    memset ((void *) &ut, 0, sizeof (ut));
#ifdef _HAVE_UT_PID
    ut.ut_pid = getpid ();
#endif
    sprintf (ut.ut_line, "ftp%i", (int) getpid ());
    if (type)
    {
#ifdef _HAVE_UT_TYPE
        ut.ut_type = USER_PROCESS;
#endif
        strncpy (ut.ut_name, user, sizeof (ut.ut_name));
#ifdef _HAVE_UT_HOST
        strncpy (ut.ut_host, remotehostname, sizeof (ut.ut_host));
#endif
    }
    else
    {
#ifdef _HAVE_UT_TYPE
        ut.ut_type = DEAD_PROCESS;
#endif
    }
    time (&(ut.ut_time));
    fseek (wtmp, 0, SEEK_END);
    fwrite ((void *) &ut, sizeof (ut), 1, wtmp);
    fflush (wtmp);
}

void wtmp_end ()
{
    if (wtmp)
    {
        if (state >= STATE_AUTHENTICATED)
            bftpd_logwtmp (0);
        fclose (wtmp);
    }
}
#endif

void login_init ()
{
    char *foo = config_getoption ("INITIAL_CHROOT");

#ifdef HAVE_UTMP_H
    wtmp_init ();
#endif
    if (foo[0])
    {                           /* Initial chroot */
        if (chroot (foo) == -1)
        {
            control_printf (SL_FAILURE, "421 Initial chroot failed.\r\n.");
            exit (1);
        }
    }
}

int bftpd_setuid (uid_t uid)
{
    /* If we must open the data connections from port 20,
     * we have to keep the possibility to regain root privileges */
    if (!strcasecmp (config_getoption ("DATAPORT20"), "yes"))
        return seteuid (uid);
    else
        return setuid (uid);
}

int bftpd_login (char *password)
{
    char str[256];
    char *foo;
    int maxusers;
    char *file_auth;            /* if used, points to file used to auth users */
    char *home_directory = NULL;        /* retrieved from auth_file */
    char *anonymous = NULL;
    char chfolder [256]=""; /*  add, Jasmine Yang, 09/12/2007 */
    char tmpBuf[256] = "";/*, water, 11/07/2008*/

    P1MSG("%s(%d)\r\n", __FUNCTION__, __LINE__);
    str[0] = '\0';              /* avoid garbage in str */
    file_auth = config_getoption ("FILE_AUTH");

    if (!file_auth[0])          /* not using auth file */
    {
        // check to see if regular authentication is avail
#ifndef NO_GETPWNAM
        if (!getpwnam (user))
        {
            /*  added start, zacker, 09/13/2010, @chrome_login */
            if (strcasecmp (config_getoption ("ANONYMOUS_USER"), "yes")
                && !strcasecmp (user, "anonymous"))
            {
                control_printf (SL_FAILURE, "530 Sorry, no ANONYMOUS access allowed.");
                return 0; /* STATE_USER, for later command 'QUIT' handling */
            }
            /*  added end, zacker, 09/13/2010, @chrome_login */
            else
            {
                control_printf (SL_FAILURE, "421 Login incorrect.");
                //printf ("%s(%d)\r\n", __FUNCTION__, __LINE__);
                exit (0);
            }
        }
#endif
    }
    /* we are using auth_file */
    else
    {
        home_directory = check_file_password (file_auth, user, password);
        anonymous = config_getoption ("ANONYMOUS_USER");
        if (!home_directory)
        {
            if (!strcasecmp (anonymous, "yes"))
                home_directory = "/";
            else
            {
                control_printf (SL_FAILURE, "421 Authentication incorrect.");
                //printf ("%s(%d)\r\n", __FUNCTION__, __LINE__);
                exit (0);
            }
        }
    }

    if (strncasecmp (foo = config_getoption ("DENY_LOGIN"), "no", 2))
    {
        if (foo[0] != '\0')
        {
            if (strncasecmp (foo, "yes", 3))
            {
                control_printf (SL_FAILURE,
                                "421-Server disabled.\r\n421 Reason: %s",
                                foo);
                //printf ("%s(%d)\r\n", __FUNCTION__, __LINE__);
            }
            else
            {
                control_printf (SL_FAILURE, "421 Login incorrect.");
                //printf ("%s(%d)\r\n", __FUNCTION__, __LINE__);
            }
            bftpd_log ("Login as user '%s' failed: Server disabled.\n", user);
            exit (0);
        }
    }
    maxusers = strtoul (config_getoption ("USERLIMIT_GLOBAL"), NULL, 10);
    if ((maxusers) && (maxusers == bftpdutmp_usercount ("*")))
    {
        control_printf (SL_FAILURE,
                        "421 There are already %i users logged in.",
                        maxusers);
        bftpd_log ("Login as user '%s' failed. Too many users on server.\n",
                   user);
        //printf ("%s(%d)\r\n", __FUNCTION__, __LINE__);
        exit (0);
    }
    maxusers = strtoul (config_getoption ("USERLIMIT_SINGLEUSER"), NULL, 10);
    if ((maxusers) && (maxusers == bftpdutmp_usercount (user)))
    {
        control_printf (SL_FAILURE,
                        "421 User %s is already logged in %i times.", user,
                        maxusers);
        bftpd_log ("Login as user '%s' failed. Already logged in %d times.",
                   maxusers);
        //printf ("%s(%d)\r\n", __FUNCTION__, __LINE__);
        exit (0);
    }

    /* Check to see if we should block mulitple logins from the same machine.
       -- Jesse <slicer69@hotmail.com>
     */
    maxusers = strtoul (config_getoption ("USERLIMIT_HOST"), NULL, 10);
    if ((maxusers) && (maxusers == bftpdutmp_dup_ip_count (remotehostname)))
    {
        control_printf (SL_FAILURE,
                        "421 Too many connections from your IP address.");
        bftpd_log
            ("Login as user '%s' failed. Already %d connections from %s.\n",
             user, maxusers, remotehostname);
        //printf ("%s(%d)\r\n", __FUNCTION__, __LINE__);
        exit (0);
    }

    /* disable these checks when logging in via auth file */
    if (!file_auth[0])
    {
#ifndef NO_GETPWNAM
/*  add start, Jasmine Yang, 09/12/2007 */
        //if (checkuser () || checkshell ()) 
        if (checkuser ())
/*  add end, Jasmine Yang, 09/12/2007 */        	
        {
            control_printf (SL_FAILURE, "421 Login incorrect.");
            //printf ("%s(%d)\r\n", __FUNCTION__, __LINE__);
            exit (0);
        }
#endif
    }

    /* do not do this check when we are using auth_file */
    if (!file_auth[0])
    {
#ifndef NO_GETPWNAM
        if (checkpass (password))
            return 1;
#endif
    }

    if (strcasecmp ((char *) config_getoption ("RATIO"), "none"))
    {
        sscanf ((char *) config_getoption ("RATIO"), "%i/%i", &ratio_send,
                &ratio_recv);
    }

    /* do these checks if logging in via normal methods */
    if (!file_auth[0])
    {
        strcpy (str, config_getoption ("ROOTDIR"));
        if (!str[0])
            strcpy (str, "%h");
        P1MSG("userinfo.pw_name=%s,userinfo.pw_dir=%s\r\n",  userinfo.pw_name, userinfo.pw_dir);
        replace (str, "%u", userinfo.pw_name);
        strcpy(tmpBuf, userinfo.pw_dir);/*, water, 11/07/2008*/
        replace (str, "%h", userinfo.pw_dir);
        if (!strcasecmp (config_getoption ("RESOLVE_UIDS"), "yes"))
        {
            passwdfile = fopen ("/etc/passwd", "r");
            groupfile = fopen ("/etc/group", "r");
        }

        setgid (userinfo.pw_gid);
        initgroups (userinfo.pw_name, userinfo.pw_gid);
        /*  add start, Jasmine Yang, 09/12/2007 */
        scanPartitons(); //zzz, get first partition, 12/11/2007
        /* Make sure the login folder is user's account name */

        /*  Add Start : Steve Hsieh : 01/22/2008, @ftpRW {*/
        /* -- should use shared dir in usb_setting page as root dir --*/
        
        /* modified start, water, 11/07/2008, 
          I think it isn't a good solution, need further implement later*/
        //sprintf (chfolder, "%s/%s", mount_path, userinfo.pw_name);
        sprintf (chfolder, "%s%s", mount_path, tmpBuf);
        /* modified end, water, 11/07/2008, it isn't a good solution, need further implement*/
        
        /*  Add End : Steve Hsieh : 01/22/2008, @ftpRW }*/
        
        //water add temporarily, no usb now, it will be removed soon, @debug 05/30/2008
        //sprintf (chfolder, "/tmp");
        
        
        /*
        **  @ftpRW
        **  if readonly fs, just use the "/" as root dir
        */
        //if (access (chfolder, 7) != 0)
        if(readOnlyOnPart1)
        {
            /* zzz add. 12/11/2007 */
            //set ftp root to "/" if the mounted partition is read only
            bftpd_log("partition 1 is readonly\n");    
            sprintf (chfolder, "%s", mount_path); 

            printf("checking %s\n", mount_path);

            if (access(chfolder, R_OK|X_OK) == 0 )
            {
                sprintf (chfolder, "%s", mount_path); 
            }
            else 
            {
                //zzz: Is this correct? Think twice??
            sprintf(chfolder, "/tmp/samba/share/%s", userinfo.pw_name);
            }
            /* zzz added. 12/11/2007 */
        }
        else    //RW file system
        {
            /*
            **  mkdir to the target shared forder if the dir is not exist
            */
            if (access (chfolder, R_OK) != 0)
            {
                char cmdx[512]="";
                sprintf(cmdx,"mkdir -p %s",chfolder) ;
                bftpd_log("create dir [%s]\n",chfolder);
                system(cmdx);
            }
        }
        
        /*  modified start pling 05/14/2009 */
        /* Change rootdir to "/tmp" */
        //strcpy(str,chfolder );
        strcpy(str, "/tmp" );
        /*  modified end pling 05/14/2009 */
	/*  added start by Jenny Zhao, 06/10/2011 @USB log */
        /* In fact, we want to write USB remote access log after "230
	 * User logged in.". But the log file /dev/aglog can't be opened
	 * after chroot to "/tmp",we write log at here before do chroot
	 * function */
        write_usb_access_log();
        /*  added end by Jenny Zhao, 06/10/2011 */

        /*  add end, Jasmine Yang, 09/12/2007 */
        if (strcasecmp (config_getoption ("DO_CHROOT"), "no"))
        {
            bftpd_log("change for ROOTDIR [%s]\n",chfolder);
            if (chroot (str))
            {
                control_printf (SL_FAILURE,
                                "421 Unable to change root directory.\r\n%s.",
                                strerror (errno));
                //printf ("%s(%d)\r\n", __FUNCTION__, __LINE__);
                exit (0);
            }
            if (bftpd_setuid (userinfo.pw_uid))
            {
                control_printf (SL_FAILURE, "421 Unable to change uid.");
                //printf ("%s(%d)\r\n", __FUNCTION__, __LINE__);
                exit (0);
            }
            if (chdir ("/"))
            {
                control_printf (SL_FAILURE,
                                "421 Unable to change working directory.\r\n%s.",
                                strerror (errno));
                //printf ("%s(%d)\r\n", __FUNCTION__, __LINE__);
                exit (0);
            }
        }
        else
        {
            if (bftpd_setuid (userinfo.pw_uid))
            {
                control_printf (SL_FAILURE, "421 Unable to change uid.");
                //printf ("%s(%d)\r\n", __FUNCTION__, __LINE__);
                exit (0);
            }
            if (chdir (str))
            {
                control_printf (SL_FAILURE,
                                "230 Couldn't change cwd to '%s': %s.", str,
                                strerror (errno));
                //printf ("%s(%d)\r\n", __FUNCTION__, __LINE__);
                chdir ("/");
            }
        }

    }                           /* end of if we are using regular authentication methods */

    else                        /* we are using file authentication */
    {
        /* get home directory */
        strcpy (str, config_getoption ("ROOTDIR"));
        if (!str[0])
            strcpy (str, "%h");
        replace (str, "%h", home_directory);
        replace (str, "%u", user);

        /* see if we should change root */
        if (!strcasecmp (config_getoption ("DO_CHROOT"), "yes"))
        {
            if (chroot (home_directory))
            {
                control_printf (SL_FAILURE,
                                "421 Unable to change root directory.\r\n");
                //printf ("%s(%d)\r\n", __FUNCTION__, __LINE__);
                exit (0);
            }
            if (chdir ("/"))
            {
                control_printf (SL_FAILURE,
                                "421 Unable to change working directory.\r\n");
                //printf ("%s(%d)\r\n", __FUNCTION__, __LINE__);
                exit (0);
            }
        }

    }                           /* end of using file auth */

    new_umask ();
    print_file (230, config_getoption ("MOTD_USER"));
    control_printf (SL_SUCCESS, "230 User logged in.");
#ifdef HAVE_UTMP_H
    bftpd_logwtmp (1);
#endif
    bftpdutmp_log (1);
    bftpd_log ("Successfully logged in as user '%s'.\n", user);
    if (config_getoption ("AUTO_CHDIR")[0])
        chdir (config_getoption ("AUTO_CHDIR"));

    state = STATE_AUTHENTICATED;
    bftpd_cwd_init ();

    /* a little clean up before we go */
    if ((home_directory) && (strcmp (home_directory, "/")))
        free (home_directory);
    return 0;
}


/* Return 1 on failure and 0 on success. */
int checkpass (char *password)
{
#ifndef NO_GETPWNAM
    if (!getpwnam (user))
        return 1;
#endif

    if (!strcasecmp (config_getoption ("ANONYMOUS_USER"), "yes"))
        return 0;

#ifdef WANT_PAM
    if (!strcasecmp (config_getoption ("AUTH"), "pam"))
        return checkpass_pam (password);
    else
#endif
        return checkpass_pwd (password);
}



void login_end ()
{
#ifdef WANT_PAM
    if (usepam)
        return end_pam ();
#endif
#ifdef HAVE_UTMP_H
    wtmp_end ();
#endif
}

int checkpass_pwd (char *password)
{
#ifdef HAVE_SHADOW_H
    struct spwd *shd;
#endif
/*  add start, Jasmine Yang, 09/12/2007 */
    //if (strcmp(userinfo.pw_passwd, (char *) crypt(password, userinfo.pw_passwd))) {
    
    P1MSG("%s(%d)userinfo.pw_passwd=%s , password=%s \r\n", __FUNCTION__,
            __LINE__, userinfo.pw_passwd, password);

/* Foxconn add start, Ken Chen, 02/07/2017, no plain text in /etc/passwd */
    int i;
    char * pData = NULL;
#ifdef SHA256_DIGEST_LENGTH
    //using SHA256
    char password_hash[SHA256_DIGEST_LENGTH] = "";   
    char password_hash_str[2*SHA256_DIGEST_LENGTH+1] = "";
    pData = password_hash_str;
    SHA256(password, strlen(password), password_hash);
    for (i=0; i<SHA256_DIGEST_LENGTH; i++) {
        sprintf(pData,"%02x",(unsigned char)password_hash[i]);
        pData += 2;
    }
#else
    //using SHA1
    char password_hash[SHA_DIGEST_LENGTH] = "";  
    char password_hash_str[2*SHA_DIGEST_LENGTH+1] = "";
    pData = password_hash_str; 
    SHA1(password, strlen(password), password_hash);
    for (i=0; i<SHA_DIGEST_LENGTH; i++) {
        sprintf(pData,"%02x",(unsigned char)password_hash[i]);
        pData += 2;
    }
#endif
    //if (strcmp (userinfo.pw_passwd, password))
    if (strcmp (userinfo.pw_passwd, password_hash_str))
    {
/*  add end, Jasmine Yang, 09/12/2007 */
#ifdef HAVE_SHADOW_H
        if (!(shd = getspnam (user)))
            return 1;
        if (strcmp (shd->sp_pwdp, (char *) crypt (password, shd->sp_pwdp)))
#endif
            return 1;
    }
    return 0;
}

#ifdef WANT_PAM
int conv_func (int num_msg, const struct pam_message **msgm,
               struct pam_response **resp, void *appdata_ptr)
{
    struct pam_response *response;
    int i;
    response =
        (struct pam_response *) malloc (sizeof (struct pam_response) *
                                        num_msg);
    for (i = 0; i < num_msg; i++)
    {
        response[i].resp = (char *) strdup (appdata_ptr);
        response[i].resp_retcode = 0;
    }
    *resp = response;
    return 0;
}

int checkpass_pam (char *password)
{
    struct pam_conv conv = { conv_func, password };
    int retval = pam_start ("bftpd", user, (struct pam_conv *) &conv,
                            (pam_handle_t **) & pamh);

    if (retval != PAM_SUCCESS)
    {
        printf ("Error while initializing PAM: %s\n",
                pam_strerror (pamh, retval));
        return 1;
    }
    pam_fail_delay (pamh, 0);
    retval = pam_authenticate (pamh, 0);
    if (retval == PAM_SUCCESS)
        retval = pam_acct_mgmt (pamh, 0);
    if (retval == PAM_SUCCESS)
        pam_open_session (pamh, 0);
    if (retval != PAM_SUCCESS)
        return 1;
    else
        return 0;
}

void end_pam ()
{
    if (pamh)
    {
        pam_close_session (pamh, 0);
        pam_end (pamh, 0);
    }
}
#endif

int checkuser ()
{

    FILE *fd;
    char *p;
    char line[256];

    if ((fd = fopen (config_getoption ("PATH_FTPUSERS"), "r")))
    {
        while (fgets (line, sizeof (line), fd))
            if ((p = strchr (line, '\n')))
            {
                *p = '\0';
                if (line[0] == '#')
                    continue;
                if (!strcasecmp (line, user))
                {
                    fclose (fd);
                    return 1;
                }
            }
        fclose (fd);
    }
    return 0;
}

int checkshell ()
{
#ifdef HAVE_GETUSERSHELL
    char *cp;
    struct passwd *pwd;

    if (!strcasecmp (config_getoption ("AUTH_ETCSHELLS"), "no"))
        return 0;

    pwd = getpwnam (user);
    while ((cp = getusershell ()))
        if (!strcmp (cp, pwd->pw_shell))
            break;
    endusershell ();

    if (!cp)
        return 1;
    else
        return 0;
#else
    return 0;
#   warning "Your system doesn't have getusershell(). You can not"
#   warning "use /etc/shells authentication with bftpd."
#endif
}




/*
This function searches through a text file for a matching
username. If a match is found, the password in the
text file is compared to the password passed in to
the function. If the password matches, the function
returns the fourth field (home directory). On failure,
it returns NULL.
-- Jesse
*/
char *check_file_password (char *my_filename, char *my_username,
                           char *my_password)
{
    FILE *my_file;
    int found_user = 0;
    char user[32], password[32], group[32], home_dir[32];
    char *my_home_dir = NULL;
    int return_value;

    my_file = fopen (my_filename, "r");
    if (!my_file)
        return NULL;

    return_value =
        fscanf (my_file, "%s %s %s %s", user, password, group, home_dir);
    if (!strcmp (user, my_username))
        found_user = 1;

    while ((!found_user) && (return_value != EOF))
    {
        return_value =
            fscanf (my_file, "%s %s %s %s", user, password, group, home_dir);
        if (!strcmp (user, my_username))
            found_user = 1;
    }

    fclose (my_file);
    if (found_user)
    {
        /* check password */
        if (!strcmp (password, "*"))
        {
        }
        else if (strcmp (password, my_password))
            return NULL;

        my_home_dir = calloc (strlen (home_dir), sizeof (char));
        if (!my_home_dir)
            return NULL;
        strcpy (my_home_dir, home_dir);
    }

    return my_home_dir;
}

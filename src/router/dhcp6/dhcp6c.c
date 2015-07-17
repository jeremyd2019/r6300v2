/*	$Id: dhcp6c.c,v 1.1.1.1 2006/12/04 00:45:23 Exp $	*/
/*	ported from KAME: dhcp6c.c,v 1.97 2002/09/24 14:20:49 itojun Exp */

/*
 * Copyright (C) 1998 and 1999 WIDE Project.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <sys/timeb.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#include <net/if.h>
#include <linux/sockios.h>
#if defined(__FreeBSD__) && __FreeBSD__ >= 3
#include <net/if_var.h>
#endif

#include <arpa/inet.h>
#include <netdb.h>

#include <signal.h>
#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <err.h>
#include <ifaddrs.h>

#include "queue.h"
#include "dhcp6.h"
#include "config.h"
#include "common.h"
#include "timer.h"
#include "lease.h"

static int debug = 0;
static u_long sig_flags = 0;
#define SIGF_TERM 0x1
#define SIGF_HUP 0x2
#define SIGF_CLEAN 0x4

#define DHCP6S_VALID_REPLY(a) \
	(a == DHCP6S_REQUEST || a == DHCP6S_RENEW || \
	 a == DHCP6S_REBIND || a == DHCP6S_DECLINE || \
	 a == DHCP6S_RELEASE || a == DHCP6S_CONFIRM || \
	 a == DHCP6S_INFOREQ)


const dhcp6_mode_t dhcp6_mode = DHCP6_MODE_CLIENT;

static char *device = NULL;
static int num_device = 0;
static struct iaid_table iaidtab[100];
static u_int8_t client6_request_flag = 0;
static	char leasename[100];

#define CLIENT6_RELEASE_ADDR	0x1
#define CLIENT6_CONFIRM_ADDR	0x2
#define CLIENT6_REQUEST_ADDR	0x4
#define CLIENT6_DECLINE_ADDR	0x8

#define CLIENT6_INFO_REQ	0x10

int insock;	/* inbound udp port */
//int outsock;	/* outbound udp port */     //  removed pling 08/15/2009
int nlsock;	

extern char *raproc_file;
extern char *ifproc_file;
extern struct ifproc_info *dadlist;
extern FILE *client6_lease_file;
extern struct dhcp6_iaidaddr client6_iaidaddr;
FILE *dhcp6_resolv_file;
static const struct sockaddr_in6 *sa6_allagent;
static struct duid client_duid;

static void usage __P((void));
static void client6_init __P((char *));
static void client6_ifinit __P((char *));
void free_servers __P((struct dhcp6_if *));
static void free_resources __P((struct dhcp6_if *));
static int create_request_list __P((int));
static void client6_mainloop __P((void));
static void process_signals __P((void));
static struct dhcp6_serverinfo *find_server __P((struct dhcp6_if *,
						 struct duid *));
static struct dhcp6_serverinfo *allocate_newserver __P((struct dhcp6_if *, struct dhcp6_optinfo *));
static struct dhcp6_serverinfo *select_server __P((struct dhcp6_if *));
void client6_send __P((struct dhcp6_event *));
int client6_send_newstate __P((struct dhcp6_if *, int));
static void client6_recv __P((void));
static int client6_recvadvert __P((struct dhcp6_if *, struct dhcp6 *,
				   ssize_t, struct dhcp6_optinfo *));
static int client6_recvreply __P((struct dhcp6_if *, struct dhcp6 *,
				  ssize_t, struct dhcp6_optinfo *));
static void client6_signal __P((int));
static struct dhcp6_event *find_event_withid __P((struct dhcp6_if *,
						  u_int32_t));
static struct dhcp6_timer *check_lease_file_timo __P((void *));
static struct dhcp6_timer *check_link_timo __P((void *));
static struct dhcp6_timer *check_dad_timo __P((void *));
static void setup_check_timer __P((struct dhcp6_if *));
static void setup_interface __P((char *));
struct dhcp6_timer *client6_timo __P((void *));
extern int client6_ifaddrconf __P((ifaddrconf_cmd_t, struct dhcp6_addr *));
extern struct dhcp6_timer *syncfile_timo __P((void *));
extern int radvd_parse (struct dhcp6_iaidaddr *, int);

#define DHCP6C_CONF "/tmp/dhcp6c.conf"
#define DHCP6C_USER_CONF "/tmp/dhcp6c_user.conf"    //  added pling 09/27/2010
#define DHCP6C_PIDFILE "/var/run/dhcpv6/dhcp6c.pid"
#define DUID_FILE "/tmp/dhcp6c_duid"

static int pid;
char client6_lease_temp[256];
struct dhcp6_list request_list;
struct dhcp6_list request_prefix_list;  //  added pling 09/24/2009

/*  added start pling 10/07/2010 */
/* For testing purpose */
u_int32_t xid_solicit = 0;
u_int32_t xid_request = 0;
/*  added end pling 10/07/2010 */

/*  added start pling 09/07/2010 */
/* User secondary conf file to store info such as user-class */
int parse_user_file(char *user_file)
{
    FILE *fp = NULL;
    char line[1024];
    char user_class_key[] = "user_class";
    char *newline1;
    char *newline2;

    /* Initial client user class to empty string */
    set_dhcpc_user_class("");

    fp = fopen(user_file, "r");
    if (fp != NULL)
    {
        while (!feof(fp))
        {
            fgets(line, sizeof(line), fp);

            /* Remove trailing newline characters, if any */
            newline1 = strstr(line, "\r");
            newline2 = strstr(line, "\n");
            if (newline1)
                *newline1 = '\0';
            if (newline2)
                *newline2 = '\0';

            /* Extract the key */
            if (strncmp(line, user_class_key, strlen(user_class_key)) == 0)
                set_dhcpc_user_class(&line[strlen(user_class_key)+1]);
        }
        fclose(fp);
    }
    return 0;
}
/*  added end pling 09/07/2010 */

/*  added start pling 09/23/2009 */
/* Return the interface where dhcp is run on */
char* get_dhcpc_dev_name(void)
{
    return device;
}
/*  added end pling 09/23/2009 */

int
main(argc, argv)
	int argc;
	char **argv;
{
	int ch;
	char *progname, *conffile = DHCP6C_CONF;
    char *user_file = DHCP6C_USER_CONF;
	FILE *pidfp;
	char *addr;

	pid = getpid();
	srandom(time(NULL) & pid);

	if ((progname = strrchr(*argv, '/')) == NULL)
		progname = *argv;
	else
		progname++;

	TAILQ_INIT(&request_list);
	TAILQ_INIT(&request_prefix_list);	//  added pling 09/23/2009
 
    /*  modified start pling 09/17/2010 */
    /* Since 'user class' string may contain any printable char.
     * The current dhcp6s config file parsing (using yyparse)
     * can't handle this properly.
     * So we put user-class string in a separate file,
     *  specified using '-u' option.
     */
	//while ((ch = getopt(argc, argv, "c:r:R:P:dDfI")) != -1) {
	while ((ch = getopt(argc, argv, "c:u:r:R:P:dDfI")) != -1) {
    /*  modified end pling 09/17/2010 */
		switch (ch) {
		case 'c':
			conffile = optarg;
			break;
        /*  added start pling 09/17/2010 */
        /* Another config file to store info, such as user-class, etc */
        case 'u':
            user_file = optarg;
            break;
        /*  added end pling 09/17/2010 */
		case 'P':
			client6_request_flag |= CLIENT6_REQUEST_ADDR;
			for (addr = strtok(optarg, " "); addr; addr = strtok(NULL, " ")) {
				struct dhcp6_listval *lv;
				if ((lv = (struct dhcp6_listval *)malloc(sizeof(*lv)))
				    == NULL) {
					dprintf(LOG_ERR, "failed to allocate memory");
					exit(1);
				}
				memset(lv, 0, sizeof(*lv));
				if (inet_pton(AF_INET6, strtok(addr, "/"), 
				    &lv->val_dhcp6addr.addr) < 1) {
					dprintf(LOG_ERR, 
						"invalid ipv6address for release");
					usage();
					exit(1);
				}
				lv->val_dhcp6addr.type = IAPD;
				lv->val_dhcp6addr.plen = atoi(strtok(NULL, "/"));
				lv->val_dhcp6addr.status_code = DH6OPT_STCODE_UNDEFINE;
				TAILQ_INSERT_TAIL(&request_list, lv, link);
			} 
			break;

		case 'R':
			client6_request_flag |= CLIENT6_REQUEST_ADDR;
			for (addr = strtok(optarg, " "); addr; addr = strtok(NULL, " ")) {
				struct dhcp6_listval *lv;
				if ((lv = (struct dhcp6_listval *)malloc(sizeof(*lv)))
				    == NULL) {
					dprintf(LOG_ERR, "failed to allocate memory");
					exit(1);
				}
				memset(lv, 0, sizeof(*lv));
				if (inet_pton(AF_INET6, addr, &lv->val_dhcp6addr.addr) < 1) {
					dprintf(LOG_ERR, 
						"invalid ipv6address for release");
					usage();
					exit(1);
				}
				lv->val_dhcp6addr.type = IANA;
				lv->val_dhcp6addr.status_code = DH6OPT_STCODE_UNDEFINE;
				TAILQ_INSERT_TAIL(&request_list, lv, link);
			} 
			break;
		case 'r':
			client6_request_flag |= CLIENT6_RELEASE_ADDR;
			if (strcmp(optarg, "all")) {
				for (addr = strtok(optarg, " "); addr; 
				     addr = strtok(NULL, " ")) {
					struct dhcp6_listval *lv;
					if ((lv = (struct dhcp6_listval *)malloc(sizeof(*lv)))
					    == NULL) {
						dprintf(LOG_ERR, "failed to allocate memory");
						exit(1);
					}
					memset(lv, 0, sizeof(*lv));
					if (inet_pton(AF_INET6, addr, 
					    &lv->val_dhcp6addr.addr) < 1) {
						dprintf(LOG_ERR, 
							"invalid ipv6address for release");
						usage();
						exit(1);
					}
					lv->val_dhcp6addr.type = IANA;
					TAILQ_INSERT_TAIL(&request_list, lv, link);
				}
			} 
			break;
		case 'I':
			client6_request_flag |= CLIENT6_INFO_REQ;
			break;
		case 'd':
			debug = 1;
			break;
		case 'D':
			debug = 2;
			break;
		case 'f':
			foreground++;
			break;
		default:
			usage();
			exit(0);
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 1) {
		usage();
		exit(0);
	}
	device = argv[0];

	if (foreground == 0) {
		if (daemon(0, 0) < 0)
			err(1, "daemon");
		openlog(progname, LOG_NDELAY|LOG_PID, LOG_DAEMON);
	}
	setloglevel(debug);

	/* dump current PID */
	if ((pidfp = fopen(DHCP6C_PIDFILE, "w")) != NULL) {
		fprintf(pidfp, "%d\n", pid);
		fclose(pidfp);
	}
 
	ifinit(device);

    /*  added start pling 09/17/2010 */
    /* Parse the second conf file, before reading original config file */
    parse_user_file(user_file);
    /*  added end pling 09/17/2010 */

	if ((cfparse(conffile)) != 0) {
		dprintf(LOG_ERR, "%s" "failed to parse configuration file",
			FNAME);
		exit(1);
	}

    /*  added start pling 01/25/2010 */
    /* Clear SIP and NTP servers params upon restart */
    system("nvram set ipv6_sip_servers=\"\"");
    system("nvram set ipv6_ntp_servers=\"\"");
    /*  added end pling 01/25/2010 */

	client6_init(device);
	client6_ifinit(device);
	client6_mainloop();
	exit(0);
}

static void
usage()
{

	fprintf(stderr, 
	"usage: dhcpc [-c configfile] [-r all or (ipv6address ipv6address...)]\n"
	"       [-R (ipv6 address ipv6address...) [-dDIf] interface\n");
}

/*------------------------------------------------------------*/

void
client6_init(device)
	char *device;
{
	struct addrinfo hints, *res;
	static struct sockaddr_in6 sa6_allagent_storage;
	int error, on = 1;
	struct dhcp6_if *ifp;
	int ifidx;
	char linklocal[64];
	struct in6_addr lladdr;
	
	ifidx = if_nametoindex(device);
	if (ifidx == 0) {
		dprintf(LOG_ERR, "if_nametoindex(%s)", device);
		exit(1);
	}

	/* get our DUID */
	if (get_duid(DUID_FILE, device, &client_duid)) {
		dprintf(LOG_ERR, "%s" "failed to get a DUID", FNAME);
		exit(1);
	}
	if (get_linklocal(device, &lladdr) < 0) {
		exit(1);
	}
	if (inet_ntop(AF_INET6, &lladdr, linklocal, sizeof(linklocal)) < 0) {
		exit(1);
	}
	dprintf(LOG_DEBUG, "link local addr is %s", linklocal);
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_INET6;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_flags = 0;
	error = getaddrinfo(linklocal, DH6PORT_DOWNSTREAM, &hints, &res);
	if (error) {
		dprintf(LOG_ERR, "%s" "getaddrinfo: %s",
			FNAME, strerror(error));
		exit(1);
	}
	insock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (insock < 0) {
		dprintf(LOG_ERR, "%s" "socket(inbound)", FNAME);
		exit(1);
	}
#ifdef IPV6_RECVPKTINFO
	if (setsockopt(insock, IPPROTO_IPV6, IPV6_RECVPKTINFO, &on,
		       sizeof(on)) < 0) {
		dprintf(LOG_ERR, "%s"
			"setsockopt(inbound, IPV6_RECVPKTINFO): %s",
			FNAME, strerror(errno));
		exit(1);
	}
#else
	if (setsockopt(insock, IPPROTO_IPV6, IPV6_PKTINFO, &on,
		       sizeof(on)) < 0) {
		dprintf(LOG_ERR, "%s"
			"setsockopt(inbound, IPV6_PKTINFO): %s",
			FNAME, strerror(errno));
		exit(1);
	}
#endif
	((struct sockaddr_in6 *)(res->ai_addr))->sin6_scope_id = ifidx;
	dprintf(LOG_DEBUG, "res addr is %s/%d", addr2str(res->ai_addr), res->ai_addrlen);
	if (bind(insock, res->ai_addr, res->ai_addrlen) < 0) {
		dprintf(LOG_ERR, "%s" "bind(inbound): %s",
			FNAME, strerror(errno));
		exit(1);
	}
	freeaddrinfo(res);

	hints.ai_flags = 0;
	error = getaddrinfo(linklocal, DH6PORT_UPSTREAM, &hints, &res);
	if (error) {
		dprintf(LOG_ERR, "%s" "getaddrinfo: %s",
			FNAME, gai_strerror(error));
		exit(1);
	}
    /*  removed start pling 08/15/2009 */
#if 0
	outsock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (outsock < 0) {
		dprintf(LOG_ERR, "%s" "socket(outbound): %s",
			FNAME, strerror(errno));
		exit(1);
	}
	if (setsockopt(outsock, IPPROTO_IPV6, IPV6_MULTICAST_IF,
			&ifidx, sizeof(ifidx)) < 0) {
		dprintf(LOG_ERR, "%s"
			"setsockopt(outbound, IPV6_MULTICAST_IF): %s",
			FNAME, strerror(errno));
		exit(1);
	}
	((struct sockaddr_in6 *)(res->ai_addr))->sin6_scope_id = ifidx;
	if (bind(outsock, res->ai_addr, res->ai_addrlen) < 0) {
		dprintf(LOG_ERR, "%s" "bind(outbound): %s",
			FNAME, strerror(errno));
		exit(1);
	}
#endif
    /*  removed end pling 08/15/2009 */
	freeaddrinfo(res);
	/* open a socket to watch the off-on link for confirm messages */
	if ((nlsock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		dprintf(LOG_ERR, "%s" "open a socket: %s",
			FNAME, strerror(errno));
		exit(1);
	}
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_INET6;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	error = getaddrinfo(DH6ADDR_ALLAGENT, DH6PORT_UPSTREAM, &hints, &res);
	if (error) {
		dprintf(LOG_ERR, "%s" "getaddrinfo: %s",
			FNAME, gai_strerror(error));
		exit(1);
	}
	memcpy(&sa6_allagent_storage, res->ai_addr, res->ai_addrlen);
	sa6_allagent = (const struct sockaddr_in6 *)&sa6_allagent_storage;
	freeaddrinfo(res);

	/* client interface configuration */
	if ((ifp = find_ifconfbyname(device)) == NULL) {
		dprintf(LOG_ERR, "%s" "interface %s not configured",
			FNAME, device);
		exit(1);
	}
	//ifp->outsock = outsock;       //  removed pling 08/15/2009 

	if (signal(SIGHUP, client6_signal) == SIG_ERR) {
		dprintf(LOG_WARNING, "%s" "failed to set signal: %s",
			FNAME, strerror(errno));
		exit(1);
	}
	if (signal(SIGTERM|SIGKILL, client6_signal) == SIG_ERR) {
		dprintf(LOG_WARNING, "%s" "failed to set signal: %s",
			FNAME, strerror(errno));
		exit(1);
	}
	if (signal(SIGINT, client6_signal) == SIG_ERR) {
		dprintf(LOG_WARNING, "%s" "failed to set signal: %s",
			FNAME, strerror(errno));
		exit(1);
	}
}

static void
client6_ifinit(char *device)
{
	struct dhcp6_if *ifp = dhcp6_if;
	struct dhcp6_event *ev;
	char iaidstr[20];

	dhcp6_init_iaidaddr();
	/* get iaid for each interface */
	if (num_device == 0) {
		if ((num_device = create_iaid(&iaidtab[0], num_device)) < 0)
			exit(1);
        /*  modified start pling 08/20/2009 */
        /* Per Netgear spec, use 1 as the IAID for IA_NA */
		//ifp->iaidinfo.iaid = get_iaid(ifp->ifname, &iaidtab[0], num_device);
		ifp->iaidinfo.iaid = 1; //get_iaid(ifp->ifname, &iaidtab[0], num_device);
        /*  modified end pling 08/20/2009 */
		if (ifp->iaidinfo.iaid == 0) {
			dprintf(LOG_DEBUG, "%s" 
				"interface %s iaid failed to be created", 
				FNAME, ifp->ifname);
			exit(1);
		}
		dprintf(LOG_DEBUG, "%s" "interface %s iaid is %u", 
			FNAME, ifp->ifname, ifp->iaidinfo.iaid);
	}
	client6_iaidaddr.ifp = ifp;
	memcpy(&client6_iaidaddr.client6_info.iaidinfo, &ifp->iaidinfo, 
			sizeof(client6_iaidaddr.client6_info.iaidinfo));
	duidcpy(&client6_iaidaddr.client6_info.clientid, &client_duid);
	/* parse the lease file */
	strcpy(leasename, PATH_CLIENT6_LEASE);
	sprintf(iaidstr, "%u", ifp->iaidinfo.iaid);
	strcat(leasename, iaidstr);
	if ((client6_lease_file = 
		init_leases(leasename)) == NULL) {
			dprintf(LOG_ERR, "%s" "failed to parse lease file", FNAME);
		exit(1);
	}
	strcpy(client6_lease_temp, leasename);
	strcat(client6_lease_temp, "XXXXXX");
	client6_lease_file = 
		sync_leases(client6_lease_file, leasename, client6_lease_temp);
	if (client6_lease_file == NULL)
		exit(1);
	if (!TAILQ_EMPTY(&client6_iaidaddr.lease_list)) {
		struct dhcp6_listval *lv;
		if (!(client6_request_flag & CLIENT6_REQUEST_ADDR) && 
				!(client6_request_flag & CLIENT6_RELEASE_ADDR))
			client6_request_flag |= CLIENT6_CONFIRM_ADDR;
		if (TAILQ_EMPTY(&request_list)) {
			if (create_request_list(1) < 0) 
				exit(1);
		} else if (client6_request_flag & CLIENT6_RELEASE_ADDR) {
			for (lv = TAILQ_FIRST(&request_list); lv; 
					lv = TAILQ_NEXT(lv, link)) {
				if (dhcp6_find_lease(&client6_iaidaddr, 
						&lv->val_dhcp6addr) == NULL) {
					dprintf(LOG_INFO, "this address %s is not"
						" leased by this client", 
					    in6addr2str(&lv->val_dhcp6addr.addr,0));
					exit(0);
				}
			}
		}	
	} else if (client6_request_flag & CLIENT6_RELEASE_ADDR) {
		dprintf(LOG_INFO, "no ipv6 addresses are leased by client");
		exit(0);
	}
	setup_interface(ifp->ifname);
	ifp->link_flag |= IFF_RUNNING;

	/* get addrconf prefix from kernel */
	(void)get_if_rainfo(ifp);

	/* set up check link timer and sync file timer */	
	if ((ifp->link_timer =
	    dhcp6_add_timer(check_link_timo, ifp)) < 0) {
		dprintf(LOG_ERR, "%s" "failed to create a timer", FNAME);
		exit(1);
	}
	if ((ifp->sync_timer = dhcp6_add_timer(check_lease_file_timo, ifp)) < 0) {
		dprintf(LOG_ERR, "%s" "failed to create a timer", FNAME);
		exit(1);
	}
	/* DAD timer set up after getting the address */
	ifp->dad_timer = NULL;
	/* create an event for the initial delay */
	if ((ev = dhcp6_create_event(ifp, DHCP6S_INIT)) == NULL) {
		dprintf(LOG_ERR, "%s" "failed to create an event",
			FNAME);
		exit(1);
	}
	ifp->servers = NULL;
	ev->ifp->current_server = NULL;
	TAILQ_INSERT_TAIL(&ifp->event_list, ev, link);
	if ((ev->timer = dhcp6_add_timer(client6_timo, ev)) == NULL) {
		dprintf(LOG_ERR, "%s" "failed to add a timer for %s",
			FNAME, ifp->ifname);
		exit(1);
	}
	dhcp6_reset_timer(ev);
}

static void
free_resources(struct dhcp6_if *ifp)
{
	struct dhcp6_event *ev, *ev_next;
	struct dhcp6_lease *sp, *sp_next;
	struct stat buf;
	if (client6_iaidaddr.client6_info.type == IAPD && 
	    !TAILQ_EMPTY(&client6_iaidaddr.lease_list))
		radvd_parse(&client6_iaidaddr, ADDR_REMOVE);
	else {
		for (sp = TAILQ_FIRST(&client6_iaidaddr.lease_list); sp; sp = sp_next) { 
			sp_next = TAILQ_NEXT(sp, link);
			if (client6_ifaddrconf(IFADDRCONF_REMOVE, &sp->lease_addr) != 0) 
				dprintf(LOG_INFO, "%s" "deconfiging address %s failed",
					FNAME, in6addr2str(&sp->lease_addr.addr, 0));
		}
	}
	dprintf(LOG_DEBUG, "%s" " remove all events on interface", FNAME);
	/* cancel all outstanding events for each interface */
	for (ev = TAILQ_FIRST(&ifp->event_list); ev; ev = ev_next) {
		ev_next = TAILQ_NEXT(ev, link);
		dhcp6_remove_event(ev);
	}
	{
		/* restore /etc/radv.conf.bak back to /etc/radvd.conf */
		if (!lstat(RADVD_CONF_BAK_FILE, &buf))
			rename(RADVD_CONF_BAK_FILE, RADVD_CONF_FILE);
		/* restore /etc/resolv.conf.dhcpv6.bak back to /etc/resolv.conf */
		if (!lstat(RESOLV_CONF_BAK_FILE, &buf)) {
			if (rename(RESOLV_CONF_BAK_FILE, RESOLV_CONF_FILE)) 
				dprintf(LOG_ERR, "%s" " failed to backup resolv.conf", FNAME);
		}
	}
	free_servers(ifp);
}

static void
process_signals()
{
	if ((sig_flags & SIGF_TERM)) {
		dprintf(LOG_INFO, FNAME "exiting");

        /*  added start pling 09/10/2010 */
        /* Release IANA/IAPD before exiting */
        create_request_list(0);
        client6_send_newstate(dhcp6_if, DHCP6S_RELEASE);
        /*  added end pling 09/10/2010 */

		free_resources(dhcp6_if);
		unlink(DHCP6C_PIDFILE);
		exit(0);
	}
	if ((sig_flags & SIGF_HUP)) {
		dprintf(LOG_INFO, FNAME "restarting");

        /*  added start pling 09/10/2010 */
        /* Release IANA/IAPD before restarting */
        create_request_list(0);
        client6_send_newstate(dhcp6_if, DHCP6S_RELEASE);
        /*  added end pling 09/10/2010 */

		free_resources(dhcp6_if);
		client6_ifinit(device);
	}
	if ((sig_flags & SIGF_CLEAN)) {

        /*  added start pling 09/10/2010 */
        /* Release IANA/IAPD before exiting */
        create_request_list(0);
        client6_send_newstate(dhcp6_if, DHCP6S_RELEASE);
        /*  added end pling 09/10/2010 */

		free_resources(dhcp6_if);
		exit(0);
	}
	sig_flags = 0;
}

static void
client6_mainloop()
{
	struct timeval *w;
	int ret;
	fd_set r;

	while(1) {
		if (sig_flags)
			process_signals();
		w = dhcp6_check_timer();

		FD_ZERO(&r);
		FD_SET(insock, &r);

		ret = select(insock + 1, &r, NULL, NULL, w);
		switch (ret) {
		case -1:
			if (errno != EINTR) {
				dprintf(LOG_ERR, "%s" "select: %s",
				    FNAME, strerror(errno));
				exit(1);
			}
			break;
		case 0:	/* timeout */
			break;	/* dhcp6_check_timer() will treat the case */
		default: /* received a packet */
			client6_recv();
		}
	}
}

struct dhcp6_timer *
client6_timo(arg)
	void *arg;
{
	struct dhcp6_event *ev = (struct dhcp6_event *)arg;
	struct dhcp6_if *ifp;
	struct timeval now;
	
	ifp = ev->ifp;
	ev->timeouts++;
	gettimeofday(&now, NULL);
	if ((ev->max_retrans_cnt && ev->timeouts >= ev->max_retrans_cnt) ||
	    (ev->max_retrans_dur && (now.tv_sec - ev->start_time.tv_sec) 
	     >= ev->max_retrans_dur)) {
		dprintf(LOG_INFO, "%s" "no responses were received", FNAME);

        /*  added start pling 10/07/2010 */
        /* WNR3500L TD170, after multiple re-transmit of REQUEST 
         * message, if we did not receive a valid response, 
         * go back to SOLICIT state */
        if (ev->state == DHCP6S_REQUEST) {
            ev->timeouts = 0;
            free_servers(ifp);
            ev->state = DHCP6S_SOLICIT;
            dhcp6_set_timeoparam(ev);
            client6_send(ev);
            dhcp6_reset_timer(ev);
            return (ev->timer);
        }
        /*  added end pling 10/07/2010 */

		dhcp6_remove_event(ev);
		return (NULL);
	}

	switch(ev->state) {
	case DHCP6S_INIT:
		/* From INIT state client could
		 * go to CONFIRM state if the client reboots;
		 * go to RELEASE state if the client issues a release;
		 * go to INFOREQ state if the client requests info-only;
		 * go to SOLICIT state if the client requests addresses;
		 */
		ev->timeouts = 0; /* indicate to generate a new XID. */
		/* 
		 * three cases client send information request:
		 * 1. configuration file includes information-only
		 * 2. command line includes -I
		 * 3. check interface flags if managed bit isn't set and
		 *    if otherconf bit set by RA
		 *    and information-only, conmand line -I are not set.
		 */
		if ((ifp->send_flags & DHCIFF_INFO_ONLY) || 
			/*  modified start pling 09/15/2011 */
			/* Ignore the "M" and "O" flags in RA. Just send DHCP solicit */
			(client6_request_flag & CLIENT6_INFO_REQ) /* ||
			(!(ifp->ra_flag & IF_RA_MANAGED) && 
			 (ifp->ra_flag & IF_RA_OTHERCONF))*/ )
			/*  modified end pling 09/15/2011 */
			ev->state = DHCP6S_INFOREQ;
		else if (client6_request_flag & CLIENT6_RELEASE_ADDR) 
			/* do release */
			ev->state = DHCP6S_RELEASE;
		else if (client6_request_flag & CLIENT6_CONFIRM_ADDR) {
			struct dhcp6_listval *lv;
			/* do confirm for reboot for IANA, IATA*/
			if (client6_iaidaddr.client6_info.type == IAPD)
				ev->state = DHCP6S_REBIND;
			else
				ev->state = DHCP6S_CONFIRM;
			for (lv = TAILQ_FIRST(&request_list); lv; 
					lv = TAILQ_NEXT(lv, link)) {
				lv->val_dhcp6addr.preferlifetime = 0;
				lv->val_dhcp6addr.validlifetime = 0;
			}
		} else
			ev->state = DHCP6S_SOLICIT;
		dhcp6_set_timeoparam(ev);

        /*  added start pling 10/14/2010 */
        /* In auto-detect mode, we only send the SOLICIT messages
         * 3 times.
         */
        if (ifp->send_flags & DHCIFF_SOLICIT_ONLY) {
            ev->max_retrans_cnt = SOL_MAX_RC_AUTODETECT;
			dprintf(LOG_ERR, "%s" "Set SOLICIT message to %d times", 
                    FNAME, SOL_MAX_RC_AUTODETECT);
        }
        /*  added end pling 10/14/2010 */

	case DHCP6S_SOLICIT:
		if (ifp->servers) {
			ifp->current_server = select_server(ifp);
			if (ifp->current_server == NULL) {
				/* this should not happen! */
				dprintf(LOG_ERR, "%s" "can't find a server",
					FNAME);
				exit(1);
			}
			/* if get the address assginment break */
			if (!TAILQ_EMPTY(&client6_iaidaddr.lease_list)) {
				dhcp6_remove_event(ev);
				return (NULL);
			}
			ev->timeouts = 0;
			ev->state = DHCP6S_REQUEST;
			dhcp6_set_timeoparam(ev);
		}
        /*  added start pling 10/14/2010 */
        /* In auto-detect mode, we need to send two SOLICIT
         * messages, one with IANA+IAPD, one with IAPD only.
         */
        if (ifp->send_flags & DHCIFF_SOLICIT_ONLY) {
            client6_send(ev);
            ifp->send_flags |= DHCIFF_PREFIX_DELEGATION;
            client6_send(ev);
            ifp->send_flags &=~ DHCIFF_PREFIX_DELEGATION;
            break;  /* don't fall through */
        }
        /*  added end pling 10/14/2010 */
	//case DHCP6S_INFOREQ:  //  removed pling 10/05/2010 */
	case DHCP6S_REQUEST:
		client6_send(ev);
		break;

    /*  added start pling 10/05/2010 */
    /* Use different function to send INFO-REQ */
    case DHCP6S_INFOREQ:
        client6_send_info_req(ev);
        break;
    /*  added end pling 10/05/2010 */

	case DHCP6S_RELEASE:
	case DHCP6S_DECLINE:
	case DHCP6S_CONFIRM:
	case DHCP6S_RENEW:
	case DHCP6S_REBIND:
		if (!TAILQ_EMPTY(&request_list))
			client6_send(ev);
		else {
			dprintf(LOG_INFO, "%s"
		    		"all information to be updated were canceled",
		    		FNAME);
			dhcp6_remove_event(ev);
			return (NULL);
		}
		break;
	default:
		break;
	}
	dhcp6_reset_timer(ev);
	return (ev->timer);
}

static struct dhcp6_serverinfo *
select_server(ifp)
	struct dhcp6_if *ifp;
{
	struct dhcp6_serverinfo *s;

	for (s = ifp->servers; s; s = s->next) {
		if (s->active) {
			dprintf(LOG_DEBUG, "%s" "picked a server (ID: %s)",
				FNAME, duidstr(&s->optinfo.serverID));
			return (s);
		}
	}

	return (NULL);
}

static void
client6_signal(sig)
	int sig;
{

	dprintf(LOG_INFO, FNAME "received a signal (%d)", sig);

	switch (sig) {
	case SIGTERM:
		sig_flags |= SIGF_TERM;
		break;
	case SIGHUP:
		sig_flags |= SIGF_HUP;
		break;
	case SIGINT:
	case SIGKILL:
		sig_flags |= SIGF_CLEAN;
		break;
	default:
		break;
	}
}

void
client6_send(ev)
	struct dhcp6_event *ev;
{
	struct dhcp6_if *ifp;
	char buf[BUFSIZ];
	struct sockaddr_in6 dst;
	struct dhcp6 *dh6;
	struct dhcp6_optinfo optinfo;
	ssize_t optlen, len;
	struct timeval duration, now;

	ifp = ev->ifp;

	dh6 = (struct dhcp6 *)buf;
	memset(dh6, 0, sizeof(*dh6));

	switch(ev->state) {
	case DHCP6S_SOLICIT:
		dh6->dh6_msgtype = DH6_SOLICIT;
		break;
	case DHCP6S_REQUEST:
		if (ifp->current_server == NULL) {
			dprintf(LOG_ERR, "%s" "assumption failure", FNAME);
			exit(1);
		}
		dh6->dh6_msgtype = DH6_REQUEST;
		break;
	case DHCP6S_RENEW:
		if (ifp->current_server == NULL) {
			dprintf(LOG_ERR, "%s" "assumption failure", FNAME);
			exit(1);
		}
		dh6->dh6_msgtype = DH6_RENEW;
		break;
	case DHCP6S_DECLINE:
		if (ifp->current_server == NULL) {
			dprintf(LOG_ERR, "%s" "assumption failure", FNAME);
			exit(1);
		}
		dh6->dh6_msgtype = DH6_DECLINE;
		break;
	case DHCP6S_INFOREQ:	
		dh6->dh6_msgtype = DH6_INFORM_REQ;
		break;
	case DHCP6S_REBIND:
		dh6->dh6_msgtype = DH6_REBIND;
		break;
	case DHCP6S_CONFIRM:
		dh6->dh6_msgtype = DH6_CONFIRM;
		break;
	case DHCP6S_RELEASE:
		dh6->dh6_msgtype = DH6_RELEASE;
		break;
	default:
		dprintf(LOG_ERR, "%s" "unexpected state %d", FNAME, ev->state);
		exit(1);
	}
	/*
	 * construct options
	 */
	dhcp6_init_options(&optinfo);
	if (ev->timeouts == 0) {
		gettimeofday(&ev->start_time, NULL);
		optinfo.elapsed_time = 0;
		/*
		 * A client SHOULD generate a random number that cannot easily
		 * be guessed or predicted to use as the transaction ID for
		 * each new message it sends.
		 *
		 * A client MUST leave the transaction-ID unchanged in
		 * retransmissions of a message. [dhcpv6-26 15.1]
		 */
		ev->xid = random() & DH6_XIDMASK;

        /*  added start pling 10/07/2010 */
        /* For Testing purposes !!! */
        if (ev->state == DHCP6S_SOLICIT && xid_solicit) {
            dprintf(LOG_DEBUG, "%s"
                "**TESTING** Use user-defined xid_sol %lu", FNAME, xid_solicit);
            ev->xid = xid_solicit & DH6_XIDMASK;
        } else if (ev->state == DHCP6S_REQUEST && xid_request) {
            dprintf(LOG_DEBUG, "%s"
                "**TESTING** Use user-defined xid_req %lu", FNAME, xid_request);
            ev->xid = xid_request & DH6_XIDMASK;
        }
        /*  added end pling 10/07/2010 */

		dprintf(LOG_DEBUG, "%s" "ifp %p event %p a new XID (%x) is generated",
			FNAME, ifp, ev, ev->xid);
	} else {
		unsigned int etime;
		gettimeofday(&now, NULL);
		timeval_sub(&now, &(ev->start_time), &duration);
		optinfo.elapsed_time = 
		etime = (duration.tv_sec) * 100 + (duration.tv_usec) / 10000;
		if (etime > DHCP6_ELAPSEDTIME_MAX)
			optinfo.elapsed_time = DHCP6_ELAPSEDTIME_MAX;
		else
			optinfo.elapsed_time = etime;
	}
	dh6->dh6_xid &= ~ntohl(DH6_XIDMASK);
	dh6->dh6_xid |= htonl(ev->xid);
	len = sizeof(*dh6);


	/* server ID */
	switch(ev->state) {
	case DHCP6S_REQUEST:
	case DHCP6S_RENEW:
	case DHCP6S_DECLINE:
		if (&ifp->current_server->optinfo == NULL)
			exit(1);
		dprintf(LOG_DEBUG, "current server ID %s",
			duidstr(&ifp->current_server->optinfo.serverID));
		if (duidcpy(&optinfo.serverID,
		    &ifp->current_server->optinfo.serverID)) {
			dprintf(LOG_ERR, "%s" "failed to copy server ID",
			    FNAME);
			goto end;
		}
		break;
	case DHCP6S_RELEASE:
		if (duidcpy(&optinfo.serverID, &client6_iaidaddr.client6_info.serverid)) {
			dprintf(LOG_ERR, "%s" "failed to copy server ID", FNAME);
			goto end;
		}
		break;
	}
	/* client ID */
	if (duidcpy(&optinfo.clientID, &client_duid)) {
		dprintf(LOG_ERR, "%s" "failed to copy client ID", FNAME);
		goto end;
	}

	/*  added start pling 09/07/2010 */
	/* User-class */
	strcpy(optinfo.user_class, ifp->user_class);
	/*  added end pling 09/07/2010 */

	/* option request options */
    /*  added start pling 10/01/2010 */
    /* DHCPv6 readylogo: DHCP confirm message should not
     *  have request for DNS/Domain list.
     */
    if (ev->state != DHCP6S_CONFIRM)
    /*  added end pling 10/01/2010 */
	if (dhcp6_copy_list(&optinfo.reqopt_list, &ifp->reqopt_list)) {
		dprintf(LOG_ERR, "%s" "failed to copy requested options",
		    FNAME);
		goto end;
	}
	
	switch(ev->state) {
	case DHCP6S_SOLICIT:
		/* rapid commit */
		if (ifp->send_flags & DHCIFF_RAPID_COMMIT) 
			optinfo.flags |= DHCIFF_RAPID_COMMIT;
		if (!(ifp->send_flags & DHCIFF_INFO_ONLY) ||
		    (client6_request_flag & CLIENT6_REQUEST_ADDR)) {
			memcpy(&optinfo.iaidinfo, &client6_iaidaddr.client6_info.iaidinfo,
					sizeof(optinfo.iaidinfo));
			if (ifp->send_flags & DHCIFF_PREFIX_DELEGATION)
				optinfo.type = IAPD;
			else if (ifp->send_flags & DHCIFF_TEMP_ADDRS)
				optinfo.type = IATA;
			else
				optinfo.type = IANA;
		}
		/* support for client preferred ipv6 address */
		if (client6_request_flag & CLIENT6_REQUEST_ADDR) {
			if (dhcp6_copy_list(&optinfo.addr_list, &request_list))
				goto end;
		}
        /*  added start pling 10/14/2010 */
        /* In auto-detect mode, we need to send two SOLICIT
         * messages. 2nd one has IAPD only.
         */
        if ((ifp->send_flags & DHCIFF_SOLICIT_ONLY) &&
            (ifp->send_flags & DHCIFF_PREFIX_DELEGATION)) {
            optinfo.type = IAPD;
        }
        /*  added end pling 10/14/2010 */
		break;
	case DHCP6S_REQUEST:
		if (!(ifp->send_flags & DHCIFF_INFO_ONLY)) {
            /*  modified start pling 08/20/2009 */
            /* Should copy 'current_server's addr info */
#if 0
			memcpy(&optinfo.iaidinfo, &client6_iaidaddr.client6_info.iaidinfo,
					sizeof(optinfo.iaidinfo));
#endif
			memcpy(&optinfo.iaidinfo, &(ifp->current_server->optinfo.iaidinfo),
                    sizeof(optinfo.iaidinfo));
			if (dhcp6_copy_list(&optinfo.addr_list, &(ifp->current_server->optinfo.addr_list)))
                goto end;
			if (dhcp6_copy_list(&optinfo.prefix_list, 
						&(ifp->current_server->optinfo.prefix_list)))
                goto end;
            /*  modified end pling 08/20/2009 */
			dprintf(LOG_DEBUG, "%s IAID is %u", FNAME, optinfo.iaidinfo.iaid);
			if (ifp->send_flags & DHCIFF_TEMP_ADDRS) 
				optinfo.type = IATA;
			else if (ifp->send_flags & DHCIFF_PREFIX_DELEGATION)
				optinfo.type = IAPD;
			else
				optinfo.type = IANA;
		}
		break;
	case DHCP6S_RENEW:
	case DHCP6S_REBIND:
	case DHCP6S_RELEASE:
	case DHCP6S_CONFIRM:
	case DHCP6S_DECLINE:
		memcpy(&optinfo.iaidinfo, &client6_iaidaddr.client6_info.iaidinfo,
			sizeof(optinfo.iaidinfo));
		optinfo.type = client6_iaidaddr.client6_info.type;
		if (ev->state == DHCP6S_CONFIRM) {
			optinfo.iaidinfo.renewtime = 0;
			optinfo.iaidinfo.rebindtime = 0;
		}
		if (!TAILQ_EMPTY(&request_list)) {
			if (dhcp6_copy_list(&optinfo.addr_list, &request_list))
				goto end;
		} else {
			if (ev->state == DHCP6S_RELEASE) {
				dprintf(LOG_INFO, "release empty address list");
				exit(1);
			}
		}
        /*  added start pling 09/24/2009 */
        /* PUt the IAPD prefix in the DHCP packet */
		if (!TAILQ_EMPTY(&request_prefix_list)) {
			if (dhcp6_copy_list(&optinfo.prefix_list, &request_prefix_list))
				goto end;
        }
        /*  added end pling 09/24/2009 */
		if (client6_request_flag & CLIENT6_RELEASE_ADDR) {
			if (dhcp6_update_iaidaddr(&optinfo, ADDR_REMOVE)) {
				dprintf(LOG_INFO, "client release failed");
				exit(1);
			}
			if (client6_iaidaddr.client6_info.type == IAPD)
				radvd_parse(&client6_iaidaddr, ADDR_REMOVE);
		}
		break;
	default:
		break;
	}
	/* set options in the message */
	if ((optlen = dhcp6_set_options((struct dhcp6opt *)(dh6 + 1),
					(struct dhcp6opt *)(buf + sizeof(buf)),
					&optinfo)) < 0) {
		dprintf(LOG_INFO, "%s" "failed to construct options", FNAME);
		goto end;
	}
	len += optlen;

	/*
	 * Unless otherwise specified, a client sends DHCP messages to the
	 * All_DHCP_Relay_Agents_and_Servers or the DHCP_Anycast address.
	 * [dhcpv6-26 Section 13.]
	 * Our current implementation always follows the case.
	 */
	switch(ev->state) {
	case DHCP6S_REQUEST:
	case DHCP6S_RENEW:
	case DHCP6S_DECLINE:
	case DHCP6S_RELEASE:
		if (ifp->current_server && 
		    !IN6_IS_ADDR_UNSPECIFIED(&ifp->current_server->server_addr)) {
			struct addrinfo hints, *res;
			int error;
			memset(&hints, 0, sizeof(hints));
			hints.ai_family = PF_INET6;
			hints.ai_socktype = SOCK_DGRAM;
			hints.ai_protocol = IPPROTO_UDP;
			error = getaddrinfo(in6addr2str(&ifp->current_server->server_addr,0),
				DH6PORT_UPSTREAM, &hints, &res);
			if (error) {
				dprintf(LOG_ERR, "%s" "getaddrinfo: %s",
					FNAME, gai_strerror(error));
				exit(1);
			}
			memcpy(&dst, res->ai_addr, res->ai_addrlen);
			break;
		}
	default:
		dst = *sa6_allagent;
		break;
	}
	dst.sin6_scope_id = ifp->linkid;
	dprintf(LOG_DEBUG, "send dst if %s addr is %s scope id is %d", 
		ifp->ifname, addr2str((struct sockaddr *)&dst), ifp->linkid);
    /*  modified start pling 08/15/2009 */
    /* why use 'outsock' here? */
	//if (sendto(ifp->outsock, buf, len, MSG_DONTROUTE, (struct sockaddr *)&dst,
	if (sendto(insock, buf, len, MSG_DONTROUTE, (struct sockaddr *)&dst,
    /*  modified end pling 08/15/2009 */
	    sizeof(dst)) == -1) {
		dprintf(LOG_ERR, FNAME "transmit failed: %s", strerror(errno));
		goto end;
	}

	dprintf(LOG_DEBUG, "%s" "send %s to %s", FNAME,
		dhcp6msgstr(dh6->dh6_msgtype),
		addr2str((struct sockaddr *)&dst));

  end:
	dhcp6_clear_options(&optinfo);
	return;
}
	
/*  added end pling 01/25/2010 */
void
client6_send_info_req(ev)
	struct dhcp6_event *ev;
{
	struct dhcp6_if *ifp;
	char buf[BUFSIZ];
	struct sockaddr_in6 dst;
	struct dhcp6 *dh6;
	struct dhcp6_optinfo optinfo;
	ssize_t optlen, len;
	struct timeval duration, now;

	ifp = ev->ifp;

	dh6 = (struct dhcp6 *)buf;
	memset(dh6, 0, sizeof(*dh6));

	dh6->dh6_msgtype = DH6_INFORM_REQ;

	/*
	 * construct options
	 */
	dhcp6_init_options(&optinfo);
	if (ev->timeouts == 0) {
		gettimeofday(&ev->start_time, NULL);
		optinfo.elapsed_time = 0;
		/*
		 * A client SHOULD generate a random number that cannot easily
		 * be guessed or predicted to use as the transaction ID for
		 * each new message it sends.
		 *
		 * A client MUST leave the transaction-ID unchanged in
		 * retransmissions of a message. [dhcpv6-26 15.1]
		 */
		ev->xid = random() & DH6_XIDMASK;
		dprintf(LOG_DEBUG, "%s" "ifp %p event %p a new XID (%x) is generated",
			FNAME, ifp, ev, ev->xid);
	} else {
		unsigned int etime;
		gettimeofday(&now, NULL);
		timeval_sub(&now, &(ev->start_time), &duration);
		optinfo.elapsed_time = 
		etime = (duration.tv_sec) * 100 + (duration.tv_usec) / 10000;
		if (etime > DHCP6_ELAPSEDTIME_MAX)
			optinfo.elapsed_time = DHCP6_ELAPSEDTIME_MAX;
		else
			optinfo.elapsed_time = etime;
	}
	dh6->dh6_xid &= ~ntohl(DH6_XIDMASK);
	dh6->dh6_xid |= htonl(ev->xid);
	len = sizeof(*dh6);

	/* client ID */
	if (duidcpy(&optinfo.clientID, &client_duid)) {
		dprintf(LOG_ERR, "%s" "failed to copy client ID", FNAME);
		goto end;
	}

	/*  added start pling 09/07/2010 */
	/* User-class */
	strcpy(optinfo.user_class, ifp->user_class);
	/*  added end pling 09/07/2010 */

	/* option request options */
	if (dhcp6_copy_list(&optinfo.reqopt_list, &ifp->reqopt_list)) {
		dprintf(LOG_ERR, "%s" "failed to copy requested options",
		    FNAME);
		goto end;
	}
	
	/* set options in the message */
	if ((optlen = dhcp6_set_options((struct dhcp6opt *)(dh6 + 1),
					(struct dhcp6opt *)(buf + sizeof(buf)),
					&optinfo)) < 0) {
		dprintf(LOG_INFO, "%s" "failed to construct options", FNAME);
		goto end;
	}
	len += optlen;

    /* Special hack to add SIP server and NTP server options */
    buf[len-3] += 4;
    buf[len++] = 0;
    buf[len++] = DH6OPT_SIP_SERVERS;
    buf[len++] = 0;
    buf[len++] = DH6OPT_NTP_SERVERS;

    /*
	 * Unless otherwise specified, a client sends DHCP messages to the
	 * All_DHCP_Relay_Agents_and_Servers or the DHCP_Anycast address.
	 * [dhcpv6-26 Section 13.]
	 * Our current implementation always follows the case.
	 */
	switch(ev->state) {
	case DHCP6S_REQUEST:
	case DHCP6S_RENEW:
	case DHCP6S_DECLINE:
	case DHCP6S_RELEASE:
		if (ifp->current_server && 
		    !IN6_IS_ADDR_UNSPECIFIED(&ifp->current_server->server_addr)) {
			struct addrinfo hints, *res;
			int error;
			memset(&hints, 0, sizeof(hints));
			hints.ai_family = PF_INET6;
			hints.ai_socktype = SOCK_DGRAM;
			hints.ai_protocol = IPPROTO_UDP;
			error = getaddrinfo(in6addr2str(&ifp->current_server->server_addr,0),
				DH6PORT_UPSTREAM, &hints, &res);
			if (error) {
				dprintf(LOG_ERR, "%s" "getaddrinfo: %s",
					FNAME, gai_strerror(error));
				exit(1);
			}
			memcpy(&dst, res->ai_addr, res->ai_addrlen);
			break;
		}
	default:
		dst = *sa6_allagent;
		break;
	}
	dst.sin6_scope_id = ifp->linkid;
	dprintf(LOG_DEBUG, "send dst if %s addr is %s scope id is %d", 
		ifp->ifname, addr2str((struct sockaddr *)&dst), ifp->linkid);
    /*  modified start pling 08/15/2009 */
    /* why use 'outsock' here? */
	//if (sendto(ifp->outsock, buf, len, MSG_DONTROUTE, (struct sockaddr *)&dst,
	if (sendto(insock, buf, len, MSG_DONTROUTE, (struct sockaddr *)&dst,
    /*  modified end pling 08/15/2009 */
	    sizeof(dst)) == -1) {
		dprintf(LOG_ERR, FNAME "transmit failed: %s", strerror(errno));
		goto end;
	}

	dprintf(LOG_DEBUG, "%s" "send %s to %s", FNAME,
		dhcp6msgstr(dh6->dh6_msgtype),
		addr2str((struct sockaddr *)&dst));

  end:
	dhcp6_clear_options(&optinfo);
	return;
}
/*  added end pling 01/25/2010 */

static void
client6_recv()
{
	char rbuf[BUFSIZ], cmsgbuf[BUFSIZ];
	struct msghdr mhdr;
	struct iovec iov;
	struct sockaddr_storage from;
	struct dhcp6_if *ifp;
	struct dhcp6opt *p, *ep;
	struct dhcp6_optinfo optinfo;
	ssize_t len;
	struct dhcp6 *dh6;
	struct cmsghdr *cm;
	struct in6_pktinfo *pi = NULL;

	memset(&iov, 0, sizeof(iov));
	memset(&mhdr, 0, sizeof(mhdr));

	iov.iov_base = (caddr_t)rbuf;
	iov.iov_len = sizeof(rbuf);
	mhdr.msg_name = (caddr_t)&from;
	mhdr.msg_namelen = sizeof(from);
	mhdr.msg_iov = &iov;
	mhdr.msg_iovlen = 1;
	mhdr.msg_control = (caddr_t)cmsgbuf;
	mhdr.msg_controllen = sizeof(cmsgbuf);
	if ((len = recvmsg(insock, &mhdr, 0)) < 0) {
		dprintf(LOG_ERR, "%s" "recvmsg: %s", FNAME, strerror(errno));
		return;
	}

	/* detect receiving interface */
	for (cm = (struct cmsghdr *)CMSG_FIRSTHDR(&mhdr); cm;
	     cm = (struct cmsghdr *)CMSG_NXTHDR(&mhdr, cm)) {
		if (cm->cmsg_level == IPPROTO_IPV6 &&
		    cm->cmsg_type == IPV6_PKTINFO &&
		    cm->cmsg_len == CMSG_LEN(sizeof(struct in6_pktinfo))) {
			pi = (struct in6_pktinfo *)(CMSG_DATA(cm));
		}
	}
	if (pi == NULL) {
		dprintf(LOG_NOTICE, "%s" "failed to get packet info", FNAME);
		return;
	}
	if ((ifp = find_ifconfbyid(pi->ipi6_ifindex)) == NULL) {
		dprintf(LOG_INFO, "%s" "unexpected interface (%d)", FNAME,
			(unsigned int)pi->ipi6_ifindex);
		return;
	}
	dprintf(LOG_DEBUG, "receive packet info ifname %s, addr is %s scope id is %d", 
		ifp->ifname, in6addr2str(&pi->ipi6_addr, 0), pi->ipi6_ifindex);
	dh6 = (struct dhcp6 *)rbuf;

	dprintf(LOG_DEBUG, "%s" "receive %s from %s scope id %d %s", FNAME,
		dhcp6msgstr(dh6->dh6_msgtype),
		addr2str((struct sockaddr *)&from),
		((struct sockaddr_in6 *)&from)->sin6_scope_id,
		ifp->ifname);

	/* get options */
	dhcp6_init_options(&optinfo);
	p = (struct dhcp6opt *)(dh6 + 1);
	ep = (struct dhcp6opt *)((char *)dh6 + len);

    /*  modified start pling 10/04/2010 */
    /* Pass some extra arguments to 'dhcp6_get_options'
     * to better determine whether this packet is ok or not.
     */
    struct dhcp6_event *ev;
    ev = find_event_withid(ifp, ntohl(dh6->dh6_xid) & DH6_XIDMASK);
    if (ev == NULL) {
        dprintf(LOG_INFO, "%s" "XID mismatch", FNAME);
        return;
    }
    if (dhcp6_get_options(p, ep, &optinfo, dh6->dh6_msgtype, 
                ev->state, ifp->send_flags) < 0) {
	//if (dhcp6_get_options(p, ep, &optinfo) < 0) {
    /*  modified end pling 10/04/2010 */
		dprintf(LOG_INFO, "%s" "failed to parse options", FNAME);
#ifdef TEST
		return;
#endif
	}

	switch(dh6->dh6_msgtype) {
	case DH6_ADVERTISE:
		(void)client6_recvadvert(ifp, dh6, len, &optinfo);
		break;
	case DH6_REPLY:
		(void)client6_recvreply(ifp, dh6, len, &optinfo);
		break;
	default:
		dprintf(LOG_INFO, "%s" "received an unexpected message (%s) "
			"from %s", FNAME, dhcp6msgstr(dh6->dh6_msgtype),
			addr2str((struct sockaddr *)&from));
		break;
	}

	dhcp6_clear_options(&optinfo);
	return;
}

static int
client6_recvadvert(ifp, dh6, len, optinfo0)
	struct dhcp6_if *ifp;
	struct dhcp6 *dh6;
	ssize_t len;
	struct dhcp6_optinfo *optinfo0;
{
	struct dhcp6_serverinfo *newserver;
	struct dhcp6_event *ev;
	struct dhcp6_listval *lv;

	/* find the corresponding event based on the received xid */
	ev = find_event_withid(ifp, ntohl(dh6->dh6_xid) & DH6_XIDMASK);
	if (ev == NULL) {
		dprintf(LOG_INFO, "%s" "XID mismatch", FNAME);
		return -1;
	}
	/* if server policy doesn't allow rapid commit
	if (ev->state != DHCP6S_SOLICIT ||
	    (ifp->send_flags & DHCIFF_RAPID_COMMIT)) {
	*/
	if (ev->state != DHCP6S_SOLICIT) { 
		dprintf(LOG_INFO, "%s" "unexpected advertise", FNAME);
		return -1;
	}
	
	/* packet validation based on Section 15.3 of dhcpv6-26. */
	if (optinfo0->serverID.duid_len == 0) {
		dprintf(LOG_INFO, "%s" "no server ID option", FNAME);
		return -1;
	} else {
		dprintf(LOG_DEBUG, "%s" "server ID: %s, pref=%2x", FNAME,
			duidstr(&optinfo0->serverID),
			optinfo0->pref);
	}
	if (optinfo0->clientID.duid_len == 0) {
		dprintf(LOG_INFO, "%s" "no client ID option", FNAME);
		return -1;
	}
	if (duidcmp(&optinfo0->clientID, &client_duid)) {
		dprintf(LOG_INFO, "%s" "client DUID mismatch", FNAME);
		return -1;
	}

	/*
	 * The client MUST ignore any Advertise message that includes a Status
	 * Code option containing any error.
	 */
	for (lv = TAILQ_FIRST(&optinfo0->stcode_list); lv;
	     lv = TAILQ_NEXT(lv, link)) {
		dprintf(LOG_INFO, "%s" "status code: %s",
		    FNAME, dhcp6_stcodestr(lv->val_num));
		if (lv->val_num != DH6OPT_STCODE_SUCCESS) {
			return (-1);
		}
	}

	/* ignore the server if it is known */
	if (find_server(ifp, &optinfo0->serverID)) {
		dprintf(LOG_INFO, "%s" "duplicated server (ID: %s)",
			FNAME, duidstr(&optinfo0->serverID));
		return -1;
	}

    /*  added start pling 08/26/2009 */
    /* In Ipv6 auto mode, write result to a file */
	if (ifp->send_flags & DHCIFF_SOLICIT_ONLY) {
        FILE *fp = NULL;
        /*  modified start pling 10/14/2010 */
        /* For auto-detect mode, if recv ADVERTISE mesg with 
         * IAPD-only, write to different file.
         */
        if (optinfo0->type == IAPD)
            fp = fopen("/tmp/wan_dhcp6c_iapd", "w");
        else
            fp = fopen("/tmp/wan_dhcp6c", "w");
        /*  modified end pling 10/14/2010 */
        if (fp) {
            fprintf(fp, "1");
            fclose(fp);
        }
        return 0;
    }
    /*  added end pling 08/26/2009 */

	newserver = allocate_newserver(ifp, optinfo0);
	if (newserver == NULL)
		return (-1);
		
    /*  added start pling 08/21/2009 */
    /* for some reason, 'allocate_newserver' did not copy
     * the IAID info. So we do it here...
     */
    memcpy(&(newserver->optinfo.iaidinfo),
           &(optinfo0->iaidinfo),
           sizeof(struct dhcp6_iaid_info));
    /*  added end pling 08/21/2009 */

	/* if the server has an extremely high preference, just use it. */
	if (newserver->pref == DH6OPT_PREF_MAX) {
		ev->timeouts = 0;
		ev->state = DHCP6S_REQUEST;
		ifp->current_server = newserver;
		dhcp6_set_timeoparam(ev);
		dhcp6_reset_timer(ev);
		client6_send(ev);

	} else if (ifp->servers->next == NULL) {
		struct timeval *rest, elapsed, tv_rt, tv_irt, timo;

		rest = dhcp6_timer_rest(ev->timer);
		tv_rt.tv_sec = (ev->retrans * 1000) / 1000000;
		tv_rt.tv_usec = (ev->retrans * 1000) % 1000000;
		tv_irt.tv_sec = (ev->init_retrans * 1000) / 1000000;
		tv_irt.tv_usec = (ev->init_retrans * 1000) % 1000000;
		timeval_sub(&tv_rt, rest, &elapsed);
		if (TIMEVAL_LEQ(elapsed, tv_irt))
			timeval_sub(&tv_irt, &elapsed, &timo);
		else
			timo.tv_sec = timo.tv_usec = 0;

		dprintf(LOG_DEBUG, "%s" "reset timer for %s to %d.%06d",
			FNAME, ifp->ifname,
			(int)timo.tv_sec, (int)timo.tv_usec);

		dhcp6_set_timer(&timo, ev->timer);
	}
	/* if the client send preferred addresses reqeust in SOLICIT */
	if (!TAILQ_EMPTY(&optinfo0->addr_list))
		dhcp6_copy_list(&request_list, &optinfo0->addr_list);
	/*  added start pling 09/23/2009 */
	/* Store IAPD to the request_prefix_list, for later use by DHCP renew */
	if (!TAILQ_EMPTY(&optinfo0->prefix_list))
		dhcp6_copy_list(&request_prefix_list, &optinfo0->prefix_list);
	/*  added end pling 09/23/2009 */
	return 0;
}

static struct dhcp6_serverinfo *
find_server(ifp, duid)
	struct dhcp6_if *ifp;
	struct duid *duid;
{
	struct dhcp6_serverinfo *s;

	for (s = ifp->servers; s; s = s->next) {
		if (duidcmp(&s->optinfo.serverID, duid) == 0)
			return (s);
	}

	return (NULL);
}

static struct dhcp6_serverinfo *
allocate_newserver(ifp, optinfo)
	struct dhcp6_if *ifp;
	struct dhcp6_optinfo *optinfo;
{
	struct dhcp6_serverinfo *newserver, **sp;

	/* keep the server */
	if ((newserver = malloc(sizeof(*newserver))) == NULL) {
		dprintf(LOG_ERR, "%s" "memory allocation failed for server",
			FNAME);
		return (NULL);
	}
	memset(newserver, 0, sizeof(*newserver));
	dhcp6_init_options(&newserver->optinfo);
	if (dhcp6_copy_options(&newserver->optinfo, optinfo)) {
		dprintf(LOG_ERR, "%s" "failed to copy options", FNAME);
		free(newserver);
		return (NULL);
	}
	dprintf(LOG_DEBUG, "%s" "new server DUID %s, len %d ", 
		FNAME, duidstr(&newserver->optinfo.serverID), 
		newserver->optinfo.serverID.duid_len);
	if (optinfo->pref != DH6OPT_PREF_UNDEF)
		newserver->pref = optinfo->pref;
	if (optinfo->flags & DHCIFF_UNICAST)
		memcpy(&newserver->server_addr, &optinfo->server_addr,
		       sizeof(newserver->server_addr));
	newserver->active = 1;
	for (sp = &ifp->servers; *sp; sp = &(*sp)->next) {
		if ((*sp)->pref != DH6OPT_PREF_MAX &&
		    (*sp)->pref < newserver->pref) {
			break;
		}
	}
	newserver->next = *sp;
	*sp = newserver;
	return newserver;
}

void
free_servers(ifp)
	struct dhcp6_if *ifp;
{
	struct dhcp6_serverinfo *sp, *sp_next;
	/* free all servers we've seen so far */
	for (sp = ifp->servers; sp; sp = sp_next) {
		sp_next = sp->next;
		dprintf(LOG_DEBUG, "%s" "removing server (ID: %s)",
		    FNAME, duidstr(&sp->optinfo.serverID));
		dhcp6_clear_options(&sp->optinfo);
		free(sp);
	}
	ifp->servers = NULL;
	ifp->current_server = NULL;
}

static int not_on_link_count = 0;    //  added pling 10/07/2010

static int
client6_recvreply(ifp, dh6, len, optinfo)
	struct dhcp6_if *ifp;
	struct dhcp6 *dh6;
	ssize_t len;
	struct dhcp6_optinfo *optinfo;
{
	struct dhcp6_listval *lv;
	struct dhcp6_event *ev;
	int addr_status_code = DH6OPT_STCODE_UNSPECFAIL;
	struct dhcp6_serverinfo *newserver;
	int newstate = 0;
	/* find the corresponding event based on the received xid */
	dprintf(LOG_DEBUG, "%s" "reply message XID is (%x)",
		FNAME, ntohl(dh6->dh6_xid) & DH6_XIDMASK);
	ev = find_event_withid(ifp, ntohl(dh6->dh6_xid) & DH6_XIDMASK);
	if (ev == NULL) {
		dprintf(LOG_INFO, "%s" "XID mismatch", FNAME);
		return -1;
	}

	if (!(DHCP6S_VALID_REPLY(ev->state)) &&
	    (ev->state != DHCP6S_SOLICIT ||
	     !(optinfo->flags & DHCIFF_RAPID_COMMIT))) {
		dprintf(LOG_INFO, "%s" "unexpected reply", FNAME);
		return -1;
	}

	dhcp6_clear_list(&request_list);

	/* A Reply message must contain a Server ID option */
	if (optinfo->serverID.duid_len == 0) {
		dprintf(LOG_INFO, "%s" "no server ID option", FNAME);
		return -1;
	}
	dprintf(LOG_DEBUG, "%s" "serverID is %s len is %d", FNAME,
		duidstr(&optinfo->serverID), optinfo->serverID.duid_len); 
	/* get current server */
	switch (ev->state) {
	case DHCP6S_SOLICIT:
	case DHCP6S_CONFIRM:
	case DHCP6S_REBIND:
		newserver = allocate_newserver(ifp, optinfo);
		if (newserver == NULL)
			return (-1);
		ifp->current_server = newserver;
		duidcpy(&client6_iaidaddr.client6_info.serverid, 
			&ifp->current_server->optinfo.serverID);
		break;
	default:
		break;
	}
	/*
	 * DUID in the Client ID option (which must be contained for our
	 * client implementation) must match ours.
	 */
	if (optinfo->clientID.duid_len == 0) {
		dprintf(LOG_INFO, "%s" "no client ID option", FNAME);
		return -1;
	}
	if (duidcmp(&optinfo->clientID, &client_duid)) {
		dprintf(LOG_INFO, "%s" "client DUID mismatch", FNAME);
		return -1;
	}

	if (!TAILQ_EMPTY(&optinfo->dns_list.addrlist) || 
	    optinfo->dns_list.domainlist != NULL) {
		resolv_parse(&optinfo->dns_list);
	}
	/*
	 * The client MAY choose to report any status code or message from the
	 * status code option in the Reply message.
	 * [dhcpv6-26 Section 18.1.8]
	 */
	addr_status_code = 0;
	for (lv = TAILQ_FIRST(&optinfo->stcode_list); lv;
	     lv = TAILQ_NEXT(lv, link)) {
		dprintf(LOG_INFO, "%s" "status code: %s",
		    FNAME, dhcp6_stcodestr(lv->val_num));
		switch (lv->val_num) {
		case DH6OPT_STCODE_SUCCESS:
		case DH6OPT_STCODE_UNSPECFAIL:
		case DH6OPT_STCODE_NOADDRAVAIL:
		case DH6OPT_STCODE_NOPREFIXAVAIL:
		case DH6OPT_STCODE_NOBINDING:
		case DH6OPT_STCODE_NOTONLINK:
		case DH6OPT_STCODE_USEMULTICAST:
            if (addr_status_code == 0)      //  added pling 10/07/2010, don't override error status if already set
    			addr_status_code = lv->val_num;
		default:
			break;
		}
	}
	switch (addr_status_code) {
	case DH6OPT_STCODE_UNSPECFAIL:
	case DH6OPT_STCODE_USEMULTICAST:
		dprintf(LOG_INFO, "%s" "status code: %s", FNAME, 
			dhcp6_stcodestr(addr_status_code));
		/* retransmit the message with multicast address */
		/* how many time allow the retransmission with error status code? */
		return -1;
	}
	
	/*
	 * Update configuration information to be renewed or rebound
	 * declined, confirmed, released.
	 * Note that the returned list may be empty, in which case
	 * the waiting information should be removed.
	 */
	switch (ev->state) {
	case DHCP6S_SOLICIT:
		if (optinfo->iaidinfo.iaid == 0)
			break;
		else if (!optinfo->flags & DHCIFF_RAPID_COMMIT) {
			newstate = DHCP6S_REQUEST;
			break;
		}
	case DHCP6S_REQUEST:
		/* NotOnLink: 1. SOLICIT 
		 * NoAddrAvail: Information Request */
		switch(addr_status_code) {
		case DH6OPT_STCODE_NOTONLINK:
			dprintf(LOG_DEBUG, "%s" 
			    "got a NotOnLink reply for request/rapid commit,"
			    " sending solicit.", FNAME);
            /*  modified start pling 10/07/2010 */
            /* WNR3500L TD170, need to send request without any IP for
             *  3 times, then back to solicit state.
             */
			//newstate = DHCP6S_SOLICIT;
            not_on_link_count++;
            if (not_on_link_count <= REQ_MAX_RC_NOTONLINK) {
                /* Clear the IA / PD address, so they won't appear in the
                 * request pkt. */
                dhcp6_clear_list(&(ifp->current_server->optinfo.addr_list));
                dhcp6_clear_list(&(ifp->current_server->optinfo.prefix_list));
                newstate = DHCP6S_REQUEST;
            } else {
                /* Three times, back to SOLICIT state */
                not_on_link_count = 0;
                free_servers(ifp);
                newstate = DHCP6S_SOLICIT;
            }
            /*  modified end pling 10/07/2010 */
			break;
		case DH6OPT_STCODE_NOADDRAVAIL:
		case DH6OPT_STCODE_NOPREFIXAVAIL:
			dprintf(LOG_DEBUG, "%s" 
			    "got a NoAddrAvail reply for request/rapid commit,"
			    " sending inforeq.", FNAME);
            not_on_link_count = 0;  //  added pling 10/07/2010
			optinfo->iaidinfo.iaid = 0;
			newstate = DHCP6S_INFOREQ;
			break;
		case DH6OPT_STCODE_SUCCESS:
		case DH6OPT_STCODE_UNDEFINE:
		default:
            not_on_link_count = 0;  //  added pling 10/07/2010
			if (!TAILQ_EMPTY(&optinfo->addr_list)) {
				(void)get_if_rainfo(ifp);
				dhcp6_add_iaidaddr(optinfo);
				if (optinfo->type == IAPD) {
					radvd_parse(&client6_iaidaddr, ADDR_UPDATE);
                    /*  added start pling 10/12/2010 */
                    /* 1. Execute callback now as IAPD only does not need DAD.
                     * 2. Send Info-req to get additional info
                     */
                    dhcp6c_dad_callback();
                    newstate = DHCP6S_INFOREQ;
                    /*  added end pling 10/12/2010 */
                }
				else if (ifp->dad_timer == NULL && (ifp->dad_timer =
					  dhcp6_add_timer(check_dad_timo, ifp)) < 0) {
					dprintf(LOG_INFO, "%s" "failed to create a timer for "
						" DAD", FNAME); 
				}
				setup_check_timer(ifp);
                /*  removed start pling 10/05/2010 */
                /* WNR3500L TD#175, send info-req after we complete the 
                 *  DAD check. */
                //client6_send_info_req(ev);
                /*  removed end pling 10/05/2010 */
			}
			break;
		}
		break;
	case DHCP6S_RENEW:
	case DHCP6S_REBIND:
		if (client6_request_flag & CLIENT6_CONFIRM_ADDR) 
			goto rebind_confirm;
		/* NoBinding for RENEW, REBIND, send REQUEST */
		switch(addr_status_code) {
		case DH6OPT_STCODE_NOBINDING:
		/*  modified start pling 10/01/2014 */
		/* R7000 TD486: WAN IPv6 address not update if receive 
		 * status code "Not-On-Link", and "No-binding"
		 * Copy code from above "NOTONLINK" handling */
#if 0
			newstate = DHCP6S_REQUEST;
			dprintf(LOG_DEBUG, "%s" 
			    	  "got a NoBinding reply, sending request.", FNAME);
			dhcp6_remove_iaidaddr(&client6_iaidaddr);
			break;
#endif
		case DH6OPT_STCODE_NOTONLINK:
		case DH6OPT_STCODE_NOADDRAVAIL:
		case DH6OPT_STCODE_NOPREFIXAVAIL:
		case DH6OPT_STCODE_UNSPECFAIL:
			dprintf(LOG_DEBUG, "%s" "got a NotOnLink reply for renew/rebind", FNAME);
			dhcp6_remove_iaidaddr(&client6_iaidaddr);
			not_on_link_count++;
			if (not_on_link_count <= REQ_MAX_RC_NOTONLINK) {
				/* Clear the IA / PD address, so they won't appear in the
				 * request pkt. */
				dhcp6_clear_list(&(ifp->current_server->optinfo.addr_list));
				dhcp6_clear_list(&(ifp->current_server->optinfo.prefix_list));
				newstate = DHCP6S_REQUEST;
			} else {
				/* Three times, back to SOLICIT state */
				not_on_link_count = 0;
				free_servers(ifp);
				newstate = DHCP6S_SOLICIT;
			}
			break;
		/*  modified end pling 10/01/2014 */

		/*  removed start pling 10/01/2014 */
		/* Handle these status codes above */
#if 0
		case DH6OPT_STCODE_NOADDRAVAIL:
		case DH6OPT_STCODE_NOPREFIXAVAIL:
		case DH6OPT_STCODE_UNSPECFAIL:
			break;
#endif
		/*  removed end pling 10/01/2014 */

		case DH6OPT_STCODE_SUCCESS:
		case DH6OPT_STCODE_UNDEFINE:
		default:
			dhcp6_update_iaidaddr(optinfo, ADDR_UPDATE);
			if (optinfo->type == IAPD)
				radvd_parse(&client6_iaidaddr, ADDR_UPDATE);
			/*  added start pling 12/22/2011 */
			/* WNDR4500 TD#156: Send signal to radvd to refresh 
			 * the prefix lifetime */
			system("killall -SIGUSR1 radvd");
			/*  added end pling 12/22/2011 */
            /*  added start pling 01/25/2010 */
            /* Send info-req to get SIP server and NTP server */
            client6_send_info_req(ev);
            /*  added end pling 01/25/2010 */
			break;
		}
		break;
	case DHCP6S_CONFIRM:
		/* NOtOnLink for a Confirm, send SOLICIT message */
rebind_confirm:	client6_request_flag &= ~CLIENT6_CONFIRM_ADDR;
	switch(addr_status_code) {
		struct timeb now;
		struct timeval timo;
		time_t offset;
		case DH6OPT_STCODE_NOTONLINK:
		case DH6OPT_STCODE_NOBINDING:
		case DH6OPT_STCODE_NOADDRAVAIL:
			dprintf(LOG_DEBUG, "%s" 
				"got a NotOnLink reply for confirm, sending solicit.", FNAME);
			/* remove event data list */
			free_servers(ifp);
			newstate = DHCP6S_SOLICIT;
			break;
		case DH6OPT_STCODE_SUCCESS:
		case DH6OPT_STCODE_UNDEFINE:
			dprintf(LOG_DEBUG, "%s" "got an expected reply for confirm", FNAME);
			ftime(&now);
			client6_iaidaddr.state = ACTIVE;
			if ((client6_iaidaddr.timer = dhcp6_add_timer(dhcp6_iaidaddr_timo, 
						&client6_iaidaddr)) == NULL) {
		 		dprintf(LOG_ERR, "%s" "failed to add a timer for iaid %u",
					FNAME, client6_iaidaddr.client6_info.iaidinfo.iaid);
		 		return (-1);
			}
			if (client6_iaidaddr.client6_info.iaidinfo.renewtime == 0) {
				client6_iaidaddr.client6_info.iaidinfo.renewtime 
					= get_min_preferlifetime(&client6_iaidaddr)/2;
			}
			if (client6_iaidaddr.client6_info.iaidinfo.rebindtime == 0) {
				client6_iaidaddr.client6_info.iaidinfo.rebindtime 
					= (get_min_preferlifetime(&client6_iaidaddr)*4)/5;
			}
			offset = now.time - client6_iaidaddr.start_date;
			if ( offset > client6_iaidaddr.client6_info.iaidinfo.renewtime) 
				timo.tv_sec = 0;
			else
				timo.tv_sec = client6_iaidaddr.client6_info.iaidinfo.renewtime 						- offset; 
			timo.tv_usec = 0;
			dhcp6_set_timer(&timo, client6_iaidaddr.timer);
			/* check DAD */
			if (optinfo->type != IAPD && ifp->dad_timer == NULL && 
			    (ifp->dad_timer = dhcp6_add_timer(check_dad_timo, ifp)) < 0) {
				dprintf(LOG_INFO, "%s" "failed to create a timer for "
					" DAD", FNAME); 
			}
			setup_check_timer(ifp);
			break;
		default:
			break;
		}
		break;
	case DHCP6S_DECLINE:
		/* send REQUEST message to server with none decline address */
		dprintf(LOG_DEBUG, "%s" 
		    "got an expected reply for decline, sending request.", FNAME);
        /*  modified start pling 10/04/2010 */
        /* Should restart the 4-packet process, from SOLICIT */
#if 0
		create_request_list(0);
		/* remove event data list */
		newstate = DHCP6S_REQUEST;
#endif
        free_servers(ifp);
        newstate = DHCP6S_SOLICIT;
        /*  modified end pling 10/04/2010 */
		break;
	case DHCP6S_RELEASE:
		dprintf(LOG_INFO, "%s" "got an expected release, exit.", FNAME);
		dhcp6_remove_event(ev);
		exit(0);
	default:
		break;
	}
	dhcp6_remove_event(ev);
	if (newstate) {
		client6_send_newstate(ifp, newstate);
	} else 
		dprintf(LOG_DEBUG, "%s" "got an expected reply, sleeping.", FNAME);
	TAILQ_INIT(&request_list);
	return 0;
}

int 
client6_send_newstate(ifp, state)
	struct dhcp6_if *ifp;
	int state;
{
	struct dhcp6_event *ev;
	if ((ev = dhcp6_create_event(ifp, state)) == NULL) {
		dprintf(LOG_ERR, "%s" "failed to create an event",
			FNAME);
		return (-1);
	}
	if ((ev->timer = dhcp6_add_timer(client6_timo, ev)) == NULL) {
		dprintf(LOG_ERR, "%s" "failed to add a timer for %s",
			FNAME, ifp->ifname);
		free(ev);
		return(-1);
	}
	TAILQ_INSERT_TAIL(&ifp->event_list, ev, link);
	ev->timeouts = 0;
	dhcp6_set_timeoparam(ev);
    /*  added start pling 10/07/2010 */
    /* WNR3500L TD170, modify the maximum re-send counter of 
     *  Request message to 3 if a "NotOnLink" status is
     *  received. 
     */
    if (state == DHCP6S_REQUEST && not_on_link_count)
        ev->max_retrans_cnt = REQ_MAX_RC_NOTONLINK;
    /*  added end pling 10/07/2010 */
	dhcp6_reset_timer(ev);

    /*  modified start pling 10/05/2010 */
    /* Use diff function to send INFO-REQ */
    if (state == DHCP6S_INFOREQ)
        client6_send_info_req(ev);
    else
        client6_send(ev);
    /*  modified end pling 10/05/2010 */
	return 0;
}

static struct dhcp6_event *
find_event_withid(ifp, xid)
	struct dhcp6_if *ifp;
	u_int32_t xid;
{
	struct dhcp6_event *ev;

	for (ev = TAILQ_FIRST(&ifp->event_list); ev;
	     ev = TAILQ_NEXT(ev, link)) {
		dprintf(LOG_DEBUG, "%s" "ifp %p event %p id is %x", 
			FNAME, ifp, ev, ev->xid);
		if (ev->xid == xid)
			return (ev);
	}

	return (NULL);
}

static int 
create_request_list(int reboot)
{	
	struct dhcp6_lease *cl;
	struct dhcp6_listval *lv;
	/* create an address list for release all/confirm */
	for (cl = TAILQ_FIRST(&client6_iaidaddr.lease_list); cl; 
		cl = TAILQ_NEXT(cl, link)) {
		/* IANA, IAPD */
		if ((lv = malloc(sizeof(*lv))) == NULL) {
			dprintf(LOG_ERR, "%s" 
				"failed to allocate memory for an ipv6 addr", FNAME);
			 exit(1);
		}
		memcpy(&lv->val_dhcp6addr, &cl->lease_addr, 
			sizeof(lv->val_dhcp6addr));
		lv->val_dhcp6addr.status_code = DH6OPT_STCODE_UNDEFINE;
		TAILQ_INSERT_TAIL(&request_list, lv, link);
		/* config the interface for reboot */
		if (reboot && client6_iaidaddr.client6_info.type != IAPD && 
		    (client6_request_flag & CLIENT6_CONFIRM_ADDR)) {
			if (client6_ifaddrconf(IFADDRCONF_ADD, &cl->lease_addr) != 0) {
				dprintf(LOG_INFO, "config address failed: %s",
					in6addr2str(&cl->lease_addr.addr, 0));
				return (-1);
			}
		}
	}
	/* update radvd.conf for prefix delegation */
	if (reboot && client6_iaidaddr.client6_info.type == IAPD &&
	    (client6_request_flag & CLIENT6_CONFIRM_ADDR))
		radvd_parse(&client6_iaidaddr, ADDR_UPDATE);
	return (0);
}

static void setup_check_timer(struct dhcp6_if *ifp)
{
	double d;
	struct timeval timo;
	d = DHCP6_CHECKLINK_TIME;
	timo.tv_sec = (long)d;
	timo.tv_usec = 0;
	dprintf(LOG_DEBUG, "set timer for checking link ...");
	dhcp6_set_timer(&timo, ifp->link_timer);
	if (ifp->dad_timer != NULL) {
		d = DHCP6_CHECKDAD_TIME;
		timo.tv_sec = (long)d;
		timo.tv_usec = 0;
		dprintf(LOG_DEBUG, "set timer for checking DAD ...");
		dhcp6_set_timer(&timo, ifp->dad_timer);
	}
	d = DHCP6_SYNCFILE_TIME;
	timo.tv_sec = (long)d;
	timo.tv_usec = 0;
	dprintf(LOG_DEBUG, "set timer for syncing file ...");
	dhcp6_set_timer(&timo, ifp->sync_timer);
	return;
}

static struct dhcp6_timer
*check_lease_file_timo(void *arg)
{
	struct dhcp6_if *ifp = (struct dhcp6_if *)arg;
	double d;
	struct timeval timo;
	struct stat buf;
	FILE *file;
	stat(leasename, &buf);
	strcpy(client6_lease_temp, leasename);
	strcat(client6_lease_temp, "XXXXXX");
	if (buf.st_size > MAX_FILE_SIZE) {
		file = sync_leases(client6_lease_file, leasename, client6_lease_temp);
		if ( file != NULL)
			client6_lease_file = file;
	}	
	d = DHCP6_SYNCFILE_TIME;
	timo.tv_sec = (long)d;
	timo.tv_usec = 0;
	dhcp6_set_timer(&timo, ifp->sync_timer);
	return ifp->sync_timer;
}

static struct dhcp6_timer
*check_dad_timo(void *arg)
{
	struct dhcp6_if *ifp = (struct dhcp6_if *)arg;
	int newstate = DHCP6S_REQUEST;   //  modified pling 10/04/2010
	if (client6_iaidaddr.client6_info.type == IAPD)
		goto end;
	dprintf(LOG_DEBUG, "enter checking dad ...");
	if (dad_parse(ifproc_file) < 0) {
		dprintf(LOG_ERR, "parse /proc/net/if_inet6 failed");
		goto end;
	}
	if (TAILQ_EMPTY(&request_list))
		goto end;
	/* remove RENEW timer for client6_iaidaddr */
	if (client6_iaidaddr.timer != NULL)
		dhcp6_remove_timer(client6_iaidaddr.timer);
	newstate = DHCP6S_DECLINE;
	client6_send_newstate(ifp, newstate);
end:
	/* one time check for DAD */	
	dhcp6_remove_timer(ifp->dad_timer);
	ifp->dad_timer = NULL;

    /*  added start pling 10/04/2010 */
    /* Send info-req to get DNS/SIP/NTP, etc, per Netgear spec. */
    if (newstate != DHCP6S_DECLINE) {
        dhcp6c_dad_callback();
        dprintf(LOG_DEBUG, "send info-req");
        newstate = DHCP6S_INFOREQ;
        client6_send_newstate(ifp, newstate);
    }
    /*  added end pling 10/04/2010 */

	return NULL;
}
	
static struct dhcp6_timer 
*check_link_timo(void *arg)
{
	struct dhcp6_if *ifp = (struct dhcp6_if *)arg;
	struct ifreq ifr;
	struct timeval timo;
	double d;
	int newstate;
	dprintf(LOG_DEBUG, "enter checking link ...");
	strncpy(ifr.ifr_name, dhcp6_if->ifname, IFNAMSIZ);
	if (ioctl(nlsock, SIOCGIFFLAGS, &ifr) < 0) {
		dprintf(LOG_DEBUG, "ioctl SIOCGIFFLAGS failed");
		goto settimer;
	}
	if (ifr.ifr_flags & IFF_RUNNING) {
		/* check previous flag 
		 * set current flag UP */
		if (ifp->link_flag & IFF_RUNNING) {
			goto settimer;
		}
		/* check current state ACTIVE */
		if (client6_iaidaddr.state == ACTIVE) {
			/* remove timer for renew/rebind
			 * send confirm for ipv6address or 
			 * rebind for prefix delegation */
			dhcp6_remove_timer(client6_iaidaddr.timer);
			client6_request_flag &= CLIENT6_CONFIRM_ADDR;
			create_request_list(0);
			if (client6_iaidaddr.client6_info.type == IAPD)
				newstate = DHCP6S_REBIND;
			else
				newstate = DHCP6S_CONFIRM;
			client6_send_newstate(ifp, newstate);
		}
		dprintf(LOG_INFO, "interface is from down to up");
		ifp->link_flag |= IFF_RUNNING;
	} else {
		dprintf(LOG_INFO, "interface is down");
		/* set flag_prev flag DOWN */
		ifp->link_flag &= ~IFF_RUNNING;
	}
settimer:
	d = DHCP6_CHECKLINK_TIME;
	timo.tv_sec = (long)d;
	timo.tv_usec = 0;
	dhcp6_set_timer(&timo, ifp->link_timer);
	return ifp->link_timer;
}

static void
setup_interface(char *ifname)
{
	struct ifreq ifr;
	/* check the interface */
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
again:
	if (ioctl(nlsock, SIOCGIFFLAGS, &ifr) < 0) {
		dprintf(LOG_ERR, "ioctl SIOCGIFFLAGS failed");
		exit(1);
	}
	if (!ifr.ifr_flags & IFF_UP) {
		ifr.ifr_flags |= IFF_UP;
		if (ioctl(nlsock, SIOCSIFFLAGS, &ifr) < 0) {
			dprintf(LOG_ERR, "ioctl SIOCSIFFLAGS failed");
			exit(1);
		}
		if (ioctl(nlsock, SIOCGIFFLAGS, &ifr) < 0) {
			dprintf(LOG_ERR, "ioctl SIOCGIFFLAGS failed");
			exit(1);
		}
	}
	if (!ifr.ifr_flags & IFF_RUNNING) {
		dprintf(LOG_INFO, "NIC is not connected to the network, "
			"please connect it. dhcp6c is sleeping ...");
		sleep(10);
		goto again;
	}
	return;
}

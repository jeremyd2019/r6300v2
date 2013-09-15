/*	$Id: config.h,v 1.1.1.1 2006/12/04 00:45:21 Exp $	*/
/*	ported from KAME: config.h,v 1.18 2002/06/14 15:32:55 jinmei Exp */

/*
 * Copyright (C) 2002 WIDE Project.
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

#define MAX_DEVICE 100

struct hardware {
	u_int16_t type;
	u_int8_t len;
	unsigned char data[6];
};

struct iaid_table {
	/* so far we support ethernet cards only */
	struct hardware  hwaddr;
	u_int32_t iaid;
};

struct ra_info {
	struct ra_info *next;
	struct in6_addr prefix;
	int plen;
	int flags;
};

struct dhcp6_option {
	TAILQ_ENTRY(dhcp6_option) link;
	int type;
	int len;
	void *val;
};

TAILQ_HEAD(dhcp6_option_list, dhcp6_option);

/* per-interface information */
struct dhcp6_if {
	struct dhcp6_if *next;

	int outsock;

	/* timer for the interface to sync file every 5 mins*/
	struct dhcp6_timer *sync_timer;
#define DHCP6_SYNCFILE_TIME	60
	/* timer to check interface off->on link to send confirm message*/
	struct dhcp6_timer *link_timer;	
#define DHCP6_CHECKLINK_TIME	5	
	struct dhcp6_timer *dad_timer;
#define DHCP6_CHECKDAD_TIME	5	
	/* event queue */
	TAILQ_HEAD(, dhcp6_event) event_list;	

	/* static parameters of the interface */
	char *ifname;
	unsigned int ifid;
	struct ra_info *ralist;
	struct dns_list dnslist;
	struct dhcp6_list siplist;
	struct dhcp6_list ntplist;
	u_int32_t linkid;	/* to send link-local packets */
	struct dhcp6_iaid_info iaidinfo;	
	
	u_int16_t ra_flag;
	u_int16_t link_flag;	
	/* configuration parameters */
	u_long send_flags;
	u_long allow_flags;

#define DHCIFF_INFO_ONLY 0x1
#define DHCIFF_RAPID_COMMIT 0x2
#define DHCIFF_TEMP_ADDRS 0x4
#define DHCIFF_PREFIX_DELEGATION 0x8
#define DHCIFF_UNICAST 0x10
#define DHCIFF_SOLICIT_ONLY 0x20		//  added pling 08/26/2009
#define DHCIFF_IANA_ONLY    0x40		//  added pling 09/21/2010
#define DHCIFF_IAPD_ONLY    0x80		//  added pling 09/21/2010

	struct in6_addr linklocal;
	int server_pref;	/* server preference (server only) */

	struct dhcp6_list reqopt_list;
	/* request specific addresses list from client */
	struct dhcp6_list addr_list;
	struct dhcp6_list prefix_list;
	struct dhcp6_option_list option_list;
	struct dhcp6_serverinfo *current_server;
	struct dhcp6_serverinfo *servers;

	char user_class[MAX_USER_CLASS_LEN];	//  added pling 09/07/2010
};

struct dhcp6_event {
	TAILQ_ENTRY(dhcp6_event) link;

	struct dhcp6_if *ifp;
	struct dhcp6_timer *timer;

	struct duid serverid;

	/* internal timer parameters */
	struct timeval start_time;
	long retrans;
	long init_retrans;
	long max_retrans_cnt;
	long max_retrans_time;
	long max_retrans_dur;
	int timeouts;		/* number of timeouts */

	u_int32_t xid;		/* current transaction ID */
	int state;

	TAILQ_HEAD(, dhcp6_eventdata) data_list;
};

typedef enum { DHCP6_DATA_PREFIX, DHCP6_DATA_ADDR } dhcp6_eventdata_t;

struct dhcp6_eventdata {
	TAILQ_ENTRY(dhcp6_eventdata) link;

	struct dhcp6_event *event;
	dhcp6_eventdata_t type;
	void *data;
};

struct dhcp6_serverinfo {
	struct dhcp6_serverinfo *next;

	/* option information provided in the advertisement */
	struct dhcp6_optinfo optinfo;
	struct in6_addr server_addr;
	u_int8_t pref;		/* preference */
	int active;		/* bool; if this server is active or not */
	/* TODO: remember available information from the server */
};

/* client status code */
enum {DHCP6S_INIT, DHCP6S_SOLICIT, DHCP6S_INFOREQ, DHCP6S_REQUEST,
      DHCP6S_RENEW, DHCP6S_REBIND, DHCP6S_CONFIRM, DHCP6S_DECLINE,
      DHCP6S_RELEASE, DHCP6S_IDLE};
      
struct dhcp6_ifconf {
	struct dhcp6_ifconf *next;

	char *ifname;

	/* configuration flags */
	u_long send_flags;
	u_long allow_flags;

	int server_pref;	/* server preference (server only) */
	struct dhcp6_iaid_info iaidinfo;

	struct dhcp6_list prefix_list;
	struct dhcp6_list addr_list;
	struct dhcp6_list reqopt_list;

	struct dhcp6_option_list option_list;
	
	char user_class[MAX_USER_CLASS_LEN];	//  added pling 09/07/2010
};

struct prefix_ifconf {
	struct prefix_ifconf *next;

	char *ifname;		/* interface name such as eth0 */
	int sla_len;		/* SLA ID length in bits */
	u_int32_t sla_id;	/* need more than 32bits? */
	int ifid_len;		/* interface ID length in bits */
	int ifid_type;		/* EUI-64 and manual (unused?) */
	char ifid[16];		/* Interface ID, up to 128bits */
};
#define IFID_LEN_DEFAULT 64
#define SLA_LEN_DEFAULT 16

/* per-host configuration */
struct host_conf {
	struct host_conf *next;

	char *name;		/* host name to identify the host */
	struct duid duid;	/* DUID for the host */
	struct dhcp6_iaid_info iaidinfo;
	struct in6_addr linklocal;
	/* delegated prefixes for the host: */
	struct dhcp6_list prefix_list;

	/* bindings of delegated prefixes */
	struct dhcp6_list prefix_binding_list;

	struct dhcp6_list addr_list;
	struct dhcp6_list addr_binding_list;
};

/* structures and definitions used in the config file parser */
struct cf_namelist {
	struct cf_namelist *next;
	char *name;
	int line;		/* the line number of the config file */
	struct cf_list *params;
};

struct cf_list {
	struct cf_list *next;
	struct cf_list *tail;
	int type;
	int line;		/* the line number of the config file */

	/* type dependent values: */
	long long num;
	struct cf_list *list;
	void *ptr;
};

/* Some systems define thes in in.h */
#ifndef IN6_IS_ADDR_UNSPECIFIED
#define IN6_IS_ADDR_UNSPECIFIED(a) \
	(((__const u_int32_t *) (a))[0] == 0				\
	 && ((__const u_int32_t *) (a))[1] == 0				\
	 && ((__const u_int32_t *) (a))[2] == 0				\
	 && ((__const u_int32_t *) (a))[3] == 0)
#endif
	
#ifndef IN6_IS_ADDR_LOOPBACK
#define IN6_IS_ADDR_LOOPBACK(a) \
	(((__const u_int32_t *) (a))[0] == 0				\
	 && ((__const u_int32_t *) (a))[1] == 0				\
	 && ((__const u_int32_t *) (a))[2] == 0				\
	 && ((__const u_int32_t *) (a))[3] == htonl (1))
#endif

#ifndef IN6_IS_ADDR_MULTICAST
#define IN6_IS_ADDR_MULTICAST(a) (((__const u_int8_t *) (a))[0] == 0xff)
#endif

#ifndef IN6_IS_ADDR_LINKLOCAL
#define IN6_IS_ADDR_LINKLOCAL(a) \
	((((__const u_int32_t *) (a))[0] & htonl (0xffc00000))		\
	 == htonl (0xfe800000))
#endif

#ifndef IN6_IS_ADDR_SITELOCAL
#define IN6_IS_ADDR_SITELOCAL(a) \
	((((__const u_int32_t *) (a))[0] & htonl (0xffc00000))		\
	 == htonl (0xfec00000))
#endif

#ifndef IN6_ARE_ADDR_EQUAL
#define IN6_ARE_ADDR_EQUAL(a,b) \
	((((__const u_int32_t *) (a))[0] == ((__const u_int32_t *) (b))[0])     \
	 && (((__const u_int32_t *) (a))[1] == ((__const u_int32_t *) (b))[1])  \
	 && (((__const u_int32_t *) (a))[2] == ((__const u_int32_t *) (b))[2])  \
	 && (((__const u_int32_t *) (a))[3] == ((__const u_int32_t *) (b))[3]))
#endif

#ifndef IN6_IS_ADDR_RESERVED
#define IN6_IS_ADDR_RESERVED(a) \
	IN6_IS_ADDR_MULTICAST(a) || IN6_IS_ADDR_LOOPBACK(a) 		\
	|| IN6_IS_ADDR_UNSPECIFIED(a)			
#endif
/* ANYCAST later */

enum {DECL_SEND, DECL_ALLOW, DECL_INFO_ONLY, DECL_TEMP_ADDR, DECL_REQUEST, DECL_DUID, DECL_SOLICIT_ONLY,
      DECL_IANA_ONLY, DECL_IAPD_ONLY,
      DECL_PREFIX, DECL_PREFERENCE, DECL_IAID, DECL_RENEWTIME, DECL_REBINDTIME,
      DECL_ADDRESS, DECL_LINKLOCAL, DECL_PREFIX_INFO, DECL_PREFIX_REQ, DECL_PREFIX_DELEGATION_INTERFACE, DECL_USER_CLASS,
      DECL_XID_SOL, DECL_XID_REQ, DECL_DUID_TIME,
      DHCPOPT_PREFIX_DELEGATION, IFPARAM_SLA_ID, IFPARAM_SLA_LEN,
      DHCPOPT_RAPID_COMMIT, 
      DHCPOPT_DNS, DHCPOPT_DOMAIN_LIST, DHCPOPT_SIP, DHCPOPT_NTP, ADDRESS_LIST_ENT };

typedef enum {DHCP6_MODE_SERVER, DHCP6_MODE_CLIENT, DHCP6_MODE_RELAY }
dhcp6_mode_t;

extern const dhcp6_mode_t dhcp6_mode;
extern struct cf_list *cf_dns_list;
extern const char *configfilename;

extern struct dhcp6_if *dhcp6_if;
extern struct dhcp6_ifconf *dhcp6_iflist;
extern struct prefix_ifconf *prefix_ifconflist;
extern struct dns_list dnslist;
extern struct dhcp6_list siplist;
extern struct dhcp6_list ntplist;

extern int configure_interface (const struct cf_namelist *);
extern int configure_prefix_interface (struct cf_namelist *);
extern int configure_host (const struct cf_namelist *);
extern int configure_global_option (void);
extern void configure_cleanup (void);
extern void configure_commit (void);
extern int cfparse (const char *);
extern int resolv_parse (struct dns_list *);
extern int get_if_rainfo(struct dhcp6_if *ifp);

extern void *get_if_option( struct dhcp6_option_list *, int);

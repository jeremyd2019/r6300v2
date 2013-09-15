/*	$Id: common.c,v 1.1.1.1 2006/12/04 00:45:20 Exp $	*/
/*	ported from KAME: common.c,v 1.65 2002/12/06 01:41:29 suz Exp	*/

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
#include <linux/sockios.h>
#include <sys/ioctl.h>

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#include <net/if.h>
#if defined(__FreeBSD__) && __FreeBSD__ >= 3
#include <net/if_var.h>
#endif
#include <net/if_arp.h>

#include <netinet/in.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <err.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <resolv.h>

#ifdef HAVE_GETIFADDRS 
# ifdef HAVE_IFADDRS_H
#  define USE_GETIFADDRS
#  include <ifaddrs.h>
# endif
#endif

#include "queue.h"
#include "dhcp6.h"
#include "config.h"
#include "common.h"
#include "timer.h"
#include "lease.h"

int foreground;
int debug_thresh;
struct dhcp6_if *dhcp6_if;
struct dns_list dnslist;
/*  added start pling 01/25/2010 */
struct dhcp6_list siplist;
struct dhcp6_list ntplist;
/*  added end pling 01/25/2010 */
static struct host_conf *host_conflist;
static int in6_matchflags __P((struct sockaddr *, char *, int));
ssize_t gethwid __P((char *, int, const char *, u_int16_t *));
static int get_assigned_ipv6addrs __P((char *, char *,
					struct dhcp6_optinfo *));

/*  added start pling 10/07/2010 */
/* For testing purpose */
u_int32_t duid_time = 0;
/*  added end pling 10/07/2010 */

/*  added start pling 09/21/2010 */
/* Global flags for dhcpc configuration, e.g. IANA_ONLY, IAPD_ONLY */
static u_int32_t   dhcp6c_flags = 0;
int set_dhcp6c_flags(u_int32_t flags)
{
    dhcp6c_flags |= flags;
    return 0;
}
/*  added end pling 09/21/2010 */

struct dhcp6_if *
find_ifconfbyname(const char *ifname)
{
	struct dhcp6_if *ifp;

	for (ifp = dhcp6_if; ifp; ifp = ifp->next) {
		if (strcmp(ifp->ifname, ifname) == 0)
			return (ifp);
	}

	return (NULL);
}

struct dhcp6_if *
find_ifconfbyid(unsigned int id)
{
	struct dhcp6_if *ifp;

	for (ifp = dhcp6_if; ifp; ifp = ifp->next) {
		if (ifp->ifid == id)
			return (ifp);
	}

	return (NULL);
}

struct host_conf *
find_hostconf(const struct duid *duid)
{
	struct host_conf *host;

	for (host = host_conflist; host; host = host->next) {
		if (host->duid.duid_len == duid->duid_len &&
		    memcmp(host->duid.duid_id, duid->duid_id,
			   host->duid.duid_len) == 0) {
			return (host);
		}
	}

	return (NULL);
}
void
ifinit(const char *ifname)
{
	struct dhcp6_if *ifp;

	if ((ifp = find_ifconfbyname(ifname)) != NULL) {
		dprintf(LOG_NOTICE, "%s" "duplicated interface: %s",
			FNAME, ifname);
		return;
	}

	if ((ifp = malloc(sizeof(*ifp))) == NULL) {
		dprintf(LOG_ERR, "%s" "malloc failed", FNAME);
		goto die;
	}
	memset(ifp, 0, sizeof(*ifp));

	TAILQ_INIT(&ifp->event_list);

	if ((ifp->ifname = strdup(ifname)) == NULL) {
		dprintf(LOG_ERR, "%s" "failed to copy ifname", FNAME);
		goto die;
	}

	if ((ifp->ifid = if_nametoindex(ifname)) == 0) {
		dprintf(LOG_ERR, "%s" "invalid interface(%s): %s", FNAME,
			ifname, strerror(errno));
		goto die;
	}
#ifdef HAVE_SCOPELIB
	if (inet_zoneid(AF_INET6, 2, ifname, &ifp->linkid)) {
		dprintf(LOG_ERR, "%s" "failed to get link ID for %s",
			FNAME, ifname);
		goto die;
	}
#else
	ifp->linkid = ifp->ifid;
#endif
	if (get_linklocal(ifname, &ifp->linklocal) < 0)
		goto die;
	ifp->next = dhcp6_if;
	dhcp6_if = ifp;
	return;

  die:
	exit(1);
}

int
dhcp6_copy_list(struct dhcp6_list *dst, 
		const struct dhcp6_list *src)
{
	const struct dhcp6_listval *ent;
	struct dhcp6_listval *dent;

	for (ent = TAILQ_FIRST(src); ent; ent = TAILQ_NEXT(ent, link)) {
		if ((dent = malloc(sizeof(*dent))) == NULL)
			goto fail;

		memset(dent, 0, sizeof(*dent));
		memcpy(&dent->uv, &ent->uv, sizeof(ent->uv));

		TAILQ_INSERT_TAIL(dst, dent, link);
	}

	return 0;

  fail:
	dhcp6_clear_list(dst);
	return -1;
}

void
dhcp6_clear_list(head)
	struct dhcp6_list *head;
{
	struct dhcp6_listval *v;

	while ((v = TAILQ_FIRST(head)) != NULL) {
		TAILQ_REMOVE(head, v, link);
		free(v);
	}

	return;
}

void
relayfree(head)
	struct relay_list *head;
{
	struct relay_listval *v;

	while ((v = TAILQ_FIRST(head)) != NULL) {
		TAILQ_REMOVE(head, v, link);
		if (v->intf_id != NULL) {
			if (v->intf_id->intf_id != NULL) 
				free(v->intf_id->intf_id);
			free (v->intf_id);
		}
		free(v);
	}

	return;
}

int
dhcp6_count_list(head)
	struct dhcp6_list *head;
{
	struct dhcp6_listval *v;
	int i;

	for (i = 0, v = TAILQ_FIRST(head); v; v = TAILQ_NEXT(v, link))
		i++;

	return i;
}

struct dhcp6_listval *
dhcp6_find_listval(head, val, type)
	struct dhcp6_list *head;
	void *val;
	dhcp6_listval_type_t type;
{
	struct dhcp6_listval *lv;

	for (lv = TAILQ_FIRST(head); lv; lv = TAILQ_NEXT(lv, link)) {
		switch(type) {
		case DHCP6_LISTVAL_NUM:
			if (lv->val_num == *(int *)val)
				return (lv);
			break;
		case DHCP6_LISTVAL_ADDR6:
			if (IN6_ARE_ADDR_EQUAL(&lv->val_addr6,
			    (struct in6_addr *)val)) {
				return (lv);
			}
			break;
		case DHCP6_LISTVAL_DHCP6ADDR:
			if (IN6_ARE_ADDR_EQUAL(&lv->val_dhcp6addr.addr,
			    &((struct dhcp6_addr *)val)->addr) && 
			    (lv->val_dhcp6addr.plen == ((struct dhcp6_addr *)val)->plen)) {
				return (lv);
			}
			break;
		/* DHCP6_LISTVAL_DHCP6LEASE is missing? */
		}

	}

	return (NULL);
}

struct dhcp6_listval *
dhcp6_add_listval(head, val, type)
	struct dhcp6_list *head;
	void *val;
	dhcp6_listval_type_t type;
{
	struct dhcp6_listval *lv;

	if ((lv = malloc(sizeof(*lv))) == NULL) {
		dprintf(LOG_ERR, "%s" "failed to allocate memory for list "
		    "entry", FNAME);
		return (NULL);
	}
	memset(lv, 0, sizeof(*lv));

	switch(type) {
	case DHCP6_LISTVAL_NUM:
		lv->val_num = *(int *)val;
		break;
	case DHCP6_LISTVAL_ADDR6:
		lv->val_addr6 = *(struct in6_addr *)val;
		break;
	case DHCP6_LISTVAL_DHCP6ADDR:
		lv->val_dhcp6addr = *(struct dhcp6_addr *)val;
		break;
	default:
		dprintf(LOG_ERR, "%s" "unexpected list value type (%d)",
		    FNAME, type);
		return (NULL);
	}
	TAILQ_INSERT_TAIL(head, lv, link);
	return (lv);
}

struct dhcp6_event *
dhcp6_create_event(ifp, state)
	struct dhcp6_if *ifp;
	int state;
{
	struct dhcp6_event *ev;

	if ((ev = malloc(sizeof(*ev))) == NULL) {
		dprintf(LOG_ERR, "%s" "failed to allocate memory for an event",
			FNAME);
		return (NULL);
	}
	/* for safety */
	memset(ev, 0, sizeof(*ev));
	ev->serverid.duid_id = NULL;
	
	ev->ifp = ifp;
	ev->state = state;
	TAILQ_INIT(&ev->data_list);
	dprintf(LOG_DEBUG, "%s" "create an event %p xid %d for state %d", 
		FNAME, ev, ev->xid, ev->state);
	return (ev);
}

void
dhcp6_remove_event(ev)
	struct dhcp6_event *ev;
{
	dprintf(LOG_DEBUG, "%s" "removing an event %p on %s, state=%d, xid=%x", FNAME,
		ev, ev->ifp->ifname, ev->state, ev->xid);

	if (!TAILQ_EMPTY(&ev->data_list)) {
		dprintf(LOG_ERR, "%s" "assumption failure: "
			"event data list is not empty", FNAME);
		exit(1);
	}
	if (ev->serverid.duid_id != NULL)
		duidfree(&ev->serverid);
	if (ev->timer)
		dhcp6_remove_timer(ev->timer);
	TAILQ_REMOVE(&ev->ifp->event_list, ev, link);
	free(ev);
	ev = NULL;
}


int
getifaddr(addr, ifnam, prefix, plen, strong, ignoreflags)
	struct in6_addr *addr;
	char *ifnam;
	struct in6_addr *prefix;
	int plen;
	int strong;		/* if strong host model is required or not */
	int ignoreflags;
{
	struct ifaddrs *ifap, *ifa;
	struct sockaddr_in6 sin6;
	int error = -1;

	if (getifaddrs(&ifap) != 0) {
		err(1, "getifaddr: getifaddrs");
		/*NOTREACHED*/
	}

	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		int s1, s2;

		if (strong && strcmp(ifnam, ifa->ifa_name) != 0)
			continue;

		/* in any case, ignore interfaces in different scope zones. */
		if ((s1 = in6_addrscopebyif(prefix, ifnam)) < 0 ||
		    (s2 = in6_addrscopebyif(prefix, ifa->ifa_name)) < 0 ||
		     s1 != s2)
			continue;

		if (ifa->ifa_addr->sa_family != AF_INET6)
			continue;
		if (sizeof(*(ifa->ifa_addr)) > sizeof(sin6))
			continue;

		if (in6_matchflags(ifa->ifa_addr, ifa->ifa_name, ignoreflags))
			continue;

		memcpy(&sin6, ifa->ifa_addr, sizeof(sin6));
#ifdef __KAME__
		if (IN6_IS_ADDR_LINKLOCAL(&sin6.sin6_addr)) {
			sin6.sin6_addr.s6_addr[2] = 0;
			sin6.sin6_addr.s6_addr[3] = 0;
		}
#endif
		if (plen % 8 == 0) {
			if (memcmp(&sin6.sin6_addr, prefix, plen / 8) != 0)
				continue;
		} else {
			struct in6_addr a, m;
			int i;

			memcpy(&a, &sin6.sin6_addr, sizeof(a));
			memset(&m, 0, sizeof(m));
			memset(&m, 0xff, plen / 8);
			m.s6_addr[plen / 8] = (0xff00 >> (plen % 8)) & 0xff;
			for (i = 0; i < sizeof(a); i++)
				a.s6_addr[i] &= m.s6_addr[i];

			if (memcmp(&a, prefix, plen / 8) != 0 ||
			    a.s6_addr[plen / 8] !=
			    (prefix->s6_addr[plen / 8] & m.s6_addr[plen / 8]))
				continue;
		}
		memcpy(addr, &sin6.sin6_addr, sizeof(*addr));
#ifdef __KAME__
		if (IN6_IS_ADDR_LINKLOCAL(addr))
			addr->s6_addr[2] = addr->s6_addr[3] = 0; 
#endif
		error = 0;
		break;
	}

	freeifaddrs(ifap);
	return (error);
}

int
in6_addrscopebyif(addr, ifnam)
	struct in6_addr *addr;
	char *ifnam;
{
	u_int ifindex; 

	if ((ifindex = if_nametoindex(ifnam)) == 0)
		return (-1);

	if (IN6_IS_ADDR_LINKLOCAL(addr) || IN6_IS_ADDR_MC_LINKLOCAL(addr))
		return (ifindex);

	if (IN6_IS_ADDR_SITELOCAL(addr) || IN6_IS_ADDR_MC_SITELOCAL(addr))
		return (1);

	if (IN6_IS_ADDR_MC_ORGLOCAL(addr))
		return (1);

	return (1);		/* treat it as global */
}

const char *
getdev(addr)
	struct sockaddr_in6 *addr;
{
	struct ifaddrs *ifap, *ifa;
	struct sockaddr_in6 *a6;
	static char ret_ifname[IFNAMSIZ+1];

	if (getifaddrs(&ifap) != 0) {
		err(1, "getdev: getifaddrs");
		/* NOTREACHED */
	}

	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr->sa_family != AF_INET6)
			continue;

		a6 = (struct sockaddr_in6 *)ifa->ifa_addr;
		if (!IN6_ARE_ADDR_EQUAL(&a6->sin6_addr, &addr->sin6_addr) ||
		    a6->sin6_scope_id != addr->sin6_scope_id)
			continue;

		break;
	}

	if (ifa)
		strncpy(ret_ifname, ifa->ifa_name, IFNAMSIZ);
	freeifaddrs(ifap);

	return (ifa ? ret_ifname : NULL);
}

int
transmit_sa(s, sa, buf, len)
	int s;
	struct sockaddr_in6 *sa;
	char *buf;
	size_t len;
{
	int error;

	error = sendto(s, buf, len, MSG_DONTROUTE, (struct sockaddr *)sa, sizeof(*sa));

	return (error != len) ? -1 : 0;
}

long
random_between(x, y)
	long x;
	long y;
{
	long ratio;

	ratio = 1 << 16;
	while ((y - x) * ratio < (y - x))
		ratio = ratio / 2;
	return x + ((y - x) * (ratio - 1) / random() & (ratio - 1));
}

int
prefix6_mask(in6, plen)
	struct in6_addr *in6;
	int plen;
{
	struct sockaddr_in6 mask6;
	int i;

	if (sa6_plen2mask(&mask6, plen))
		return (-1);

	for (i = 0; i < 16; i++)
		in6->s6_addr[i] &= mask6.sin6_addr.s6_addr[i];

	return (0);
}

int
sa6_plen2mask(sa6, plen)
	struct sockaddr_in6 *sa6;
	int plen;
{
	u_char *cp;

	if (plen < 0 || plen > 128)
		return (-1);

	memset(sa6, 0, sizeof(*sa6));
	sa6->sin6_family = AF_INET6;
	
	for (cp = (u_char *)&sa6->sin6_addr; plen > 7; plen -= 8)
		*cp++ = 0xff;
	*cp = 0xff << (8 - plen);

	return (0);
}

char *
addr2str(sa)
	struct sockaddr *sa;
{
	static char addrbuf[8][NI_MAXHOST];
	static int round = 0;
	char *cp;

	round = (round + 1) & 7;
	cp = addrbuf[round];

	if (getnameinfo(sa, NI_MAXSERV, cp, NI_MAXHOST, NULL, 
				0, NI_NUMERICHOST) != 0)
		dprintf(LOG_ERR, "%s getnameinfo return error", FNAME);

	return (cp);
}

char *
in6addr2str(in6, scopeid)
	struct in6_addr *in6;
	int scopeid;
{
	struct sockaddr_in6 sa6;

	memset(&sa6, 0, sizeof(sa6));
	sa6.sin6_family = AF_INET6;
	sa6.sin6_addr = *in6;
	sa6.sin6_scope_id = scopeid;

	return (addr2str((struct sockaddr *)&sa6));
}

/* return IPv6 address scope type. caller assumes that smaller is narrower. */
int
in6_scope(addr)
	struct in6_addr *addr;
{
	int scope;

	if (addr->s6_addr[0] == 0xfe) {
		scope = addr->s6_addr[1] & 0xc0;

		switch (scope) {
		case 0x80:
			return 2; /* link-local */
			break;
		case 0xc0:
			return 5; /* site-local */
			break;
		default:
			return 14; /* global: just in case */
			break;
		}
	}

	/* multicast scope. just return the scope field */
	if (addr->s6_addr[0] == 0xff)
		return (addr->s6_addr[1] & 0x0f);

	if (bcmp(&in6addr_loopback, addr, sizeof(addr) - 1) == 0) {
		if (addr->s6_addr[15] == 1) /* loopback */
			return 1;
		if (addr->s6_addr[15] == 0) /* unspecified */
			return 0;
	}

	return 14;		/* global */
}

static int
in6_matchflags(addr, ifnam, flags)
	struct sockaddr *addr;
	char *ifnam;
	int flags;
{
	int s;
	struct ifreq ifr;

	if ((s = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
		warn("in6_matchflags: socket(DGRAM6)");
		return (-1);
	}
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifnam, sizeof(ifr.ifr_name));
	ifr.ifr_addr = *(struct sockaddr *)addr;

	if (ioctl(s, SIOCGIFFLAGS, &ifr) < 0) {
		warn("in6_matchflags: ioctl(SIOCGIFFLAGS, %s)",
		     addr2str(addr));
		close(s);
		return (-1);
	}

	close(s);

	return (ifr.ifr_ifru.ifru_flags & flags);
}

int
configure_duid(const char *str,
	       struct duid *duid)
{
	const char *cp;
	char  *bp, *idbuf = NULL;
	int duidlen, slen;
	unsigned int x;

	/* calculate DUID len */
	slen = strlen(str);
	if (slen < 2)
		goto bad;
	duidlen = 1;
	slen -= 2;
	if ((slen % 3) != 0)
		goto bad;
	duidlen += (slen / 3);
	if (duidlen > 256) {
		dprintf(LOG_ERR, "%s" "too long DUID (%d)", FNAME, duidlen);
		return (-1);
	}

	if ((idbuf = (char *)malloc(duidlen)) == NULL) {
		dprintf(LOG_ERR, "%s" "memory allocation failed", FNAME);
		return (-1);
	}

	for (cp = str, bp = idbuf; *cp;) {
		if (*cp == ':') {
			cp++;
			continue;
		}

		if (sscanf(cp, "%02x", &x) != 1)
			goto bad;
		*bp = x;
		cp += 2;
		bp++;
	}

	duid->duid_len = duidlen;
	duid->duid_id = idbuf;
	dprintf(LOG_DEBUG, "configure duid is %s", duidstr(duid));
	return (0);

  bad:
	if (idbuf)
		free(idbuf);
	dprintf(LOG_ERR, "%s" "assumption failure (bad string)", FNAME);
	return (-1);
}

int
get_duid(const 	char *idfile, const char *ifname,
	 struct duid *duid)
{
	FILE *fp = NULL;
	u_int16_t len = 0, hwtype;
	struct dhcp6_duid_type1 *dp; /* we only support the type1 DUID */
	char tmpbuf[256];	/* DUID should be no more than 256 bytes */

	if ((fp = fopen(idfile, "r")) == NULL && errno != ENOENT)
		dprintf(LOG_NOTICE, "%s" "failed to open DUID file: %s",
		    FNAME, idfile);

	if (fp) {
		/* decode length */
		if (fread(&len, sizeof(len), 1, fp) != 1) {
			dprintf(LOG_ERR, "%s" "DUID file corrupted", FNAME);
			goto fail;
		}
	} else {
		int l;

		if ((l = gethwid(tmpbuf, sizeof(tmpbuf), ifname, &hwtype)) < 0) {
			dprintf(LOG_INFO, "%s"
			    "failed to get a hardware address", FNAME);
			goto fail;
		}
		len = l + sizeof(struct dhcp6_duid_type1);
	}

	memset(duid, 0, sizeof(*duid));
	duid->duid_len = len;
	if ((duid->duid_id = (char *)malloc(len)) == NULL) {
		dprintf(LOG_ERR, "%s" "failed to allocate memory", FNAME);
		goto fail;
	}

	/* copy (and fill) the ID */
	if (fp) {
		if (fread(duid->duid_id, len, 1, fp) != 1) {
			dprintf(LOG_ERR, "%s" "DUID file corrupted", FNAME);
			goto fail;
		}

		dprintf(LOG_DEBUG, "%s"
		    "extracted an existing DUID from %s: %s", FNAME,
		    idfile, duidstr(duid));
	} else {
		u_int64_t t64;

		dp = (struct dhcp6_duid_type1 *)duid->duid_id;
		/*  modifed start pling 04/26/2011 */
		/* Netgear Router Spec requires DUID to be type DUID-LL, not DUID-LLT */
		/* dp->dh6duid1_type = htons(1); */ /* type 1 */
		dp->dh6duid1_type = htons(3); /* type 3: DUID-LL */
		/*  modifed end pling 04/26/2011 */
		dp->dh6duid1_hwtype = htons(hwtype);
		/* time is Jan 1, 2000 (UTC), modulo 2^32 */
		t64 = (u_int64_t)(time(NULL) - 946684800);
        /*  added start pling 10/07/2010 */
        /* For testing purposes !!! */
        if (duid_time) {
            dprintf(LOG_DEBUG, "%s"
                "**TESTING** Use user-defined duid_time %lu", FNAME, duid_time);
            t64 = (u_int64_t)duid_time;
        }
        /*  added end pling 10/07/2010 */
		/*  removed start pling 04/26/2011 */
		/* Netgear Router Spec requires DUID to be type DUID-LL, not DUID-LLT */
		/* dp->dh6duid1_time = htonl((u_long)(t64 & 0xffffffff)); */
		/*  removed end pling 04/26/2011 */
		memcpy((void *)(dp + 1), tmpbuf, (len - sizeof(*dp)));

		dprintf(LOG_DEBUG, "%s" "generated a new DUID: %s", FNAME,
			duidstr(duid));
	}

	/* save the (new) ID to the file for next time */
	if (!fp) {
		if ((fp = fopen(idfile, "w+")) == NULL) {
			dprintf(LOG_ERR, "%s"
			    "failed to open DUID file for save", FNAME);
			goto fail;
		}
		if ((fwrite(&len, sizeof(len), 1, fp)) != 1) {
			dprintf(LOG_ERR, "%s" "failed to save DUID", FNAME);
			goto fail;
		}
		if ((fwrite(duid->duid_id, len, 1, fp)) != 1) {
			dprintf(LOG_ERR, "%s" "failed to save DUID", FNAME);
			goto fail;
		}

		dprintf(LOG_DEBUG, "%s" "saved generated DUID to %s", FNAME,
			idfile);
	}

	if (fp)
		fclose(fp);
	return (0);

  fail:
	if (fp)
		fclose(fp);
	if (duid->duid_id != NULL) {
		duidfree(duid);
	}
	return (-1);
}

ssize_t
gethwid(buf, len, ifname, hwtypep)
	char *buf;
	int len;
	const char *ifname;
	u_int16_t *hwtypep;
{
	int skfd;
	ssize_t l;
	struct ifreq if_hwaddr;
	
	if ((skfd = socket(AF_INET6, SOCK_DGRAM, 0 )) < 0)
		return -1;

	strcpy(if_hwaddr.ifr_name, ifname);
	if (ioctl(skfd, SIOCGIFHWADDR, &if_hwaddr) < 0)
		return -1;
	/* only support Ethernet */
	switch (if_hwaddr.ifr_hwaddr.sa_family) {
	case ARPHRD_ETHER:
	case ARPHRD_IEEE802:
		*hwtypep = ARPHRD_ETHER;
		l = 6;
		break;
	case ARPHRD_PPP:
		*hwtypep = ARPHRD_PPP;
		l = 0;
		return l;
	default:
		dprintf(LOG_INFO, "dhcpv6 doesn't support hardware type %d",
			if_hwaddr.ifr_hwaddr.sa_family);
		return -1;
	}
	memcpy(buf, if_hwaddr.ifr_hwaddr.sa_data, l);
	dprintf(LOG_DEBUG, "%s found an interface %s hardware %p",
		FNAME, ifname, buf);
	return l;
}

void
dhcp6_init_options(optinfo)
	struct dhcp6_optinfo *optinfo;
{
	memset(optinfo, 0, sizeof(*optinfo));
	/* for safety */
	optinfo->clientID.duid_id = NULL;
	optinfo->serverID.duid_id = NULL;
	optinfo->pref = DH6OPT_PREF_UNDEF;
	TAILQ_INIT(&optinfo->addr_list);
	/*  added start pling 09/23/2009 */
	TAILQ_INIT(&optinfo->prefix_list);
	/*  added end pling 09/23/2009 */
	TAILQ_INIT(&optinfo->reqopt_list);
	TAILQ_INIT(&optinfo->stcode_list);
	TAILQ_INIT(&optinfo->dns_list.addrlist);
    /*  added start pling 01/25/2010 */
	TAILQ_INIT(&optinfo->sip_list);
	TAILQ_INIT(&optinfo->ntp_list);
    /*  added end pling 01/25/2010 */
	TAILQ_INIT(&optinfo->relay_list);
	optinfo->dns_list.domainlist = NULL;
}

void
dhcp6_clear_options(optinfo)
	struct dhcp6_optinfo *optinfo;
{
	struct domain_list *dlist, *dlist_next;
	duidfree(&optinfo->clientID);
	duidfree(&optinfo->serverID);

	dhcp6_clear_list(&optinfo->addr_list);
	/*  added start pling 09/23/2009 */
	dhcp6_clear_list(&optinfo->prefix_list);
	/*  added end pling 09/23/2009 */
	dhcp6_clear_list(&optinfo->reqopt_list);
	dhcp6_clear_list(&optinfo->stcode_list);
	dhcp6_clear_list(&optinfo->dns_list.addrlist);
	relayfree(&optinfo->relay_list);
	if (dhcp6_mode == DHCP6_MODE_CLIENT) {
		for (dlist = optinfo->dns_list.domainlist; dlist; dlist = dlist_next) {
			dlist_next = dlist->next;
			free(dlist);
		}
	}
	optinfo->dns_list.domainlist = NULL;
	dhcp6_init_options(optinfo);
}

int
dhcp6_copy_options(dst, src)
	struct dhcp6_optinfo *dst, *src;
{
	if (duidcpy(&dst->clientID, &src->clientID))
		goto fail;
	if (duidcpy(&dst->serverID, &src->serverID))
		goto fail;
	dst->flags = src->flags;
	
	if (dhcp6_copy_list(&dst->addr_list, &src->addr_list))
		goto fail;
	/*  added start pling 09/23/2009 */
	if (dhcp6_copy_list(&dst->prefix_list, &src->prefix_list))
		goto fail;
	/*  added end pling 09/23/2009 */
	if (dhcp6_copy_list(&dst->reqopt_list, &src->reqopt_list))
		goto fail;
	if (dhcp6_copy_list(&dst->stcode_list, &src->stcode_list))
		goto fail;
	if (dhcp6_copy_list(&dst->dns_list.addrlist, &src->dns_list.addrlist))
		goto fail;
	memcpy(&dst->server_addr, &src->server_addr, sizeof(dst->server_addr));
	dst->pref = src->pref;

	return 0;

  fail:
	/* cleanup temporary resources */
	dhcp6_clear_options(dst);
	return -1;
}

/*  added start pling 10/04/2010 */
/* Add two extra arguments for DHCP client to use. 
 * These two args are ignored in DHCP server mode (currently) 
 */
#if 0
int
dhcp6_get_options(p, ep, optinfo)
	struct dhcp6opt *p, *ep;
	struct dhcp6_optinfo *optinfo;
#endif
int
dhcp6_get_options(p, ep, optinfo, msgtype, state, send_flags)
    struct dhcp6opt *p, *ep;
    struct dhcp6_optinfo *optinfo;
    int msgtype, state, send_flags;
/*  added end pling 10/04/2010 */
{
	struct dhcp6opt *np, opth;
	int i, opt, optlen, reqopts, num;
	char *cp, *val;
	u_int16_t val16;

    /*  added start pling 09/24/2009 */
    int has_iana = 0;
    int has_iapd = 0;
    /*  added end pling 09/24/2009 */
    /*  added start pling 10/04/2010 */
    int has_dns = 0;
    int has_ntp = 0;
    int has_sip = 0;
    /*  added end pling 10/04/2010 */
    /*  added start pling 01/25/2010 */
    char buf[1204];
    char tmp_buf[1024];
    char command[1024];
    /*  added end pling 01/25/2010 */
    int  type_set = 0;      // pling added 10/22/2010

	for (; p + 1 <= ep; p = np) {
		struct duid duid0;

		memcpy(&opth, p, sizeof(opth));
		optlen = ntohs(opth.dh6opt_len);
		opt = ntohs(opth.dh6opt_type);

		cp = (char *)(p + 1);
		np = (struct dhcp6opt *)(cp + optlen);

		dprintf(LOG_DEBUG, "%s" "get DHCP option %s, len %d",
		    FNAME, dhcp6optstr(opt), optlen);

		/* option length field overrun */
		if (np > ep) {
			dprintf(LOG_INFO,
			    "%s" "malformed DHCP options", FNAME);
			return -1;
		}

		switch (opt) {
		case DH6OPT_CLIENTID:
			if (optlen == 0)
				goto malformed;
			duid0.duid_len = optlen;
			duid0.duid_id = cp;
			dprintf(LOG_DEBUG, "  DUID: %s", duidstr(&duid0));
			if (duidcpy(&optinfo->clientID, &duid0)) {
				dprintf(LOG_ERR, "%s" "failed to copy DUID",
					FNAME);
				goto fail;
			}
			break;
		case DH6OPT_SERVERID:
			if (optlen == 0)
				goto malformed;
			duid0.duid_len = optlen;
			duid0.duid_id = cp;
			dprintf(LOG_DEBUG, "  DUID: %s", duidstr(&duid0));
			if (duidcpy(&optinfo->serverID, &duid0)) {
				dprintf(LOG_ERR, "%s" "failed to copy DUID",
					FNAME);
				goto fail;
			}
			break;
		case DH6OPT_ELAPSED_TIME:
			if (optlen != sizeof(u_int16_t))
				goto malformed;
			memcpy(&val16, cp, sizeof(val16));
			num = ntohs(val16);
			dprintf(LOG_DEBUG, " this message elapsed time is: %d",
				num);
			break;
		case DH6OPT_STATUS_CODE:
			if (optlen < sizeof(u_int16_t))
				goto malformed;
			memcpy(&val16, cp, sizeof(val16));
			num = ntohs(val16);
			dprintf(LOG_DEBUG, "  this message status code: %s",
			    dhcp6_stcodestr(num));
			
			
			/* need to check duplication? */

			if (dhcp6_add_listval(&optinfo->stcode_list,
			    &num, DHCP6_LISTVAL_NUM) == NULL) {
				dprintf(LOG_ERR, "%s" "failed to copy "
				    "status code", FNAME);
				goto fail;
			}

			break;
		case DH6OPT_ORO:
			if ((optlen % 2) != 0 || optlen == 0)
				goto malformed;
			reqopts = optlen / 2;
			for (i = 0, val = cp; i < reqopts;
			     i++, val += sizeof(u_int16_t)) {
				u_int16_t opttype;

				memcpy(&opttype, val, sizeof(u_int16_t));
				num = ntohs(opttype);

				dprintf(LOG_DEBUG, "  requested option: %s",
					dhcp6optstr(num));

				if (dhcp6_find_listval(&optinfo->reqopt_list,
				    &num, DHCP6_LISTVAL_NUM)) {
					dprintf(LOG_INFO, "%s" "duplicated "
					    "option type (%s)", FNAME,
					    dhcp6optstr(opttype));
					goto nextoption;
				}

				if (dhcp6_add_listval(&optinfo->reqopt_list,
				    &num, DHCP6_LISTVAL_NUM) == NULL) {
					dprintf(LOG_ERR, "%s" "failed to copy "
					    "requested option", FNAME);
					goto fail;
				}
			nextoption: ;
			}
			break;
		case DH6OPT_PREFERENCE:
			if (optlen != 1)
				goto malformed;
			optinfo->pref = (u_int8_t)*(u_char *)cp;
			dprintf(LOG_DEBUG, "%s" "get option preferrence is %2x", 
					FNAME, optinfo->pref);
			break;
		case DH6OPT_RAPID_COMMIT:
			if (optlen != 0)
				goto malformed;
			optinfo->flags |= DHCIFF_RAPID_COMMIT;
			break;
		case DH6OPT_UNICAST:
			if (optlen != sizeof(struct in6_addr)
			    && dhcp6_mode != DHCP6_MODE_CLIENT)
				goto malformed;
			optinfo->flags |= DHCIFF_UNICAST;
			memcpy(&optinfo->server_addr,
			       (struct in6_addr *)cp, sizeof(struct in6_addr));
			break;
		case DH6OPT_IA_TA:
			if (optlen < sizeof(u_int32_t))
				goto malformed;
			/* check iaid */
			optinfo->flags |= DHCIFF_TEMP_ADDRS;
			optinfo->type = IATA;
			dprintf(LOG_DEBUG, "%s" "get option iaid is %u", 
				FNAME, optinfo->iaidinfo.iaid);
			optinfo->iaidinfo.iaid = ntohl(*(u_int32_t *)cp);
			if (get_assigned_ipv6addrs(cp + 4, cp + optlen, optinfo))
				goto fail;
			break;
		case DH6OPT_IA_NA:
		case DH6OPT_IA_PD:
            /*  modified start pling 09/23/2009 */
			if (dhcp6_mode == DHCP6_MODE_SERVER ||
                (dhcp6_mode == DHCP6_MODE_CLIENT && (send_flags & DHCIFF_SOLICIT_ONLY)) ||
                (dhcp6_mode == DHCP6_MODE_CLIENT && (dhcp6c_flags & DHCIFF_IAPD_ONLY))) {
                /* pling modified start 10/22/2010 */
                /* For each packet, we set to one type only (IANA/IAPD)
                 * but not both.
                 */
    			if (opt == DH6OPT_IA_NA && !type_set)
                {
	    			optinfo->type = IANA;
                    type_set = 1;
                }
			    else if (opt == DH6OPT_IA_PD && !type_set)
                {
				    optinfo->type = IAPD;
                    type_set = 1;
                }
                /* pling modified end 10/22/2010 */
            } else {
                /* don't set optinfo->type to IAPD as this version
                 * of dhcp6c can't handle IANA and IAPD concurrently.
                 */
    			if (opt == DH6OPT_IA_NA)
	    			optinfo->type = IANA;
            }
            /*  modified end pling 09/23/2009 */
			/* check iaid */
			if (optlen < sizeof(struct dhcp6_iaid_info)) 
				goto malformed;
			if (dhcp6_mode == DHCP6_MODE_CLIENT && opt == DH6OPT_IA_PD)
                ;// If this is IAPD, don't modify the IAID
            else
    			optinfo->iaidinfo.iaid = ntohl(*(u_int32_t *)cp);
			optinfo->iaidinfo.renewtime = 
				ntohl(*(u_int32_t *)(cp + sizeof(u_int32_t)));
			optinfo->iaidinfo.rebindtime = 
				ntohl(*(u_int32_t *)(cp + 2 * sizeof(u_int32_t)));
			dprintf(LOG_DEBUG, "get option iaid is %u, renewtime %u, "
				"rebindtime %u", optinfo->iaidinfo.iaid,
				optinfo->iaidinfo.renewtime, optinfo->iaidinfo.rebindtime);
            /*  added start pling 10/07/2010 */
            /* DHCPv6 client readylogo:
             * Ignore IA with T1 > T2 */
            if (optinfo->iaidinfo.renewtime > optinfo->iaidinfo.rebindtime)
                goto fail;
            /*  added end pling 10/07/2010 */
			if (get_assigned_ipv6addrs(cp + 3 * sizeof(u_int32_t), 
						cp + optlen, optinfo))
				goto fail;

            /*  added start pling 09/24/2009 */
			if (dhcp6_mode == DHCP6_MODE_CLIENT) {
			    if (opt == DH6OPT_IA_NA)
                    has_iana = 1;
                else
                    has_iapd = 1;
            }
            /*  added end pling 09/24/2009 */
			break;
		case DH6OPT_DNS_SERVERS:
			if (optlen % sizeof(struct in6_addr) || optlen == 0)
				goto malformed;
			for (val = cp; val < cp + optlen;
			     val += sizeof(struct in6_addr)) {
				if (dhcp6_find_listval(&optinfo->dns_list.addrlist,
				    val, DHCP6_LISTVAL_ADDR6)) {
					dprintf(LOG_INFO, "%s" "duplicated "
					    "DNS address (%s)", FNAME,
					    in6addr2str((struct in6_addr *)val,
						0));
					goto nextdns;
				}

				if (dhcp6_add_listval(&optinfo->dns_list.addrlist,
				    val, DHCP6_LISTVAL_ADDR6) == NULL) {
					dprintf(LOG_ERR, "%s" "failed to copy "
					    "DNS address", FNAME);
					goto fail;
				}
			nextdns: ;
			}
            /*  added start pling 10/04/2010 */
            if (dhcp6_mode == DHCP6_MODE_CLIENT)
                has_dns = 1;
            /*  added end pling 10/04/2010 */
			break;

        /*  added start pling 01/25/2010 */
        case DH6OPT_SIP_SERVERS:
            memset(buf, 0, sizeof(buf));
            memset(tmp_buf, 0, sizeof(tmp_buf));
            if (optlen % sizeof(struct in6_addr) || optlen == 0)
                goto malformed;
            for (val = cp; val < cp + optlen;
                 val += sizeof(struct in6_addr)) {
                if (dhcp6_find_listval(&optinfo->sip_list,
                    val, DHCP6_LISTVAL_ADDR6)) {
                    dprintf(LOG_INFO, "%s" "duplicated "
                        "SIP address (%s)", FNAME,
                        in6addr2str((struct in6_addr *)val,
                        0));
                    goto nextsip;
                }

                if (dhcp6_add_listval(&optinfo->sip_list,
                    val, DHCP6_LISTVAL_ADDR6) == NULL) {
                        dprintf(LOG_ERR, "%s" "failed to copy "
                        "SIP address", FNAME);
                    goto fail;
                }
                /* Save SIP server to NVRAM */
                sprintf(tmp_buf, "%s ", in6addr2str((struct in6_addr *)val, 0));
                strcat(buf, tmp_buf);
            nextsip: ;
            }
            /* Save SIP server to NVRAM */
            if (dhcp6_mode == DHCP6_MODE_CLIENT && strlen(buf)) {
                sprintf(command, "nvram set ipv6_sip_servers=\"%s\"", buf);
                system(command);
                has_sip = 1;    //  added pling 10/04/2010
            }
            break;
		case DH6OPT_NTP_SERVERS:
            memset(buf, 0, sizeof(buf));
            memset(tmp_buf, 0, sizeof(tmp_buf));
			if (optlen % sizeof(struct in6_addr) || optlen == 0)
				goto malformed;
			for (val = cp; val < cp + optlen;
			     val += sizeof(struct in6_addr)) {
				if (dhcp6_find_listval(&optinfo->ntp_list,
				    val, DHCP6_LISTVAL_ADDR6)) {
					dprintf(LOG_INFO, "%s" "duplicated "
					    "NTP address (%s)", FNAME,
					    in6addr2str((struct in6_addr *)val,
						0));
					goto nextntp;
				}

				if (dhcp6_add_listval(&optinfo->ntp_list,
				    val, DHCP6_LISTVAL_ADDR6) == NULL) {
					dprintf(LOG_ERR, "%s" "failed to copy "
					    "NTP address", FNAME);
					goto fail;
				}
                /* Save SIP server to NVRAM */
                sprintf(tmp_buf, "%s ", in6addr2str((struct in6_addr *)val, 0));
                strcat(buf, tmp_buf);
			nextntp: ;
			}
            /* Save NTP server to NVRAM */
            if (dhcp6_mode == DHCP6_MODE_CLIENT && strlen(buf)) {
                sprintf(command, "nvram set ipv6_ntp_servers=\"%s\"", buf);
                system(command);
                has_ntp = 1;    //  added pling 10/04/2010
            }
			break;
        /*  added end pling 01/25/2010 */

		case DH6OPT_DOMAIN_LIST:
			if (optlen == 0)
				goto malformed;
			/* dependency on lib resolv */
			for (val = cp; val < cp + optlen;) {
				int n;
				struct domain_list *dname, *dlist;
				dname = malloc(sizeof(*dname));
				if (dname == NULL) {
					dprintf(LOG_ERR, "%s" "failed to allocate memory", 
						FNAME);
					goto fail;
				}
				n =  dn_expand(cp, cp + optlen, val, dname->name, MAXDNAME);
				if (n < 0) 
					goto malformed;
				else {
					val += n;
					dprintf(LOG_DEBUG, "expand domain name %s, size %d", 
						dname->name, strlen(dname->name));
				}
				dname->next = NULL;
				if (optinfo->dns_list.domainlist == NULL) {
					optinfo->dns_list.domainlist = dname;
				} else {
					for (dlist = optinfo->dns_list.domainlist; dlist;
					     dlist = dlist->next) {
						if (dlist->next == NULL) {
							dlist->next = dname;
							break;
						}
					}
				}
			}
			break;
		default:
			/* no option specific behavior */
			dprintf(LOG_INFO, "%s"
			    "unknown or unexpected DHCP6 option %s, len %d",
			    FNAME, dhcp6optstr(opt), optlen);
			break;
		}
	}

    /*  added start pling 09/24/2009 */
    /* Per Netgear spec, an acceptable DHCP advertise 
     *  must have both IANA and IAPD option.
     */
    if (dhcp6_mode == DHCP6_MODE_CLIENT) {
        /*  added start pling 09/21/2010 */
        /* Check flag to see if we accept IANA/IAPD only
         *  for DHCPv6 readylogo test.
         */
        if ((dhcp6c_flags & DHCIFF_IANA_ONLY) && has_iana)
        {
            dprintf(LOG_INFO, "%s" "recv IANA. OK!", FNAME);
        }
        else
        if (dhcp6c_flags & DHCIFF_INFO_ONLY)
        {
            dprintf(LOG_INFO, "%s" "Info-only. OK!", FNAME);
        }
        else
        if  ((dhcp6c_flags & DHCIFF_IAPD_ONLY) && has_iapd)
        {
            dprintf(LOG_INFO, "%s" "recv IAPD. OK!", FNAME);
        }
        else
        /*  added end pling 09/21/2010 */
        /*  added start pling 10/04/2010 */
        /* Handle DHCP messages properly in different states */
        if (state == DHCP6S_INFOREQ && msgtype == DH6_REPLY &&
            has_dns && has_ntp && has_sip)
        {
            dprintf(LOG_INFO, "%s" "valid INFOREQ/REPLY. OK!", FNAME);
        }
        else
        if (state == DHCP6S_DECLINE && msgtype == DH6_REPLY)
        {
            dprintf(LOG_INFO, "%s" "got REPLY to DECLINE.", FNAME);
        }
        else
        /*  added end pling 10/04/2010 */
        /*  added start pling 09/16/2011 */
        /* In auto-detect mode, we don't accept Advert pkt with IANA only. */
        if ((send_flags & DHCIFF_SOLICIT_ONLY) && has_iana && !has_iapd)
        {
            dprintf(LOG_INFO, "%s" "got IANA only in auto-detect mode. NG!", FNAME);
            goto fail;
        }
        else
        /*  added end pling 09/16/2011 */
        /*  added start pling 10/14/2010 */
        if ((send_flags & DHCIFF_SOLICIT_ONLY) && 
            (has_iana || has_iapd) )
        {
            dprintf(LOG_INFO, "%s" "got IANA/IAPD in auto-detect mode", FNAME);
        }
        else
        /*  added end pling 10/14/2010 */
        if (!has_iana || !has_iapd) {
            dprintf(LOG_INFO, "%s" "no IANA/IAPD", FNAME);
            goto fail;
        }
    }
    /*  added end pling 09/24/2009 */

	return (0);

  malformed:
	dprintf(LOG_INFO, "%s" "malformed DHCP option: type %d, len %d",
	    FNAME, opt, optlen);
  fail:
	dhcp6_clear_options(optinfo);
	return (-1);
}

static int
get_assigned_ipv6addrs(p, ep, optinfo)
	char *p, *ep;
	struct dhcp6_optinfo *optinfo;
{
	char *np, *cp;
	struct dhcp6opt opth;
	struct dhcp6_addr_info ai;
	struct dhcp6_prefix_info pi;
	struct dhcp6_addr addr6;
	int optlen, opt;
	u_int16_t val16;
	int num;
    int has_status_code = 0;    /*  added pling 09/15/2011 */

	/*  added start pling 12/22/2011 */
	char iapd_valid_lifetime_cmd_buf[1024];
	char iapd_preferred_lifetime_cmd_buf[1024];
	/*  added end pling 12/22/2011 */

    /*  modified start pling 09/15/2011 */
    /* To work around IANA/IAPD without status code */
	//for (; p + sizeof(struct dhcp6opt) <= ep; p = np) {
	for (; /*p + sizeof(struct dhcp6opt) <= ep*/; p = np) {

        if (p + sizeof(struct dhcp6opt) > ep)
        {
            /*  added start pling 10/19/2011 */
            /* for server, use original logic (break for loop) */
            if (dhcp6_mode == DHCP6_MODE_SERVER)
                break;
            /*  added end pling 10/19/2011 */

            /* Client check status code below */
            if (has_status_code)
                break;
            else {
                has_status_code = 1;
                goto no_status_code;
            }
        }
    /*  modified end pling 09/15/2011 */
		memcpy(&opth, p, sizeof(opth));
		optlen =  ntohs(opth.dh6opt_len);
		opt = ntohs(opth.dh6opt_type);
		cp = p + sizeof(opth);
		np = cp + optlen;
		dprintf(LOG_DEBUG, "  IA address option: %s, "
			"len %d", dhcp6optstr(opt), optlen);

		if (np > ep) {
			dprintf(LOG_INFO, "%s" "malformed DHCP options",
			    FNAME);
			return -1;
		}
		switch(opt) {
		case DH6OPT_STATUS_CODE:
			if (optlen < sizeof(val16))
				goto malformed;
			memcpy(&val16, cp, sizeof(val16));
			num = ntohs(val16);
			dprintf(LOG_INFO, "status code for this address is: %s",
				dhcp6_stcodestr(num));
			if (optlen > sizeof(val16)) {
				dprintf(LOG_INFO, 
					"status message for this address is: %-*s",
					(int)(optlen-sizeof(val16)), p+(val16));
			}
			if (dhcp6_add_listval(&optinfo->stcode_list,
			    &num, DHCP6_LISTVAL_NUM) == NULL) {
				dprintf(LOG_ERR, "%s" "failed to copy "
				    "status code", FNAME);
				goto fail;
			}
            has_status_code = 1;    /*  added pling 09/15/2011 */
			break;
		case DH6OPT_IADDR:
			if (optlen < sizeof(ai) - sizeof(u_int32_t))
				goto malformed;
			memcpy(&ai, p, sizeof(ai));
			/* copy the information into internal format */
			memset(&addr6, 0, sizeof(addr6));
			memcpy(&addr6.addr, (struct in6_addr *)cp, sizeof(struct in6_addr));
			addr6.preferlifetime = ntohl(ai.preferlifetime);
			addr6.validlifetime = ntohl(ai.validlifetime);
			dprintf(LOG_DEBUG, "  get IAADR address information: "
			    "%s preferlifetime %d validlifetime %d",
			    in6addr2str(&addr6.addr, 0),
			    addr6.preferlifetime, addr6.validlifetime);
			/* It shouldn't happen, since Server will do the check before 
			 * sending the data to clients */
			if (addr6.preferlifetime > addr6.validlifetime) {
				dprintf(LOG_INFO, "preferred life time"
				    "(%d) is greater than valid life time"
				    "(%d)", addr6.preferlifetime, addr6.validlifetime);
				goto malformed;
			}
			if (optlen == sizeof(ai) - sizeof(u_int32_t)) {
				addr6.status_code = DH6OPT_STCODE_UNDEFINE;
				break;
			}
			/* address status code might be added after IADDA option */
			memcpy(&opth, p + sizeof(ai), sizeof(opth));
			optlen =  ntohs(opth.dh6opt_len);
			opt = ntohs(opth.dh6opt_type);
			switch(opt) {	
			case DH6OPT_STATUS_CODE:
				if (optlen < sizeof(val16))
					goto malformed;
				memcpy(&val16, p + sizeof(ai) + sizeof(opth), sizeof(val16));
				num = ntohs(val16);
				dprintf(LOG_INFO, "status code for this address is: %s",
					dhcp6_stcodestr(num));
				addr6.status_code = num;
				if (optlen > sizeof(val16)) {
					dprintf(LOG_INFO, 
						"status message for this address is: %-*s",
						(int)(optlen-sizeof(val16)), p+(val16));
				}
				break;
			default:
				goto malformed;
			}
			break;
		case DH6OPT_IAPREFIX:
			if (optlen < sizeof(pi) - sizeof(u_int32_t))
			       goto malformed;
			memcpy(&pi, p, sizeof(pi));
			/* copy the information into internal format */
			memset(&addr6, 0, sizeof(addr6));
			addr6.preferlifetime = ntohl(pi.preferlifetime);
			addr6.validlifetime = ntohl(pi.validlifetime);
			addr6.plen = pi.plen;
			memcpy(&addr6.addr, &pi.prefix, sizeof(struct in6_addr));
			dprintf(LOG_DEBUG, "  get IAPREFIX prefix information: "
			    "%s/%d preferlifetime %d validlifetime %d",
			    in6addr2str(&addr6.addr, 0), addr6.plen,
			    addr6.preferlifetime, addr6.validlifetime);
			/* It shouldn't happen, since Server will do the check before 
			 * sending the data to clients */
			if (addr6.preferlifetime > addr6.validlifetime) {
				dprintf(LOG_INFO, "preferred life time"
				    "(%d) is greater than valid life time"
				    "(%d)", addr6.preferlifetime, addr6.validlifetime);
				goto malformed;
			}

			/*  added start pling 12/22/2011 */
			/* WNDR4500 TD#156: Record the IAPD valid and preferred lifetime */
			if (dhcp6_mode == DHCP6_MODE_CLIENT) {
				sprintf(iapd_valid_lifetime_cmd_buf,
						"nvram set RA_AdvValidLifetime_from_IAPD=%u",
						addr6.validlifetime);
				sprintf(iapd_preferred_lifetime_cmd_buf,
						"nvram set RA_AdvPreferredLifetime_from_IAPD=%u",
						addr6.preferlifetime);
			}
			/*  added end pling 12/22/2011 */

            if (!(dhcp6c_flags & DHCIFF_IAPD_ONLY))
    			addr6.type = IAPD;      /*  added pling 01/25/2009 */
			if (optlen == sizeof(pi) - sizeof(u_int32_t)) {
				addr6.status_code = DH6OPT_STCODE_UNDEFINE;
				break;
			}
			/* address status code might be added after IADDA option */
			memcpy(&opth, p + sizeof(pi), sizeof(opth));
			optlen =  ntohs(opth.dh6opt_len);
			opt = ntohs(opth.dh6opt_type);
			switch(opt) {	
			case DH6OPT_STATUS_CODE:
				if (optlen < sizeof(val16))
					goto malformed;
				memcpy(&val16, p + sizeof(pi) + sizeof(opth), sizeof(val16));
				num = ntohs(val16);
				dprintf(LOG_INFO, "status code for this prefix is: %s",
					dhcp6_stcodestr(num));
				addr6.status_code = num;
				if (optlen > sizeof(val16)) {
					dprintf(LOG_INFO, 
						"status message for this prefix is: %-*s",
						(int)(optlen-sizeof(val16)), p+(val16));
				}
				break;
			default:
				goto malformed;
			}
			break;
		default:
			goto malformed;
		}

no_status_code: /*  added start pling 09/15/2011 */
		/* set up address type */
		/*  added start pling 09/23/2009 */
		if (addr6.type == IAPD) {
		    /*  added start pling 01/25/2010 */
    		if (dhcp6_find_listval(&optinfo->prefix_list,
	    			&addr6, DHCP6_LISTVAL_DHCP6ADDR)) {
		    	dprintf(LOG_INFO, "duplicated prefix (%s/%d)", 
			    	in6addr2str(&addr6.addr, 0), addr6.plen);
    			continue;	
	    	}
		    /*  added end pling 01/25/2010 */
			if (dhcp6_add_listval(&optinfo->prefix_list, &addr6,
			    DHCP6_LISTVAL_DHCP6ADDR) == NULL) {
				dprintf(LOG_ERR, "%s" "failed to copy prefix", FNAME);
				goto fail;
			}
		} else {
		/*  added end pling 09/23/2009 */
		addr6.type = optinfo->type; 
		if (dhcp6_find_listval(&optinfo->addr_list,
				&addr6, DHCP6_LISTVAL_DHCP6ADDR)) {
			dprintf(LOG_INFO, "duplicated address (%s/%d)", 
				in6addr2str(&addr6.addr, 0), addr6.plen);
			continue;	
		}
		if (dhcp6_add_listval(&optinfo->addr_list, &addr6,
		    DHCP6_LISTVAL_DHCP6ADDR) == NULL) {
			dprintf(LOG_ERR, "%s" "failed to copy an "
			    "address", FNAME);
			goto fail;
		}
		/*  added start pling 09/23/2009 */
		} /* if (addr6.type == IAPD) */
		/*  added end pling 09/23/2009 */
	}

	/*  added start pling 12/22/2011 */
	/* WNDR4500 TD#156: Set the IAPD valid and preferred lifetime
	 * to NVRAM for acos rc to use */
	if (dhcp6_mode == DHCP6_MODE_CLIENT) {
		system(iapd_valid_lifetime_cmd_buf);
		system(iapd_preferred_lifetime_cmd_buf);
		system("nvram set RA_use_dynamic_lifetime=1");
	}
	/*  added end pling 12/22/2011 */

	return (0);

  malformed:
	dprintf(LOG_INFO,
		"  malformed IA option: type %d, len %d",
		opt, optlen);
  fail:
	return (-1);
}

#define COPY_OPTION(t, l, v, p) do { \
	if ((void *)(ep) - (void *)(p) < (l) + sizeof(struct dhcp6opt)) { \
		dprintf(LOG_INFO, "%s" "option buffer short for %s", FNAME, dhcp6optstr((t))); \
		goto fail; \
	} \
	opth.dh6opt_type = htons((t)); \
	opth.dh6opt_len = htons((l)); \
	memcpy((p), &opth, sizeof(opth)); \
	if ((l)) \
		memcpy((p) + 1, (v), (l)); \
	(p) = (struct dhcp6opt *)((char *)((p) + 1) + (l)); \
 	(len) += sizeof(struct dhcp6opt) + (l); \
	dprintf(LOG_DEBUG, "%s" "set %s", FNAME, dhcp6optstr((t))); \
} while (0)

int
dhcp6_set_options(bp, ep, optinfo)
	struct dhcp6opt *bp, *ep;
	struct dhcp6_optinfo *optinfo;
{
	struct dhcp6opt *p = bp, opth;
	struct dhcp6_listval *stcode;
	int len = 0, optlen = 0;
	char *tmpbuf = NULL;

	if (optinfo->clientID.duid_len) {
		COPY_OPTION(DH6OPT_CLIENTID, optinfo->clientID.duid_len,
			    optinfo->clientID.duid_id, p);
	}

	if (optinfo->serverID.duid_len) {
		COPY_OPTION(DH6OPT_SERVERID, optinfo->serverID.duid_len,
			    optinfo->serverID.duid_id, p);
	}
	if (dhcp6_mode == DHCP6_MODE_CLIENT) 
    {
        /*  modified start pling 10/01/2010 */
        /* Take care of endian issue */
		// COPY_OPTION(DH6OPT_ELAPSED_TIME, 2, &optinfo->elapsed_time, p);
        u_int16_t elapsed_time = htons(optinfo->elapsed_time);
        COPY_OPTION(DH6OPT_ELAPSED_TIME, 2, &elapsed_time, p);
        /*  modified end pling 10/01/2010 */
    }

    /*  added start pling 09/07/2010 */
    /* For dhcp6c, add user-class if specified */
    if (dhcp6_mode == DHCP6_MODE_CLIENT) 
    {
        /* user class option in this format (RFC3315):
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        | OPTION_USER_CLASS               | option-len                  |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        | user-class-data                                               .
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

         user-class-data in this format:
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-...-+-+-+-+-+-+-+
        | user-class-len (2 bytes)        | opaque-data                 |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-...-+-+-+-+-+-+-+
         */
        int option_len;
        unsigned short *user_class_len;
        char user_class_data[MAX_USER_CLASS_LEN+2];

        if (strlen(optinfo->user_class))
        {
            option_len = strlen(optinfo->user_class) + 2;
            user_class_len = (unsigned short *)&user_class_data;
            *user_class_len = htons(strlen(optinfo->user_class));
            strcpy(&user_class_data[2], optinfo->user_class);
            COPY_OPTION(DH6OPT_USER_CLASS, option_len, user_class_data, p);
        }
    }
    /*  added end pling 09/07/2010 */

	if (optinfo->flags & DHCIFF_RAPID_COMMIT)
		COPY_OPTION(DH6OPT_RAPID_COMMIT, 0, NULL, p);

	if ((dhcp6_mode == DHCP6_MODE_SERVER) && (optinfo->flags & DHCIFF_UNICAST)) {
		if (!IN6_IS_ADDR_UNSPECIFIED(&optinfo->server_addr)) {
			COPY_OPTION(DH6OPT_UNICAST, sizeof(optinfo->server_addr),
				    &optinfo->server_addr, p);
		}
	}
	switch(optinfo->type) {
	int buflen;
	char *tp;
	u_int32_t iaid;
	struct dhcp6_iaid_info opt_iana;
	struct dhcp6_iaid_info opt_iapd;
	struct dhcp6_prefix_info pi;
	struct dhcp6_addr_info ai;
	struct dhcp6_status_info status;
	struct dhcp6_listval *dp;
	case IATA:
	case IANA:
		if (optinfo->iaidinfo.iaid == 0)
			break;
		if (optinfo->type == IATA) {
			optlen = sizeof(iaid);
			dprintf(LOG_DEBUG, "%s" "set IA_TA iaid information: %d", FNAME,
				optinfo->iaidinfo.iaid);
			iaid = htonl(optinfo->iaidinfo.iaid); 
		} else if (optinfo->type == IANA) {
			optlen = sizeof(opt_iana);
			dprintf(LOG_DEBUG, "set IA_NA iaidinfo: "
		   		"iaid %u renewtime %u rebindtime %u", 
		   		optinfo->iaidinfo.iaid, optinfo->iaidinfo.renewtime, 
		   		optinfo->iaidinfo.rebindtime);
			opt_iana.iaid = htonl(optinfo->iaidinfo.iaid);
		    /*  modified start pling 01/25/2010 */
    		/* Per Netgear spec, use IAID '11' for IAPD in dhcp6c */
	    	if (dhcp6_mode == DHCP6_MODE_CLIENT)
		    	opt_iana.iaid = htonl(IANA_IAID);
    		/*  modified end pling 01/25/2010 */
			opt_iana.renewtime = htonl(optinfo->iaidinfo.renewtime);
			opt_iana.rebindtime = htonl(optinfo->iaidinfo.rebindtime);
		}
        /*  modified start pling 09/24/2009 */
		if (dhcp6_mode == DHCP6_MODE_SERVER ||
            dhcp6_mode == DHCP6_MODE_CLIENT && dhcp6c_flags & DHCIFF_IAPD_ONLY) {
		    buflen = sizeof(opt_iana) + dhcp6_count_list(&optinfo->addr_list) *
			    	(sizeof(ai) + sizeof(status));
        } else {
            /* Client don't need to send the status code */
		    buflen = sizeof(opt_iana) + dhcp6_count_list(&optinfo->addr_list) *
			    	 sizeof(ai);
        }
        /*  modified end pling 09/24/2009 */
		tmpbuf = NULL;
		if ((tmpbuf = malloc(buflen)) == NULL) {
			dprintf(LOG_ERR, "%s"
				"memory allocation failed for options", FNAME);
			goto fail;
		}
		if (optinfo->type == IATA) 
			memcpy(tmpbuf, &iaid, sizeof(iaid));
		else
			memcpy(tmpbuf, &opt_iana, sizeof(opt_iana));
		tp = tmpbuf + optlen;
		optlen += dhcp6_count_list(&optinfo->addr_list) * sizeof(ai);
		if (!TAILQ_EMPTY(&optinfo->addr_list)) {
			for (dp = TAILQ_FIRST(&optinfo->addr_list); dp; 
			     dp = TAILQ_NEXT(dp, link)) {
				int iaddr_len = 0;
				memset(&ai, 0, sizeof(ai));
				ai.dh6_ai_type = htons(DH6OPT_IADDR);
				if (dp->val_dhcp6addr.status_code != DH6OPT_STCODE_UNDEFINE) {
                    /*  modified start pling 09/24/2009 */
		            if (dhcp6_mode == DHCP6_MODE_SERVER)
					    iaddr_len = sizeof(ai) - sizeof(u_int32_t) 
						    		+ sizeof(status);
                    else
					    iaddr_len = sizeof(ai) - sizeof(u_int32_t);
                } else 
					iaddr_len = sizeof(ai) - sizeof(u_int32_t);
				ai.dh6_ai_len = htons(iaddr_len);
				ai.preferlifetime = htonl(dp->val_dhcp6addr.preferlifetime);
				ai.validlifetime = htonl(dp->val_dhcp6addr.validlifetime);
				memcpy(&ai.addr, &dp->val_dhcp6addr.addr,
			       		sizeof(ai.addr));
				memcpy(tp, &ai, sizeof(ai));
				tp += sizeof(ai);
				dprintf(LOG_DEBUG, "set IADDR address option len %d: "
			    		"%s preferlifetime %d validlifetime %d", 
			    		iaddr_len, in6addr2str(&ai.addr, 0), 
			    		ntohl(ai.preferlifetime), 
					ntohl(ai.validlifetime));
				/* set up address status code if any */
                /*  added start pling 09/24/2009 */
                /* Don't add status code in client reqeust */
		        if (dhcp6_mode == DHCP6_MODE_CLIENT)
                    ;
                else
                /*  added end pling 09/24/2009 */
				if (dp->val_dhcp6addr.status_code != DH6OPT_STCODE_UNDEFINE) {
					status.dh6_status_type = htons(DH6OPT_STATUS_CODE);
					status.dh6_status_len = 
						htons(sizeof(status.dh6_status_code));
					status.dh6_status_code = 
						htons(dp->val_dhcp6addr.status_code);
					memcpy(tp, &status, sizeof(status));
					dprintf(LOG_DEBUG, "  this address status code: %s",
			    		dhcp6_stcodestr(ntohs(status.dh6_status_code)));
					optlen += sizeof(status);
					tp += sizeof(status);
                }
			}
		} else if (dhcp6_mode == DHCP6_MODE_SERVER) {
			int num;
			num = DH6OPT_STCODE_NOADDRAVAIL;
			dprintf(LOG_DEBUG, "  status code: %s",
			    dhcp6_stcodestr(num));
			if (dhcp6_add_listval(&optinfo->stcode_list,
			    &num, DHCP6_LISTVAL_NUM) == NULL) {
				dprintf(LOG_ERR, "%s" "failed to copy "
				    "status code", FNAME);
				goto fail;
			}
		}
		if (optinfo->type == IATA)
			COPY_OPTION(DH6OPT_IA_TA, optlen, tmpbuf, p);
		else if (optinfo->type == IANA)
			COPY_OPTION(DH6OPT_IA_NA, optlen, tmpbuf, p);
		free(tmpbuf);
		/*  modified start pling 09/22/2009 */
		/* Per Netgear spec, dhcp6c need to send IAPD, 
		 *  so we fall through to do IAPD.
		 */
		if (dhcp6_mode == DHCP6_MODE_SERVER)
			break;
		/*  modified end pling 09/22/2009 */
        /*  added start pling 10/01/2010 */
        /* For DHCPv6 readylogo test, send IANA only */
        if (dhcp6_mode == DHCP6_MODE_CLIENT &&
            dhcp6c_flags & DHCIFF_IANA_ONLY)
            break;
        /*  added end pling 10/01/2010 */
	case IAPD:
		/*  modified start pling 09/22/2009 */
		/* Per Netgear spec, use IAID '11' for IAPD in dhcp6c */
		if (dhcp6_mode == DHCP6_MODE_CLIENT)
			optinfo->iaidinfo.iaid = IAPD_IAID;
		/*  modified end pling 09/22/2009 */
		if (optinfo->iaidinfo.iaid == 0)
			break;
		optlen = sizeof(opt_iapd);
		dprintf(LOG_DEBUG, "set IA_PD iaidinfo: "
		 	"iaid %u renewtime %u rebindtime %u", 
		  	optinfo->iaidinfo.iaid, optinfo->iaidinfo.renewtime, 
		   	optinfo->iaidinfo.rebindtime);
		opt_iapd.iaid = htonl(optinfo->iaidinfo.iaid);
		opt_iapd.renewtime = htonl(optinfo->iaidinfo.renewtime);
		opt_iapd.rebindtime = htonl(optinfo->iaidinfo.rebindtime);
		/*  modified start pling 09/23/2009 */
        /* In DHCP client mode, copy the prefix, 
         * but not include the status code
         */
		if (dhcp6_mode == DHCP6_MODE_SERVER ||
            dhcp6_mode == DHCP6_MODE_CLIENT && dhcp6c_flags & DHCIFF_IAPD_ONLY)
		    buflen = sizeof(opt_iapd) + dhcp6_count_list(&optinfo->addr_list) *
			    	(sizeof(pi) + sizeof(status));
        else
    		buflen = sizeof(opt_iapd) + dhcp6_count_list(&optinfo->prefix_list) *
 	    			sizeof(pi);
		/*  modified end pling 09/23/2009 */
		tmpbuf = NULL;
		if ((tmpbuf = malloc(buflen)) == NULL) {
			dprintf(LOG_ERR, "%s"
				"memory allocation failed for options", FNAME);
			goto fail;
		}
		memcpy(tmpbuf, &opt_iapd, sizeof(opt_iapd));
		tp = tmpbuf + optlen;
		/*  modified start pling 09/23/2009 */
        /* IAPD is handle differently in server and client mode */
		if (dhcp6_mode == DHCP6_MODE_SERVER ||
            dhcp6_mode == DHCP6_MODE_CLIENT && dhcp6c_flags & DHCIFF_IAPD_ONLY) {
		    optlen += dhcp6_count_list(&optinfo->addr_list) * sizeof(pi);
    		if (!TAILQ_EMPTY(&optinfo->addr_list)) {
	    		for (dp = TAILQ_FIRST(&optinfo->addr_list); dp; 
			        dp = TAILQ_NEXT(dp, link)) {
    				int iaddr_len = 0;
	    			memset(&pi, 0, sizeof(pi));
		    		pi.dh6_pi_type = htons(DH6OPT_IAPREFIX);
			    	if (dp->val_dhcp6addr.status_code != DH6OPT_STCODE_UNDEFINE) 
    					iaddr_len = sizeof(pi) - sizeof(u_int32_t) 
	    					+ sizeof(status); 
				    else 
					    iaddr_len = sizeof(pi) - sizeof(u_int32_t);
    				pi.dh6_pi_len = htons(iaddr_len);
	    			pi.preferlifetime = htonl(dp->val_dhcp6addr.preferlifetime);
		    		pi.validlifetime = htonl(dp->val_dhcp6addr.validlifetime);
			    	pi.plen = dp->val_dhcp6addr.plen; 
				    memcpy(&pi.prefix, &dp->val_dhcp6addr.addr, sizeof(pi.prefix));
    				memcpy(tp, &pi, sizeof(pi));
	    			tp += sizeof(pi);
		    		dprintf(LOG_DEBUG, "set IAPREFIX option len %d: "
			        		"%s/%d preferlifetime %d validlifetime %d", 
			        		iaddr_len, in6addr2str(&pi.prefix, 0), pi.plen,
			    	    	ntohl(pi.preferlifetime), ntohl(pi.validlifetime));
    				/* set up address status code if any */
    				if (dp->val_dhcp6addr.status_code != DH6OPT_STCODE_UNDEFINE) {
	    				status.dh6_status_type = htons(DH6OPT_STATUS_CODE);
		    			status.dh6_status_len = 
			    			htons(sizeof(status.dh6_status_code));
				    	status.dh6_status_code = 
					    	htons(dp->val_dhcp6addr.status_code);
    					memcpy(tp, &status, sizeof(status));
	    				dprintf(LOG_DEBUG, "  this address status code: %s",
		    	    		dhcp6_stcodestr(ntohs(status.dh6_status_code)));
			    		optlen += sizeof(status);
				    	tp += sizeof(status);
					    /* copy status message if any */
    				}
                }
		    } else if (dhcp6_mode == DHCP6_MODE_SERVER) {
			    int num;
    			num = DH6OPT_STCODE_NOPREFIXAVAIL;
	    		dprintf(LOG_DEBUG, "  status code: %s",
		    	    dhcp6_stcodestr(num));
			    if (dhcp6_add_listval(&optinfo->stcode_list,
			        &num, DHCP6_LISTVAL_NUM) == NULL) {
    				dprintf(LOG_ERR, "%s" "failed to copy "
	    			    "status code", FNAME);
		    		goto fail;
			    }
    		}
        } else {
            /* Client mode */
            /* Use 'prefix_list' instead of 'addr_list' for IAPD */
    		optlen += dhcp6_count_list(&optinfo->prefix_list) * sizeof(pi);
	    	if (!TAILQ_EMPTY(&optinfo->prefix_list)) {
		    	for (dp = TAILQ_FIRST(&optinfo->prefix_list); dp; 
			         dp = TAILQ_NEXT(dp, link)) {
				    int iaddr_len = 0;
    				memset(&pi, 0, sizeof(pi));
	    			pi.dh6_pi_type = htons(DH6OPT_IAPREFIX);
		    		if (dp->val_dhcp6addr.status_code != DH6OPT_STCODE_UNDEFINE) 
					    iaddr_len = sizeof(pi) - sizeof(u_int32_t);
    				else 
	    				iaddr_len = sizeof(pi) - sizeof(u_int32_t);
		    		pi.dh6_pi_len = htons(iaddr_len);
			    	pi.preferlifetime = htonl(dp->val_dhcp6addr.preferlifetime);
				    pi.validlifetime = htonl(dp->val_dhcp6addr.validlifetime);
    				pi.plen = dp->val_dhcp6addr.plen; 
	    			memcpy(&pi.prefix, &dp->val_dhcp6addr.addr, sizeof(pi.prefix));
		    		memcpy(tp, &pi, sizeof(pi));
			    	tp += sizeof(pi);
				    dprintf(LOG_DEBUG, "set IAPREFIX option len %d: "
			    	    	"%s/%d preferlifetime %d validlifetime %d", 
			    		    iaddr_len, in6addr2str(&pi.prefix, 0), pi.plen,
    			    		ntohl(pi.preferlifetime), ntohl(pi.validlifetime));
                }
			}
		}
        /*  modified end 09/23/2009 */
		COPY_OPTION(DH6OPT_IA_PD, optlen, tmpbuf, p);
		free(tmpbuf);
		break;
	default:
		break;
	}
	if (dhcp6_mode == DHCP6_MODE_SERVER && optinfo->pref != DH6OPT_PREF_UNDEF) {
		u_int8_t p8 = (u_int8_t)optinfo->pref;
		dprintf(LOG_DEBUG, "server preference %2x", optinfo->pref);
		COPY_OPTION(DH6OPT_PREFERENCE, sizeof(p8), &p8, p);
	}

	for (stcode = TAILQ_FIRST(&optinfo->stcode_list); stcode;
	     stcode = TAILQ_NEXT(stcode, link)) {
		u_int16_t code;

		code = htons(stcode->val_num);
		COPY_OPTION(DH6OPT_STATUS_CODE, sizeof(code), &code, p);
	}

	if (!TAILQ_EMPTY(&optinfo->reqopt_list)) {
		struct dhcp6_listval *opt;
		u_int16_t *valp;

		tmpbuf = NULL;
		optlen = dhcp6_count_list(&optinfo->reqopt_list) *
			sizeof(u_int16_t);
		if ((tmpbuf = malloc(optlen)) == NULL) {
			dprintf(LOG_ERR, "%s"
			    "memory allocation failed for options", FNAME);
			goto fail;
		}
		valp = (u_int16_t *)tmpbuf;
		for (opt = TAILQ_FIRST(&optinfo->reqopt_list); opt;
		     opt = TAILQ_NEXT(opt, link), valp++) {
			*valp = htons((u_int16_t)opt->val_num);
		}
		COPY_OPTION(DH6OPT_ORO, optlen, tmpbuf, p);
		free(tmpbuf);
	}

	if (!TAILQ_EMPTY(&optinfo->dns_list.addrlist)) {
		struct in6_addr *in6;
		struct dhcp6_listval *d;

		tmpbuf = NULL;
		optlen = dhcp6_count_list(&optinfo->dns_list.addrlist) *
			sizeof(struct in6_addr);
		if ((tmpbuf = malloc(optlen)) == NULL) {
			dprintf(LOG_ERR, "%s"
			    "memory allocation failed for DNS options", FNAME);
			goto fail;
		}
		in6 = (struct in6_addr *)tmpbuf;
		for (d = TAILQ_FIRST(&optinfo->dns_list.addrlist); d;
		     d = TAILQ_NEXT(d, link), in6++) {
			memcpy(in6, &d->val_addr6, sizeof(*in6));
		}
		COPY_OPTION(DH6OPT_DNS_SERVERS, optlen, tmpbuf, p);
		free(tmpbuf);
	}

    /*  added start pling 01/25/2010 */
	if (!TAILQ_EMPTY(&optinfo->sip_list)) {
		struct in6_addr *in6;
		struct dhcp6_listval *d;

		tmpbuf = NULL;
		optlen = dhcp6_count_list(&optinfo->sip_list) *
			sizeof(struct in6_addr);
		if ((tmpbuf = malloc(optlen)) == NULL) {
			dprintf(LOG_ERR, "%s"
			    "memory allocation failed for SIP options", FNAME);
			goto fail;
		}
		in6 = (struct in6_addr *)tmpbuf;
		for (d = TAILQ_FIRST(&optinfo->sip_list); d;
		     d = TAILQ_NEXT(d, link), in6++) {
			memcpy(in6, &d->val_addr6, sizeof(*in6));
		}
		COPY_OPTION(DH6OPT_SIP_SERVERS, optlen, tmpbuf, p);
		free(tmpbuf);
	}
	if (!TAILQ_EMPTY(&optinfo->ntp_list)) {
		struct in6_addr *in6;
		struct dhcp6_listval *d;

		tmpbuf = NULL;
		optlen = dhcp6_count_list(&optinfo->ntp_list) *
			sizeof(struct in6_addr);
		if ((tmpbuf = malloc(optlen)) == NULL) {
			dprintf(LOG_ERR, "%s"
			    "memory allocation failed for NTP options", FNAME);
			goto fail;
		}
		in6 = (struct in6_addr *)tmpbuf;
		for (d = TAILQ_FIRST(&optinfo->ntp_list); d;
		     d = TAILQ_NEXT(d, link), in6++) {
			memcpy(in6, &d->val_addr6, sizeof(*in6));
		}
		COPY_OPTION(DH6OPT_NTP_SERVERS, optlen, tmpbuf, p);
		free(tmpbuf);
	}
    /*  added end pling 01/25/2010 */

	if (optinfo->dns_list.domainlist != NULL) {
		struct domain_list *dlist;
		u_char *dst;
		optlen = 0;
		tmpbuf = NULL;
		if ((tmpbuf = malloc(MAXDNAME * MAXDN)) == NULL) {
			dprintf(LOG_ERR, "%s"
			    "memory allocation failed for DNS options", FNAME);
			goto fail;
		}
		dst = tmpbuf;
		for (dlist = optinfo->dns_list.domainlist; dlist; dlist = dlist->next) {
			int n;
			n = dn_comp(dlist->name, dst, MAXDNAME, NULL, NULL);
			if (n < 0) {
				dprintf(LOG_ERR, "%s" "compress domain name failed", FNAME);
				goto fail;
			} else 
				dprintf(LOG_DEBUG, "compress domain name %s", dlist->name);
			optlen += n ;
			dst += n;
		}
		COPY_OPTION(DH6OPT_DOMAIN_LIST, optlen, tmpbuf, p);
		free(tmpbuf);
	}
		
	
	return (len);

  fail:
	if (tmpbuf)
		free(tmpbuf);
	return (-1);
}
#undef COPY_OPTION

void
dhcp6_set_timeoparam(ev)
	struct dhcp6_event *ev;
{
	ev->retrans = 0;
	ev->init_retrans = 0;
	ev->max_retrans_cnt = 0;
	ev->max_retrans_dur = 0;
	ev->max_retrans_time = 0;

	switch(ev->state) {
	case DHCP6S_SOLICIT:
		ev->init_retrans = SOL_TIMEOUT;
		ev->max_retrans_time = SOL_MAX_RT;
		break;
	case DHCP6S_INFOREQ:
		ev->init_retrans = INF_TIMEOUT;
		ev->max_retrans_time = INF_MAX_RT;
		break;
	case DHCP6S_REQUEST:
		ev->init_retrans = REQ_TIMEOUT;
		ev->max_retrans_time = REQ_MAX_RT;
		ev->max_retrans_cnt = REQ_MAX_RC;
		break;
	case DHCP6S_RENEW:
		ev->init_retrans = REN_TIMEOUT;
		ev->max_retrans_time = REN_MAX_RT;
		break;
	case DHCP6S_REBIND:
		ev->init_retrans = REB_TIMEOUT;
		ev->max_retrans_time = REB_MAX_RT;
		break;
        case DHCP6S_DECLINE:
                ev->init_retrans = DEC_TIMEOUT;
                ev->max_retrans_cnt = DEC_MAX_RC;
                break;
        case DHCP6S_RELEASE:
                ev->init_retrans = REL_TIMEOUT;
                ev->max_retrans_cnt = REL_MAX_RC;
                break;
        case DHCP6S_CONFIRM:
                ev->init_retrans = CNF_TIMEOUT;
                ev->max_retrans_dur = CNF_MAX_RD;
                ev->max_retrans_time = CNF_MAX_RT;
		break;
	default:
		dprintf(LOG_INFO, "%s" "unexpected event state %d on %s",
		    FNAME, ev->state, ev->ifp->ifname);
		exit(1);
	}
}

void
dhcp6_reset_timer(ev)
	struct dhcp6_event *ev;
{
	double n, r;
	char *statestr;
	struct timeval interval;

	switch(ev->state) {
	case DHCP6S_INIT:
		/*
		 * The first Solicit message from the client on the interface
		 * MUST be delayed by a random amount of time between
		 * MIN_SOL_DELAY and MAX_SOL_DELAY.
		 * [dhcpv6-28 14.]
		 */
        /*  modified start pling 08/26/2009 */
        /* In IPv6 auto mode (when DHCIFF_SOLICIT_ONLY is set), 
         * send immediately.
         */
        if ((ev->ifp->send_flags & DHCIFF_SOLICIT_ONLY))
            ev->retrans = 0;
        else
		    ev->retrans = (random() % (MAX_SOL_DELAY - MIN_SOL_DELAY)) +
			    MIN_SOL_DELAY;
        /*  modified end pling 08/26/2009 */
		break;
	default:
		if (ev->timeouts == 0) {
			/*
			 * The first RT MUST be selected to be strictly
			 * greater than IRT by choosing RAND to be strictly
			 * greater than 0.
			 * [dhcpv6-28 14.]
			 */
			r = (double)((random() % 1000) + 1) / 10000;
			n = ev->init_retrans + r * ev->init_retrans;
		} else {
			r = (double)((random() % 2000) - 1000) / 10000;

			if (ev->timeouts == 0) {
				n = ev->init_retrans + r * ev->init_retrans;
			} else
				n = 2 * ev->retrans + r * ev->retrans;
		}
		if (ev->max_retrans_time && n > ev->max_retrans_time)
			n = ev->max_retrans_time + r * ev->max_retrans_time;
        /*  modified start pling 08/26/2009 */
        /* In IPv6 auto mode (when DHCIFF_SOLICIT_ONLY is set),
         * then send 1 DHCP Solicit every 1 sec.
         */
        if ((ev->ifp->send_flags & DHCIFF_SOLICIT_ONLY))
            ev->retrans = 1000;
        else
    		ev->retrans = (long)n;
        /*  modified end pling 08/26/2009 */
		break;
	}

	switch(ev->state) {
	case DHCP6S_INIT:
		statestr = "INIT";
		break;
	case DHCP6S_SOLICIT:
		statestr = "SOLICIT";
		break;
	case DHCP6S_INFOREQ:
		statestr = "INFOREQ";
		break;
	case DHCP6S_REQUEST:
		statestr = "REQUEST";
		break;
	case DHCP6S_RENEW:
		statestr = "RENEW";
		break;
	case DHCP6S_REBIND:
		statestr = "REBIND";
		break;
	case DHCP6S_CONFIRM:
		statestr = "CONFIRM";
		break;
	case DHCP6S_DECLINE:
		statestr = "DECLINE";
		break;
	case DHCP6S_RELEASE:
		statestr = "RELEASE";
		break;
	case DHCP6S_IDLE:
		statestr = "IDLE";
		break;
	default:
		statestr = "???";
		break;
	}

	interval.tv_sec = (ev->retrans * 1000) / 1000000;
	interval.tv_usec = (ev->retrans * 1000) % 1000000;
	dhcp6_set_timer(&interval, ev->timer);

	dprintf(LOG_DEBUG, "%s" "reset a timer on %s, "
		"state=%s, timeo=%d, retrans=%ld", FNAME,
		ev->ifp->ifname, statestr, ev->timeouts, (long) ev->retrans);
}

int
duidcpy(struct duid *dd, const struct duid *ds)
{
	dd->duid_len = ds->duid_len;
	if ((dd->duid_id = malloc(dd->duid_len)) == NULL) {
		dprintf(LOG_ERR, "%s" "len %d memory allocation failed", FNAME, dd->duid_len);
		return (-1);
	}
	memcpy(dd->duid_id, ds->duid_id, dd->duid_len);

	return (0);
}

int
duidcmp(const struct duid *d1, 
	const struct duid *d2)
{
	if (d1->duid_len == d2->duid_len) {
		return (memcmp(d1->duid_id, d2->duid_id, d1->duid_len));
	} else
		return (-1);
}

void
duidfree(duid)
	struct duid *duid;
{
	dprintf(LOG_DEBUG, "%s" "DUID is %s, DUID_LEN is %d", 
			FNAME, duidstr(duid), duid->duid_len);
	if (duid->duid_id != NULL && duid->duid_len != 0) {
		dprintf(LOG_DEBUG, "%s" "removing ID (ID: %s)",
		    FNAME, duidstr(duid));
		free(duid->duid_id);
		duid->duid_id = NULL;
		duid->duid_len = 0;
	}
	duid->duid_len = 0;
}

char *
dhcp6optstr(type)
	int type;
{
	static char genstr[sizeof("opt_65535") + 1];

	if (type > 65535)
		return "INVALID option";

	switch(type) {
	case DH6OPT_CLIENTID:
		return "client ID";
	case DH6OPT_SERVERID:
		return "server ID";
	case DH6OPT_ORO:
		return "option request";
	case DH6OPT_PREFERENCE:
		return "preference";
	case DH6OPT_STATUS_CODE:
		return "status code";
	case DH6OPT_RAPID_COMMIT:
		return "rapid commit";
	case DH6OPT_DNS_SERVERS:
		return "DNS_SERVERS";
    /*  added start pling 09/23/2010 */
    case DH6OPT_DOMAIN_LIST:
        return "DOMAIN_LIST";
    /*  added end pling 09/23/2010 */
    /*  added start pling 01/25/2010 */
	case DH6OPT_SIP_SERVERS:
		return "SIP_SERVERS";
	case DH6OPT_NTP_SERVERS:
		return "NTP_SERVERS";
    /*  added end pling 01/25/2010 */
	default:
		sprintf(genstr, "opt_%d", type);
		return (genstr);
	}
}

char *
dhcp6msgstr(type)
	int type;
{
	static char genstr[sizeof("msg255") + 1];

	if (type > 255)
		return "INVALID msg";

	switch(type) {
	case DH6_SOLICIT:
		return "solicit";
	case DH6_ADVERTISE:
		return "advertise";
	case DH6_RENEW:
		return "renew";
	case DH6_REBIND:
		return "rebind";
	case DH6_REQUEST:
		return "request";
	case DH6_REPLY:
		return "reply";
	case DH6_CONFIRM:
		return "confirm";
	case DH6_RELEASE:
		return "release";
	case DH6_DECLINE:
		return "decline";
	case DH6_INFORM_REQ:
		return "information request";
	case DH6_RECONFIGURE:
		return "reconfigure";
	case DH6_RELAY_FORW:
		return "relay forwarding";
	case DH6_RELAY_REPL:
		return "relay reply";
	default:
		sprintf(genstr, "msg%d", type);
		return (genstr);
	}
}

char *
dhcp6_stcodestr(code)
	int code;
{
	static char genstr[sizeof("code255") + 1];

	if (code > 255)
		return "INVALID code";

	switch(code) {
	case DH6OPT_STCODE_SUCCESS:
		return "success";
	case DH6OPT_STCODE_UNSPECFAIL:
		return "unspec failure";
	case DH6OPT_STCODE_AUTHFAILED:
		return "auth fail";
	case DH6OPT_STCODE_ADDRUNAVAIL:
		return "address unavailable";
	case DH6OPT_STCODE_NOADDRAVAIL:
		return "no addresses";
	case DH6OPT_STCODE_NOBINDING:
		return "no binding";
	case DH6OPT_STCODE_CONFNOMATCH:
		return "confirm no match";
	case DH6OPT_STCODE_NOTONLINK:
		return "not on-link";
	case DH6OPT_STCODE_USEMULTICAST:
		return "use multicast";
	default:
		sprintf(genstr, "code%d", code);
		return (genstr);
	}
}

char *
duidstr(const struct duid *duid)
{
	int i;
	char *cp;
	static char duidstr[sizeof("xx:") * 256 + sizeof("...")];
	
	duidstr[0] ='\0';
	
	cp = duidstr;
	for (i = 0; i < duid->duid_len && i <= 256; i++) {
		cp += sprintf(cp, "%s%02x", i == 0 ? "" : ":",
			      duid->duid_id[i] & 0xff);
	}
	if (i < duid->duid_len)
		sprintf(cp, "%s", "...");

	return (duidstr);
}

void
setloglevel(debuglevel)
	int debuglevel;
{
	if (foreground) {
		switch(debuglevel) {
		case 0:
			debug_thresh = LOG_ERR;
			break;
		case 1:
			debug_thresh = LOG_INFO;
			break;
		default:
			debug_thresh = LOG_DEBUG;
			break;
		}
	} else {
		switch(debuglevel) {
		case 0:
			setlogmask(LOG_UPTO(LOG_ERR));
			break;
		case 1:
			setlogmask(LOG_UPTO(LOG_INFO));
			break;
		}
	}
}

void
dprintf(int level, const char *fmt, ...)
{
	va_list ap;
	char logbuf[LINE_MAX];

	va_start(ap, fmt);
	vsnprintf(logbuf, sizeof(logbuf), fmt, ap);

	if (foreground && debug_thresh >= level) {
		time_t now;
		struct tm *tm_now;
		const char *month[] = {
			"Jan", "Feb", "Mar", "Apr", "May", "Jun",
			"Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
		};

		if ((now = time(NULL)) < 0)
			exit(1);
		tm_now = localtime(&now);
		fprintf(stderr, "%3s/%02d/%04d %02d:%02d:%02d %s\n",
			month[tm_now->tm_mon], tm_now->tm_mday,
			tm_now->tm_year + 1900,
			tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec,
			logbuf);
	} else
		syslog(level, "%s", logbuf);
}

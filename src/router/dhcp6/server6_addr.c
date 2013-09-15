/*	$Id: server6_addr.c 241182 2011-02-17 21:50:03Z $	*/

/*
 * Copyright (C) International Business Machines  Corp., 2003
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

/* Author: Shirley Ma, xma@us.ibm.com */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
//#include <openssl/md5.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>


#include <net/if.h>
#include <netinet/in.h>

#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>

#include "queue.h"
#include "dhcp6.h"
#include "config.h"
#include "common.h"
#include "server6_conf.h"
#include "lease.h"
#include "timer.h"
#include "hash.h"

extern FILE *server6_lease_file;

struct dhcp6_lease *
dhcp6_find_lease __P((struct dhcp6_iaidaddr *, struct dhcp6_addr *));
static int dhcp6_add_lease __P((struct dhcp6_iaidaddr *, struct dhcp6_addr *));
static int dhcp6_update_lease __P((struct dhcp6_addr *, struct dhcp6_lease *));
static int addr_on_segment __P((struct v6addrseg *, struct dhcp6_addr *));
static void  server6_get_newaddr __P((iatype_t, struct dhcp6_addr *, struct v6addrseg *));
static void  server6_get_addrpara __P((struct dhcp6_addr *, struct v6addrseg *));
static void  server6_get_prefixpara __P((struct dhcp6_addr *, struct v6prefix *));

struct link_decl *dhcp6_allocate_link __P((struct dhcp6_if *, struct rootgroup *, 
			struct in6_addr *));
struct host_decl *dhcp6_allocate_host __P((struct dhcp6_if *, struct rootgroup *, 
			struct dhcp6_optinfo *));
int dhcp6_get_hostconf __P((struct dhcp6_optinfo *, struct dhcp6_optinfo *,
			struct dhcp6_iaidaddr *, struct host_decl *)); 

struct host_decl
*find_hostdecl(duid, iaid, hostlist)
	struct duid *duid;
	u_int32_t iaid;
	struct host_decl *hostlist;
{
	struct host_decl *host;
	for (host = hostlist; host; host = host->next) {
		if (!duidcmp(duid, &host->cid) && host->iaidinfo.iaid == iaid)
			return host;
		continue;
	}
		
	return NULL;
}

/* for request/solicit rapid commit */
int
dhcp6_add_iaidaddr(optinfo)
	struct dhcp6_optinfo *optinfo;
{
	struct dhcp6_iaidaddr *iaidaddr;
	struct dhcp6_listval *lv, *lv_next = NULL;
	struct timeval timo;
	double d;
	
	iaidaddr = (struct dhcp6_iaidaddr *)malloc(sizeof(*iaidaddr));
	if (iaidaddr == NULL) {
		dprintf(LOG_ERR, "%s" "failed to allocate memory", FNAME);
		return (-1);
	}
	memset(iaidaddr, 0, sizeof(*iaidaddr));
	duidcpy(&iaidaddr->client6_info.clientid, &optinfo->clientID);
	iaidaddr->client6_info.iaidinfo.iaid = optinfo->iaidinfo.iaid;
	iaidaddr->client6_info.type = optinfo->type;
	TAILQ_INIT(&iaidaddr->lease_list);
	/* add new leases */
	for (lv = TAILQ_FIRST(&optinfo->addr_list); lv; lv = lv_next) {
		lv_next = TAILQ_NEXT(lv, link);
		if ((hash_search(lease_hash_table, (void *)&lv->val_dhcp6addr)) != NULL) {
			dprintf(LOG_INFO, "%s" "address for %s has been used",
				FNAME, in6addr2str(&lv->val_dhcp6addr.addr, 0));
			TAILQ_REMOVE(&optinfo->addr_list, lv, link);
			continue;
		}
		if (dhcp6_add_lease(iaidaddr, &lv->val_dhcp6addr) != 0)
			TAILQ_REMOVE(&optinfo->addr_list, lv, link); 
	}
	/* it's meaningless to have an iaid without any leases */	
	if (TAILQ_EMPTY(&iaidaddr->lease_list)) {
		dprintf(LOG_INFO, "%s" "no leases are added for duid %s iaid %u", 
			FNAME, duidstr(&iaidaddr->client6_info.clientid), 
				iaidaddr->client6_info.iaidinfo.iaid);
		return (0);
	}
	if (hash_add(server6_hash_table, &iaidaddr->client6_info, iaidaddr)) {
		dprintf(LOG_ERR, "%s" "failed to hash_add an iaidaddr %u for client duid %s", 
			FNAME, iaidaddr->client6_info.iaidinfo.iaid,
				duidstr(&iaidaddr->client6_info.clientid));
		dhcp6_remove_iaidaddr(iaidaddr);
		return (-1);
	}
	dprintf(LOG_DEBUG, "%s" "hash_add an iaidaddr %u for client duid %s", 
		FNAME, iaidaddr->client6_info.iaidinfo.iaid,
			duidstr(&iaidaddr->client6_info.clientid));
	/* set up timer for iaidaddr */
	if ((iaidaddr->timer =
	    dhcp6_add_timer(dhcp6_iaidaddr_timo, iaidaddr)) == NULL) {
		dprintf(LOG_ERR, "%s" "failed to add a timer for iaid %u",
			FNAME, iaidaddr->client6_info.iaidinfo.iaid);
		dhcp6_remove_iaidaddr(iaidaddr);
		return (-1);
	}
	time(&iaidaddr->start_date);
	iaidaddr->state = ACTIVE;
	d = get_max_validlifetime(iaidaddr);
	timo.tv_sec = (long)d;
	timo.tv_usec = 0;
	dhcp6_set_timer(&timo, iaidaddr->timer);
	return (0);
}

int 
dhcp6_remove_iaidaddr(iaidaddr)
	struct dhcp6_iaidaddr *iaidaddr;
{
	struct dhcp6_lease *lv, *lv_next;
	struct dhcp6_lease *lease;
	
	/* remove all the leases in this iaid */
	for (lv = TAILQ_FIRST(&iaidaddr->lease_list); lv; lv = lv_next) {
		lv_next = TAILQ_NEXT(lv, link);
		if ((lease = hash_search(lease_hash_table, (void *)&lv->lease_addr)) != NULL) {
			if (dhcp6_remove_lease(lv)) {
				dprintf(LOG_ERR, "%s" "failed to remove an iaid %u", FNAME,
					 iaidaddr->client6_info.iaidinfo.iaid);
				return (-1);
			}
		}
	}
	if (hash_delete(server6_hash_table, &iaidaddr->client6_info) != 0) {
		dprintf(LOG_ERR, "%s" "failed to remove an iaid %u from hash",
			FNAME, iaidaddr->client6_info.iaidinfo.iaid);
		return (-1);
	}
	if (iaidaddr->timer)
		dhcp6_remove_timer(iaidaddr->timer);
	dprintf(LOG_DEBUG, "%s" "removed iaidaddr %u", FNAME,
		iaidaddr->client6_info.iaidinfo.iaid);
	free(iaidaddr);
	return (0);
}

struct dhcp6_iaidaddr
*dhcp6_find_iaidaddr(optinfo)
	struct dhcp6_optinfo *optinfo;
{
	struct dhcp6_iaidaddr *iaidaddr;
	struct client6_if client6_info;
	duidcpy(&client6_info.clientid, &optinfo->clientID);
	client6_info.iaidinfo.iaid = optinfo->iaidinfo.iaid;
	client6_info.type = optinfo->type;
	if ((iaidaddr = hash_search(server6_hash_table, (void *)&client6_info)) == NULL) {
		dprintf(LOG_DEBUG, "%s" "iaid %u iaidaddr for client duid %s doesn't exists", 
			FNAME, client6_info.iaidinfo.iaid, 
			duidstr(&client6_info.clientid));
	}
	duidfree(&client6_info.clientid);
	return iaidaddr;
}

int
dhcp6_remove_lease(lease)
	struct dhcp6_lease *lease;
{
	lease->state = INVALID;
	if (write_lease(lease, server6_lease_file) != 0) {
		dprintf(LOG_ERR, "%s" "failed to write an invalid lease %s to lease file", 
			FNAME, in6addr2str(&lease->lease_addr.addr, 0));
		return (-1);
	}
	if (hash_delete(lease_hash_table, &lease->lease_addr) != 0) {
		dprintf(LOG_ERR, "%s" "failed to remove an address %s from hash", 
			FNAME, in6addr2str(&lease->lease_addr.addr, 0));
		return (-1);
	}
	if (lease->timer)
		dhcp6_remove_timer(lease->timer);
	TAILQ_REMOVE(&lease->iaidaddr->lease_list, lease, link);
	dprintf(LOG_DEBUG, "%s" "removed lease %s", FNAME,
		in6addr2str(&lease->lease_addr.addr, 0));
	free(lease);
	return 0;
}
		
/* for renew/rebind/release/decline */
int
dhcp6_update_iaidaddr(optinfo, flag)
	struct dhcp6_optinfo *optinfo;
	int flag;
{
	struct dhcp6_iaidaddr *iaidaddr;
	struct dhcp6_lease *lease, *lease_next = NULL;
	struct dhcp6_listval *lv, *lv_next = NULL;
	struct timeval timo;
	double d;

	if ((iaidaddr = dhcp6_find_iaidaddr(optinfo)) == NULL) {
		return (-1);
	}
	
	if (flag == ADDR_UPDATE) {		
		/* add or update new lease */
		for (lv = TAILQ_FIRST(&optinfo->addr_list); lv; lv = lv_next) {
			lv_next = TAILQ_NEXT(lv, link);
			dprintf(LOG_DEBUG, "%s" "address is %s " , FNAME, 
					in6addr2str(&lv->val_dhcp6addr.addr,0));
			if ((lease = dhcp6_find_lease(iaidaddr, &lv->val_dhcp6addr)) 
					!= NULL) {
				dhcp6_update_lease(&lv->val_dhcp6addr, lease);
			} else {
				dhcp6_add_lease(iaidaddr, &lv->val_dhcp6addr);
			}
		}
		/* remove leases that not on the reply list */
		for (lease = TAILQ_FIRST(&iaidaddr->lease_list); lease; lease = lease_next) {
			lease_next = TAILQ_NEXT(lease, link);
			if (!addr_on_addrlist(&optinfo->addr_list, &lease->lease_addr)) {
				dprintf(LOG_DEBUG, "%s" "lease %s is not on the link", 
						FNAME, in6addr2str(&lease->lease_addr.addr,0));
				dhcp6_remove_lease(lease);
			}
		}
		dprintf(LOG_DEBUG, "%s" "update iaidaddr for iaid %u", FNAME, 
			iaidaddr->client6_info.iaidinfo.iaid);
	} else {
		/* remove leases */
		for (lv = TAILQ_FIRST(&optinfo->addr_list); lv; lv = lv_next) {
			lv_next = TAILQ_NEXT(lv, link);
			lease = dhcp6_find_lease(iaidaddr, &lv->val_dhcp6addr);
			if (lease) {
				if (flag == ADDR_ABANDON) {
				}
				dhcp6_remove_lease(lease);
			} else {
				dprintf(LOG_INFO, "%s" "address is not on the iaid", FNAME);
			}
		}
	
	}
	/* it's meaningless to have an iaid without any leases */	
	if (TAILQ_EMPTY(&iaidaddr->lease_list)) {
		dprintf(LOG_INFO, "%s" "no leases are added for duid %s iaid %u", 
			FNAME, duidstr(&iaidaddr->client6_info.clientid), 
				iaidaddr->client6_info.iaidinfo.iaid);
		dhcp6_remove_iaidaddr(iaidaddr);
		return (0);
	}
	/* update the start date and timer */
	if (iaidaddr->timer == NULL) {
		if ((iaidaddr->timer = 
		     dhcp6_add_timer(dhcp6_iaidaddr_timo, iaidaddr)) == NULL) {
	 		dprintf(LOG_ERR, "%s" "failed to add a timer for iaid %u",
				FNAME, iaidaddr->client6_info.iaidinfo.iaid);
	 		return (-1);
	    	}
	}
	time(&iaidaddr->start_date);
	iaidaddr->state = ACTIVE;
	d = get_max_validlifetime(iaidaddr);
	timo.tv_sec = (long)d;
	timo.tv_usec = 0;
	dhcp6_set_timer(&timo, iaidaddr->timer);
	return (0);
}

int
dhcp6_validate_bindings(optinfo, iaidaddr)
	struct dhcp6_optinfo *optinfo;
	struct dhcp6_iaidaddr *iaidaddr;
{
	struct dhcp6_listval *lv;

	for (lv = TAILQ_FIRST(&optinfo->addr_list); lv; lv = TAILQ_NEXT(lv, link)) {
		if (dhcp6_find_lease(iaidaddr, &lv->val_dhcp6addr) == NULL) 
			return (-1);
	}
	return 0;
}

int
dhcp6_add_lease(iaidaddr, addr)
	struct dhcp6_iaidaddr *iaidaddr;
	struct dhcp6_addr *addr;
{
	struct dhcp6_lease *sp;
	struct timeval timo;
	double d;
	if (addr->status_code != DH6OPT_STCODE_SUCCESS &&
			addr->status_code != DH6OPT_STCODE_UNDEFINE) {
		dprintf(LOG_ERR, "%s" "not successful status code for %s is %s", FNAME,
			in6addr2str(&addr->addr, 0), dhcp6_stcodestr(addr->status_code));
		return (0);
	}
	/* ignore meaningless address, this never happens */
	if (addr->validlifetime == 0 || addr->preferlifetime == 0) {
		dprintf(LOG_INFO, "%s" "zero address life time for %s",
			FNAME, in6addr2str(&addr->addr, 0));
		return (0);
	}

	if (((sp = hash_search(lease_hash_table, (void *)addr))) != NULL) {
		dprintf(LOG_INFO, "%s" "duplicated address: %s",
		    FNAME, in6addr2str(&addr->addr, 0));
		return (-1);
	}

	if ((sp = (struct dhcp6_lease *)malloc(sizeof(*sp))) == NULL) {
		dprintf(LOG_ERR, "%s" "failed to allocate memory"
			" for an address", FNAME);
		return (-1);
	}
	memset(sp, 0, sizeof(*sp));
	memcpy(&sp->lease_addr, addr, sizeof(sp->lease_addr));
	sp->iaidaddr = iaidaddr;	
	/* ToDo: preferlifetime EXPIRED; validlifetime DELETED; */
	/* if a finite lease perferlifetime is specified, set up a timer. */
	time(&sp->start_date);
	dprintf(LOG_DEBUG, "%s" "start date is %ld", FNAME, sp->start_date);
	sp->state = ACTIVE;
	if (write_lease(sp, server6_lease_file) != 0) {
		dprintf(LOG_ERR, "%s" "failed to write a new lease address %s to lease file", 
			FNAME, in6addr2str(&sp->lease_addr.addr, 0));
		free(sp->timer);
		free(sp);
		return (-1);
	}
	dprintf(LOG_DEBUG, "%s" "write lease %s/%d to lease file", FNAME,
		in6addr2str(&sp->lease_addr.addr, 0), sp->lease_addr.plen);
	if (hash_add(lease_hash_table, &sp->lease_addr, sp)) {
		dprintf(LOG_ERR, "%s" "failed to add hash for an address", FNAME);
			free(sp->timer);
			free(sp);
			return (-1);
	}
	TAILQ_INSERT_TAIL(&iaidaddr->lease_list, sp, link);
	if (sp->lease_addr.validlifetime == DHCP6_DURATITION_INFINITE || 
	    sp->lease_addr.preferlifetime == DHCP6_DURATITION_INFINITE) {
		dprintf(LOG_INFO, "%s" "infinity address life time for %s",
			FNAME, in6addr2str(&addr->addr, 0));
		return (0);
	}
	if ((sp->timer = dhcp6_add_timer(dhcp6_lease_timo, sp)) == NULL) {
		dprintf(LOG_ERR, "%s" "failed to create a new event "
	    		"timer", FNAME);
		free(sp);
		return (-1); 
	}
	d = sp->lease_addr.preferlifetime; 
	timo.tv_sec = (long)d;
	timo.tv_usec = 0;
	dhcp6_set_timer(&timo, sp->timer);
	dprintf(LOG_DEBUG, "%s" "add lease for %s/%d iaid %u with preferlifetime %u"
			" with validlifetime %u", FNAME,
		in6addr2str(&sp->lease_addr.addr, 0), sp->lease_addr.plen, 
		sp->iaidaddr->client6_info.iaidinfo.iaid,
		sp->lease_addr.preferlifetime, sp->lease_addr.validlifetime);
	return (0);
}

/* assume we've found the updated lease already */
int
dhcp6_update_lease(addr, sp)
	struct dhcp6_addr *addr;
	struct dhcp6_lease *sp;
{
	struct timeval timo;
	double d;
	if (addr->status_code != DH6OPT_STCODE_SUCCESS &&
			addr->status_code != DH6OPT_STCODE_UNDEFINE) {
		dprintf(LOG_ERR, "%s" "not successful status code for %s is %s", FNAME,
			in6addr2str(&addr->addr, 0), dhcp6_stcodestr(addr->status_code));
		dhcp6_remove_lease(sp);
		return (0);
	}
	/* remove lease with perferlifetime or validlifetime 0 */
	if (addr->validlifetime == 0 || addr->preferlifetime == 0) {
		dprintf(LOG_INFO, "%s" "zero address life time for %s",
			FNAME, in6addr2str(&addr->addr, 0));
		dhcp6_remove_lease(sp);
		return (0);
	}
	memcpy(&sp->lease_addr, addr, sizeof(sp->lease_addr));
	time(&sp->start_date);
	sp->state = ACTIVE;
	if (write_lease(sp, server6_lease_file) != 0) {
		dprintf(LOG_ERR, "%s" "failed to write an updated lease %s to lease file", 
			FNAME, in6addr2str(&sp->lease_addr.addr, 0));
		return (-1);
	}
	if (sp->lease_addr.validlifetime == DHCP6_DURATITION_INFINITE || 
	    sp->lease_addr.preferlifetime == DHCP6_DURATITION_INFINITE) {
		dprintf(LOG_INFO, "%s" "infinity address life time for %s",
			FNAME, in6addr2str(&addr->addr, 0));
		return (0);
	}
	if (sp->timer == NULL) {
		if ((sp->timer = dhcp6_add_timer(dhcp6_lease_timo, sp)) == NULL) {
			dprintf(LOG_ERR, "%s" "failed to create a new event "
	    			"timer", FNAME);
			return (-1); 
		}
	}
	d = sp->lease_addr.preferlifetime; 
	timo.tv_sec = (long)d;
	timo.tv_usec = 0;
	dhcp6_set_timer(&timo, sp->timer);
	return (0);
}

struct dhcp6_lease *
dhcp6_find_lease(iaidaddr, ifaddr)
	struct dhcp6_iaidaddr *iaidaddr;
	struct dhcp6_addr *ifaddr;
{
	struct dhcp6_lease *sp;
	for (sp = TAILQ_FIRST(&iaidaddr->lease_list); sp;
	     sp = TAILQ_NEXT(sp, link)) {
		/* check for prefix length
		 * sp->lease_addr.plen == ifaddr->plen &&
		 */
      		dprintf(LOG_DEBUG, "%s" "request address is %s/%d ", FNAME,
			in6addr2str(&ifaddr->addr, 0), ifaddr->plen);		
      		dprintf(LOG_DEBUG, "%s" "lease address is %s/%d ", FNAME,
			in6addr2str(&sp->lease_addr.addr, 0), ifaddr->plen);		
	      	if (IN6_ARE_ADDR_EQUAL(&sp->lease_addr.addr, &ifaddr->addr)) {
			if (ifaddr->type == IAPD) {
				if (sp->lease_addr.plen == ifaddr->plen)
					return (sp);
			} else if (ifaddr->type == IANA || ifaddr->type == IATA)
				return (sp);
		}
	}
	return (NULL);
}

struct dhcp6_timer *
dhcp6_iaidaddr_timo(void *arg)
{
	struct dhcp6_iaidaddr *sp = (struct dhcp6_iaidaddr *)arg;

	dprintf(LOG_DEBUG, "server6_iaidaddr timeout for %u, state=%d", 
		sp->client6_info.iaidinfo.iaid, sp->state);
	switch(sp->state) {
	case ACTIVE:
	case EXPIRED:
		sp->state = EXPIRED;
		dhcp6_remove_iaidaddr(sp);
	default:
		break;
	}
	return (NULL);
}
		
struct dhcp6_timer *
dhcp6_lease_timo(arg)
	void *arg;
{
	struct dhcp6_lease *sp = (struct dhcp6_lease *)arg;
	struct timeval timeo;
	double d;

	dprintf(LOG_DEBUG, "%s" "lease timeout for %s, state=%d", FNAME,
		in6addr2str(&sp->lease_addr.addr, 0), sp->state);

	switch(sp->state) {
	case ACTIVE:
		sp->state = EXPIRED;
		d = sp->lease_addr.validlifetime - sp->lease_addr.preferlifetime;
		timeo.tv_sec = (long)d;
		timeo.tv_usec = 0;
		dhcp6_set_timer(&timeo, sp->timer);
		break;
	case EXPIRED:
	case INVALID:
		sp->state = INVALID;
		dhcp6_remove_lease(sp);
		return (NULL);
	default:
		return (NULL);
	}
	return (sp->timer);
}

static void
get_random_bytes(u_int8_t seed[], int num)
{
	int i;
	for (i = 0; i < num; i++)
		seed[i] = random();
	return;
}


static void 
create_tempaddr(prefix, plen, tempaddr)
	struct in6_addr *prefix;
	int plen;
	struct in6_addr *tempaddr;
{
	int i, num_bytes;
	u_int8_t seed[16];

	get_random_bytes(seed, 16);
	/* address mask */
	memset(tempaddr, 0, sizeof(*tempaddr));	
	num_bytes = plen / 8;
	for (i = 0; i < num_bytes; i++) {
		tempaddr->s6_addr[i] = prefix->s6_addr[i];
	}
	tempaddr->s6_addr[num_bytes] = (prefix->s6_addr[num_bytes] | (0xFF >> plen % 8)) 
		& (seed[num_bytes] | ((0xFF << 8) - plen % 8));
	
	for (i = num_bytes + 1; i < 16; i++) {
		tempaddr->s6_addr[i] = seed[i];
	}
	return;
}

int
dhcp6_get_hostconf(roptinfo, optinfo, iaidaddr, host)
	struct host_decl *host;
	struct dhcp6_iaidaddr *iaidaddr;
	struct dhcp6_optinfo *optinfo, *roptinfo;
{
	struct dhcp6_list *reply_list = &roptinfo->addr_list;
	
	if (!(host->hostscope.allow_flags & DHCIFF_TEMP_ADDRS)) {
		roptinfo->iaidinfo.renewtime = host->hostscope.renew_time;
		roptinfo->iaidinfo.rebindtime = host->hostscope.rebind_time;
		roptinfo->type = optinfo->type;
		switch (optinfo->type) {
		case IANA:
			dhcp6_copy_list(reply_list, &host->addrlist);
			break;
		case IATA:
			break;
		case IAPD:
			dhcp6_copy_list(reply_list, &host->prefixlist);
			break;
		}
	}
	return 0;
}

int
dhcp6_create_addrlist(roptinfo, optinfo, iaidaddr, subnet)
	struct dhcp6_optinfo *roptinfo;
	struct dhcp6_optinfo *optinfo; 
	const struct dhcp6_iaidaddr *iaidaddr;
	const struct link_decl *subnet;
{
	struct dhcp6_listval *v6addr;
	struct v6addrseg *seg;
	struct dhcp6_list *reply_list = &roptinfo->addr_list;
	struct dhcp6_list *req_list = &optinfo->addr_list;
	int numaddr;
	struct dhcp6_listval *lv, *lv_next = NULL;

	roptinfo->iaidinfo.renewtime = subnet->linkscope.renew_time;
	roptinfo->iaidinfo.rebindtime = subnet->linkscope.rebind_time;
	roptinfo->type = optinfo->type;
	/* check the duplication */
	for (lv = TAILQ_FIRST(req_list); lv; lv = lv_next) {
		lv_next = TAILQ_NEXT(lv, link);
		if (addr_on_addrlist(reply_list, &lv->val_dhcp6addr)) {
			TAILQ_REMOVE(req_list, lv, link);
		}
	}	       
	dhcp6_copy_list(reply_list, req_list);
	for (lv = TAILQ_FIRST(reply_list); lv; lv = lv_next) {
			lv_next = TAILQ_NEXT(lv, link);
		lv->val_dhcp6addr.type = optinfo->type;
		lv->val_dhcp6addr.status_code = DH6OPT_STCODE_UNDEFINE;
		lv->val_dhcp6addr.status_msg = NULL;
	}
	for (seg = subnet->seglist; seg; seg = seg->next) {
		numaddr = 0;
		for (lv = TAILQ_FIRST(reply_list); lv; lv = lv_next) {
			lv_next = TAILQ_NEXT(lv, link);
			/* skip checked segment */
			if (lv->val_dhcp6addr.status_code == DH6OPT_STCODE_SUCCESS) 
				continue;
			if (IN6_IS_ADDR_RESERVED(&lv->val_dhcp6addr.addr) ||
			    is_anycast(&lv->val_dhcp6addr.addr, seg->prefix.plen)) {
				lv->val_dhcp6addr.status_code = DH6OPT_STCODE_NOTONLINK;
				dprintf(LOG_DEBUG, "%s" " %s address not on link", FNAME, 
					in6addr2str(&lv->val_dhcp6addr.addr, 0));
				continue;
			}
			lv->val_dhcp6addr.type = optinfo->type;
			if (addr_on_segment(seg, &lv->val_dhcp6addr)) {
				if (numaddr == 0) {
					lv->val_dhcp6addr.type = optinfo->type;
					server6_get_addrpara(&lv->val_dhcp6addr, seg);
					numaddr += 1;
				} else {
			/* check the addr count per seg, we only allow one address
			 * per segment, set the status code */
					lv->val_dhcp6addr.status_code 
							= DH6OPT_STCODE_NOADDRAVAIL;
				}
			} else {
				lv->val_dhcp6addr.status_code = DH6OPT_STCODE_NOTONLINK;
				dprintf(LOG_DEBUG, "%s" " %s address not on link", FNAME, 
					in6addr2str(&lv->val_dhcp6addr.addr, 0));
			}
		}
		if (iaidaddr != NULL) {
			struct dhcp6_lease *cl;
			for (cl = TAILQ_FIRST(&iaidaddr->lease_list); cl; 
					cl = TAILQ_NEXT(cl, link)) {
				if (addr_on_segment(seg, &cl->lease_addr)) { 
					if (addr_on_addrlist(reply_list, 
								&cl->lease_addr)) {
						continue;
					} else if (numaddr == 0) {
						v6addr = (struct dhcp6_listval *)malloc(sizeof(*v6addr));
						if (v6addr == NULL) {
							dprintf(LOG_ERR, "%s" 
								"fail to allocate memory %s", 
								FNAME, strerror(errno));
							return (-1);
						}
						memset(v6addr, 0, sizeof(*v6addr));
						memcpy(&v6addr->val_dhcp6addr, &cl->lease_addr,
							sizeof(v6addr->val_dhcp6addr));	
						v6addr->val_dhcp6addr.type = optinfo->type;
						server6_get_addrpara(&v6addr->val_dhcp6addr, 
									seg);
						numaddr += 1;
						TAILQ_INSERT_TAIL(reply_list, v6addr, link);
						continue;
					}
				}
					
			}
		}
		if (numaddr == 0) {
			v6addr = (struct dhcp6_listval *)malloc(sizeof(*v6addr));
			if (v6addr == NULL) {
				dprintf(LOG_ERR, "%s" "fail to allocate memory %s", 
					FNAME, strerror(errno));
				return (-1);
			}
			memset(v6addr, 0, sizeof(*v6addr));
			v6addr->val_dhcp6addr.type = optinfo->type;
			server6_get_newaddr(optinfo->type, &v6addr->val_dhcp6addr, seg);
			if (IN6_IS_ADDR_UNSPECIFIED(&v6addr->val_dhcp6addr.addr)) {
				free(v6addr);
				continue;
			}
			TAILQ_INSERT_TAIL(reply_list, v6addr, link);
		}
	}
	return (0);
}

static int
addr_on_segment(seg, addr)
	struct v6addrseg *seg;
	struct dhcp6_addr *addr;
{
	int onseg = 0;
	struct v6addr *prefix;
	dprintf(LOG_DEBUG, "%s" " checking address %s on segment", 
		FNAME, in6addr2str(&addr->addr, 0));
	switch (addr->type) {
	case IATA:
		prefix = getprefix(&addr->addr, seg->prefix.plen);
		if (prefix && !memcmp(&seg->prefix, prefix, sizeof(seg->prefix))) {
			dprintf(LOG_DEBUG, "%s" " address is on link", FNAME);
			onseg = 1;
		} else
			onseg = 0;
		free(prefix);
		break;
	case IANA:
		if (ipv6addrcmp(&addr->addr, &seg->min) >= 0 &&
		    ipv6addrcmp(&seg->max, &addr->addr) >= 0) {
			dprintf(LOG_DEBUG, "%s" " address is on link", FNAME);
			onseg = 1;
		}
		else 
			onseg = 0;
		break;
	default:
		break;
	}
	return onseg;
	
}

static void 
server6_get_newaddr(type, v6addr, seg)
	iatype_t type;
	struct dhcp6_addr *v6addr;
	struct v6addrseg *seg;
{
	struct in6_addr current;
	int round = 0;
	memcpy(&current, &seg->free, sizeof(current));
	do {
		v6addr->type = type;
		switch(type) {
		case IATA:
			/* assume the temp addr never being run out */
			create_tempaddr(&seg->prefix.addr, seg->prefix.plen, &v6addr->addr);
			break;	
		case IANA:
			memcpy(&v6addr->addr, &seg->free, sizeof(v6addr->addr));
			if (round && IN6_ARE_ADDR_EQUAL(&current, &v6addr->addr)) {
				memset(&v6addr->addr, 0, sizeof(v6addr->addr));
				break;
			}
			inc_ipv6addr(&seg->free);
			if (ipv6addrcmp(&seg->free, &seg->max) == 1 ) {
				round = 1;
				memcpy(&seg->free, &seg->min, sizeof(seg->free));
			}
			break;
		default:
			break;
		}

	} while ((hash_search(lease_hash_table, (void *)v6addr) != NULL) ||
		 (hash_search(host_addr_hash_table, (void *)&v6addr->addr) != NULL) ||
		 (is_anycast(&v6addr->addr, seg->prefix.plen)));
	if (IN6_IS_ADDR_UNSPECIFIED(&v6addr->addr)) {
		return;
	}
	dprintf(LOG_DEBUG, "new address %s is got", in6addr2str(&v6addr->addr, 0));
	server6_get_addrpara(v6addr, seg);
	return;
}

static void
server6_get_prefixpara(v6addr, seg)
	struct dhcp6_addr *v6addr;
	struct v6prefix *seg;
{
	v6addr->plen = seg->prefix.plen;
	if (seg->parainfo.prefer_life_time == 0 && seg->parainfo.valid_life_time == 0) {
		seg->parainfo.valid_life_time = DEFAULT_VALID_LIFE_TIME;
		seg->parainfo.prefer_life_time = DEFAULT_PREFERRED_LIFE_TIME;
	} else if (seg->parainfo.prefer_life_time == 0) {
		seg->parainfo.prefer_life_time = seg->parainfo.valid_life_time / 2;
	} else if (seg->parainfo.valid_life_time == 0) {
		seg->parainfo.valid_life_time = 2 * seg->parainfo.prefer_life_time;
	}
	dprintf(LOG_DEBUG, " preferlifetime %u, validlifetime %u", 
		seg->parainfo.prefer_life_time, seg->parainfo.valid_life_time);

	dprintf(LOG_DEBUG, " renewtime %u, rebindtime %u", 
		seg->parainfo.renew_time, seg->parainfo.rebind_time);
	v6addr->preferlifetime = seg->parainfo.prefer_life_time;
	v6addr->validlifetime = seg->parainfo.valid_life_time;
	v6addr->status_code = DH6OPT_STCODE_SUCCESS;
	v6addr->status_msg = NULL;
	return;
}

static void
server6_get_addrpara(v6addr, seg)
	struct dhcp6_addr *v6addr;
	struct v6addrseg *seg;
{
	v6addr->plen = seg->prefix.plen;
	if (seg->parainfo.prefer_life_time == 0 && seg->parainfo.valid_life_time == 0) {
		seg->parainfo.valid_life_time = DEFAULT_VALID_LIFE_TIME;
		seg->parainfo.prefer_life_time = DEFAULT_PREFERRED_LIFE_TIME;
	} else if (seg->parainfo.prefer_life_time == 0) {
		seg->parainfo.prefer_life_time = seg->parainfo.valid_life_time / 2;
	} else if (seg->parainfo.valid_life_time == 0) {
		seg->parainfo.valid_life_time = 2 * seg->parainfo.prefer_life_time;
	}
	dprintf(LOG_DEBUG, " preferlifetime %u, validlifetime %u",
		seg->parainfo.prefer_life_time, seg->parainfo.valid_life_time);

	dprintf(LOG_DEBUG, " renewtime %u, rebindtime %u", 
		seg->parainfo.renew_time, seg->parainfo.rebind_time);
	v6addr->preferlifetime = seg->parainfo.prefer_life_time;
	v6addr->validlifetime = seg->parainfo.valid_life_time;
	v6addr->status_code = DH6OPT_STCODE_SUCCESS;
	v6addr->status_msg = NULL;
	return;
}

int
dhcp6_create_prefixlist(roptinfo, optinfo, iaidaddr, subnet)
	struct dhcp6_optinfo *roptinfo;
	const struct dhcp6_optinfo *optinfo; 
	const struct dhcp6_iaidaddr *iaidaddr;
	const struct link_decl *subnet;
{
	struct dhcp6_listval *v6addr;
	struct v6prefix *prefix6;
	struct dhcp6_list *reply_list = &roptinfo->addr_list;
	const struct dhcp6_list *req_list = &optinfo->addr_list;
	struct dhcp6_listval *lv, *lv_next = NULL;

	roptinfo->iaidinfo.renewtime = subnet->linkscope.renew_time;
	roptinfo->iaidinfo.rebindtime = subnet->linkscope.rebind_time;
	roptinfo->type = optinfo->type;
	for (prefix6 = subnet->prefixlist; prefix6; prefix6 = prefix6->next) {
		v6addr = (struct dhcp6_listval *)malloc(sizeof(*v6addr));
		if (v6addr == NULL) {
			dprintf(LOG_ERR, "%s" "fail to allocate memory", FNAME);
			return (-1);
		}
		memset(v6addr, 0, sizeof(*v6addr));
		memcpy(&v6addr->val_dhcp6addr.addr, &prefix6->prefix.addr, 
				sizeof(v6addr->val_dhcp6addr.addr));
		v6addr->val_dhcp6addr.plen = prefix6->prefix.plen;
		v6addr->val_dhcp6addr.type = IAPD;
		server6_get_prefixpara(&v6addr->val_dhcp6addr, prefix6);
		dprintf(LOG_DEBUG, " get prefix %s/%d, "
			"preferlifetime %u, validlifetime %u",
			in6addr2str(&v6addr->val_dhcp6addr.addr, 0), 
			v6addr->val_dhcp6addr.plen,
			v6addr->val_dhcp6addr.preferlifetime, 
			v6addr->val_dhcp6addr.validlifetime);
		TAILQ_INSERT_TAIL(reply_list, v6addr, link);
	}
	for (prefix6 = subnet->prefixlist; prefix6; prefix6 = prefix6->next) {
		for (lv = TAILQ_FIRST(req_list); lv; lv = lv_next) {
			lv_next = TAILQ_NEXT(lv, link);
			if (IN6_IS_ADDR_RESERVED(&lv->val_dhcp6addr.addr) ||
			    is_anycast(&lv->val_dhcp6addr.addr, prefix6->prefix.plen) ||
			    !addr_on_addrlist(reply_list, &lv->val_dhcp6addr)) {
				lv->val_dhcp6addr.status_code = DH6OPT_STCODE_NOTONLINK;
				dprintf(LOG_DEBUG, " %s prefix not on link", 
					in6addr2str(&lv->val_dhcp6addr.addr, 0));
				lv->val_dhcp6addr.type = IAPD;
				TAILQ_INSERT_TAIL(reply_list, lv, link);
			}
		}
	}
	return (0);
}

struct host_decl *
dhcp6_allocate_host(ifp, rootgroup, optinfo)
	struct dhcp6_if *ifp;
	struct rootgroup *rootgroup;
	struct dhcp6_optinfo *optinfo;
{
	struct host_decl *host = NULL;
	struct interface *ifnetwork;
	struct duid *duid = &optinfo->clientID;
	u_int32_t iaid = optinfo->iaidinfo.iaid;
	for (ifnetwork = rootgroup->iflist; ifnetwork; ifnetwork = ifnetwork->next) {
		if (strcmp(ifnetwork->name, ifp->ifname) != 0)
			continue;
		else {
			host = find_hostdecl(duid, iaid, ifnetwork->hostlist);
			break;
		}
	}
	return host;
}

struct link_decl *
dhcp6_allocate_link(ifp, rootgroup, relay)
	struct dhcp6_if *ifp;
	struct rootgroup *rootgroup;
	struct in6_addr *relay;
{
	struct link_decl *link;
	struct interface *ifnetwork;
	ifnetwork = rootgroup->iflist;
	for (ifnetwork = rootgroup->iflist; ifnetwork; ifnetwork = ifnetwork->next) {
		if (strcmp(ifnetwork->name, ifp->ifname) != 0)
			continue;
		else {
			for (link = ifnetwork->linklist; link; link = link->next) {
				/* if relay is NULL, assume client and server are on the
				 * same link (which cannot have a relay configuration option)
				 */
				struct v6addrlist *temp;
				if (relay == NULL) {
					if (link->relaylist != NULL) 
						continue;
					else
						return link;
				} else {
					for (temp = link->relaylist; temp; temp = temp->next) {
						/* only compare the prefix configured to the relay
						   link address */
						if (!prefixcmp (relay, 
									    &temp->v6addr.addr,
						                temp->v6addr.plen))
							return link;
						else
							continue;
					}
				}
			}
		}
	}
	return NULL;
}

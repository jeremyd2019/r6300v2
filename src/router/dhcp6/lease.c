/*	$Id: lease.c 241182 2011-02-17 21:50:03Z $	*/

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
#include <string.h>
#include <time.h>
#include <syslog.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <ifaddrs.h>

#include "queue.h"
#include "dhcp6.h"
#include "hash.h"
#include "config.h"
#include "common.h"
#include "lease.h"

extern struct dhcp6_iaidaddr client6_iaidaddr;
extern FILE *server6_lease_file;
extern char *server6_lease_temp;
extern FILE *client6_lease_file;
extern char *client6_lease_temp;
u_int32_t do_hash __P((const void *, u_int8_t ));
static int init_lease_hashes __P((void));

int 
write_lease(const struct dhcp6_lease *lease_ptr,
	    FILE *file)
{
	struct tm brokendown_time;
	char addr_str[64];
	
	if ((inet_ntop(AF_INET6, &lease_ptr->lease_addr.addr, 
		addr_str, sizeof(addr_str))) == 0) {
		dprintf(LOG_DEBUG, "%s" "inet_ntop %s", FNAME, strerror(errno));
		return (-1);
	}
	gmtime_r(&lease_ptr->start_date, &brokendown_time);
	fprintf(file, "lease %s/%d { \n", addr_str, lease_ptr->lease_addr.plen);
	fprintf(file, "\t DUID: %s;\n", 
		duidstr(&lease_ptr->iaidaddr->client6_info.clientid));
	if (dhcp6_mode == DHCP6_MODE_CLIENT) 
		fprintf(file, "\t SDUID: %s;\n", 
			duidstr(&lease_ptr->iaidaddr->client6_info.serverid));
	fprintf(file, "\t IAID: %u ", lease_ptr->iaidaddr->client6_info.iaidinfo.iaid);
	fprintf(file, "\t type: %d;\n", lease_ptr->iaidaddr->client6_info.type);
	fprintf(file, "\t RenewTime: %u;\n", 
		lease_ptr->iaidaddr->client6_info.iaidinfo.renewtime);
	fprintf(file, "\t RebindTime: %u;\n",
		lease_ptr->iaidaddr->client6_info.iaidinfo.rebindtime);
	if (!IN6_IS_ADDR_UNSPECIFIED(&lease_ptr->linklocal)) {
		if ((inet_ntop(AF_INET6, &lease_ptr->linklocal, addr_str, 
			sizeof(struct in6_addr))) == 0) {
			dprintf(LOG_DEBUG, "%s" "inet_ntop %s", FNAME, strerror(errno));
			return (-1);
		}
		fprintf(file, "\t linklocal: %s;\n", addr_str);
	}
	fprintf(file, "\t state: %d;\n", lease_ptr->state);
	if (lease_ptr->hostname != NULL)
		fprintf(file, "\t hostname: %s;\n",lease_ptr->hostname);
	fprintf(file, "\t (start_date: %d %d/%d/%d %d:%d:%d UTC);\n",
		     brokendown_time.tm_wday,
		     brokendown_time.tm_year + 1900,
		     brokendown_time.tm_mon + 1,
		     brokendown_time.tm_mday,
		     brokendown_time.tm_hour,
		     brokendown_time.tm_min,
		     brokendown_time.tm_sec);
	fprintf(file, "\t start date: %lu;\n", lease_ptr->start_date);
	fprintf(file, "\t PreferredLifeTime: %u;\n",
                             lease_ptr->lease_addr.preferlifetime);
	fprintf(file, "\t ValidLifeTime: %u;\n",
                             lease_ptr->lease_addr.validlifetime);
	fprintf(file, "}\n");
	if (fflush(file) == EOF) {
		dprintf(LOG_INFO, "%s" "write lease fflush failed %s", 
			FNAME, strerror(errno));
		return -1;
	}
	if (fsync(fileno(file)) < 0) {
		dprintf(LOG_INFO, "%s" "write lease fsync failed %s", 
			FNAME, strerror(errno));
		return -1;
	}
	return 0;
}

FILE *
sync_leases (FILE *file, const char *original, char *template)
{
	int i, fd;
	struct hashlist_element *element;
	fd = mkstemp(template);
        if (fd < 0 || (sync_file = fdopen(fd, "w")) == NULL) {
		dprintf(LOG_ERR, "%s" "could not open sync file", FNAME);
                return (NULL);
        }
	if (dhcp6_mode == DHCP6_MODE_SERVER) {
		for (i = 0; i < lease_hash_table->hash_size; i++) {
			element = lease_hash_table->hash_list[i];
			while (element) {
				if (write_lease((struct dhcp6_lease *)element->data, 
							sync_file) < 0) {
					dprintf(LOG_ERR, "%s" "write lease failed", FNAME);
					return (NULL);
				}
				element = element->next;
			}
		}
	} else if (dhcp6_mode == DHCP6_MODE_CLIENT) {
		struct dhcp6_lease *lv, *lv_next;
		for (lv = TAILQ_FIRST(&client6_iaidaddr.lease_list); lv; lv = lv_next) {
			lv_next = TAILQ_NEXT(lv, link);
			if (write_lease(lv, sync_file) < 0)  
				dprintf(LOG_ERR, "%s" "write lease failed", FNAME);
		}
	}
	fclose(sync_file);
	fclose(file);
	if (rename(template, original) < 0) { 
		dprintf(LOG_ERR, "%s" "Could not rename sync file", FNAME);
		return (NULL);
	}
        if ((file = fopen(original, "a+")) == NULL) {
                dprintf(LOG_ERR, "%s" "could not open sync file", FNAME);
		return (NULL);
	}
	return file; 
}

struct dhcp6_timer *
syncfile_timo(void *arg)
{
	return NULL;
}

FILE *
init_leases(const char *name)
{
	FILE *file;
	file = fopen(name, "a+");
	if(!file) {
		dprintf(LOG_ERR, "%s" "could not open lease file", FNAME);
		return (NULL);
	}
	if (dhcp6_mode == DHCP6_MODE_SERVER) {
		if (0 != init_lease_hashes()) {
			dprintf(LOG_ERR, "%s" "Could not initialize hash arrays", FNAME);
			return (NULL);
		}
	}
	lease_parse(file);
	return file;
} 

int 
init_lease_hashes(void) 
{

	hash_anchors = (struct hash_table **)malloc(HASH_TABLE_COUNT*sizeof(*hash_anchors));
	if (!hash_anchors) {
		dprintf(LOG_ERR, "%s" "Couldn't malloc hash anchors", FNAME);
		return (-1);
	}
        host_addr_hash_table = hash_table_create(DEFAULT_HASH_SIZE, 
			addr_hash, v6addr_findkey, v6addr_key_compare);
	if (!host_addr_hash_table) {
		dprintf(LOG_ERR, "%s" "Couldn't create hash table", FNAME);
		return (-1);
	}
        lease_hash_table = hash_table_create(DEFAULT_HASH_SIZE, 
			addr_hash, lease_findkey, lease_key_compare);
	if (!lease_hash_table) {
		dprintf(LOG_ERR, "%s" "Couldn't create hash table", FNAME);
		return (-1);
	}
        server6_hash_table = hash_table_create(DEFAULT_HASH_SIZE, 
			iaid_hash, iaid_findkey, iaid_key_compare);
	if (!server6_hash_table) {
		dprintf(LOG_ERR, "%s" "Couldn't create hash table", FNAME);
		return (-1);
	}
	return 0;

}

u_int32_t 
do_hash(const void *key, u_int8_t len)
{
	int i;
	u_int32_t *p;
	u_int32_t index = 0;
	u_int32_t tempkey;
	for (i = 0, p = (u_int32_t *)key; i < len/sizeof(tempkey); i++, p++ ) {
		memcpy(&tempkey, p, sizeof(tempkey));
		index ^= tempkey;
	}
	memcpy(&tempkey, p, len%(sizeof(tempkey)));
	index ^= tempkey;
	return index;
}

unsigned int
iaid_hash(const void *key)
{
	const struct client6_if *iaidkey = (const struct client6_if *)key;
	const struct duid *duid = &iaidkey->clientid;
	unsigned int index;
	index = do_hash((const void *) duid->duid_id, duid->duid_len);
	return index;
}

unsigned int
addr_hash(const void *key)
{
	const struct in6_addr *addrkey 
		= (const struct in6_addr *)&(((const struct dhcp6_addr *)key)->addr);
	unsigned int index;
	index = do_hash((const void *)addrkey, sizeof(*addrkey));
	return index;
}

void * 
v6addr_findkey(const void *data)
{
        const struct dhcp6_addr *v6addr = (const struct dhcp6_addr *)data;
	return (void *)(&(v6addr->addr));
}

int 
v6addr_key_compare(const void *data, const void *key)
{ 
	struct dhcp6_addr *v6addr = (struct dhcp6_addr *)data;	
	if (IN6_ARE_ADDR_EQUAL(&v6addr->addr, (struct in6_addr *)key)) {
		return MATCH;
	} else
		return MISCOMPARE;
}

void * 
lease_findkey(const void *data)
{
        const struct dhcp6_lease *lease = (const struct dhcp6_lease *)data;
	return (void *)(&(lease->lease_addr));
}

int 
lease_key_compare(const void *data, const void *key)
{ 
	struct dhcp6_lease *lease = (struct dhcp6_lease *)data;	
	struct dhcp6_addr *lease_address = &lease->lease_addr;
	struct dhcp6_addr *addr6 = (struct dhcp6_addr *)key;
	if (IN6_ARE_ADDR_EQUAL(&lease_address->addr, &addr6->addr)) {
		/* prefix match */
		if (addr6->type == IAPD) {
		 	if (lease_address->plen == addr6->plen)
	       			return MATCH;
		/* ipv6 address match */
		} else if (addr6->type == IANA || addr6->type == IATA)
			return MATCH;
	}
	return MISCOMPARE;
}

void *
iaid_findkey(const void *data)
{
        struct dhcp6_iaidaddr *iaidaddr = (struct dhcp6_iaidaddr *)data;
	return (void *)(&(iaidaddr->client6_info));
}

int 
iaid_key_compare(const void *data,
		 const void *key)
{ 	
        const struct dhcp6_iaidaddr *iaidaddr = (const struct dhcp6_iaidaddr *)data;
	const struct client6_if *client_key = (const struct client6_if *)key;

	if (0 == duidcmp(&client_key->clientid, &iaidaddr->client6_info.clientid)){
		if ((client_key->type == iaidaddr->client6_info.type) &&
		    (client_key->iaidinfo.iaid == iaidaddr->client6_info.iaidinfo.iaid)) {
			return MATCH;
		}
	}
	return MISCOMPARE;
}

int
prefixcmp(addr, prefix, len)
	struct in6_addr *addr;
	struct in6_addr *prefix;
	int len;
{
	int i, num_bytes;
	struct in6_addr mask;
	num_bytes = len / 8;
	for (i = 0; i < num_bytes; i++) {
		mask.s6_addr[i] = 0xFF;
	}
	mask.s6_addr[num_bytes] = 0xFF << (8 - len % 8);
	for (i = 0; i < num_bytes; i++) {
		if (addr->s6_addr[i] != prefix->s6_addr[i]) return -1;
	}
	if((addr->s6_addr[num_bytes] & mask.s6_addr[num_bytes]) != 
	   (prefix->s6_addr[num_bytes] & mask.s6_addr[num_bytes]))
 		return -1;
	return 0;
}

int
get_linklocal(const char *ifname,
	      struct in6_addr *linklocal)
{	
	struct ifaddrs *ifa, *ifap;
	struct sockaddr *sd;
	if (getifaddrs(&ifap) < 0) {
		dprintf(LOG_ERR, "getifaddrs error");
		return -1;
	}
	/* ifa->ifa_addr is sockaddr_in6 */
	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		if (strcmp(ifa->ifa_name, ifname)) continue;
		sd = (struct sockaddr *)ifa->ifa_addr;
		if (!sd || sd->sa_family != AF_INET6) continue;
		if (!IN6_IS_ADDR_LINKLOCAL(&sd->sa_data[6])) continue;
		/* which linklocal do we want, if find many 
		 * from scope id??? sa_data[32]
		 * */
		memcpy(linklocal, &sd->sa_data[6], sizeof(*linklocal));
		break;
	}
	freeifaddrs(ifap);
	return 0;
}

int
dhcp6_get_prefixlen(addr, ifp)
	struct in6_addr *addr;
	struct dhcp6_if *ifp;
{
	struct ra_info *rainfo;
	for (rainfo = ifp->ralist; rainfo; rainfo = rainfo->next) {
		/* prefixes are sorted by plen */
		if (prefixcmp(addr, &rainfo->prefix, rainfo->plen) == 0)	
			return rainfo->plen;
	}
	return PREFIX_LEN_NOTINRA;
}

int 
addr_on_addrlist(addrlist, addr6)
	struct dhcp6_list *addrlist;
	struct dhcp6_addr *addr6;
{
	struct dhcp6_listval *lv;

	for (lv = TAILQ_FIRST(addrlist); lv;
	     lv = TAILQ_NEXT(lv, link)) {
		if (IN6_ARE_ADDR_EQUAL(&lv->val_dhcp6addr.addr, &addr6->addr)) {
			if ((lv->val_dhcp6addr.type != IAPD) 
			    || ((lv->val_dhcp6addr.type == IAPD) 
			    && (lv->val_dhcp6addr.plen == addr6->plen)))
				return (1);
		}
	}
	return (0);
}

u_int32_t
get_min_preferlifetime(struct dhcp6_iaidaddr *sp)
{
	struct dhcp6_lease *lv, *first;
	u_int32_t min;
	if (TAILQ_EMPTY(&sp->lease_list))
		return 0;
	first = TAILQ_FIRST(&sp->lease_list);
	min = first->lease_addr.preferlifetime;
	for (lv = TAILQ_FIRST(&sp->lease_list); lv; lv = TAILQ_NEXT(lv, link)) {
		min = MIN(min, lv->lease_addr.preferlifetime);
	}
	return min;
}

u_int32_t
get_max_validlifetime(struct dhcp6_iaidaddr *sp)
{
	struct dhcp6_lease *lv, *first;
	u_int32_t max;
	if (TAILQ_EMPTY(&sp->lease_list))
		return 0;
	first = TAILQ_FIRST(&sp->lease_list);
	max = first->lease_addr.validlifetime;
	for (lv = TAILQ_FIRST(&sp->lease_list); lv; lv = TAILQ_NEXT(lv, link)) {
		max = MAX(max, lv->lease_addr.validlifetime);
	}
	return max;
}

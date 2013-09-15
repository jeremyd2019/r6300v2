/*    $Id: server6_conf.c,v 1.1.1.1 2006/12/04 00:45:34 Exp $   */

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
#include <syslog.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <net/if.h>
//#include <openssl/md5.h>

#include "queue.h"
#include "dhcp6.h"
#include "config.h"
#include "common.h"
#include "server6_conf.h"

#define NMASK(n) htonl((1<<(n))-1)

static void download_scope __P((struct scope *, struct scope *));

int 
ipv6addrcmp(addr1, addr2)
	struct in6_addr *addr1;
	struct in6_addr *addr2;
{
	int i;
	for (i = 0; i < 16; i++) {
		if (addr1->s6_addr[i] < addr2->s6_addr[i]) return (-1);
		else if (addr1->s6_addr[i] > addr2->s6_addr[i]) return 1;
	}
	return 0;
}


struct in6_addr 
*inc_ipv6addr(current)
	struct in6_addr *current;
{
	int i;
	for (i = 15; i >= 0; i--) {
		current->s6_addr[i]++;
		if (current->s6_addr[i] != 0x00) break;
	}
	return current;
}
			
struct v6addr
*getprefix(addr, len)
	struct in6_addr *addr;
	int len;
{
	int i, num_bytes;
	struct v6addr *prefix;
	prefix = (struct v6addr *)malloc(sizeof(*prefix));
	if (prefix == NULL) {
		dprintf(LOG_ERR, "%s" "fail to malloc memory", FNAME);
		return NULL;
	}
	memset(prefix, 0, sizeof(*prefix));
	prefix->plen = len;
	num_bytes = len / 8;
	for (i = 0; i < num_bytes; i++) {
		prefix->addr.s6_addr[i] = 0xFF;
	}
	prefix->addr.s6_addr[num_bytes] = (0xFF << 8) - len % 8 ;
	for (i = 0; i <= num_bytes; i++) {
		prefix->addr.s6_addr[i] &= addr->s6_addr[i];
	}
	for (i = num_bytes + 1; i < 16; i++) {
		prefix->addr.s6_addr[i] = 0x00;
	}
	return prefix;
}


int
get_numleases(currentpool, poolfile)
	struct pool_decl *currentpool;
       	char *poolfile;
{
	return 0;
}


struct scopelist
*push_double_list(current, scope)
	struct scopelist *current;
	struct scope *scope;
{
	struct scopelist *item;
	item = (struct scopelist *)malloc(sizeof(*item));
	if (item == NULL) {
		dprintf(LOG_ERR, "%s" "fail to allocate memory", FNAME);
		return NULL;
	}
	memset(item, 0, sizeof(*item));
	item->scope = scope;
	if (current) {
		if (current->next) 
			current->next->prev = item;
		item->next = current->next;
		current->next = item;
	} else
		item->next = NULL;
	item->prev = current;
	current = item;
	return current;
}

struct scopelist
*pop_double_list(current)
       struct scopelist *current;	
{
	struct scopelist *temp;
	temp = current;
	/* current must not be NULL */
	if (current->next)
		current->next->prev = current->prev;
	if (current->prev) 
		current->prev->next = current->next;
	current = current->prev;
	temp->prev = NULL;
	temp->next = NULL;
	temp->scope = NULL;
	free(temp);
	return current;
}

void
post_config(root)
	struct rootgroup *root;
{
	struct interface *ifnetwork;
	struct link_decl *link;
	struct host_decl *host;
	struct v6addrseg *seg;
	struct v6prefix *prefix6;
	struct scope *current;
	struct scope *up;
	
	if (root->group)
		download_scope(root->group, &root->scope);
	up = &root->scope;
	for (ifnetwork = root->iflist; ifnetwork; ifnetwork = ifnetwork->next) {
		if (ifnetwork->group)
			download_scope(ifnetwork->group, &ifnetwork->ifscope);
		current = &ifnetwork->ifscope;
		download_scope(up, current);
		up = &ifnetwork->ifscope;
		for (host = ifnetwork->hostlist; host; host = host->next) {
			if (host->group)
				download_scope(host->group, &host->hostscope);
			current = &host->hostscope;
			download_scope(up, current);
		}
			
	}
	for (ifnetwork = root->iflist; ifnetwork; ifnetwork = ifnetwork->next) {
		if (ifnetwork->group)
			download_scope(ifnetwork->group, &ifnetwork->ifscope);
		current = &ifnetwork->ifscope;
		download_scope(up, current);
		up = &ifnetwork->ifscope;
		for (link = ifnetwork->linklist; link; link = link->next) {
				if (link->group)
					download_scope(link->group, &link->linkscope);
				current = &link->linkscope;
				download_scope(up, current);
				up = &link->linkscope;
				for (seg = link->seglist; seg; seg = seg->next) {
					if (seg->pool) {
						if (seg->pool->group)
							download_scope(seg->pool->group, 
									&seg->pool->poolscope);
						current = &seg->pool->poolscope;
						download_scope(up, current);
						if (current->prefer_life_time != 0 && 
						    current->valid_life_time != 0 &&
						    current->prefer_life_time >
						    current->valid_life_time) {
							dprintf(LOG_ERR, "%s" 
					"preferlife time is greater than validlife time",
							    FNAME);
							exit (1);
						}
						memcpy(&seg->parainfo, current, 
								sizeof(seg->parainfo));
					} else {
						memcpy(&seg->parainfo, up, 
								sizeof(seg->parainfo));
					}
				}
				for (prefix6 = link->prefixlist; prefix6; 
						prefix6 = prefix6->next) {
					if (prefix6->pool) {
						if (prefix6->pool->group)
							download_scope(prefix6->pool->group, 
								&prefix6->pool->poolscope);
						current = &prefix6->pool->poolscope;
						download_scope(up, current);
						if (current->prefer_life_time != 0 && 
						    current->valid_life_time != 0 &&
						    current->prefer_life_time >
						    current->valid_life_time) {
							dprintf(LOG_ERR, "%s" 
					"preferlife time is greater than validlife time",
							    FNAME);
							exit (1);
						}
						memcpy(&prefix6->parainfo, current, 
								sizeof(prefix6->parainfo));
					} else {
						memcpy(&prefix6->parainfo, up, 
								sizeof(prefix6->parainfo));
					}
				}
			}
	}
	return;				
}

static void
download_scope(up, current)
	struct scope *up;
	struct scope *current;
{
	if (current->prefer_life_time == 0 && up->prefer_life_time != 0)
		current->prefer_life_time = up->prefer_life_time;	
	if (current->valid_life_time == 0 && up->valid_life_time != 0)
		current->valid_life_time = up->valid_life_time;
	if (current->renew_time == 0 && up->renew_time != 0)
		current->renew_time = up->renew_time;
	if (current->rebind_time == 0 && up->rebind_time != 0)
		current->rebind_time = up->rebind_time;
	if (current->renew_time > current->rebind_time) {
		dprintf(LOG_ERR, "dhcpv6 server defines T1 > T2");
		exit(1);
	}
	if (current->server_pref == 0 || current->server_pref == DH6OPT_PREF_UNDEF) {
		if (up->server_pref != 0)
			current->server_pref = up->server_pref;
		else
			current->server_pref = DH6OPT_PREF_UNDEF;
	}
	current->allow_flags |= up->allow_flags;
	current->send_flags |= up->send_flags;
	if (TAILQ_EMPTY(&current->dnslist.addrlist))
		dhcp6_copy_list(&current->dnslist.addrlist, &up->dnslist.addrlist);
	if (TAILQ_EMPTY(&current->siplist))
		dhcp6_copy_list(&current->siplist, &up->siplist);
	if (TAILQ_EMPTY(&current->ntplist))
		dhcp6_copy_list(&current->ntplist, &up->ntplist);
	if (current->dnslist.domainlist == NULL)
		current->dnslist.domainlist = up->dnslist.domainlist;
	return;
}

int 
is_anycast(struct in6_addr *in, int plen)
{
	int wc;

	if (plen == 64) { /* assume EUI64 */
		/* doesn't cover none EUI64 */
		return in->s6_addr32[2] == htonl(0xFDFFFFFF) &&
			(in->s6_addr32[3] | htonl(0x7f)) ==
				(u_int32_t) ~0;
	}
	/* not EUI64 */
	if (plen > 121) 
		return 0;
	wc = plen / 32;
	if (plen) {
		if (in->s6_addr32[wc] != NMASK(32 - (plen%32)))
			return 0;
		wc++;
		
	}
	for (/*empty*/; wc < 2; wc++)
		if (in->s6_addr32[wc] != (u_int32_t) ~0)
			return 0;
	return (in->s6_addr32[3] | htonl(0x7f)) == (u_int32_t)~0;
}

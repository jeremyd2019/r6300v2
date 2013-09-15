/*	$Id: server6_conf.h,v 1.1.1.1 2006/12/04 00:45:34 Exp $	*/

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

#ifndef __SERVER6_CONF_H_DEFINED
#define __SERVER6_CONF_H_DEFINED

#define DEFAULT_PREFERRED_LIFE_TIME 360000
#define DEFAULT_VALID_LIFE_TIME 720000


struct rootgroup *globalgroup;

/* provide common paramters within scopes */
struct scope {
	int32_t prefer_life_time;
	int32_t valid_life_time;
	int32_t renew_time;
	int32_t rebind_time;
	int8_t server_pref;
	u_int8_t send_flags;
	u_int8_t allow_flags;
	struct dns_list dnslist;
	struct dhcp6_list siplist;
	struct dhcp6_list ntplist;
};

struct scopelist {
	struct scopelist *prev;
	struct scopelist *next;
	struct scope *scope;
};

struct rootgroup {
	struct scope scope;
	struct scope *group;
	struct interface *iflist;
};

struct v6addr {
	struct in6_addr addr;
	u_int8_t plen;
};
/* interface network declaration */
/* interface declaration is used to inform DHCPv6 server that the links */
/* and pool declared within it are connected to the same network segment */
struct interface {
	struct interface *next;
	char name[IFNAMSIZ];
	struct hardware hw_address;
	struct in6_addr primary_v6addr;
	struct in6_addr linklocal;
	struct link_decl *linklist;
	struct host_decl *hostlist;
	struct scope ifscope;
	struct scope *group;
};

/* link declaration */
/* link declaration is used to provide the DHCPv6 server with enough   */
/* information to determin whether a particular IPv6 addresses is on the */
/* link */
struct link_decl {
	struct link_decl *next;
	char name[IFNAMSIZ];
	struct v6addrlist *relaylist;
	struct v6addrseg *seglist;
	struct v6prefix *prefixlist;
	struct pool_decl *poollist;
	struct interface *network;
	struct scope linkscope;
	struct scope *group;
};


struct v6addrseg {
	struct v6addrseg *next;
	struct v6addrseg *prev;
	struct link_decl *link;
	struct pool_decl *pool;
	struct in6_addr min;
	struct in6_addr max;
	struct in6_addr free;
	struct v6addr prefix;
	struct lease *active;
	struct lease *expired;
	struct lease *abandoned;
	struct scope parainfo;
};

struct v6prefix {
	struct v6prefix *next;
	struct v6prefix *prev;
	struct link_decl *link;
	struct pool_decl *pool;
	struct v6addr prefix;
	struct scope parainfo;
};

/* The pool declaration is used to declare an address pool from which IPv6 */
/* address can be allocated, with its own permit to control client access  */
/* and its own scopt in which you can declare pool-specific parameter*/
struct pool_decl {
	struct pool_decl *next;
	struct interface *network;
	struct link_decl *link;
	struct scope poolscope;
	struct scope *group;
};

struct v6addrlist {
	struct v6addrlist *next;
	struct v6addr v6addr;
};

/* host declaration */
/* host declaration provides information about a particular DHCPv6 client */
struct host_decl {
	struct host_decl *next;
	char name[IFNAMSIZ];
	struct duid cid;
	struct dhcp6_iaid_info iaidinfo;
	struct dhcp6_list addrlist;
	struct dhcp6_list prefixlist;
	struct interface *network;
	struct scope hostscope;
	struct scope *group;
};

int is_anycast __P((struct in6_addr *, int));	
extern void printf_in6addr __P((struct in6_addr *));
void post_config(struct rootgroup *);
int sfparse __P((char *));
int ipv6addrcmp __P((struct in6_addr *, struct in6_addr *));
struct v6addr *getprefix __P((struct in6_addr *, int));
struct in6_addr *inc_ipv6addr __P((struct in6_addr *));
struct scopelist *push_double_list __P((struct scopelist *, struct scope *));
struct scopelist *pop_double_list __P((struct scopelist *));
int get_primary_ipv6addr __P((const char *device));

#endif

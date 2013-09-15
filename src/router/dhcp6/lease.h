/*	$Id: lease.h 241182 2011-02-17 21:50:03Z $	*/
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

#ifndef __LEASE_H_DEFINED
#define __LEASE_H_DEFINED

#define LEASE_FILENAME_SIZE
#define ADDR_UPDATE     0
#define ADDR_REMOVE     1
#define ADDR_VALIDATE	2
#define ADDR_ABANDON	3

#define PATH_SERVER6_LEASE "/tmp/server6.leases" //Modified. "/var/lib/dhcpv6/server6.leases"
#define PATH_CLIENT6_LEASE "/tmp/client6.leases" //Modified. "/var/lib/dhcpv6/client6.leases"

#define HASH_TABLE_COUNT 	4

extern struct hash_table **hash_anchors;
#define server6_hash_table hash_anchors[HT_IAIDADDR]
#define lease_hash_table hash_anchors[HT_IPV6LEASE]
#define host_addr_hash_table hash_anchors[HT_IPV6ADDR]
#define PREFIX_LEN_NOTINRA 64
#define MAX_FILE_SIZE 512*1024


typedef enum { IFADDRCONF_ADD, IFADDRCONF_REMOVE } ifaddrconf_cmd_t;

enum hash_type{HT_IPV6ADDR = 0, HT_IPV6LEASE, HT_IAIDADDR};

struct dhcp6_iaidaddr client6_iaidaddr;
FILE *server6_lease_file;
FILE *client6_lease_file;
FILE *lease_file;
FILE *sync_file;
struct hash_table **hash_anchors;

struct client6_if {
	iatype_t type;
	struct dhcp6_iaid_info iaidinfo;
	struct duid clientid;
	struct duid serverid;
};

struct dhcp6_iaidaddr {
	TAILQ_ENTRY(dhcp6_iaidaddr) link;
	struct client6_if client6_info;
	time_t start_date;
	state_t state;
	struct dhcp6_if *ifp;
	struct dhcp6_timer *timer;
	/* list of client leases */
	TAILQ_HEAD(,dhcp6_lease) lease_list;
};

extern u_int32_t do_hash __P((const void *, u_int8_t ));
int get_linklocal __P((const char *, struct in6_addr *));
extern void dhcp6_init_iaidaddr __P((void));
extern int dhcp6_remove_iaidaddr __P((struct dhcp6_iaidaddr *));
extern int dhcp6_add_iaidaddr __P((struct dhcp6_optinfo *));
extern int dhcp6_update_iaidaddr __P((struct dhcp6_optinfo *, int));
extern struct dhcp6_timer *dhcp6_iaidaddr_timo __P((void *));
extern struct dhcp6_timer *dhcp6_lease_timo __P((void *));
extern u_int32_t get_min_preferlifetime __P((struct dhcp6_iaidaddr *));
extern u_int32_t get_max_validlifetime __P((struct dhcp6_iaidaddr *));
extern struct dhcp6_iaidaddr *dhcp6_find_iaidaddr __P((struct dhcp6_optinfo *));
extern struct dhcp6_lease *dhcp6_find_lease __P((struct dhcp6_iaidaddr *,
			struct dhcp6_addr *));
extern int dhcp6_remove_lease __P((struct dhcp6_lease *));
extern int dhcp6_validate_bindings __P((struct dhcp6_optinfo *, struct dhcp6_iaidaddr *));
extern int get_iaid __P((const char *, const struct iaid_table *, int));
extern int create_iaid __P((struct iaid_table *, int));
extern FILE *init_leases __P((const char *));
extern void lease_parse __P((FILE *));
extern int do_iaidaddr_hash __P((struct dhcp6_lease *, struct client6_if *));
extern int write_lease __P((const struct dhcp6_lease *, FILE *));
extern FILE *sync_leases __P((FILE *, const char *, char *));
extern struct dhcp6_timer *syncfile_timo __P((void *));
extern unsigned int addr_hash __P((const void *));
extern unsigned int iaid_hash __P((const void *));
extern void * iaid_findkey __P((const void *));
extern int iaid_key_compare __P((const void *, const void *));
extern void * lease_findkey __P((const void *));
extern int lease_key_compare __P((const void *, const void *));
extern void * v6addr_findkey __P((const void *));
extern int v6addr_key_compare __P((const void *, const void *));
extern int client6_ifaddrconf __P((ifaddrconf_cmd_t , struct dhcp6_addr *));
extern int dhcp6_get_prefixlen __P((struct in6_addr *, struct dhcp6_if *));
extern int prefixcmp __P((struct in6_addr *, struct in6_addr *, int));
extern int addr_on_addrlist __P((struct dhcp6_list *, struct dhcp6_addr *));
struct link_decl;
extern int dhcp6_create_prefixlist __P((struct dhcp6_optinfo *,
					const struct dhcp6_optinfo *,
					const struct dhcp6_iaidaddr *,
					const struct link_decl *));
extern int dhcp6_create_addrlist __P((	struct dhcp6_optinfo *,
					struct dhcp6_optinfo *,
					const struct dhcp6_iaidaddr *,
					const struct link_decl *));
extern int dad_parse(const char *file);
#endif

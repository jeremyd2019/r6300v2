/*	$Id: lease_token.l,v 1.1.1.1 2006/12/04 00:45:31 Exp $	*/

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

%option noyywrap

%{

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <syslog.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <net/if.h>
#include <linux/sockios.h>
#include "queue.h"
#include "dhcp6.h"
#include "config.h"
#include "common.h"
#include "lease.h"
#include "hash.h"
#include "timer.h"

#define LEASE_ADDR_FLAG	0x01
#define LEASE_DUID_FLAG 0x02
#define LEASE_IAID_FLAG 0x04
#define LEASE_SDATE_FLAG 0x08

#define LEASE_VTIME_FLAG 0x10
#define LEASE_PTIME_FLAG 0x20
#define LEASE_RNTIME_FLAG 0x40
#define LEASE_RBTIME_FLAG 0x80

#define LEASE_HNAME_FLAG 0x100
#define LEASE_LL_FLAG 0x200

#define VALID_LEASE_FLAG(a) 					\
	((a & LEASE_ADDR_FLAG) && (a & LEASE_SDATE_FLAG) && 	\
	 (a & LEASE_VTIME_FLAG) && (a & LEASE_PTIME_FLAG) &&	\
	 (a & LEASE_IAID_FLAG) && (a & LEASE_RNTIME_FLAG) &&	\
	 (a & LEASE_RBTIME_FLAG) && (a & LEASE_DUID_FLAG))

#define YYABORT(msg) dprintf(LOG_ERR, msg " %s lineno %d.", \
		yytext, num_lines)

#define	ABORT	do { 	\
	YYABORT("lease parse error");	\
	exit(1);	\
} while (0)

extern struct dhcp6_timer *dhcp6_lease_timo __P((void *));
extern struct dhcp6_iaidaddr client6_iaidaddr;

static int num_lines = 1;
static struct dhcp6_lease *lease_rec;
static struct client6_if client6_info;

static u_int16_t lease_flags = 0;

static int add_lease __P((struct dhcp6_iaidaddr *, struct dhcp6_lease *));
static int remove_lease __P((struct dhcp6_lease *));

%}

digit           [0-9]
number          ({digit})+
decimal         ({number}"."{number})
hexdigit        ([a-f]|[A-F]|[0-9])
hexpair		({hexdigit}{hexdigit})
ipv4addr	({digit}{1,3}"."{digit}{1,3}"."{digit}{1,3}"."{digit}{1,3})
addr_head	("::"|{hexdigit}{1,4}(":"|"::"))
addr_tail	({hexdigit}{1,4}|({hexdigit}{1,4}"::")|{ipv4addr})?
addr_body	({hexdigit}{1,4}(":"|"::"))*
ipv6addr	{addr_head}{addr_body}{addr_tail}
duid_id 	{hexpair}(:{hexpair})*
whitespace      ([ \t])+
string          [a-zA-Z]([a-zA-Z]|{digit})*(":"{digit}+)?
lbrace		\{
rbrace		\}
slash		\/
colon		\:
semi		\;
nl		\n
comment		\#.*

%s S_LEASE 
%s S_PLEN
%s S_HNAME
%s S_LL
%s S_DUID
%s S_SDUID
%s S_IAID
%s S_IATYPE
%s S_RNTIME
%s S_RBTIME
%s S_PTIME
%s S_VTIME
%s S_DATE
%s S_STATE
%s S_IGNORE

%%

{comment}	{;/* ignore comments */}
{nl}		{num_lines++; BEGIN INITIAL;}
{whitespace}	{;}
{semi}		{;}
	/* lease parser */
"lease" {
	lease_rec = (struct dhcp6_lease *)malloc(sizeof(*lease_rec));
	if (lease_rec == NULL) {
		YYABORT("failed to allocate memory for a lease");
		ABORT;
	}
	memset(lease_rec, 0, sizeof(*lease_rec));
	memset(&client6_info, 0, sizeof(client6_info));
	lease_flags = 0;
	BEGIN S_LEASE;}
"hostname:" {BEGIN S_HNAME;}
"linklocal:" {BEGIN S_LL;}
"DUID:" {BEGIN S_DUID;}
"SDUID:" {BEGIN S_SDUID;}
"IAID:" {BEGIN S_IAID;}
"RenewTime:" {BEGIN S_RNTIME;}
"RebindTime:" {BEGIN S_RBTIME;}
"ValidLifeTime:"	{BEGIN S_VTIME;}
"PreferredLifeTime:" {BEGIN S_PTIME;}
"start date:" {BEGIN S_DATE;}
"(start_date" {BEGIN S_IGNORE;}
"state:" {BEGIN S_STATE;}
{rbrace} {if (do_iaidaddr_hash(lease_rec, &client6_info) != 0) {
		ABORT;
	  }
	  else {
		BEGIN INITIAL;
	 }
}
<S_LEASE>{ipv6addr} {
	struct in6_addr addr;
	if(inet_pton(AF_INET6, yytext, &addr) < 1) {
		YYABORT("invalid address");
		free(lease_rec);
		ABORT;
	}
	memcpy(&lease_rec->lease_addr.addr, &addr, 
			sizeof(lease_rec->lease_addr.addr));
	lease_flags |= LEASE_ADDR_FLAG;}
<S_LEASE>{slash} {BEGIN S_PLEN;}
<S_LEASE>. {ABORT;}
<S_PLEN>{number} {lease_rec->lease_addr.plen = (u_int8_t)atoi(yytext);} 
<S_PLEN>{lbrace} {;}
<S_PLEN>. {ABORT;}
<S_HNAME>{string} {strncpy(lease_rec->hostname, yytext, sizeof(yytext)); 
		lease_flags |= LEASE_HNAME_FLAG;}
<S_HNAME>. {ABORT;}
<S_LL>{ipv6addr} { 
	struct in6_addr addr;
	if(inet_pton(AF_INET6, yytext, &addr) < 1) {
		YYABORT("invalid address"); 
		free(lease_rec);
		ABORT;
	}	
	memcpy(&lease_rec->linklocal, &addr, 
			sizeof(lease_rec->linklocal));
	lease_flags |= LEASE_LL_FLAG;}
<S_LL>. {ABORT;}
<S_DUID>{duid_id} {configure_duid(yytext, &client6_info.clientid);
		lease_flags |= LEASE_DUID_FLAG;}
<S_DUID>. {ABORT;}
<S_SDUID>{duid_id} {configure_duid(yytext, &client6_info.serverid);}
<S_SDUID>. {ABORT;}
<S_IAID>{number} {client6_info.iaidinfo.iaid = strtoll(yytext, NULL, 10);
		lease_flags |= LEASE_IAID_FLAG;}
<S_IAID>"type:" {BEGIN S_IATYPE;}
<S_IAID>. {ABORT;}
<S_IATYPE>{number} {lease_rec->lease_addr.type = (u_int8_t)atoi(yytext);
		client6_info.type = (u_int8_t)atoi(yytext);}
<S_IATYPE>. {ABORT;}
<S_RNTIME>{number} {client6_info.iaidinfo.renewtime = (u_int32_t)strtoul(yytext, NULL, 10);
		lease_flags |= LEASE_RNTIME_FLAG;}
<S_RNTIME>. {ABORT;}
<S_RBTIME>{number} {client6_info.iaidinfo.rebindtime = (u_int32_t)strtoul(yytext, NULL, 10);
		lease_flags |= LEASE_RBTIME_FLAG;}
<S_RBTIME>. {ABORT;}
<S_VTIME>{number} {lease_rec->lease_addr.validlifetime = (u_int32_t)strtoul(yytext, NULL, 10);
		lease_flags |= LEASE_VTIME_FLAG;}
<S_VTIME>. {ABORT;}
<S_PTIME>{number} {lease_rec->lease_addr.preferlifetime = (u_int32_t)strtoul(yytext, NULL, 10);
		lease_flags |= LEASE_PTIME_FLAG;}
<S_PTIME>. {ABORT;}
<S_DATE>{number} {lease_rec->start_date = (time_t)strtoul(yytext, NULL, 10);
		lease_flags |= LEASE_SDATE_FLAG;}
<S_DATE>. {ABORT;}
<S_STATE>{number} {lease_rec->state = (state_t)atoi(yytext);}
<S_STATE>. {ABORT;}
<S_IGNORE>.	{;}

. {ABORT;}

%%


void
lease_parse(file)
        FILE *file;
{
	
	fseek(file, 0, 0);
	yyin = file;
	yylex(); 
	return;
}

int 
do_iaidaddr_hash(lease_rec, key) 
	struct dhcp6_lease *lease_rec;
	struct client6_if *key;
{
	struct dhcp6_iaidaddr *iaidaddr;
	struct dhcp6_lease *found_lease;
	struct timeb now;
	time_t offset;
	struct timeval timo;
	double d;

	if (!VALID_LEASE_FLAG(lease_flags) ||
	    (lease_rec->lease_addr.preferlifetime > lease_rec->lease_addr.validlifetime)) {
			dprintf(LOG_ERR, "parse an invalid lease %s/%d within line %d", 
				in6addr2str(&lease_rec->lease_addr.addr, 0), 
				lease_rec->lease_addr.plen, num_lines);
			return (-1);
	}
	ftime(&now);
	offset = now.time - lease_rec->start_date;
	dprintf(LOG_DEBUG, "now : %ld; offset: %ld ", now.time, offset);
	if (offset > lease_rec->lease_addr.validlifetime) {
		dprintf(LOG_INFO, "This lease addr %s/%d has been expired.", 
			in6addr2str(&lease_rec->lease_addr.addr, 0), 
			lease_rec->lease_addr.plen);
		free(lease_rec);
		return (0);
	}

	if (lease_rec->state == INVALID) {
		dprintf(LOG_INFO, "This lease addr %s/%d is invalid. Removing.",
		        in6addr2str(&lease_rec->lease_addr.addr, 0),
		        lease_rec->lease_addr.plen);
		found_lease = hash_search(lease_hash_table, &lease_rec->lease_addr);
		/* remove the previous old lease */
		if(found_lease) {
			remove_lease(found_lease);
		}
		free(lease_rec);
		return (0);
	}
	if (dhcp6_mode == DHCP6_MODE_CLIENT) {
		if (add_lease(&client6_iaidaddr, lease_rec) != 0)
			return (-1);
		else {
			memcpy(&client6_iaidaddr.client6_info, &client6_info, 
				sizeof(client6_iaidaddr.client6_info));
			dprintf(LOG_DEBUG, "hash add client iaidaddr type %d for "
				" duid %s for iaid %u",
				client6_iaidaddr.client6_info.type, 
				duidstr(&client6_iaidaddr.client6_info.clientid), 
				client6_iaidaddr.client6_info.iaidinfo.iaid);
			return (0);
		}
	}
	iaidaddr = (struct dhcp6_iaidaddr *)hash_search(server6_hash_table, key); 
	if (iaidaddr){
		if (iaidaddr->client6_info.iaidinfo.iaid != client6_info.iaidinfo.iaid ||
		    duidcmp(&iaidaddr->client6_info.clientid, &client6_info.clientid)) {
			dprintf(LOG_DEBUG, "%s" 
				"this client iaidinfo is different to others", FNAME);
			return (-1);
		}
		found_lease = hash_search(lease_hash_table, &lease_rec->lease_addr);
		/* remove the previous old lease */
		if(found_lease) {
			if (found_lease->start_date < lease_rec->start_date) {
				dprintf(LOG_DEBUG, "%s" "remove old lease %s/%d", FNAME, 
					in6addr2str(&lease_rec->lease_addr.addr, 0), 
					lease_rec->lease_addr.plen);
				/* update the client6_info too */
				memcpy(&iaidaddr->client6_info, &client6_info, 
					sizeof(iaidaddr->client6_info));
				iaidaddr->start_date = lease_rec->start_date;	
				remove_lease(found_lease);
			} else {
				free(lease_rec);
				return (0);
			}
		}
	} else {
		iaidaddr = (struct dhcp6_iaidaddr *)malloc(sizeof(*iaidaddr));
	  	if (!iaidaddr) {
			dprintf(LOG_ERR, "%s" "failed to allocate memory", FNAME);
			return (-1);
	  	}
		memset(iaidaddr, 0, sizeof(*iaidaddr));
		TAILQ_INIT(&iaidaddr->lease_list);
		/* get the client info */
		memcpy(&iaidaddr->client6_info, &client6_info, sizeof(iaidaddr->client6_info));
		if (hash_add(server6_hash_table, 
		    &iaidaddr->client6_info, iaidaddr) != 0) {
			dprintf(LOG_ERR, "%s" "hash add failed", FNAME);
			free(iaidaddr);
			return (-1);
		}
		dprintf(LOG_DEBUG, "hash add client iaidaddr %u type %d for duid %s",
			iaidaddr->client6_info.iaidinfo.iaid,
			iaidaddr->client6_info.type, 
			duidstr(&iaidaddr->client6_info.clientid)); 
	}
	if (add_lease(iaidaddr, lease_rec) != 0)
		return (-1);
	iaidaddr->state = ACTIVE;
	iaidaddr->start_date = lease_rec->start_date;	
	d = get_max_validlifetime(iaidaddr) - offset;
	timo.tv_sec = (long)d;
	timo.tv_usec = 0;
	/* update the start date and timer */
	if (!iaidaddr->timer && (iaidaddr->timer = 
	     dhcp6_add_timer(dhcp6_iaidaddr_timo, iaidaddr)) == NULL) {
			dprintf(LOG_ERR, "%s" "failed to add a timer for iaid %u",
			FNAME, iaidaddr->client6_info.iaidinfo.iaid);
	 	return (-1);
	}
	dhcp6_set_timer(&timo, iaidaddr->timer);
	return (0);
}

static int 
add_lease(iaidaddr, lease)
	struct dhcp6_iaidaddr *iaidaddr;
	struct dhcp6_lease *lease;
{
	struct timeval timo;
	struct timeb now;
	time_t offset;
	struct dhcp6_lease *found_lease;
	double d;
	/* set up timer out for lease, iaidaddr timer out will be set up in
	 * post_conf_lease */
	ftime(&now);
	offset = now.time - lease_rec->start_date;
	if (dhcp6_mode == DHCP6_MODE_CLIENT) {
		if ((found_lease = dhcp6_find_lease(iaidaddr, &lease_rec->lease_addr)) 
					!= NULL) {
			if (!(found_lease->start_date > lease_rec->start_date)) {
				dprintf(LOG_DEBUG, "%s" "remove old lease %s/%d", FNAME, 
					in6addr2str(&lease_rec->lease_addr.addr, 0), 
					lease_rec->lease_addr.plen);
				if (iaidaddr->client6_info.iaidinfo.iaid != 
						client6_info.iaidinfo.iaid) {
					dprintf(LOG_DEBUG, "%s" 
					" this lease %s client iaidinfo is different"
					" to the previous lease", 
					FNAME,  in6addr2str(&lease_rec->lease_addr.addr, 0));
					return (-1);
				}
				iaidaddr->client6_info.iaidinfo.renewtime = 
						client6_info.iaidinfo.renewtime;
				iaidaddr->client6_info.iaidinfo.rebindtime = 
						client6_info.iaidinfo.rebindtime;
				iaidaddr->start_date = lease_rec->start_date;
				remove_lease(found_lease);
			} else {
				free(lease_rec);
				return (0);
			}
		}	 
		/* if this is the first lease */
		if (TAILQ_EMPTY(&iaidaddr->lease_list)) {
			if (iaidaddr->client6_info.iaidinfo.iaid != 
					client6_info.iaidinfo.iaid) {
				dprintf(LOG_DEBUG, "%s" 
					" this lease %s client iaidinfo is different"
					" to the previous lease", 
					FNAME,  in6addr2str(&lease_rec->lease_addr.addr, 0));
				return (-1);
			}
			iaidaddr->client6_info.iaidinfo.renewtime = 
					client6_info.iaidinfo.renewtime;
			iaidaddr->client6_info.iaidinfo.rebindtime = 
					client6_info.iaidinfo.rebindtime;
			iaidaddr->start_date = lease_rec->start_date;
		}
		if (!memcmp(&iaidaddr->client6_info, &client6_info, 
				sizeof(iaidaddr->client6_info))) { 
			/* iaidaddr->start_date != lease_rec->start_date) */
			dprintf(LOG_DEBUG, "%s" " this lease %s client iaidinfo is different"
				" to the previous lease", 
				FNAME,  in6addr2str(&lease_rec->lease_addr.addr, 0));
			return (-1);
		}
	} 
	if (lease_rec->state == INVALID) {
		dprintf(LOG_DEBUG, "parse an invalid state lease %s/%d in line %d",
			in6addr2str(&lease_rec->lease_addr.addr, 0), 
			lease_rec->lease_addr.plen, num_lines);
		free(lease_rec);
		return (0);
	}
	if ((lease_rec->timer = dhcp6_add_timer(dhcp6_lease_timo, lease_rec)) == NULL) {
		dprintf(LOG_ERR, "%s" " failed to create a timer for lease %s",
			FNAME, in6addr2str(&lease_rec->lease_addr.addr, 0));
			return (-1);
	}
	if (lease_rec->lease_addr.preferlifetime > offset) {
		lease_rec->state = ACTIVE;
		d = lease_rec->lease_addr.preferlifetime - offset;
	} else {
		lease_rec->state = EXPIRED;
		d = lease_rec->lease_addr.validlifetime - offset;
	} 
	timo.tv_sec = (long)d;
	timo.tv_usec = 0;
	dhcp6_set_timer(&timo, lease_rec->timer);
	if (dhcp6_mode == DHCP6_MODE_SERVER) {
		if (hash_add(lease_hash_table, &lease_rec->lease_addr, lease_rec) != 0) {
			dprintf(LOG_ERR, "%s" "hash add lease failed for %s",
				FNAME, in6addr2str(&lease_rec->lease_addr.addr, 0));
			free(lease_rec);
			return (-1);
		}
	}
	lease_rec->iaidaddr = iaidaddr;
	lease_rec->lease_addr.status_code = DH6OPT_STCODE_UNDEFINE;
	TAILQ_INSERT_TAIL(&iaidaddr->lease_list, lease_rec, link);
	dprintf(LOG_INFO, "%s" "add lease addr %s/%d type %d to %u", 
		FNAME, in6addr2str(&lease_rec->lease_addr.addr, 0), lease_rec->lease_addr.plen,
		lease_rec->lease_addr.type, lease_rec->iaidaddr->client6_info.iaidinfo.iaid);
	return (0);
}

static int
remove_lease(lease)
	struct dhcp6_lease *lease;
{
	if (dhcp6_mode == DHCP6_MODE_SERVER) {
		if (hash_delete(lease_hash_table, &lease->lease_addr) != 0) {
			dprintf(LOG_ERR, "%s" "failed to remove an address %s from hash", 
				FNAME, in6addr2str(&lease->lease_addr.addr, 0));
			return (-1);
		}
	}
	if (lease->timer)
		dhcp6_remove_timer(lease->timer);
	TAILQ_REMOVE(&lease->iaidaddr->lease_list, lease, link);
	dprintf(LOG_INFO, "%s" "removed lease addr %s/%d from %u", 
		FNAME, in6addr2str(&lease->lease_addr.addr, 0), 
		lease->lease_addr.plen, lease->iaidaddr->client6_info.iaidinfo.iaid);
	free(lease);
	return 0;
}

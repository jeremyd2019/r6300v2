/*	$Id: server6_parse.y,v 1.1.1.1 2006/12/04 00:45:34 Exp $	*/

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

%{

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <errno.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include "queue.h"
#include "dhcp6.h"
#include "config.h"
#include "common.h"
#include "server6_conf.h"
#include "hash.h"
#include "lease.h"

extern int num_lines;
extern char *sfyytext;
extern int sock;
static struct interface *ifnetworklist = NULL;
static struct link_decl *linklist = NULL;
static struct host_decl *hostlist = NULL;
static struct pool_decl *poollist = NULL;

static struct interface *ifnetwork = NULL;
static struct link_decl *link = NULL;
static struct host_decl *host = NULL;
static struct pool_decl *pool = NULL;
static struct scopelist *currentscope = NULL;
static struct scopelist *currentgroup = NULL;
static int allow = 0;

static void cleanup(void);
void sfyyerror(char *msg);

#define ABORT	do { cleanup(); YYABORT; } while (0)

extern int sfyylex __P((void));
%}
%token	<str>	INTERFACE IFNAME
%token	<str>	PREFIX
%token	<str>	LINK	
%token	<str>	RELAY

%token	<str>	STRING
%token	<num>	NUMBER
%token	<snum>	SIGNEDNUMBER
%token	<dec>	DECIMAL
%token	<bool>	BOOLEAN
%token	<addr>	IPV6ADDR
%token 	<str>	INFINITY

%token	<str>	HOST
%token	<str>	POOL
%token	<str>	RANGE
%token	<str>	GROUP
%token	<str>	LINKLOCAL
%token	<str>	OPTION ALLOW SEND
%token	<str>	PREFERENCE
%token	<str>	RENEWTIME
%token	<str>	REBINDTIME
%token	<str>	RAPIDCOMMIT
%token	<str>	ADDRESS
%token	<str>	VALIDLIFETIME
%token	<str>	PREFERLIFETIME
%token	<str>	UNICAST
%token	<str>	TEMPIPV6ADDR
%token	<str>	DNS_SERVERS
%token	<str>	SIP_SERVERS
%token	<str>	NTP_SERVERS
%token	<str>	DUID DUID_ID
%token	<str>	IAID IAIDINFO
%token  <str>	INFO_ONLY
%token	<str>	TO

%token	<str>	BAD_TOKEN
%type	<str>	name
%type   <num>	number_or_infinity
%type	<dhcp6addr>	hostaddr6 hostprefix6 addr6para v6address

%union {
	unsigned int	num;
	int 	snum;
	char	*str;
	int 	dec;
	int	bool;
	struct in6_addr	addr;
	struct dhcp6_addr *dhcp6addr;
}
%%
statements	
	: 
	| statements networkdef 
	;

networkdef	
	: ifdef
	| groupdef
	| confdecl
	| linkdef
	;

ifdef		
	: ifhead '{' ifbody  '}' ';'
	{
		if (linklist) {
			ifnetwork->linklist = linklist;
			linklist = NULL;
		}
		if (hostlist) {
			ifnetwork->hostlist = hostlist;
			hostlist = NULL;
		}
		if (currentgroup) 
			ifnetwork->group = currentgroup->scope;

		dprintf(LOG_DEBUG, "interface definition for %s is ok", ifnetwork->name);
		ifnetwork->next = ifnetworklist;
		ifnetworklist = ifnetwork;
		ifnetwork = NULL;

		globalgroup->iflist = ifnetworklist;

		/* leave interface scope we know the current scope is not point to NULL*/
		currentscope = pop_double_list(currentscope);
	} 	
	;

ifhead		
	: INTERFACE IFNAME 
	{
		struct interface *temp_if = ifnetworklist;
		while (temp_if)
		{
			if (!strcmp(temp_if->name, $2))
			{
				dprintf(LOG_ERR, "duplicate interface definition for %s",
					temp_if->name);
				ABORT;
			}
			temp_if = temp_if->next;
		}
		ifnetwork = (struct interface *)malloc(sizeof(*ifnetwork));
		if (ifnetwork == NULL) {
			dprintf(LOG_ERR, "failed to allocate memory");
			ABORT;
		}
		memset(ifnetwork, 0, sizeof(*ifnetwork));
		TAILQ_INIT(&ifnetwork->ifscope.dnslist.addrlist);
		TAILQ_INIT(&ifnetwork->ifscope.siplist);
		TAILQ_INIT(&ifnetwork->ifscope.ntplist);
		strncpy(ifnetwork->name, $2, strlen($2)); 
		if (get_linklocal(ifnetwork->name, &ifnetwork->linklocal) < 0) {
			dprintf(LOG_ERR, "get device %s linklocal failed", ifnetwork->name);
		}
		
		/* check device, if the device is not available,
		 * it is OK, it might be added later
		 * so keep this in the configuration file.
		 */
		if (if_nametoindex(ifnetwork->name) == 0) {
			dprintf(LOG_ERR, "this device %s doesn't exist.", $2);
		}
		/* set up hw_addr, link local, primary ipv6addr */
		/* enter interface scope */
		currentscope = push_double_list(currentscope, &ifnetwork->ifscope);
		if (currentscope == NULL)
			ABORT;
	}
	;
ifbody
	:
	| ifbody ifparams
	;

ifparams
	: linkdef
	| hostdef
	| groupdef
	| confdecl
	;	

linkdef	
	: linkhead '{' linkbody '}' ';'
	{
		if (poollist) {
			link->poollist = poollist;
			poollist = NULL;
		}
		if (currentgroup) 
			link->group = currentgroup->scope;

		link->next = linklist;
		linklist = link;
		link = NULL;
		/* leave iink scope we know the current scope is not point to NULL*/
		currentscope = pop_double_list(currentscope);
	}
	;

linkhead	
	: LINK name
	{
		struct link_decl *temp_sub = linklist;
		/* memory allocation for link */
		link = (struct link_decl *)malloc(sizeof(*link));
		if (link == NULL) {
			dprintf(LOG_ERR, "failed to allocate memory");
			ABORT;
		}
		memset(link, 0, sizeof(*link));
		TAILQ_INIT(&link->linkscope.dnslist.addrlist);
		TAILQ_INIT(&link->linkscope.siplist);
		TAILQ_INIT(&link->linkscope.ntplist);
		while (temp_sub) {
			if (!strcmp(temp_sub->name, $2))
			{
				dprintf(LOG_ERR, "duplicate link definition for %s", $2);
				ABORT;
			}
			temp_sub = temp_sub->next;
		}			
		/* link set */
		strncpy(link->name, $2, strlen($2));
		if (ifnetwork)
			link->network = ifnetwork;
		else {
			/* create a ifnetwork for this interface */
		}
		link->relaylist = NULL;
		link->seglist = NULL;
		/* enter link scope */
		currentscope = push_double_list(currentscope, &link->linkscope);
		if (currentscope == NULL)
			ABORT;
	}
	;	

linkbody	
	:  
	| linkbody linkparams
	;

linkparams	
	: pooldef
	| rangedef
	| prefixdef
	| hostdef
	| groupdef
	| confdecl
	| relaylist 
	;

relaylist	
	: relaylist relaypara
	| relaypara
	;

relaypara	
	: RELAY IPV6ADDR '/' NUMBER ';'
	{
		struct v6addrlist *temprelay;
		if (!link) {
			dprintf(LOG_ERR, "relay must be defined under link");
			ABORT;
		}
		temprelay = (struct v6addrlist *)malloc(sizeof(*temprelay));
		if (temprelay == NULL) {
			dprintf(LOG_ERR, "failed to allocate memory");
			ABORT;
		}
		memset(temprelay, 0, sizeof(*temprelay));
		memcpy(&temprelay->v6addr.addr, &$2, sizeof(temprelay->v6addr.addr));
		temprelay->v6addr.plen = $4;
		temprelay->next = link->relaylist;
		link->relaylist = temprelay;
		temprelay = NULL;
	}
	;

pooldef		
	: poolhead '{' poolbody '}' ';'
	{
		if (currentgroup) 
			pool->group = currentgroup->scope;
		pool->next = poollist;
		poollist = pool;
		pool = NULL;
		/* leave pool scope we know the current scope is not point to NULL*/
		currentscope = pop_double_list(currentscope);
	}
	;

poolhead	
	: POOL
	{
		if (!link) {
			dprintf(LOG_ERR, "pooldef must be defined under link");
			ABORT;
		}
		pool = (struct pool_decl *)malloc(sizeof(*pool));
		if (pool == NULL) {
			dprintf(LOG_ERR, "fail to allocate memory");
			ABORT;
		}
		memset(pool, 0, sizeof(*pool));
		TAILQ_INIT(&pool->poolscope.dnslist.addrlist);
		TAILQ_INIT(&pool->poolscope.siplist);
		TAILQ_INIT(&pool->poolscope.ntplist);
		if (link)
			pool->link = link;
			
		/* enter pool scope */
		currentscope = push_double_list(currentscope, &pool->poolscope);
		if (currentscope == NULL)
			ABORT;
	}
	;

poolbody	
	: 
	| poolbody poolparas 
	;

poolparas	
	: hostdef
	| groupdef
	| rangedef
	| prefixdef
	| confdecl
	;

prefixdef
	: PREFIX IPV6ADDR '/' NUMBER ';'
	{
		struct v6prefix *v6prefix, *v6prefix0;
		struct v6addr *prefix;
		if (!link) {
			dprintf(LOG_ERR, "prefix must be defined under link");
			ABORT;
		}
		v6prefix = (struct v6prefix *)malloc(sizeof(*v6prefix));
		if (v6prefix == NULL) {
			dprintf(LOG_ERR, "failed to allocate memory");
			ABORT;
		}
		memset(v6prefix, 0, sizeof(*v6prefix));
		v6prefix->link = link;
		if (pool)
			v6prefix->pool = pool;
		/* make sure the range ipv6 address within the prefixaddr */
		if ($4 > 128 || $4 < 0) {
			dprintf(LOG_ERR, "invalid prefix length in line %d", num_lines);
			ABORT;
		}
		prefix = getprefix(&$2, $4);
		for (v6prefix0 = link->prefixlist; v6prefix0; v6prefix0 = v6prefix0->next) {
			if (IN6_ARE_ADDR_EQUAL(prefix, &v6prefix0->prefix.addr) && 
					$4 == v6prefix0->prefix.plen)  {
				dprintf(LOG_ERR, "duplicated prefix defined within same link");
				ABORT;
			}
		}
		/* check the assigned prefix is not reserved pv6 addresses */
		if (IN6_IS_ADDR_RESERVED(prefix)) {
			dprintf(LOG_ERR, "config reserved prefix");
			ABORT;
		}
		memcpy(&v6prefix->prefix, prefix, sizeof(v6prefix->prefix));
		v6prefix->next = link->prefixlist;
		link->prefixlist = v6prefix;
		free(prefix);
	}
	;

rangedef
	: RANGE IPV6ADDR TO IPV6ADDR '/' NUMBER ';'
	{
		struct v6addrseg *seg, *temp_seg;
		struct v6addr *prefix1, *prefix2;
		if (!link) {
			dprintf(LOG_ERR, "range must be defined under link");
			ABORT;
		}
		seg = (struct v6addrseg *)malloc(sizeof(*seg));
		if (seg == NULL) {
			dprintf(LOG_ERR, "failed to allocate memory");
			ABORT;
		}
		memset(seg, 0, sizeof(*seg));
		temp_seg = link->seglist;
		seg->link = link;
		if (pool)
			seg->pool = pool;
		/* make sure the range ipv6 address within the prefixaddr */
		if ($6 > 128 || $6 < 0) {
			dprintf(LOG_ERR, "invalid prefix length in line %d", num_lines);
			ABORT;
		}
		prefix1 = getprefix(&$2, $6);
		prefix2 = getprefix(&$4, $6);
		if (!prefix1 || !prefix2) {
			dprintf(LOG_ERR, "address range defined error");
			ABORT;
		}
		if (ipv6addrcmp(&prefix1->addr, &prefix2->addr)) {
			dprintf(LOG_ERR, 
				"address range defined doesn't in the same prefix range");
			ABORT;
		}
		if (ipv6addrcmp(&$2, &$4) < 0) {
			memcpy(&seg->min, &$2, sizeof(seg->min));
			memcpy(&seg->max, &$4, sizeof(seg->max));
		} else {
			memcpy(&seg->max, &$2, sizeof(seg->max));
			memcpy(&seg->min, &$4, sizeof(seg->min));
		}
		/* check the assigned addresses are not reserved ipv6 addresses */
		if (IN6_IS_ADDR_RESERVED(&seg->max) || IN6_IS_ADDR_RESERVED(&seg->max)) {
			dprintf(LOG_ERR, "config reserved ipv6address");
			ABORT;
		}

		memcpy(&seg->prefix, prefix1, sizeof(seg->prefix));
		memcpy(&seg->free, &seg->min, sizeof(seg->free));
		if (pool)
			seg->pool = pool;
		/* make sure there is no overlap in the rangelist */
		/* the segaddr is sorted by prefix len, thus most specific
		   ipv6 address is going to be assigned.
		 */
		if (!temp_seg) {
			seg->next = NULL;
			seg->prev = NULL;
			link->seglist = seg;
		} else {
			for (; temp_seg; temp_seg = temp_seg->next) { 
				if ( prefix1->plen < temp_seg->prefix.plen) {
					if (temp_seg->next == NULL) {
						temp_seg->next = seg;
						seg->prev = temp_seg;
						seg->next = NULL;
						break;
					}
					continue;
				}
				if (prefix1->plen == temp_seg->prefix.plen) {
	     				if (!(ipv6addrcmp(&seg->min, &temp_seg->max) > 0
					    || ipv6addrcmp(&seg->max, &temp_seg->min) < 0)) {
		   				dprintf(LOG_ERR, "overlap range addr defined");
		   				ABORT;
					}
				}
				if (temp_seg->prev == NULL) { 
					link->seglist = seg;
					seg->prev = NULL;
				} else {
					temp_seg->prev->next = seg;
					seg->prev = temp_seg->prev;
				}
				temp_seg->prev = seg;
				seg->next = temp_seg;
				break;
			}
		}
		free(prefix1);
		free(prefix2);
	}
	;

groupdef	
	: grouphead '{' groupbody  '}' ';'
	{
		/* return to prev group scope if any */
		currentgroup = pop_double_list(currentgroup);

		/* leave current group scope */
		currentscope = pop_double_list(currentscope);
	}
	;

groupbody	
	: 
	| groupbody groupparas
	;

groupparas	
	: hostdef
	| pooldef
	| linkdef
	| rangedef
	| prefixdef
	| ifdef
	| confdecl
	;

grouphead	
	: GROUP
	{
		struct scope *groupscope;
		groupscope = (struct scope *)malloc(sizeof(*groupscope));
		if (groupscope == NULL) {
			dprintf(LOG_ERR, "group memory allocation failed");
			ABORT;
		}
		memset(groupscope, 0, sizeof(*groupscope));
		TAILQ_INIT(&groupscope->dnslist.addrlist);
		TAILQ_INIT(&groupscope->siplist);
		TAILQ_INIT(&groupscope->ntplist);
		/* set up current group */
		currentgroup = push_double_list(currentgroup, groupscope);
		if (currentgroup == NULL)
			ABORT;

		/* enter group scope  */
		currentscope = push_double_list(currentscope, groupscope);
		if (currentscope == NULL)
			ABORT;
	}
	;

hostdef	
	: hosthead '{' hostbody '}' ';'
	{
		struct host_decl *temp_host = hostlist;
		while (temp_host)
		{
			if (temp_host->iaidinfo.iaid == host->iaidinfo.iaid) {
				dprintf(LOG_ERR, "duplicated host %d redefined", 
					host->iaidinfo.iaid);
				ABORT;
			}
			temp_host = temp_host->next;
		}
		if (currentgroup) 
			host->group = currentgroup->scope;
		host->next = hostlist;
		hostlist = host;
		host = NULL;
		/* leave host scope we know the current scope is not point to NULL*/
		currentscope = pop_double_list(currentscope);
	}
	;


hosthead	
	: HOST name
	{
		struct host_decl *temp_host = hostlist;
		while (temp_host)
		{
			if (!strcmp(temp_host->name, $2)) {
				dprintf(LOG_ERR, "duplicated host %s redefined", $2);
				ABORT;
			}
			temp_host = temp_host->next;
		}
		host = (struct host_decl *)malloc(sizeof(*host));
		if (host == NULL) {
			dprintf(LOG_ERR, "fail to allocate memory");
			ABORT;
		}
		memset(host, 0, sizeof(*host));
		TAILQ_INIT(&host->addrlist);
		TAILQ_INIT(&host->prefixlist);
		TAILQ_INIT(&host->hostscope.dnslist.addrlist);
		TAILQ_INIT(&host->hostscope.siplist);
		TAILQ_INIT(&host->hostscope.ntplist);
		host->network = ifnetwork;
		strncpy(host->name, $2, strlen($2));
		/* enter host scope */
		currentscope = push_double_list(currentscope, &host->hostscope);
		if (currentscope == NULL)
			ABORT;
	}
	;

hostbody
	: hostbody hostdecl
	| hostdecl
	;

hostdecl
	: DUID DUID_ID ';'
	{
		if (host == NULL) {
			dprintf(LOG_DEBUG, "duid should be defined under host decl");
			ABORT;
		}
		configure_duid($2, &host->cid);
	}
	| iaiddef
	| hostparas
	;

iaiddef
	: IAIDINFO '{' iaidbody '}' ';'
	{
	}
	;

iaidbody
	: iaidbody RENEWTIME number_or_infinity ';'
	{
		host->iaidinfo.renewtime = $3;
	}
	| iaidbody REBINDTIME number_or_infinity ';'
	{
		host->iaidinfo.rebindtime = $3;
	}
	| iaidpara
	;

iaidpara
	: IAID NUMBER ';'
	{
		if (host == NULL) {
			dprintf(LOG_DEBUG, "iaid should be defined under host decl");
			ABORT;
		}
		host->iaidinfo.iaid = $2;
	}
	;	

hostparas
	: hostparas hostpara
	| hostpara
	;

hostpara
	: hostaddr6
	{
		if (host == NULL) {
			dprintf(LOG_DEBUG, "address should be defined under host decl");
			ABORT;
		}
		dhcp6_add_listval(&host->addrlist, $1, DHCP6_LISTVAL_DHCP6ADDR);
		if (hash_add(host_addr_hash_table, &($1->addr), $1) != 0) {
			dprintf(LOG_ERR, "%s" "hash add lease failed for %s",
				FNAME, in6addr2str(&($1->addr), 0));
			free($1);
			return (-1);
		}
	}
	| hostprefix6
	{
		if (host == NULL) {
			dprintf(LOG_DEBUG, "prefix should be defined under host decl");
			ABORT;
		}
		dhcp6_add_listval(&host->prefixlist, $1, DHCP6_LISTVAL_DHCP6ADDR);
	}
	| optiondecl
	;

hostaddr6
	: ADDRESS '{' addr6para '}' ';'
	{
		$3->type = IANA;
		$$ = $3;
	}
	;

hostprefix6
	: PREFIX '{' addr6para '}' ';'
	{
		$3->type = IAPD;
		$$ = $3;
	}
	;
	
addr6para
	: addr6para VALIDLIFETIME number_or_infinity ';'
	{
		$1->validlifetime = $3;
	}
	| addr6para PREFERLIFETIME number_or_infinity ';'
	{
		$1->preferlifetime = $3;
	}
	| v6address
	{
		$$ = $1;
	}
	;

v6address
	: IPV6ADDR '/' NUMBER ';'
	{
		struct dhcp6_addr *temp;
		temp = (struct dhcp6_addr *)malloc(sizeof(*temp));
		if (temp == NULL) {
			dprintf(LOG_ERR, "v6addr memory allocation failed");
			ABORT;
		}
		memset(temp, 0, sizeof(*temp));
		memcpy(&temp->addr, &$1, sizeof(temp->addr));
		if ($3 > 128 || $3 < 0) {
			dprintf(LOG_ERR, "invalid prefix length in line %d", num_lines);
			ABORT;
		}
		temp->plen = $3;
		$$ = temp;
	}
	;

optiondecl
	: optionhead optionpara
	;

optionhead
	: SEND
	{		
		if (!currentscope) { 
			currentscope = push_double_list(currentscope, &globalgroup->scope);
			if (currentscope == NULL)
				ABORT;
		}
	}	
	| ALLOW
	{		
		if (!currentscope) {
			currentscope = push_double_list(currentscope, &globalgroup->scope);
			if (currentscope == NULL)
				ABORT;
		}
		allow = 1;
	}
	| OPTION
	{		
		if (!currentscope) {
			currentscope = push_double_list(currentscope, &globalgroup->scope);
			if (currentscope == NULL)
				ABORT;
		}
	}
	;

optionpara
	: RAPIDCOMMIT ';'
	{
		if (allow) 
			currentscope->scope->allow_flags |= DHCIFF_RAPID_COMMIT;
		else 
			currentscope->scope->send_flags |= DHCIFF_RAPID_COMMIT;
	}
	| TEMPIPV6ADDR ';'
	{
		if (allow) 
			currentscope->scope->allow_flags |= DHCIFF_TEMP_ADDRS;
		else 
			currentscope->scope->send_flags |= DHCIFF_TEMP_ADDRS;
	}
	| UNICAST ';'
	{
		if (allow) 
			currentscope->scope->allow_flags |= DHCIFF_UNICAST;
		else 
			currentscope->scope->send_flags |= DHCIFF_UNICAST;
	}
	| INFO_ONLY ';'
	{
		if (allow) 
			currentscope->scope->allow_flags |= DHCIFF_INFO_ONLY;
		else
			currentscope->scope->send_flags |= DHCIFF_INFO_ONLY;
	}
	| DNS_SERVERS dns_paras ';'
	{
	}
	| SIP_SERVERS sip_paras ';'
	{
	}
	| NTP_SERVERS ntp_paras ';'
	{
	}
	;

dns_paras
	: dns_paras dns_para
	| dns_para
	;

dns_para
	: IPV6ADDR 
	{
		dhcp6_add_listval(&currentscope->scope->dnslist.addrlist, &$1,
			DHCP6_LISTVAL_ADDR6);
	}

sip_paras
	: sip_paras sip_para
	| sip_para
	;

sip_para
	: IPV6ADDR 
	{
		dhcp6_add_listval(&currentscope->scope->siplist, &$1,
			DHCP6_LISTVAL_ADDR6);
	}

ntp_paras
	: ntp_paras ntp_para
	| ntp_para
	;

ntp_para
	: IPV6ADDR 
	{
		dhcp6_add_listval(&currentscope->scope->ntplist, &$1,
			DHCP6_LISTVAL_ADDR6);
	}

	| STRING
	{
		struct domain_list *domainname, *temp;
		int len = 0;
		domainname = (struct domain_list *)malloc(sizeof(*domainname));
		if (domainname == NULL)
			ABORT;
		len = strlen($1);
		if (len > MAXDNAME) 
			ABORT;
		strncpy(domainname->name, $1, len);
		domainname->name[len] = '\0';
		domainname->next = NULL;
		if (currentscope->scope->dnslist.domainlist == NULL) {
			currentscope->scope->dnslist.domainlist = domainname;
			dprintf(LOG_DEBUG, "add domain name %s", domainname->name);
		} else {
			for (temp = currentscope->scope->dnslist.domainlist; temp;
			     temp = temp->next) {
				if (temp->next == NULL) {
					dprintf(LOG_DEBUG, "add domain name %s", 
						domainname->name);
					temp->next = domainname;
					break;
				}
			}
		}
	}
	;

confdecl	
	: paradecl
	| optiondecl
	;

paradecl
	: RENEWTIME number_or_infinity ';'
	{
		if (!currentscope) {
			currentscope = push_double_list(currentscope, &globalgroup->scope);
			if (currentscope == NULL)
				ABORT;
		}
		currentscope->scope->renew_time = $2;
	}
	| REBINDTIME number_or_infinity ';'
	{	
		if (!currentscope) {
			currentscope = push_double_list(currentscope, &globalgroup->scope);
			if (currentscope == NULL)
				ABORT;
		}
		currentscope->scope->rebind_time = $2;
	}
	| VALIDLIFETIME number_or_infinity ';'
	{
		if (!currentscope) {
			currentscope = push_double_list(currentscope, &globalgroup->scope);
			if (currentscope == NULL)
				ABORT;
		}
		currentscope->scope->valid_life_time = $2;
		if (currentscope->scope->prefer_life_time != 0 && 
		    currentscope->scope->valid_life_time <
		    currentscope->scope->prefer_life_time) {
			dprintf(LOG_ERR, "%s" 
				"validlifetime is less than preferlifetime", FNAME);
			ABORT;
		}
	}
	| PREFERLIFETIME number_or_infinity ';'
	{
		if (!currentscope) {
			currentscope = push_double_list(currentscope, &globalgroup->scope);
			if (currentscope == NULL)
				ABORT;
		}
		currentscope->scope->prefer_life_time = $2;
		if (currentscope->scope->valid_life_time != 0 &&
		    currentscope->scope->valid_life_time <
		    currentscope->scope->prefer_life_time) {
			dprintf(LOG_ERR, "%s" 
				"validlifetime is less than preferlifetime", FNAME);
			ABORT;
		}
	}
	| PREFERENCE NUMBER ';'
	{
		if (!currentscope) {
			currentscope = push_double_list(currentscope, &globalgroup->scope);
			if (currentscope == NULL)
				ABORT;
		}
		if ($2 < 0 || $2 > 255)
			dprintf(LOG_ERR, "%s" "bad server preference number", FNAME);
		currentscope->scope->server_pref = $2;
	}
	;

number_or_infinity
	: NUMBER
	{
		$$ = $1; 
	}
	| INFINITY
	{
		$$ = DHCP6_DURATITION_INFINITE;
	}
	;

name
	: STRING
	{
		$$ = $1;
	}
	;

%%


static
void cleanup(void)
{
	/* it is not necessary to free all the pre malloc(), if it fails, 
	 * exit will free them automatically.
	 */
}

void
sfyyerror(char *msg)
{
	cleanup();
	dprintf(LOG_ERR, "%s in line %d: %s ", msg, num_lines, sfyytext);
}

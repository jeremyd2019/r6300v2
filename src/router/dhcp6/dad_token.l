/*	$Id: dad_token.l,v 1.1.1.1 2006/12/04 00:45:29 Exp $	*/

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
 */

%option noyywrap

%{
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

#include <errno.h>
#include <syslog.h>
#include <string.h>

#include "queue.h"
#include "dhcp6.h"
#include "config.h"
#include "common.h"
#include "lease.h"

#define YYABORT(msg) dprintf(LOG_ERR, msg " %s lineno %d.", \
	ifyytext, num_lines)

#define ABORT   do {    \
	YYABORT("/proc/net/if_inet6 file parse error");   \
	exit(1);        \
} while (0)

#define DAD_FLAGS	0xc0

struct ifproc_info {
	struct ifproc_info *next;
	struct in6_addr addr;
	char name[IFNAMSIZ];
	int index;
	int plen;
	int scope;
	int flags;
};

extern struct dhcp6_if *dhcp6_if;
extern struct dhcp6_list request_list;
const char *ifproc_file = "/proc/net/if_inet6";
struct ifproc_info *dadlist = NULL;

static struct ifproc_info *ifinfo;
static int num_lines = 0;

%}

hexdigit        ([a-f]|[A-F]|[0-9])
hexpair		({hexdigit}{hexdigit})
hexaddr		({hexpair}{16})
whitespace      ([ \t])+
ifname          [a-zA-Z]+[0-9]+
nl              \n


%s S_CNF S_NAME S_INDEX S_PLEN S_SCOPE S_FLAGS

%%

%{
	BEGIN S_CNF;
%}

<S_CNF>{hexaddr} {struct in6_addr addr;
	char buff[64];
	int i, len = 0;
	for (i = 0; i < 32; i += 4) {
		strncpy(buff+len, &ifyytext[i], 4);
		len += 4;
		if (i < 28) {
			strcpy(buff+len, ":");
			len += 1;
		} else
			strcpy(buff+len, "\0");
	}
	if (inet_pton(AF_INET6, buff, &addr) < 1) {
		dprintf(LOG_ERR, "parse address failed");
		ABORT;
	} else {
		ifinfo = (struct ifproc_info *)malloc(sizeof(*ifinfo));
		if (ifinfo == NULL)
			ABORT;
		memcpy(&ifinfo->addr, &addr, sizeof(ifinfo->addr));
		BEGIN S_INDEX;}
	}
<S_INDEX>{hexpair} {ifinfo->index = strtol(ifyytext, NULL, 16);
		BEGIN S_PLEN;}
<S_PLEN>{hexpair} {ifinfo->plen = strtol(ifyytext, NULL, 16); 
		BEGIN S_SCOPE;}
<S_SCOPE>{hexpair} {ifinfo->scope = strtol(ifyytext, NULL, 16);
		BEGIN S_FLAGS;}
<S_FLAGS>{hexpair} {ifinfo->flags = strtol(ifyytext, NULL, 16);
		if (ifinfo->flags == DAD_FLAGS) {
			dprintf(LOG_INFO, "duplicated ipv6 address %s detected",
				in6addr2str(&ifinfo->addr, 0));
			BEGIN S_NAME;
		} else {
			free(ifinfo);
			BEGIN S_CNF;
		}
		}
<S_NAME>{ifname} {
		if (strcmp(ifyytext, dhcp6_if->ifname)) {
			free(ifinfo);
			BEGIN S_CNF;
		} else {
			struct dhcp6_listval *lv;
			struct dhcp6_lease *cl;
			strncpy(ifinfo->name, ifyytext, IFNAMSIZ);
			ifinfo->next = NULL;
			if (dadlist == NULL) {
				TAILQ_INIT(&request_list);
				dadlist = ifinfo;
			} else
				dadlist->next = ifinfo;
		
			/* check address on client6_iaidaddr list */	
			if ((lv = (struct dhcp6_listval *)malloc(sizeof(*lv)))
			    == NULL) {
				dprintf(LOG_ERR, "failed to allocate memory");
				return (-1);
			}
			
			for (cl = TAILQ_FIRST(&client6_iaidaddr.lease_list); cl;
			     cl = TAILQ_NEXT(cl, link)) {
				if (cl->lease_addr.type != IAPD && 
				    IN6_ARE_ADDR_EQUAL(&cl->lease_addr.addr, &ifinfo->addr))
					break;
				else 
					BEGIN S_CNF;
			}
			/* deconfigure the interface's the address assgined by dhcpv6 */
			if (dhcp6_remove_lease(cl) != 0) {
				dprintf(LOG_INFO,
					"remove duplicated address failed: %s",
					in6addr2str(&lv->val_dhcp6addr.addr, 0));
				return (-1);
			}
			memcpy(&lv->val_dhcp6addr.addr, &ifinfo->addr, 
				sizeof(lv->val_dhcp6addr.addr));
			lv->val_dhcp6addr.type = IANA;
			lv->val_dhcp6addr.plen = ifinfo->plen;
			lv->val_dhcp6addr.status_code = DH6OPT_STCODE_UNDEFINE;
			lv->val_dhcp6addr.preferlifetime = 0;
			lv->val_dhcp6addr.validlifetime = 0;
			TAILQ_INSERT_TAIL(&request_list, lv, link);
			BEGIN S_CNF;
		}
	}

{nl}            {num_lines++;}
{whitespace}    {;}
%%

int
dad_parse(const char *file)
{
	if ((ifyyin = fopen(file, "r")) == NULL) {
		if (errno == ENOENT)
			return (0);
		dprintf(LOG_ERR, "dad_parse: fopen(%s): %s",
			file, strerror(errno));
		return (-1);
	}
	yylex();
	return 0;
}

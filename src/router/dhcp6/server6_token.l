/*	$Id: server6_token.l,v 1.1.1.1 2006/12/04 00:45:34 Exp $	*/

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
#include <syslog.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>

#include "queue.h"
#include "dhcp6.h"
#include "config.h"
#include "common.h"
#include <server6_conf.h>
#include "sf.tab.h"

int num_lines = 1;
int sfyyerrorcount = 0;
int sfparse(char *filename);
extern void sfyyerror(char *msg);
extern struct rootgroup *globalgroup;

extern int sfyyparse(void);

%}

whitespace	([ \t])+
digit		[0-9]
number		({digit})+
snum		-?({digit})+
decimal		({number}"."{number})
hexdigit	([a-f]|[A-F]|[0-9])
hexpair		{hexdigit}{hexdigit}
duid_id		{hexpair}(":"{hexpair})*
ipv4addr        ({digit}{1,3}"."{digit}{1,3}"."{digit}{1,3}"."{digit}{1,3})
addr_head       ("::"|{hexdigit}{1,4}(":"|"::"))
addr_tail       ({hexdigit}{1,4}|({hexdigit}{1,4}"::")|{ipv4addr})?
addr_body       ({hexdigit}{1,4}(":"|"::"))*
ipv6addr        {addr_head}{addr_body}{addr_tail}
string		[a-zA-Z0-9:\._-][a-zA-Z0-9:\._-]*
ifname		[a-zA-Z]+[0-9]+

%s S_IFACE
%s S_DUID
%s S_OPTION

%%

group		{ return GROUP; }

interface	{ BEGIN S_IFACE; return INTERFACE; }
<S_IFACE>{ifname} 	{ BEGIN INITIAL; sfyylval.str = strdup(sfyytext); return IFNAME;}

link		{ return LINK; }
relay		{ return RELAY; }
host		{ return HOST; }

pool		{ return POOL; }
range		{ return RANGE; }

iaid		{ return IAID; }
iaidinfo 	{ return IAIDINFO; }

duid		{ BEGIN S_DUID; return DUID; }
<S_DUID>{duid_id} { BEGIN INITIAL; sfyylval.str = strdup(sfyytext); return DUID_ID; } 

prefix		{ return PREFIX; }
address		{ return ADDRESS;}

allow		{ BEGIN S_OPTION; return ALLOW; }
send		{ BEGIN S_OPTION; return SEND; }
option		{ BEGIN S_OPTION; return OPTION; }

<S_OPTION>unicast	{ BEGIN INITIAL; return UNICAST; }
<S_OPTION>rapid-commit	{ BEGIN INITIAL; return RAPIDCOMMIT; }
<S_OPTION>temp-address	{ BEGIN INITIAL; return TEMPIPV6ADDR; }
<S_OPTION>information-only	{ BEGIN INITIAL; return INFO_ONLY; }
<S_OPTION>dns_servers	{ BEGIN INITIAL; return DNS_SERVERS;}
<S_OPTION>sip_servers	{ BEGIN INITIAL; return SIP_SERVERS;}
<S_OPTION>ntp_servers	{ BEGIN INITIAL; return NTP_SERVERS;}

renew-time	{ return RENEWTIME; }
rebind-time	{ return REBINDTIME; }
valid-life-time	 { return VALIDLIFETIME; }
prefer-life-time { return PREFERLIFETIME; }
server-preference { return PREFERENCE; }

to	{ return TO; }
infinity	{ return INFINITY; }

#.*$			{/* ignore comments */}
\n			{num_lines++;}
{whitespace}		{}
{ipv6addr} {
			struct in6_addr addr;
			int i;
				
			i = inet_pton(AF_INET6, sfyytext, &addr);
			if (i < 1) {
				dprintf(LOG_ERR, "invalid address in line %d", num_lines); 
				return BAD_TOKEN;
			}

			sfyylval.addr = addr;
			return IPV6ADDR;
		}

{number}	{ sfyylval.num = strtoll(sfyytext, NULL, 10); return NUMBER; }
{snum}	{ sfyylval.snum = strtoll(sfyytext, NULL, 10); return SIGNEDNUMBER; }
{decimal} { sfyylval.dec = atof(sfyytext); return DECIMAL; }
{string}	{
			static char name[MAXDNAME];
			strncpy(name, sfyytext, MAXDNAME-1);
			name[MAXDNAME-1] = '\0';
			sfyylval.str = name;
			return STRING;
		}
"{"|"}"|";"|"/"	{ return *sfyytext; }
.		{ return BAD_TOKEN; }

%%

int
sfparse(conf)
        char *conf;
{
        if ((sfyyin = fopen(conf, "r")) == NULL) {
                if (errno == ENOENT)
                        return(0);
                dprintf(LOG_ERR, "sfparse: fopen(%s): %s",
                        conf, strerror(errno));
		printf("open error\n");
                return(-1);
        }

        if (sfyyparse() || sfyyerrorcount) {
                sfyyerror("fatal parse failure: exiting");
                return(-1);
        }
	post_config(globalgroup);

	return (0);
}

/*	$Id: client6_parse.y,v 1.1.1.1 2006/12/04 00:45:21 Exp $	*/
/*	ported from KAME: cfparse.y,v 1.16 2002/09/24 14:20:49 itojun Exp	*/

/*
 * Copyright (C) 2002 WIDE Project.
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
%{
#include <string.h>

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <arpa/inet.h>

#include <malloc.h>
#include "queue.h"
#include "dhcp6.h"
#include "config.h"
#include "common.h"

extern int lineno;
extern int cfdebug;

extern void cpyywarn __P((char *, ...))
	__attribute__((__format__(__printf__, 1, 2)));
extern void cpyyerror __P((char *, ...))
	__attribute__((__format__(__printf__, 1, 2)));

#define MAKE_NAMELIST(l, n, p) do { \
	(l) = (struct cf_namelist *)malloc(sizeof(*(l))); \
	if ((l) == NULL) { \
		cpyywarn("can't allocate memory"); \
		if (p) cleanup_cflist(p); \
		return (-1); \
	} \
	memset((l), 0, sizeof(*(l))); \
	l->line = lineno; \
	l->name = (n); \
	l->params = (p); \
	} while (0)

#define MAKE_CFLIST(l, t, pp, pl) do { \
	(l) = (struct cf_list *)malloc(sizeof(*(l))); \
	if ((l) == NULL) { \
		cpyywarn("can't allocate memory"); \
		if (pp) free(pp); \
		if (pl) cleanup_cflist(pl); \
		return (-1); \
	} \
	memset((l), 0, sizeof(*(l))); \
	l->line = lineno; \
	l->type = (t); \
	l->ptr = (pp); \
	l->list = (pl); \
	} while (0)

static struct cf_namelist *iflist_head;
struct cf_list *cf_dns_list;

extern int cpyylex __P((void));
static void cleanup __P((void));
static int add_namelist __P((struct cf_namelist *, struct cf_namelist **));
static void cleanup_namelist __P((struct cf_namelist *));
static void cleanup_cflist __P((struct cf_list *));
%}

%token INTERFACE IFNAME IPV6ADDR
%token REQUEST SEND 
%token RAPID_COMMIT PREFIX_DELEGATION DNS_SERVERS SIP_SERVERS NTP_SERVERS
%token INFO_ONLY TEMP_ADDR SOLICIT_ONLY
%token IANA_ONLY IAPD_ONLY DOMAIN_LIST
%token ADDRESS PREFIX IAID RENEW_TIME REBIND_TIME V_TIME P_TIME PREFIX_DELEGATION_INTERFACE USER_CLASS
%token XID_SOL XID_REQ DUID_TIME
%token NUMBER SLASH EOS BCL ECL STRING INFINITY
%token COMMA OPTION

%union {
	long long num;
	char* str;
	struct cf_list *list;
	struct in6_addr addr;
	struct dhcp6_addr *v6addr;
}

%type <str> IFNAME STRING
%type <num> NUMBER duration addrvtime addrptime
%type <list> declaration declarations dhcpoption 
%type <v6addr> addrparam addrdecl
%type <addr> IPV6ADDR

%%
statements:
		/* empty */
	|	statements statement
	;

statement:
		interface_statement
	;

interface_statement:
	INTERFACE IFNAME BCL declarations ECL EOS 
	{
		struct cf_namelist *ifl;

		MAKE_NAMELIST(ifl, $2, $4);

		if (add_namelist(ifl, &iflist_head))
			return (-1);
	}
	;

declarations:
		{ $$ = NULL; }
	| 	declarations declaration 
		{
			struct cf_list *head;

			if ((head = $1) == NULL) {
				$2->next = NULL;
				$2->tail = $2;
				head = $2;
			} else {
				head->tail->next = $2;
				head->tail = $2;
			}

			$$ = head;
		}
	;

declaration:
		SEND dhcpoption EOS
		{
			struct cf_list *l;

			MAKE_CFLIST(l, DECL_SEND, NULL, $2);

			$$ = l;
		}
	|	REQUEST dhcpoption EOS
		{
			struct cf_list *l;

			MAKE_CFLIST(l, DECL_REQUEST, NULL, $2);

			$$ = l;
		}
	|	INFO_ONLY EOS
		{
			struct cf_list *l;

			MAKE_CFLIST(l, DECL_INFO_ONLY, NULL, NULL);
			/* no value */
			$$ = l;
		}
	|	SOLICIT_ONLY EOS
		{
			struct cf_list *l;

			MAKE_CFLIST(l, DECL_SOLICIT_ONLY, NULL, NULL);
			/* no value */
			$$ = l;
		}
	|	IANA_ONLY EOS
		{
			struct cf_list *l;

			MAKE_CFLIST(l, DECL_IANA_ONLY, NULL, NULL);
			/* no value */
			$$ = l;
		}
	|	IAPD_ONLY EOS
		{
			struct cf_list *l;

			MAKE_CFLIST(l, DECL_IAPD_ONLY, NULL, NULL);
			/* no value */
			$$ = l;
		}

	|	REQUEST TEMP_ADDR EOS
		{
			struct cf_list *l;

			MAKE_CFLIST(l, DECL_TEMP_ADDR, NULL, NULL);
			/* no value */
			$$ = l;
		}
	|	ADDRESS BCL addrdecl ECL EOS
		{
			struct cf_list *l;

			MAKE_CFLIST(l, DECL_ADDRESS, $3, NULL);

			$$ = l;

		}
	
	|	PREFIX BCL addrdecl ECL EOS
		{
			struct cf_list *l;

			MAKE_CFLIST(l, DECL_PREFIX, $3, NULL);

			$$ = l;

		}
	
	|	RENEW_TIME duration EOS
		{
			struct cf_list *l;

			MAKE_CFLIST(l, DECL_RENEWTIME, NULL, NULL);
			l->num = $2;

			$$ = l;

		}
	|	REBIND_TIME duration EOS
		{
			struct cf_list *l;

			MAKE_CFLIST(l, DECL_REBINDTIME, NULL, NULL);
			l->num = $2;

			$$ = l;

		}
	|	IAID NUMBER EOS 
		{	
			struct cf_list *l;

			MAKE_CFLIST(l, DECL_IAID, NULL, NULL);
			l->num = $2;
		
			$$ = l;
		}
        |       PREFIX_DELEGATION_INTERFACE STRING EOS
                {
			struct cf_list *l;
			char *pp = (char*)malloc(strlen($2)+1);
			strcpy(pp,$2);
			MAKE_CFLIST(l, DECL_PREFIX_DELEGATION_INTERFACE, pp, NULL );
			
			$$ = l;
                }
	|	USER_CLASS STRING EOS
		{
			struct cf_list *l;
			char *pp = (char*)malloc(strlen($2)+1);
			strcpy(pp,$2);
			MAKE_CFLIST(l, DECL_USER_CLASS, pp, NULL );
			
			$$ = l;
		}
	|	XID_SOL NUMBER EOS 
		{	
			struct cf_list *l;

			MAKE_CFLIST(l, DECL_XID_SOL, NULL, NULL);
			l->num = $2;
		
			$$ = l;
		}
	|	XID_REQ NUMBER EOS 
		{	
			struct cf_list *l;

			MAKE_CFLIST(l, DECL_XID_REQ, NULL, NULL);
			l->num = $2;
		
			$$ = l;
		}
	|	DUID_TIME NUMBER EOS 
		{	
			struct cf_list *l;

			MAKE_CFLIST(l, DECL_DUID_TIME, NULL, NULL);
			l->num = $2;
		
			$$ = l;
		}
	;

dhcpoption:
		RAPID_COMMIT
		{
			struct cf_list *l;

			MAKE_CFLIST(l, DHCPOPT_RAPID_COMMIT, NULL, NULL);
			/* no value */
			$$ = l;
		}
	|	PREFIX_DELEGATION	
		{
			struct cf_list *l;

			MAKE_CFLIST(l, DHCPOPT_PREFIX_DELEGATION, NULL, NULL);
			/* currently no value */
			$$ = l;
		}
	|	DNS_SERVERS	
		{
			struct cf_list *l;

			MAKE_CFLIST(l, DHCPOPT_DNS, NULL, NULL);
			/* currently no value */
			$$ = l;
		}
	|	DOMAIN_LIST
		{
			struct cf_list *l;

			MAKE_CFLIST(l, DHCPOPT_DOMAIN_LIST, NULL, NULL);
			/* currently no value */
			$$ = l;
		}
	|	SIP_SERVERS	
		{
			struct cf_list *l;

			MAKE_CFLIST(l, DHCPOPT_SIP, NULL, NULL);
			/* currently no value */
			$$ = l;
		}
	|	NTP_SERVERS	
		{
			struct cf_list *l;

			MAKE_CFLIST(l, DHCPOPT_NTP, NULL, NULL);
			/* currently no value */
			$$ = l;
		}
	;

addrdecl:
       
		addrparam addrvtime 
		{
		    struct dhcp6_addr *addr=(struct dhcp6_addr *)$1;
		    
			addr->validlifetime = (u_int32_t)$2;
			$$ = $1;
		}
	|	 addrparam addrptime
		{
		    struct dhcp6_addr *addr=(struct dhcp6_addr *)$1;
		        addr->preferlifetime = (u_int32_t)$2;
			$$ = $1;
		}
	|	 addrparam addrvtime addrptime 
		{
		    struct dhcp6_addr *addr=(struct dhcp6_addr *)$1;
			addr->validlifetime = (u_int32_t)$2;
			addr->preferlifetime = (u_int32_t)$3;
			$$ = $1;
		}
	| 	addrparam addrptime addrvtime
		{
		    struct dhcp6_addr *addr=(struct dhcp6_addr *)$1;
			addr->validlifetime = (u_int32_t)$3;
			addr->preferlifetime = (u_int32_t)$2;
			$$ = $1;
			}
	| 	addrparam
		{
			$$ = $1;
		}
	;
addrparam:
		IPV6ADDR SLASH NUMBER EOS
		{
			struct dhcp6_addr *v6addr;		
			/* validate other parameters later */
			if ($3 < 0 || $3 > 128)
				return (-1);
			if ((v6addr = malloc(sizeof(*v6addr))) == NULL) {
				cpyywarn("can't allocate memory");
				return (-1);
			}
			memset(v6addr, 0, sizeof(*v6addr));
			memcpy(&v6addr->addr, &$1, sizeof(v6addr->addr));
			v6addr->plen = $3;
			$$ = v6addr;
		}
	;

addrvtime:
		V_TIME duration EOS
		{
			$$ = $2;
		}
	;

addrptime:
		P_TIME duration EOS
		{
			$$ = $2;
		}
	;

duration:
		INFINITY
		{
			$$ = -1;
		}
	|	NUMBER
		{
			$$ = $1;
		}
	;

%%
/* supplement routines for configuration */
static int
add_namelist(new, headp)
	struct cf_namelist *new, **headp;
{
	struct cf_namelist *ifp;

	/* check for duplicated configuration */
	for (ifp = *headp; ifp; ifp = ifp->next) {
		if (strcmp(ifp->name, new->name) == 0) {
			cpyywarn("duplicated interface: %s (ignored)",
			       new->name);
			cleanup_namelist(new);
			return (0);
		}
	}

	new->next = *headp;
	*headp = new;

	return (0);
}

/* free temporary resources */
static void
cleanup()
{
	cleanup_namelist(iflist_head);

}

static void
cleanup_namelist(head)
	struct cf_namelist *head;
{
	struct cf_namelist *ifp, *ifp_next;

	for (ifp = head; ifp; ifp = ifp_next) {
		ifp_next = ifp->next;
		cleanup_cflist(ifp->params);
		free(ifp->name);
		free(ifp);
	}
}

static void
cleanup_cflist(p)
	struct cf_list *p;
{
	struct cf_list *n;

	if (p == NULL)
		return;

	n = p->next;
	if (p->ptr)
		free(p->ptr);
	if (p->list)
		cleanup_cflist(p->list);
	free(p);

	cleanup_cflist(n);
}

#define config_fail() \
	do { cleanup(); configure_cleanup(); return (-1); } while(0)

int
cf_post_config()
{
	if (configure_interface(iflist_head))
		config_fail();

	if (configure_global_option())
		config_fail();

	configure_commit();
	cleanup();
	return (0);
}
#undef config_fail

void
cf_init()
{
	iflist_head = NULL;
}

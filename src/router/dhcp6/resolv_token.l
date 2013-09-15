/*	$Id: resolv_token.l,v 1.1.1.1 2006/12/04 00:45:33 Exp $	*/

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
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <errno.h>
#include <syslog.h>
#include <string.h>

#include "queue.h"
#include "dhcp6.h"
#include "config.h"
#include "common.h"

#undef yywrap

#define YYABORT(msg) dprintf(LOG_ERR, msg " %s lineno %d.", \
	yytext, num_lines)

#define ABORT   do {    \
	YYABORT("resolv parse error");   \
	remove(RESOLV_CONF_DHCPV6_FILE); \
	exit(1);        \
} while (0)


extern struct dhcp6_if *dhcp6_if;
static int num_lines = 0;
static struct dns_list *new_dns_list;
static FILE *dhcp6_resolv_file;
static int oldfd, newfd;

static int yywrap __P(());
int resolv_parse __P((struct dns_list *));
static int update_resolver __P((struct dns_list *));

%}

digit           [0-9]
number          ({digit})+
hexdigit        ([a-f]|[A-F]|[0-9])
ipv4addr        ({digit}{1,3}"."{digit}{1,3}"."{digit}{1,3}"."{digit}{1,3})
addr_head       ("::"|{hexdigit}{1,4}(":"|"::"))
addr_tail       ({hexdigit}{1,4}|({hexdigit}{1,4}"::")|{ipv4addr})?
addr_body       ({hexdigit}{1,4}(":"|"::"))*
ipv6addr        {addr_head}{addr_body}{addr_tail}
whitespace      ([ \t])+
domainname	[a-zA-Z0-9:\._-]+
nl              \n


%s S_ADDR S_DOMAIN

%%

{nl}            {fprintf(dhcp6_resolv_file, yytext);
		num_lines++;}
{whitespace}    {fprintf(dhcp6_resolv_file, yytext);}

"nameserver"	{fprintf(dhcp6_resolv_file, yytext); 
		BEGIN S_ADDR;}
"search"	{fprintf(dhcp6_resolv_file, yytext); 
		BEGIN S_DOMAIN;}

<S_DOMAIN>{domainname} { struct domain_list *domainname, *temp;
		domainname = (struct domain_list *)malloc(sizeof(*domainname));
		if (domainname == NULL)
			ABORT;
		if (strlen(yytext) > MAXDNAME)
			ABORT;
		fprintf(dhcp6_resolv_file, yytext);
		strcpy(domainname->name, yytext);
		dprintf(LOG_DEBUG, "parse domain name %s", domainname->name);
		domainname->next = NULL;
		if (dhcp6_if->dnslist.domainlist == NULL)
			dhcp6_if->dnslist.domainlist = domainname;
		else {
			for (temp = dhcp6_if->dnslist.domainlist; temp; temp = temp->next) {
				if (temp->next == NULL) {
					temp->next = domainname;
					break;
				}
			}
		}
		BEGIN S_DOMAIN;}

<S_DOMAIN>. {fprintf(dhcp6_resolv_file, yytext); BEGIN INITIAL;}

<S_ADDR>{ipv6addr} {struct in6_addr addr;
		fprintf(dhcp6_resolv_file, yytext);
		if (inet_pton(AF_INET6, yytext, &addr) < 1) 
			ABORT;
		dprintf(LOG_DEBUG, "parse name server %s", in6addr2str(&addr, 0));
		if (dhcp6_add_listval(&dhcp6_if->dnslist.addrlist,
		    &addr, DHCP6_LISTVAL_ADDR6) == NULL) {
			dprintf(LOG_ERR, "%s" "failed to add a DNS server", FNAME);
			ABORT;
		}
		BEGIN S_ADDR;}
<S_ADDR>. {fprintf(dhcp6_resolv_file, yytext); BEGIN INITIAL;}
.	{fprintf(dhcp6_resolv_file, yytext);}

%%

/* parse resolv.conf
 * create a new resolv.conf.dhcpv6
 * mv /etc/resolv.conf to /etc/resolv.conf.V6BAK
 * link resolv.conf to resolv.conf.dhcpv6
 */
int
resolv_parse(struct dns_list *dnslist)
{
	struct stat buf;
	char pidstr[128];
	sprintf(pidstr, "%d", getpid());
	strcpy(resolv_dhcpv6_file, RESOLV_CONF_DHCPV6_FILE);
	strcat(resolv_dhcpv6_file, pidstr);
	TAILQ_INIT(&dhcp6_if->dnslist.addrlist);
	dhcp6_if->dnslist.domainlist = NULL;
	new_dns_list = dnslist;
	while ((oldfd = open(RESOLV_CONF_FILE, O_EXCL)) < 0) {
		switch (errno) {
		case ENOENT:
			/* exclusive open of new resolv.conf.dhcpv6 file */
			if ((newfd = open(resolv_dhcpv6_file, 
			    O_CREAT|O_EXCL|O_WRONLY|O_TRUNC, 0644))
			    < 0) {
				dprintf(LOG_ERR, "%s" 
					" failed to open resolv.conf.dhcpv6 file", FNAME);
				return (-1);
			}
			if ((dhcp6_resolv_file = fdopen(newfd, "w")) == NULL) {
				dprintf(LOG_ERR, "%s" 
					" failed to fdopen resolv.conf.dhcpv6 file", FNAME);
				return (-1);
			}
			return(update_resolver(dnslist));
		case EACCES: 
			sleep(1);
			continue;
		default:
			dprintf(LOG_ERR, "resolv_parse: fopen(%s): %s",
				RESOLV_CONF_FILE, strerror(errno));
			return (-1);
		}
	}
	if (lstat(RESOLV_CONF_FILE, &buf)) {
		dprintf(LOG_ERR, "lstat resolv.conf file failed");	
		return (-1);
	}
	if ((yyin = fdopen(oldfd, "r")) == NULL) {
		dprintf(LOG_ERR, "resolv_parse: fdopen(%s): %s",
			RESOLV_CONF_FILE, strerror(errno));
		return (-1);
	}
	/* exclusive open of new resolv.conf.dhcpv6 file */
	if ((newfd = open(resolv_dhcpv6_file, 
	    O_CREAT|O_EXCL|O_WRONLY|O_TRUNC, buf.st_mode)) < 0) {
		dprintf(LOG_ERR, "%s" 
			" failed to open resolv.conf.dhcpv6 file", FNAME);
		return (-1);
	}
	if ((dhcp6_resolv_file = fdopen(newfd, "w")) == NULL) {
		dprintf(LOG_ERR, "%s" 
			" failed to fdopen resolv.conf.dhcpv6 file", FNAME);
		return (-1);
	}
	yylex();
	return 0;
}

static int
update_resolver(struct dns_list *dns_list)
{
	struct domain_list *oldlist, *dprev, *dlist, *dlist_next;
	struct dhcp6_listval *d, *d_next;
	struct stat buf;
	int i = 0;
	fseek(dhcp6_resolv_file, 0, SEEK_CUR);
	if (!TAILQ_EMPTY(&dns_list->addrlist)) {
		for (d = TAILQ_FIRST(&dns_list->addrlist); d; d = d_next, i++) {
			struct dhcp6_listval *lv;
			dprintf(LOG_DEBUG, "%s" " received nameserver[%d] %s",
				FNAME, i, in6addr2str(&d->val_addr6, 0));
			d_next = TAILQ_NEXT(d, link);
			for (lv = TAILQ_FIRST(&dhcp6_if->dnslist.addrlist); lv;
			     lv = TAILQ_NEXT(lv, link)) {
				if (IN6_ARE_ADDR_EQUAL(&lv->val_addr6, &d->val_addr6)) { 
					dprintf(LOG_DEBUG, "%s" 
						"nameserver %s found in resolv.conf",
						FNAME, in6addr2str(&d->val_addr6, 0));
					TAILQ_REMOVE(&dns_list->addrlist, d, link);
					continue;
				}
			}
		}
		if (!TAILQ_EMPTY(&dns_list->addrlist)) {
			fprintf(dhcp6_resolv_file, "nameserver ");
			for (d = TAILQ_FIRST(&dns_list->addrlist); d;
				d = d_next, i++) {
				d_next = TAILQ_NEXT(d, link);
				fprintf(dhcp6_resolv_file, "%s", 
					in6addr2str(&d->val_addr6, 0));
				if (d_next != NULL)
					fprintf(dhcp6_resolv_file, " ");
				else
					fprintf(dhcp6_resolv_file, "\n");
			}
		}
	}
			
	if (dns_list->domainlist != NULL) {
		i = 0;
		for (dlist = dns_list->domainlist, dprev = dns_list->domainlist;
		     dlist; i++, dlist = dlist_next) {
			int found = 0;
			dprintf(LOG_DEBUG, "%s" " received domainname[%d] %s",
				FNAME, i, dlist->name);
			if (dhcp6_if->dnslist.domainlist == NULL) 
				break;
			for (oldlist = dhcp6_if->dnslist.domainlist; oldlist; 
			     oldlist = oldlist->next) {
				if (strcmp(oldlist->name, dlist->name) == 0) {
					found = 1;
					dprintf(LOG_DEBUG, "%s" "domain name %s found in " 
						"resolv.conf", FNAME, dlist->name);
					if (dprev == dlist) {
						dprev = dlist->next;
						dns_list->domainlist = dlist->next;
					} else
						dprev->next = dlist->next;
					break;
				}
			}
			dlist_next = dlist->next;
			if (found == 0) 
				dprev = dlist;
			else {
				free(dlist);
			}
		}
		if (dns_list->domainlist != NULL) {
			fprintf(dhcp6_resolv_file, "search ");
			for (dlist = dns_list->domainlist; dlist; dlist = dlist->next) {
				dprintf(LOG_DEBUG, "%s" "domain name %s added in resolv.conf", 
					FNAME, dlist->name);
				fprintf(dhcp6_resolv_file, "%s", dlist->name);
				if (dlist->next != NULL)
					fprintf(dhcp6_resolv_file, " ");
				else
					fprintf(dhcp6_resolv_file, "\n");
			}
		}
	}
	if (fflush(dhcp6_resolv_file) == EOF) {
		dprintf(LOG_ERR, "%s" "write resolv.conf.dhcpv6 file fflush failed %s", 
			FNAME, strerror(errno));
		return (-1);
	}
	if (fsync(newfd) < 0) {
		dprintf(LOG_ERR, "%s" "write resolv.conf.dhcpv6 file failed %s", 
			FNAME, strerror(errno));
		return (-1);
	}
	fclose(dhcp6_resolv_file);
	close(newfd);
	
	newfd=0;
	if (!lstat(RESOLV_CONF_FILE, &buf)) {
		if ((lstat(RESOLV_CONF_BAK_FILE, &buf) < 0) && (errno == ENOENT)) {
			if (link(RESOLV_CONF_FILE, RESOLV_CONF_BAK_FILE)) {
				dprintf(LOG_ERR, "%s" 
					" backup failed for resolv.conf file", FNAME);
				return (-1);
			}
		}
	}
	if (rename(resolv_dhcpv6_file, RESOLV_CONF_FILE)) {
		dprintf(LOG_ERR, "%s" " rename failed for resolv.conf file", FNAME);
		return(-1);
	}

    /*  added start pling 09/30/2009 */
    /* Signal dnsmasq to reload resolv.conf */
    system("killall -SIGHUP dnsmasq");
    /*  added end pling 09/30/2009 */

	return (0);
}

static int 
yywrap()
{
	update_resolver(new_dns_list);
	fclose(yyin);
	close(oldfd);
	return 1;
}

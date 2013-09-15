/*	$Id: radvd_token.l,v 1.1.1.1 2006/12/04 00:45:31 Exp $	*/

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

/* Author: Shirley Ma, xma@us.ibm.com */

%option noyywrap

%{
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <errno.h>
#include <syslog.h>
#include <string.h>

#include "queue.h"
#include "dhcp6.h"
#include "config.h"
#include "common.h"
#include "lease.h"

#ifndef PATH_RADVD_PID
#define	PATH_RADVD_PID "/var/run/radvd/radvd.pid"
#endif

#define YYABORT(msg) dprintf(LOG_ERR, msg " %s lineno %d.", \
	yytext, num_lines)

#define ABORT   do {    \
	YYABORT("radvd.conf parse error");   \
	remove(RADVD_CONF_DHCPV6_FILE); \
	exit(1);        \
} while (0)

#undef yywrap

#define DHCP6_MARK "# update prefix from DHCPv6 server\n"

extern struct dhcp6_if *dhcp6_if;
int radvd_parse __P((struct dhcp6_iaidaddr *, int));

static struct dhcp6_list ori_prefix_list;
static struct dhcp6_iaidaddr *client6iaid;
static struct dhcp6_list previous_prefix_list;
static FILE *dhcp6_radvd_file;
static int update_flag;
static int oldfd, newfd;
static int num_lines = 0;
static int brace = 0;
static int update = 0;

static int yywrap __P((void));
static int update_radvd __P((void));

%}

whitespace      ([ \t])+
digit		[0-9]
string		[a-zA-Z0-9:\.\/][a-zA-Z0-9:\.\/]*
nl              \n
lbrace		"{"
rbrace		"}"
mark		DHCP6_MARK		
comment		\#.*

%s S_INTERFACE S_IFNAME S_ORIPREFIX S_NEWPREFIX S_DHCP6

%%

{nl}            {fprintf(dhcp6_radvd_file, yytext);
		num_lines++;}

{whitespace}    {fprintf(dhcp6_radvd_file, yytext);}


"interface"	{fprintf(dhcp6_radvd_file, yytext); 
		BEGIN S_INTERFACE;}

<S_INTERFACE>{string} {
		char *requested_if;
		fprintf(dhcp6_radvd_file, yytext);
		if ( ( ( (requested_if = get_if_option(&(dhcp6_if->option_list), 
							DECL_PREFIX_DELEGATION_INTERFACE
						       )
			   ) != 0L
			 )
		       &&( strcmp(yytext, requested_if) == 0)
		     )		     
                   ||((requested_if == 0L) && (strcmp(yytext, dhcp6_if->ifname)==0))
		   )
		{
			TAILQ_INIT(&ori_prefix_list);
			TAILQ_INIT(&previous_prefix_list);
			BEGIN S_IFNAME;
		}else 
		        BEGIN INITIAL;
}

<S_IFNAME>{comment} {
		if (strstr(yytext, DHCP6_MARK) != NULL) {
			BEGIN S_DHCP6;
		} else
			fprintf(dhcp6_radvd_file, yytext);}	

<S_IFNAME>"prefix" {fprintf(dhcp6_radvd_file, yytext);
		BEGIN S_ORIPREFIX;}

<S_IFNAME>{lbrace} {fprintf(dhcp6_radvd_file, yytext); 
		brace++;
		BEGIN S_IFNAME;}	

<S_IFNAME>{rbrace} {struct dhcp6_lease *lv;
		brace--;
		if (brace == 0 && update_flag == ADDR_UPDATE) {
			/* add prefix which is not on the original prefix list 
			 * check the lifetime value, AdvValidLifetime,
			 * AdvPreferredLifetime: infinity or XXX seconds */
			for (lv = TAILQ_FIRST(&client6iaid->lease_list); lv;
			     lv = TAILQ_NEXT(lv, link)) {
				if (!addr_on_addrlist(&ori_prefix_list, &lv->lease_addr) &&
				    !addr_on_addrlist(&previous_prefix_list, 
				    &lv->lease_addr)){
					fprintf(dhcp6_radvd_file, DHCP6_MARK);
					fprintf(dhcp6_radvd_file, "\tprefix %s/%d {};\n",
						in6addr2str(&(lv->lease_addr.addr), 0), 
						lv->lease_addr.plen);
					update = 1;
				}
				dprintf(LOG_INFO, "update radvd.conf with DHCPv6 prefixes");
			}
			fprintf(dhcp6_radvd_file, yytext);
			BEGIN INITIAL;
		} else {
			fprintf(dhcp6_radvd_file, yytext);
			BEGIN S_IFNAME;}
		}

<S_DHCP6>"prefix" {BEGIN S_NEWPREFIX;}
<S_NEWPREFIX>{string} {
		/* update the prefix from DHCPv6 server
		 * if the new list doesn't include this prefix remove it */
		struct dhcp6_listval *lv;
		/* create orignal prefix list */
		if ((lv = (struct dhcp6_listval *)malloc(sizeof(*lv))) == NULL) {
			ABORT;
		}
		memset(lv, 0, sizeof(*lv));
		if (strstr(yytext, "/") == NULL)
			ABORT;
		if (inet_pton(AF_INET6, strtok(yytext, "/"), &lv->val_dhcp6addr.addr) < 1) {
			ABORT;
		}
		lv->val_dhcp6addr.type = IAPD;
		lv->val_dhcp6addr.plen = atoi(strtok(NULL, "/"));
		lv->val_dhcp6addr.status_code = DH6OPT_STCODE_UNDEFINE;
		/* remove the prefix list for release */
		if (update_flag == ADDR_REMOVE && 
		    !addr_on_addrlist((struct dhcp6_list *)&client6iaid->lease_list, 
		    &lv->val_dhcp6addr)) { 
			dprintf(LOG_DEBUG, "remove DHCPv6 assgined prefix in radvd.conf");
			update = 1;
		} else {
			TAILQ_INSERT_TAIL(&previous_prefix_list, lv, link);
			fprintf(dhcp6_radvd_file, DHCP6_MARK);
			fprintf(dhcp6_radvd_file, "\tprefix %s/%d {};\n",
				in6addr2str(&(lv->val_dhcp6addr.addr), 0), 
				lv->val_dhcp6addr.plen);
		}
		BEGIN S_NEWPREFIX;}
<S_NEWPREFIX>{rbrace}";" {BEGIN S_IFNAME;}
<S_NEWPREFIX>.	{;}

<S_ORIPREFIX>{string} {
		struct dhcp6_listval *lv;
		fprintf(dhcp6_radvd_file, yytext);
		/* create orignal prefix list */
		if ((lv = (struct dhcp6_listval *)malloc(sizeof(*lv))) == NULL) {
			ABORT;
		}
		memset(lv, 0, sizeof(*lv));
		if (inet_pton(AF_INET6, strtok(yytext, "/"), &lv->val_dhcp6addr.addr) < 1) {
			ABORT;
		}
		lv->val_dhcp6addr.type = IAPD;
		lv->val_dhcp6addr.plen = atoi(strtok(NULL, "/"));
		lv->val_dhcp6addr.status_code = DH6OPT_STCODE_UNDEFINE;
		TAILQ_INSERT_TAIL(&ori_prefix_list, lv, link);
		BEGIN S_IFNAME;
		}

{comment}	{fprintf(dhcp6_radvd_file, yytext);}
.		{fprintf(dhcp6_radvd_file, yytext);}

%%

/* parse radvd.conf
 * create a new radvd.conf.dhcpv6
 * mv /etc/radvd.conf to /etc/radvd.conf.V6BAK
 * link radvd.conf to radvd.conf.dhcpv6
 */
int
radvd_parse(struct dhcp6_iaidaddr *client6_iaid, int flag)
{
	struct stat buf;
	char pidstr[128];
	sprintf(pidstr, "%d", getpid());
	strcpy(radvd_dhcpv6_file, RADVD_CONF_DHCPV6_FILE);
	strcat(radvd_dhcpv6_file, pidstr);
	while ((oldfd = open(RADVD_CONF_FILE, O_EXCL)) < 0) {
		switch (errno) {
		case ENOENT:
			/* exclusive open of new radvd.conf.dhcpv6 file */
			if ((newfd = open(radvd_dhcpv6_file, 
			    O_CREAT|O_EXCL|O_WRONLY|O_TRUNC, 0644))
			    < 0) {
				dprintf(LOG_ERR, "%s" 
					" failed to open radvd.conf.dhcpv6 file", FNAME);
				return (-1);
			}
			if ((dhcp6_radvd_file = fdopen(newfd, "w")) == NULL) {
				dprintf(LOG_ERR, "%s" 
					" failed to fdopen radvd.conf.dhcpv6 file", FNAME);
				return (-1);
			}
			return(update_radvd());
		case EACCES: 
			sleep(1);
			continue;
		default:
			dprintf(LOG_ERR, "radvd_parse: fopen(%s): %s",
				RADVD_CONF_FILE, strerror(errno));
			return (-1);
		}
	}
	if (lstat(RADVD_CONF_FILE, &buf)) {
		dprintf(LOG_ERR, "lstat radvd.conf file failed");	
		return (-1);
	}
	if ((yyin = fdopen(oldfd, "r")) == NULL) {
		dprintf(LOG_ERR, "radvd_parse: fdopen(%s): %s",
			RADVD_CONF_FILE, strerror(errno));
		return (-1);
	}
	/* exclusive open of new radvd.conf.dhcpv6$pid file */
	if ((newfd = open(radvd_dhcpv6_file, 
	    O_CREAT|O_EXCL|O_WRONLY|O_TRUNC, buf.st_mode)) < 0) {
		dprintf(LOG_ERR, "%s" 
			" failed to open radvd.conf.dhcpv6 file", FNAME);
		return (-1);
	}
	if ((dhcp6_radvd_file = fdopen(newfd, "w")) == NULL) {
		dprintf(LOG_ERR, "%s" 
			" failed to fdopen radvd.conf.dhcpv6$pid file", FNAME);
		return (-1);
	}
	client6iaid = client6_iaid;
	update_flag = flag;
	return yylex();
}

static int
update_radvd(void)
{
	struct stat buf;
	if (fflush(dhcp6_radvd_file) == EOF) {
		dprintf(LOG_ERR, "%s" "write radvd.conf.dhcpv6 file fflush failed %s", 
			FNAME, strerror(errno));
		return (-1);
	}
	if (fsync(newfd) < 0) {
		dprintf(LOG_ERR, "%s" "write radvd.conf.dhcpv6 file failed %s", 
			FNAME, strerror(errno));
		return (-1);
	}
	fclose(dhcp6_radvd_file);
	close(newfd);
	if (!lstat(RADVD_CONF_FILE, &buf)) {
		if (lstat(RADVD_CONF_BAK_FILE, &buf) < 0 && errno == ENOENT) {
			/* save the orignal file as a bak file */
			if (link(RADVD_CONF_FILE, RADVD_CONF_BAK_FILE)) {
				dprintf(LOG_ERR, "%s" 
					" backup failed for radvd.conf file", FNAME);
				return (-1);
			}
		}
	}
	/* rename the new conf file as the radvd.conf file */
	if (rename(radvd_dhcpv6_file, RADVD_CONF_FILE)) {
		dprintf(LOG_ERR, "%s" " rename failed for radvd.conf file", FNAME);
		return(-1);
	}
	return (0);
}

static int 
yywrap(void)
{
	int fd;
	char buff[8];
	pid_t radvd_pid;
	(void)update_radvd();
	if (update) {
		if ((fd = open(PATH_RADVD_PID, O_RDONLY)) > 0) {
			if (read(fd, buff, sizeof(buff)) > 0) {
				radvd_pid = strtol(buff, NULL, 10);
				if (radvd_pid == 0)
					dprintf(LOG_INFO, "invalid radvd pid");
				else if (kill(radvd_pid, SIGHUP) != 0)
					dprintf(LOG_INFO, "failed to reload radvd");
			} else
				dprintf(LOG_INFO, "failed to read radvd pid file");
			close(fd);
		} else
			dprintf(LOG_INFO, "failed to open radvd pid file");
	}
	fclose(yyin);
	close(oldfd);
	return 1;
}

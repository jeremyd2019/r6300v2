/*
 * Broadcom UPnP module linux netfilter configuration
 *
 * Copyright (C) 2010, Broadcom Corporation. All Rights Reserved.
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * $Id: upnpnat.c,v 1.5 2009/11/10 06:38:36 Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <netconf.h>

/*
 * UPnP portmapping --
 * Add port forward and a matching ACCEPT rule to the FORWARD table
 */
static void
add_nat_entry(netconf_nat_t *entry, int log_level)
{
	int dir = NETCONF_FORWARD;
	int target = (log_level & 2) ? NETCONF_LOG_ACCEPT : NETCONF_ACCEPT;
	netconf_filter_t filter;
	struct in_addr netmask = { 0xffffffff };
	netconf_nat_t nat = *entry;

	/* Set up LAN side match */
	memset(&filter, 0, sizeof(filter));
	filter.match.ipproto = nat.match.ipproto;
	filter.match.src.ports[1] = nat.match.src.ports[1];
	filter.match.dst.ipaddr.s_addr = nat.ipaddr.s_addr;
	filter.match.dst.netmask.s_addr = netmask.s_addr;
	filter.match.dst.ports[0] = nat.ports[0];
	filter.match.dst.ports[1] = nat.ports[1];
	strncpy(filter.match.in.name, nat.match.in.name, IFNAMSIZ);

	/* Accept connection */
	filter.target = target;
	filter.dir = dir;

	/* Do it */
	netconf_add_nat(&nat);
	netconf_add_filter(&filter);
}

/* Combination PREROUTING DNAT and FORWARD ACCEPT */
static void
delete_nat_entry(netconf_nat_t *entry, int log_level)
{
	int dir = NETCONF_FORWARD;
	int target = (log_level & 2) ? NETCONF_LOG_ACCEPT : NETCONF_ACCEPT;
	netconf_filter_t filter;
	struct in_addr netmask = { 0xffffffff };
	netconf_nat_t nat = *entry;

	/* Set up LAN side match */
	memset(&filter, 0, sizeof(filter));
	filter.match.ipproto = nat.match.ipproto;
	filter.match.src.ports[1] = nat.match.src.ports[1];
	filter.match.dst.ipaddr.s_addr = nat.ipaddr.s_addr;
	filter.match.dst.netmask.s_addr = netmask.s_addr;
	filter.match.dst.ports[0] = nat.ports[0];
	filter.match.dst.ports[1] = nat.ports[1];
	strncpy(filter.match.in.name, nat.match.in.name, IFNAMSIZ);

	/* Accept connection */
	filter.target = target;
	filter.dir = dir;

	/* Do it */
	errno = netconf_del_nat(&nat);
	if (errno)
		fprintf(stderr, "netconf_del_nat returned error %d\n", errno);

	errno = netconf_del_filter(&filter);
	if (errno)
		fprintf(stderr, "netconf_del_filter returned error %d\n", errno);
}

void
usage(void)
{
	fprintf(stderr,
	        "usage: upnpnat [-i ifname] [-remote value] "
		"[-eport value] [-proto value] [-eport value] "
		"[-client value] [-en vlaue] [-loglevel value]");
	exit(0);
}


/* Command to add or delete from NAT engine */
int
main(int argc, char *argv[])
{
	netconf_nat_t nat, *entry = &nat;
	char ifname[IFNAMSIZ] = {0};
	char Proto[8] = {0};
	char IntIP[sizeof("255.255.255.255")] = {0};
	char rthost[sizeof("255.255.255.255")] = {0};
	unsigned short IntPort = 0;
	unsigned short ExtPort = 0;
	int enable = 0;
	int log_level = 0;

	/* Skip program name */
	--argc;
	++argv;

	if (!*argv)
		usage();

	for (; *argv; ++argv) {
		if (!strcmp(*argv, "-i")) {
			if (*++argv) {
				strncpy(ifname, *argv, sizeof(ifname));
			}
		}
		else if (!strcmp(*argv, "-remote")) {
			if (*++argv) {
				strncpy(rthost, *argv, sizeof(rthost));
				rthost[sizeof(rthost)-1] = 0;
			}
		}
		else if (!strcmp(*argv, "-eport")) {
			if (*++argv) {
				ExtPort = atoi(*argv);
			}
		}
		else if (!strcmp(*argv, "-proto")) {
			if (*++argv) {
				strncpy(Proto, *argv, sizeof(Proto));
				Proto[sizeof(Proto)-1] = 0;
			}
		}
		else if (!strcmp(*argv, "-iport")) {
			if (*++argv) {
				IntPort = atoi(*argv);
			}
		}
		else if (!strcmp(*argv, "-client")) {
			if (*++argv) {
				strncpy(IntIP, *argv, sizeof(IntIP));
				IntIP[sizeof(IntIP)-1] = 0;
			}
		}
		else if (!strcmp(*argv, "-en")) {
			if (*++argv) {
				enable = atoi(*argv);
			}
		}
		else if (!strcmp(*argv, "-log_level")) {
			if (*++argv) {
				log_level = atoi(*argv);
			}
		}
		else {
			usage();
		}
	}

	memset(entry, 0, sizeof(netconf_nat_t));

	/* accept from any port */
	strncpy(entry->match.in.name, ifname, IFNAMSIZ);
	entry->match.src.ports[0] = 0;
	entry->match.src.ports[1] = htons(0xffff);
	entry->match.dst.ports[0] = htons(ExtPort);
	entry->match.dst.ports[1] = htons(ExtPort);

	if (strlen(rthost)) {
		inet_aton("255.255.255.255", &entry->match.dst.netmask);
		inet_aton(rthost, &entry->match.dst.ipaddr);
	}

	/* parse the specification of the internal NAT client. */
	entry->target = NETCONF_DNAT;

	if (IntPort != 0) {
		/* parse the internal ip address. */
		inet_aton(IntIP, (struct in_addr *)&entry->ipaddr);

		/* parse the internal port number */
		entry->ports[0] = htons(IntPort);
		entry->ports[1] = htons(IntPort);
	}

	if (strcasecmp(Proto, "TCP") == 0) {
		entry->match.ipproto = IPPROTO_TCP;
	}
	else if (strcasecmp(Proto, "UDP") == 0) {
		entry->match.ipproto = IPPROTO_UDP;
	}

	/* set to NAT kernel */
	if (enable) {
		entry->match.flags &= ~NETCONF_DISABLED;
		add_nat_entry(entry, log_level);
	}
	else {
		entry->match.flags |= NETCONF_DISABLED;
		delete_nat_entry(entry, log_level);
	}

	return 0;
}

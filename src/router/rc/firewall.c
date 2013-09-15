/*
 * Firewall services
 *
 * Copyright (C) 2012, Broadcom Corporation. All Rights Reserved.
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
 * $Id: firewall.c 360733 2012-10-04 07:54:28Z $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <bcmnvram.h>
#include <shutils.h>
#include <rc.h>
#include <netconf.h>
#include <nvparse.h>

/* Add filter to specified table */
static void
add_filter(netconf_filter_t *filter, int dir)
{
	filter->dir = dir;
	netconf_add_filter(filter);
}


/* Add port forward and a matching ACCEPT rule to the FORWARD table */
static void
add_forward(netconf_nat_t *nat, int dir, int target)
{
	netconf_filter_t filter;

	/* Set up LAN side match */
	memset(&filter, 0, sizeof(filter));
	filter.match.ipproto = nat->match.ipproto;
	filter.match.src.ports[1] = nat->match.src.ports[1];
	filter.match.dst.ipaddr.s_addr = nat->ipaddr.s_addr;
	filter.match.dst.netmask.s_addr = htonl(0xffffffff);
	filter.match.dst.ports[0] = nat->ports[0];
	filter.match.dst.ports[1] = nat->ports[1];
	strncpy(filter.match.in.name, nat->match.in.name, IFNAMSIZ);

	/* Accept connection */
	filter.target = target;
	filter.dir = dir;

	/* Do it */
	netconf_add_nat(nat);
	netconf_add_filter(&filter);
}


#if defined(LINUX_2_6_36) && defined(__CONFIG_SAMBA__)
static void
add_conntrack(void)
{
	char ifname[IFNAMSIZ];

	strncpy(ifname, nvram_safe_get("lan_ifname"), IFNAMSIZ);

	/* Add rules to disable conntrack on SMB ports to reduce CPU loading
	 * for SAMBA storage application
	 */
	eval("iptables", "-t", "raw", "-F");
	eval("iptables", "-t", "raw", "-A", "PREROUTING", "-i", ifname, "-p", "tcp",
		"--dport", "137:139", "-j", "NOTRACK");
	eval("iptables", "-t", "raw", "-A", "PREROUTING", "-i", ifname, "-p", "tcp",
		"--dport", "445", "-j", "NOTRACK");
	eval("iptables", "-t", "raw", "-A", "PREROUTING", "-i", ifname, "-p", "udp",
		"--dport", "137:139", "-j", "NOTRACK");
	eval("iptables", "-t", "raw", "-A", "PREROUTING", "-i", ifname, "-p", "udp",
		"--dport", "445", "-j", "NOTRACK");
	eval("iptables", "-t", "raw", "-A", "OUTPUT", "-o", ifname, "-p", "tcp",
		"--sport", "137:139", "-j", "NOTRACK");
	eval("iptables", "-t", "raw", "-A", "OUTPUT", "-o", ifname, "-p", "tcp",
		"--sport", "445", "-j", "NOTRACK");
	eval("iptables", "-t", "raw", "-A", "OUTPUT", "-o", ifname, "-p", "udp",
		"--sport", "137:139", "-j", "NOTRACK");
	eval("iptables", "-t", "raw", "-A", "OUTPUT", "-o", ifname, "-p", "udp",
		"--sport", "445", "-j", "NOTRACK");

	eval("iptables", "-t", "filter", "-I", "INPUT", "-i", ifname, "-p", "udp",
		"--dport", "137:139", "-j", "ACCEPT");
	eval("iptables", "-t", "filter", "-I", "INPUT", "-i", ifname, "-p", "udp",
		"--dport", "445", "-j", "ACCEPT");
	eval("iptables", "-t", "filter", "-I", "INPUT", "-i", ifname, "-p", "tcp",
		"--dport", "137:139", "-j", "ACCEPT");
	eval("iptables", "-t", "filter", "-I", "INPUT", "-i", ifname, "-p", "tcp",
		"--dport", "445", "-j", "ACCEPT");
}
#endif /* LINUX_2_6_36 && __CONFIG_SAMBA__ */

int
start_firewall(void)
{
#if 0
	DIR *dir;
	struct dirent *file;
	FILE *fp;
	char name[NAME_MAX];
	netconf_filter_t filter;
	netconf_app_t app;
	int i, index;
	char var[256], *next;
	int log_level, log_drop, log_accept;
	struct ipaddress_pairs {
				struct in_addr address;
				struct in_addr mask;
				} *ip_addrs = NULL;
	char lan_proto[20];
	char lan_ifname[20];
	char lan_ipaddr[20];
	char lan_netmask[20];
	char *address, *mask;
	struct ipaddress_pairs *cur_ipaddr;

	/* Reset firewall */
	netconf_reset_fw();

	/* Block obviously spoofed IP addresses */
	if (!(dir = opendir("/proc/sys/net/ipv4/conf")))
		perror("/proc/sys/net/ipv4/conf");
	while (dir && (file = readdir(dir))) {
		if (strncmp(file->d_name, ".", NAME_MAX) != 0 &&
		    strncmp(file->d_name, "..", NAME_MAX) != 0) {
			snprintf(name, sizeof(name),
				  "/proc/sys/net/ipv4/conf/%s/rp_filter", file->d_name);
			if (!(fp = fopen(name, "r+"))) {
				perror(name);
				break;
			}
			fputc('1', fp);
			fclose(fp);
		}
	}
	if (dir)
		closedir(dir);

	/* Optionally log connections */
	log_level = atoi(nvram_safe_get("log_level"));
	log_drop = (log_level & 1) ? NETCONF_LOG_DROP : NETCONF_DROP;
	log_accept = (log_level & 2) ? NETCONF_LOG_ACCEPT : NETCONF_ACCEPT;

	/*
	 * Inbound drops
	 */

	/* Drop invalid packets */
	memset(&filter, 0, sizeof(filter));
	filter.match.state = NETCONF_INVALID;
	filter.target = NETCONF_DROP;
	add_filter(&filter, NETCONF_IN);
	add_filter(&filter, NETCONF_FORWARD);

	/*
	 * Forward drops
	 */

	/* Client filters */
	for (i = 0; i < MAX_NVPARSE; i++) {
		netconf_filter_t start, end;

		if (get_filter_client(i, &start, &end) && !(start.match.flags & NETCONF_DISABLED)) {
			while (ntohl(start.match.src.ipaddr.s_addr) <=
				ntohl(end.match.src.ipaddr.s_addr)) {
				/* Override target */
				start.target = log_drop;
				add_filter(&start, NETCONF_FORWARD);
				start.match.src.ipaddr.s_addr =
					htonl(ntohl(start.match.src.ipaddr.s_addr) + 1);
			}
		}
	}

	/* Filter by MAC address */
	if (!nvram_match("filter_macmode", "disabled")) {
		memset(&filter, 0, sizeof(filter));
		strncpy(filter.match.in.name, nvram_safe_get("lan_ifname"), IFNAMSIZ);
		if (nvram_match("filter_macmode", "allow")) {
			/* Allow new connections from
			 * the LAN side only from the specified addresses
			 */
			filter.target = log_accept;
			filter.match.state = NETCONF_NEW;
		} else {
			/* Drop connections from the specified addresses */
			filter.target = log_drop;
		}
		foreach(var, nvram_safe_get("filter_maclist"), next) {
			if (ether_atoe(var, filter.match.mac.octet))
				add_filter(&filter, NETCONF_FORWARD);
		}
	}

	/*
	 * Inbound accepts
	 */
	/* Allow new connections from the loopback interface */
	memset(&filter, 0, sizeof(filter));
	filter.match.state = NETCONF_NEW;
	filter.match.src.ipaddr.s_addr = htonl(INADDR_LOOPBACK);
	filter.match.src.netmask.s_addr = htonl(INADDR_BROADCAST);
	strncpy(filter.match.in.name, "lo", IFNAMSIZ);
	filter.target = log_accept;
	add_filter(&filter, NETCONF_IN);
	add_filter(&filter, NETCONF_FORWARD);

	/* allocate memory for ip_addrs */
	ip_addrs = malloc(sizeof(struct ipaddress_pairs) * MAX_NO_BRIDGE);
	if (!ip_addrs) {
		dprintf("start_firewall()Error allocating memory\n");
		goto start_firewall_cleanup0;
	}
	memset(ip_addrs, 0, sizeof(struct ipaddress_pairs) * MAX_NO_BRIDGE);
	cur_ipaddr = ip_addrs;

	for (i = 0; i < MAX_NO_BRIDGE; i++) {
		if (!i) {
			snprintf(lan_ifname, sizeof(lan_ifname), "lan_ifname");
			snprintf(lan_proto, sizeof(lan_proto), "lan_proto");
			snprintf(lan_ipaddr, sizeof(lan_ipaddr), "lan_ipaddr");
			snprintf(lan_netmask, sizeof(lan_netmask), "lan_netmask");
		}
		else {
			snprintf(lan_proto, sizeof(lan_proto), "lan%x_proto", i);
			snprintf(lan_ifname, sizeof(lan_ifname), "lan%x_ifname", i);
			snprintf(lan_ipaddr, sizeof(lan_ipaddr), "lan%x_ipaddr", i);
			snprintf(lan_netmask, sizeof(lan_netmask), "lan%x_netmask", i);
		}
		if (!strcmp(nvram_safe_get(lan_ifname), ""))
			continue;

		/* Allow new connections from the LAN side */
		memset(&filter, 0, sizeof(filter));
		filter.match.state = NETCONF_NEW;
		strncpy(filter.match.in.name, nvram_safe_get(lan_ifname), IFNAMSIZ);
		inet_aton(nvram_safe_get(lan_ipaddr), (struct in_addr *)&filter.match.src.ipaddr);
		inet_aton(nvram_safe_get(lan_netmask), (struct in_addr *)&filter.match.src.netmask);
		filter.match.src.ipaddr.s_addr &= filter.match.src.netmask.s_addr;

		filter.target = log_accept;
		add_filter(&filter, NETCONF_IN);
		/* filter_macmode applies only to the first bridge (for now). */
		/* On other bridges, add this filter */
		if (!nvram_match("filter_macmode", "allow") || (i > 0)) {
			add_filter(&filter, NETCONF_FORWARD);
		}

		/* Allow DHCP broadcast requests */
		if (nvram_match(lan_proto, "dhcp")) {
			memset(&filter, 0, sizeof(filter));
			filter.match.state = NETCONF_NEW;
			filter.match.ipproto = IPPROTO_UDP;
			strncpy(filter.match.in.name, nvram_safe_get(lan_ifname), IFNAMSIZ);
			filter.match.src.netmask.s_addr = htonl(INADDR_BROADCAST);
			filter.match.src.ipaddr.s_addr = htonl(INADDR_ANY);
			filter.match.dst.netmask.s_addr = htonl(INADDR_BROADCAST);
			filter.match.dst.ipaddr.s_addr = htonl(INADDR_BROADCAST);
			filter.match.dst.ports[0] = htons(67);
			filter.match.dst.ports[1] = htons(67);
			filter.match.src.ports[0] = htons(68);
			filter.match.src.ports[1] = htons(68);
			filter.target = log_accept;
			add_filter(&filter, NETCONF_IN);
		};

		/* store all lanX_ipaddr lanX_netmask */
		address = nvram_get(lan_ipaddr);
		mask = nvram_get(lan_netmask);

		if (address && mask) {
			if (!inet_aton(address, &cur_ipaddr->address)) {
				cprintf("start_firewall():Invalid lan_ipaddr:%s\n", address);
				goto start_firewall_cleanup1;
		}
		if (!inet_aton(mask, &cur_ipaddr->mask)) {
			cprintf("start_firewall():Invalid lan_netmask:%s\n", mask);
			goto start_firewall_cleanup1;
		}
		cur_ipaddr++;

		} else if (address || mask) {
			dprintf("start_firewall(): "
				"Both bridge ipaddress and ipmask must be defined.\n");
			goto start_firewall_cleanup1;
		}


	}
	/* Create restriction filters among bridges */
	for (i = 1; i < MAX_NO_BRIDGE; i++) {
		if (!i) {
			snprintf(lan_ifname, sizeof(lan_ifname), "lan_ifname");
		}
		else {
			snprintf(lan_ifname, sizeof(lan_ifname), "lan%x_ifname", i);
		}
		if (!strcmp(nvram_safe_get(lan_ifname), ""))
			continue;

		for (index = 0; index < MAX_NO_BRIDGE; index++) {

		/* Skip making rule if the dest address is ours 
		 * Assume that the ranges do not overlap so we do not
		 * check the mask value
		 */
			if (index == i)
				continue;

			memset(&filter, 0, sizeof(filter));
			filter.match.state = NETCONF_NEW;
			filter.match.src.ipaddr.s_addr = ip_addrs[i].address.s_addr;
			filter.match.src.ipaddr.s_addr &= ip_addrs[i].mask.s_addr;
			filter.match.src.netmask.s_addr = ip_addrs[i].mask.s_addr;
			filter.match.dst.ipaddr.s_addr = ip_addrs[index].address.s_addr;
			filter.match.dst.ipaddr.s_addr &= ip_addrs[index].mask.s_addr;
			filter.match.dst.netmask.s_addr = ip_addrs[index].mask.s_addr;
			strncpy(filter.match.in.name, nvram_safe_get(lan_ifname), IFNAMSIZ);
			filter.target = log_drop;
			add_filter(&filter, NETCONF_IN);
			add_filter(&filter, NETCONF_FORWARD);
		}
	}

	/* Allow established and related outbound connections back in */
	memset(&filter, 0, sizeof(filter));
	filter.match.state = NETCONF_ESTABLISHED | NETCONF_RELATED;
	filter.target = log_accept;
	add_filter(&filter, NETCONF_IN);
	add_filter(&filter, NETCONF_FORWARD);

	/*
	 * NAT
	 */

	/* Enable IP masquerading */
	if ((fp = fopen("/proc/sys/net/ipv4/ip_forward", "r+"))) {
		fputc('1', fp);
		fclose(fp);
	} else
		perror("/proc/sys/net/ipv4/ip_forward");

#if !defined(AUTOFW_PORT_DEPRECATED)
	/* Application specific port forwards */
	for (i = 0; i < MAX_NVPARSE; i++) {
		memset(&app, 0, sizeof(app));
		if (get_autofw_port(i, &app) && !(app.match.flags & NETCONF_DISABLED))
			netconf_add_fw((netconf_fw_t *) &app);
	}
#endif /* !AUTOFW_PORT_DEPRECATED */

	/*
	* Inbound defaults
	*/

	/* Log refused connections */
	memset(&filter, 0, sizeof(filter));
	filter.target = log_drop;
	add_filter(&filter, NETCONF_IN);
	add_filter(&filter, NETCONF_FORWARD);

#if defined(LINUX_2_6_36) && defined(__CONFIG_SAMBA__)
	add_conntrack();
#endif /* LINUX_2_6_36 && __CONFIG_SAMBA__ */

	dprintf("done\n");
	return 0;

start_firewall_cleanup1:
	if (ip_addrs) {
	 free(ip_addrs);
	 ip_addrs = NULL;
	}

start_firewall_cleanup0:

	return 1;
#endif
    return 0;
}


int
start_firewall2(char *wan_ifname)
{
#if 0
	netconf_nat_t nat;
	netconf_filter_t filter;
	int log_level, log_accept;
	char tmp[100], prefix[] = "wanXXXXXXXXXX_";
	int i;
	char lan_ipaddr[20];
	char lan_netmask[20];

	/* Optionally log connections */
	log_level = atoi(nvram_safe_get("log_level"));
	log_accept = (log_level & 2) ? NETCONF_LOG_ACCEPT : NETCONF_ACCEPT;

	/* Allow new connections from the WAN side */
	if (nvram_match("fw_disable", "1")) {
		memset(&filter, 0, sizeof(filter));
		filter.match.state = NETCONF_NEW;
		strncpy(filter.match.in.name, wan_ifname, IFNAMSIZ);
		filter.target = log_accept;
		add_filter(&filter, NETCONF_IN);
		add_filter(&filter, NETCONF_FORWARD);
	}

	/* Enable IP masquerading for bridge */
	for (i = 0; i < MAX_NO_BRIDGE; i++) {
		if (!i) {
			snprintf(lan_ipaddr, sizeof(lan_ipaddr), "lan_ipaddr");
			snprintf(lan_netmask, sizeof(lan_netmask), "lan_netmask");
		}
		else {
			snprintf(lan_ipaddr, sizeof(lan_ipaddr), "lan%x_ipaddr", i);
			snprintf(lan_netmask, sizeof(lan_netmask), "lan%x_netmask", i);
		}


		memset(&nat, 0, sizeof(nat));
		inet_aton(nvram_safe_get(lan_ipaddr), &nat.match.src.ipaddr);
		inet_aton(nvram_safe_get(lan_netmask), &nat.match.src.netmask);
		nat.match.src.ipaddr.s_addr &= nat.match.src.netmask.s_addr;
		strncpy(nat.match.out.name, wan_ifname, IFNAMSIZ);
		nat.target = NETCONF_MASQ;
		if (nvram_match("nat_type", "cone"))
			nat.type = NETCONF_CONE_NAT;
		netconf_add_nat(&nat);
	}

	/* Enable remote management */
	if (nvram_invmatch("http_wanport", "")) {
		/* Set up WAN side match */
		memset(&nat, 0, sizeof(nat));
		nat.match.ipproto = IPPROTO_TCP;
		nat.match.src.ports[1] = htons(0xffff);
		nat.match.dst.ports[0] = htons(atoi(nvram_safe_get("http_wanport")));
		nat.match.dst.ports[1] = htons(atoi(nvram_safe_get("http_wanport")));
		strncpy(nat.match.in.name, wan_ifname, IFNAMSIZ);

		/* Set up DNAT to LAN */
		nat.target = NETCONF_DNAT;
		(void) inet_aton(nvram_safe_get("lan_ipaddr"), &nat.ipaddr);
		nat.ports[0] = htons(atoi(nvram_safe_get("http_lanport")));
		nat.ports[1] = htons(atoi(nvram_safe_get("http_lanport")));

		add_forward(&nat, NETCONF_IN, log_accept);
	}


	/* Persistent (static) port forwards */
	for (i = 0; i < MAX_NVPARSE; i++) {
		memset(&nat, 0, sizeof(nat));
		if (get_forward_port(i, &nat) && !(nat.match.flags & NETCONF_DISABLED)) {
			/* Set interface name (match packets entering WAN interface) */
			strncpy(nat.match.in.name, wan_ifname, IFNAMSIZ);
			add_forward(&nat, NETCONF_FORWARD, log_accept);
		}
	}

	/* Forward all WAN ports to DMZ IP address */
	if (nvram_invmatch("dmz_ipaddr", "")) {
		/* Set up WAN side match */
		memset(&nat, 0, sizeof(nat));
		nat.match.src.ports[1] = htons(0xffff);
		nat.match.dst.ports[1] = htons(0xffff);
		strncpy(nat.match.in.name, wan_ifname, IFNAMSIZ);

		/* Set up DNAT to LAN */
		nat.target = NETCONF_DNAT;
		(void) inet_aton(nvram_safe_get("dmz_ipaddr"), &nat.ipaddr);
		nat.ports[1] = htons(0xffff);

		nat.match.ipproto = IPPROTO_TCP;
		add_forward(&nat, NETCONF_FORWARD, log_accept);
		nat.match.ipproto = IPPROTO_UDP;
		add_forward(&nat, NETCONF_FORWARD, log_accept);
	}

	/* Clamp TCP MSS to PMTU of WAN interface */
	snprintf(prefix, sizeof(prefix), "wan%d_", wan_ifunit(wan_ifname));
	if (nvram_match(strcat_r(prefix, "proto", tmp), "pppoe"))
		netconf_clamp_mss_to_pmtu();

	dprintf("done\n");
#endif
	return 0;
}


/*
*/
#ifdef __CONFIG_IPV6__
/* Enable IPv6 protocol=41(0x29) on v4NAT */
void
add_ipv6_filter(char *wan_ifname)
{	
	netconf_filter_t filter;
	int log_level = atoi(nvram_safe_get("log_level"));

	/* Figure out nvram variable name prefix for this i/f */
	if ((wan_ifname == NULL) || nvram_match("fw_disable", "1"))
		return;

	memset(&filter, 0, sizeof(filter));
	filter.match.state = NETCONF_NEW;
	filter.match.ipproto = IPPROTO_IPV6;
	strncpy(filter.match.in.name, wan_ifname, IFNAMSIZ);
	filter.match.src.netmask.s_addr = htonl(INADDR_BROADCAST);
	filter.match.src.ipaddr.s_addr = htonl(INADDR_ANY);
	filter.match.dst.netmask.s_addr = htonl(INADDR_BROADCAST);
	filter.match.dst.ipaddr.s_addr = htonl(INADDR_ANY);
	filter.target = (log_level & 2) ? NETCONF_LOG_ACCEPT : NETCONF_ACCEPT;
	add_filter(&filter, NETCONF_IN);
}
#endif /* __CONFIG_IPV6__ */
/*
*/

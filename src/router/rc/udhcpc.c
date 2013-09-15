/*
 * udhcpc scripts
 *
 * Copyright (C) 2011, Broadcom Corporation. All Rights Reserved.
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
 * $Id: udhcpc.c 291523 2011-10-24 06:12:27Z $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

#include <bcmnvram.h>
#include <netconf.h>
#include <shutils.h>
#include <rc.h>

#ifdef __CONFIG_NAT__
static int
expires(char *wan_ifname, unsigned int in)
{
	time_t now;
	FILE *fp;
	char tmp[100];
	int unit;

	if ((unit = wan_ifunit(wan_ifname)) < 0)
		return -1;

	time(&now);
	snprintf(tmp, sizeof(tmp), "/tmp/udhcpc%d.expires", unit);
	if (!(fp = fopen(tmp, "w"))) {
		perror(tmp);
		return errno;
	}
	fprintf(fp, "%d", (unsigned int) now + in);
	fclose(fp);
	return 0;
}

/*
 * deconfig: This argument is used when udhcpc starts, and when a
 * leases is lost. The script should put the interface in an up, but
 * deconfigured state.
*/
static int
deconfig(void)
{
	char *wan_ifname = safe_getenv("interface");

	ifconfig(wan_ifname, IFUP,
		 "0.0.0.0", NULL);
	expires(wan_ifname, 0);

	wan_down(wan_ifname);

	dprintf("done\n");
	return 0;
}

/*
 * bound: This argument is used when udhcpc moves from an unbound, to
 * a bound state. All of the paramaters are set in enviromental
 * variables, The script should configure the interface, and set any
 * other relavent parameters (default gateway, dns server, etc).
*/
static int
bound(void)
{
	char *wan_ifname = safe_getenv("interface");
	char *value;
	char tmp[100], prefix[] = "wanXXXXXXXXXX_";
	int unit;

	if ((unit = wan_ifunit(wan_ifname)) < 0)
		return -1;

	snprintf(prefix, sizeof(prefix), "wan%d_", unit);

	if ((value = getenv("ip")))
		nvram_set(strcat_r(prefix, "ipaddr", tmp), value);
	if ((value = getenv("subnet")))
		nvram_set(strcat_r(prefix, "netmask", tmp), value);
	if ((value = getenv("router")))
		nvram_set(strcat_r(prefix, "gateway", tmp), value);
	if ((value = getenv("dns")))
		nvram_set(strcat_r(prefix, "dns", tmp), value);
	if ((value = getenv("wins")))
		nvram_set(strcat_r(prefix, "wins", tmp), value);
	if ((value = getenv("hostname")))
		sethostname(value, strlen(value) + 1);
	if ((value = getenv("domain")))
		nvram_set(strcat_r(prefix, "domain", tmp), value);
	if ((value = getenv("lease"))) {
		nvram_set(strcat_r(prefix, "lease", tmp), value);
		expires(wan_ifname, atoi(value));
	}

	ifconfig(wan_ifname, IFUP,
		nvram_safe_get(strcat_r(prefix, "ipaddr", tmp)),
		nvram_safe_get(strcat_r(prefix, "netmask", tmp)));

	wan_up(wan_ifname);

	dprintf("done\n");
	return 0;
}

/*
 * renew: This argument is used when a DHCP lease is renewed. All of
 * the paramaters are set in enviromental variables. This argument is
 * used when the interface is already configured, so the IP address,
 * will not change, however, the other DHCP paramaters, such as the
 * default gateway, subnet mask, and dns server may change.
 */
static int
renew(void)
{
	bound();

	dprintf("done\n");
	return 0;
}

int
udhcpc_wan(int argc, char **argv)
{
	if (!argv[1])
		return EINVAL;
	else if (strstr(argv[1], "deconfig"))
		return deconfig();
	else if (strstr(argv[1], "bound"))
		return bound();
	else if (strstr(argv[1], "renew"))
		return renew();
	else
		return EINVAL;
}
#endif	/* __CONFIG_NAT__ */

static int
expires_lan(char *lan_ifname, unsigned int in)
{
	time_t now;
	FILE *fp;
	char tmp[100];

	time(&now);
	snprintf(tmp, sizeof(tmp), "/tmp/udhcpc-%s.expires", lan_ifname);
	if (!(fp = fopen(tmp, "w"))) {
		perror(tmp);
		return errno;
	}
	fprintf(fp, "%d", (unsigned int) now + in);
	fclose(fp);
	return 0;
}

/*
 * deconfig: This argument is used when udhcpc starts, and when a
 * leases is lost. The script should put the interface in an up, but
 * deconfigured state.
*/
static int
deconfig_lan(void)
{
	char *lan_ifname = safe_getenv("interface");

	ifconfig(lan_ifname, IFUP, "0.0.0.0", NULL);
	expires_lan(lan_ifname, 0);

	lan_down(lan_ifname);

	dprintf("done\n");
	return 0;
}

/*
 * bound: This argument is used when udhcpc moves from an unbound, to
 * a bound state. All of the paramaters are set in enviromental
 * variables, The script should configure the interface, and set any
 * other relavent parameters (default gateway, dns server, etc).
*/
static int
bound_lan(void)
{
	char *lan_ifname = safe_getenv("interface");
	char *value;
	int flags;
	unsigned long addr, netmask;
	struct in_addr in_addr, in_netmask;

	if ((value = getenv("ip")))
		nvram_set("lan_ipaddr", value);
	if ((value = getenv("subnet")))
		nvram_set("lan_netmask", value);
	if ((value = getenv("router")))
		nvram_set("lan_gateway", value);
	if ((value = getenv("lease")))
		expires_lan(lan_ifname, atoi(value));

	/* Check if we do need to do ifconfig */
	if (ifconfig_get(lan_ifname, &flags, &addr, &netmask) == 0) {
		inet_aton(nvram_safe_get("lan_ipaddr"), &in_addr);
		inet_aton(nvram_safe_get("lan_netmask"), &in_netmask);
		if ((flags & IFUP) && addr == in_addr.s_addr && netmask == in_netmask.s_addr)
			goto done;
	}

	ifconfig(lan_ifname, IFUP, nvram_safe_get("lan_ipaddr"),
		nvram_safe_get("lan_netmask"));

	lan_up(lan_ifname);

#ifdef __CONFIG_WPS__
	start_wps();
#endif

done:
	dprintf("done\n");
	return 0;
}

/*
 * renew: This argument is used when a DHCP lease is renewed. All of
 * the paramaters are set in enviromental variables. This argument is
 * used when the interface is already configured, so the IP address,
 * will not change, however, the other DHCP paramaters, such as the
 * default gateway, subnet mask, and dns server may change.
 */
static int
renew_lan(void)
{
	bound_lan();

	dprintf("done\n");
	return 0;
}

/* dhcp client script entry for LAN (AP) */
int
udhcpc_lan(int argc, char **argv)
{
	if (!argv[1])
		return EINVAL;
	else if (strstr(argv[1], "deconfig"))
		return deconfig_lan();
	else if (strstr(argv[1], "bound"))
		return bound_lan();
	else if (strstr(argv[1], "renew"))
		return renew_lan();
	else
		return EINVAL;
}

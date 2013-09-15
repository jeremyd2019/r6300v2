/*
 * ppp scripts
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
 * $Id: ppp.c 241182 2011-02-17 21:50:03Z $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <ctype.h>

#include <bcmnvram.h>
#include <netconf.h>
#include <shutils.h>
#include <rc.h>

/*
* parse ifname to retrieve unit #
*/
int
ppp_ifunit(char *ifname)
{
	if (strncmp(ifname, "ppp", 3))
		return -1;
	if (!isdigit(ifname[3]))
		return -1;
	return atoi(&ifname[3]);
}

/*
 * Called when link comes up
 */
int
ipup_main(int argc, char **argv)
{
	FILE *fp;
	char *wan_ifname = safe_getenv("IFNAME");
	char *value;
	char buf[256];
	int unit;
	char tmp[100], prefix[] = "wanXXXXXXXXXX_";

	dprintf("%s\n", argv[0]);

	if ((unit = ppp_ifunit(wan_ifname)) < 0)
		return -1;
	
	snprintf(prefix, sizeof(prefix), "wan%d_", unit);

	/* Touch connection file */
	if (!(fp = fopen(strcat_r("/tmp/ppp/link.", wan_ifname, tmp), "a"))) {
		perror(tmp);
		return errno;
	}
	fclose(fp);

	if ((value = getenv("IPLOCAL"))) {
		ifconfig(wan_ifname, IFUP,
			 value, "255.255.255.255");
		nvram_set(strcat_r(prefix, "ipaddr", tmp), value);
		nvram_set(strcat_r(prefix, "netmask", tmp), "255.255.255.255");
	}

        if ((value = getenv("IPREMOTE")))
		nvram_set(strcat_r(prefix, "gateway", tmp), value);
	strcpy(buf, "");
	if (getenv("DNS1"))
		sprintf(buf, "%s", getenv("DNS1"));
	if (getenv("DNS2"))
		sprintf(buf + strlen(buf), "%s%s", strlen(buf) ? " " : "", getenv("DNS2"));
	nvram_set(strcat_r(prefix, "dns", tmp), buf);

	wan_up(wan_ifname);

	dprintf("done\n");
	return 0;
}

/*
 * Called when link goes down
 */
int
ipdown_main(int argc, char **argv)
{
	char *wan_ifname = safe_getenv("IFNAME");
	int unit;
	char tmp[100], prefix[] = "wanXXXXXXXXXX_";
	
	if ((unit = ppp_ifunit(wan_ifname)) < 0)
		return -1;
	
	snprintf(prefix, sizeof(prefix), "wan%d_", unit);

	wan_down(wan_ifname);

	unlink(strcat_r("/tmp/ppp/link.", wan_ifname, tmp));

	preset_wan_routes(wan_ifname);

	return 0;
}

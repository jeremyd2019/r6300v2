/*
 * Broadcom IGD module eCos OS dependent implementation
 *
 * Copyright (C) 2011, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: igd_ecos_osl.c 241182 2011-02-17 21:50:03Z $
 */
#include <upnp.h>
#include <InternetGatewayDevice.h>
#include <igd_mainloop.h>
#include <iflib.h>
#include <arpa/inet.h>

extern int ifr_link_baud(char *ifn); /* wan_link.c */
extern int nataddrule(char *fmt, ...);
extern int natdelrule(char *fmt, ...);

int
igd_osl_wan_ifstats(IGD_CTRL *igd_ctrl, igd_stats_t *pstats)
{
	struct if_data data;

	cyg_thread_delay(2);

	if (iflib_ioctl(igd_ctrl->wan_ifname, SIOCGIFDATA, &data))
		return -1;

	pstats->rx_packets = data.ifi_ipackets;
	pstats->tx_packets = data.ifi_opackets;
	pstats->rx_bytes = data.ifi_ibytes;
	pstats->tx_bytes = data.ifi_obytes;

	return 0;
}

unsigned int
igd_osl_wan_max_bitrates(char *wan_devname, unsigned long *rx, unsigned long *tx)
{
	*rx = *tx = ifr_link_baud(wan_devname)*1000000;
	return TRUE;
}

int
igd_osl_wan_ip(char *wan_ifname, struct in_addr *inaddr)
{
	inaddr->s_addr = 0;

	if (upnp_osl_ifaddr(wan_ifname, inaddr) == 0)
		return 1;

	return 0;
}

int
igd_osl_wan_isup(char *wan_ifname)
{
	struct in_addr inaddr = {0};

	if (upnp_osl_ifaddr(wan_ifname, &inaddr) == 0) {
		/* Check ip address */
		if (inaddr.s_addr != 0)
			return 1;
	}

	return 0;
}

void
igd_osl_nat_config(char *wan_ifname, IGD_PORTMAP *map)
{
	char *protocol;
	struct in_addr wanip = {0};
	char wanipstr[16];

	/* Check WAN ip address */
	if (upnp_osl_ifaddr(wan_ifname, &wanip) < 0 || wanip.s_addr == 0)
		return;

	strcpy(wanipstr, inet_ntoa(wanip));

	/* Get protocol string */
	if (strcasecmp(map->protocol, "TCP") == 0)
		protocol = "tcp";
	else
		protocol = "udp";

	/* Add/del rdr entry */
	if (map->enable) {
		nataddrule("rdr %s %s/32 port %u -> %s port %u %s\n",
			wan_ifname,
			wanipstr,
			map->external_port,
			map->internal_client,
			map->internal_port,
			protocol);
	}
	else {
		natdelrule("rdr %s %s/32 port %u -> %s port %u %s\n",
			wan_ifname,
			wanipstr,
			map->external_port,
			map->internal_client,
			map->internal_port,
			protocol);
	}

	return;
}

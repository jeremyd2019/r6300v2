/*
 * Broadcom IGD main loop
 *
 * Copyright (C) 2011, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: igd_mainloop.c 241182 2011-02-17 21:50:03Z $
 */
#include <upnp.h>
#include <InternetGatewayDevice.h>
#include <igd_mainloop.h>

int igd_flag;

/* Dispatch UPnP incoming messages. */
int
igd_mainloop_dispatch(UPNP_CONTEXT *context)
{
	struct timeval tv = {1, 0};    /* timed out every second */
	int n;
	int max_fd;
	fd_set  fds;

	FD_ZERO(&fds);

	/* Set select sockets */
	max_fd = upnp_fdset(context, &fds);

	/* select sockets */
	n = select(max_fd+1, &fds, (fd_set *)NULL, (fd_set *)NULL, &tv);
	if (n > 0) {
		upnp_dispatch(context, &fds);
	}

	upnp_timeout(context);

	return 0;
}

/*
 * Initialize igd
 */
static UPNP_CONTEXT *
igd_mainloop_init(int igd_port, int igd_adv_time, IGD_NET *netlist)
{
	int i;
	char **lan_ifnamelist;
	UPNP_CONTEXT *context;

	/* initialize config to default values */
	if (igd_port <= 0 || igd_port > 65535)
		igd_port = 1980;

	if (igd_adv_time <= 0) {
		/* ssdp alive notify every 10 minutes */
		igd_adv_time = 600;
	}

	/* Get net count */
	for (i = 0; netlist[i].lan_ifname; i++)
		;

	lan_ifnamelist = (char **)calloc(i + 1, sizeof(char *));
	if (lan_ifnamelist == NULL)
		return NULL;

	/* Setup lan ifname list */
	for (i = 0; netlist[i].lan_ifname; i++)
		lan_ifnamelist[i] = netlist[i].lan_ifname;

	InternetGatewayDevice.private = (void *)netlist;

	/* Init upnp context */
	context = upnp_init(igd_port, igd_adv_time, lan_ifnamelist, &InternetGatewayDevice);

	/* Clean up */
	free(lan_ifnamelist);
	return context;
}

/* 
 * UPnP portable main loop.
 * It initializes the UPnP protocol and event variables.
 * And loop handler the UPnP incoming requests.
 */
int
igd_mainloop(int igd_port, int igd_adv_time, IGD_NET *netlist)
{
	UPNP_CONTEXT *context;

	/* initialize upnp */
	igd_flag = 0;

	/* init context */
	context = igd_mainloop_init(igd_port, igd_adv_time, netlist);
	if (context == NULL)
		return IGD_FLAG_SHUTDOWN;

	/* main loop */
	while (1) {
		switch (igd_flag) {
		case IGD_FLAG_SHUTDOWN:
			upnp_deinit(context);

			printf("IGD shutdown!\n");
			return IGD_FLAG_SHUTDOWN;

		case IGD_FLAG_RESTART:
			upnp_deinit(context);

			printf("IGD restart!\n");
			return IGD_FLAG_RESTART;

		case 0:
		default:
			igd_mainloop_dispatch(context);
			break;
		}
	}

	return igd_flag;
}

void
igd_stop_handler()
{
	igd_flag = IGD_FLAG_SHUTDOWN;
}

void
igd_restart_handler()
{
	igd_flag = IGD_FLAG_RESTART;
}

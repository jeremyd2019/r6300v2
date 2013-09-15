/*
 * Broadcom IGD module main entry of eCos platform
 *
 * Copyright (C) 2011, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: igd_ecos_main.c 241182 2011-02-17 21:50:03Z $
 */
#include <stdio.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <shutils.h>
#include <upnp.h>
#include <InternetGatewayDevice.h>
#include <igd_mainloop.h>
#include <bcmnvram.h>
#include <ecos_oslib.h>


#define IGD_PRIORITY	10
#define IGD_STACKSIZE	16384

cyg_handle_t igd_main_tid;
cyg_thread igd_main_thread;
unsigned char igd_main_stack[IGD_STACKSIZE];

int igd_running = 0;
static igd_down = 0;


/* Get primary wan ifname.
 * We don't really care about the lan_ifname for now.
 */
static void
igd_wan_info(char *wan_ifname, char *wan_devname)
{
	int unit;
	char name[100];
	char prefix[32];
	char *value;
	char *proto;
	char *devname = "";
	char *ifname = "";

	/* Get primary wan config index */
	for (unit = 0; unit < 10; unit ++) {
		sprintf(name, "wan%d_primary", unit);
		value = nvram_safe_get(name);
		if (strcmp(value, "1") == 0)
			break;
	}
	if (unit == 10)
		unit = 0;

	sprintf(prefix, "wan%d_", unit);

	/* Get wan physical devname */
	devname = nvram_safe_get(strcat_r(prefix, "ifname", name));

	/* Get wan interface name */
	proto = nvram_safe_get(strcat_r(prefix, "proto", name));
	if (strcmp(proto, "pppoe") == 0) {
		ifname = nvram_safe_get(strcat_r(prefix, "pppoe_ifname", name));
	}
	else if (strcmp(proto, "disabled") != 0) {
		ifname = nvram_safe_get(strcat_r(prefix, "ifname", name));
	}

	/* Return to caller */
	strcpy(wan_ifname, ifname);
	strcpy(wan_devname, devname);
	return;
}

/* Read configuration from NVRAM */
static char igd_wan_ifname[IFNAMSIZ];
static char igd_wan_devname[IFNAMSIZ];

static int
igd_read_config(int *igd_port, int *igd_adv_time, IGD_NET **netlist)
{
	char name[32];
	char *value;
	int i;
	int num;
	IGD_NET *nets;

	/* Get igd_port */
	value = nvram_get("igd_port");
	if (value)
		*igd_port = atoi(value);

	/* Get igd adv time */
	value = nvram_get("igd_adv_time");
	if (value)
		*igd_adv_time = atoi(value);

	/* Setup wan names */
	igd_wan_info(igd_wan_ifname, igd_wan_devname);

	/* Get number of lan interfaces */
	for (i = 0, num = 0; i < 255; i++) {
		if (i == 0)
			strcpy(name, "lan_ifname");
		else
			sprintf(name, "lan%d_ifname", i);

		value = nvram_get(name);
		if (value == NULL)
			continue;

		num++;
	}
	if (num == 0)
		return -1;

	/* Allocate igd netlist */
	nets = (IGD_NET *)calloc(num + 1, sizeof(IGD_NET));
	if (nets == NULL)
		return -1;

	/* Setup igd netlist */
	for (i = 0, num = 0; i < 255; i++) {
		if (i == 0)
			strcpy(name, "lan_ifname");
		else
			sprintf(name, "lan%d_ifname", i);

		value = nvram_get(name);
		if (value == NULL)
			continue;

		nets[num].lan_ifname = value;
		nets[num].wan_ifname = igd_wan_ifname;
		nets[num].wan_devname = igd_wan_devname;

		num++;
	}

	*netlist = nets;
	return 0;
}

void
igd_main(void)
{
	/* Set running flag */
	igd_running = 1;

	/* If router is disabled, don't run igd */
	if (nvram_match("router_disable", "0")) {
		/* Reload config if user want to restart */
		int port = 0;
		int adv_time = 0;
		IGD_NET *netlist = 0;

		if (igd_read_config(&port, &adv_time, &netlist) != 0) {
			fprintf(stderr, "igd: read config error!\n");
			goto out;
		}

		/* Enter mainloop */
		igd_mainloop(port, adv_time, netlist);
		free(netlist);
	}

out:
	/* Unset running flag */
	igd_running = 0;
	igd_down = 1;
	return;
}

/* Initialize IGD */
void
igd_start(void)
{
	char *value;

	/* 
	 * Stop igd_mainloop anyway,
	 * if not enabled.
	 */
	value = nvram_get("upnp_enable");
	if (value == 0 || atoi(value) == 0) {
		igd_stop_handler();
		return;
	}

	if (igd_running == 0) {
		igd_down = 0;
		cyg_thread_create(
			IGD_PRIORITY,
			(cyg_thread_entry_t *)&igd_main,
			0,
			"IGD",
			igd_main_stack,
			sizeof(igd_main_stack),
			&igd_main_tid,
			&igd_main_thread);
		cyg_thread_resume(igd_main_tid);

		/* Wait until thread scheduled */
		while (!igd_running && !igd_down)
			cyg_thread_delay(1);
	}

	return;
}

/* Terminate IGD */
void
igd_stop(void)
{
	int pid;

	igd_stop_handler();

	/* Wait until thread exit */
	pid = oslib_getpidbyname("IGD");
	if (pid) {
		while (oslib_waitpid(pid, NULL) != 0)
			cyg_thread_delay(1);
	}
	return;
}

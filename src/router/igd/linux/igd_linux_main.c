/*
 * Broadcom UPnP module main entry of linux platform
 *
 * Copyright (C) 2011, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: igd_linux_main.c 241182 2011-02-17 21:50:03Z $
 */
#include <errno.h>
#include <error.h>
#include <signal.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <netinet/in.h>
#include <wait.h>
#include <ctype.h>
#include <shutils.h>
#include <upnp.h>
#include <InternetGatewayDevice.h>
#include <igd_mainloop.h>
#include <bcmnvram.h>

#define IGD_PID_FILE_PATH	"/tmp/igd.pid"


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

static void
reap(int sig)
{
	pid_t pid;

	if (sig == SIGPIPE)
		return;

	while ((pid = waitpid(-1, NULL, WNOHANG)) > 0)
		printf("Reaped %d\n", (int)pid);
}

int
main(int argc, char *argv[])
{
	char **argp = &argv[1];
	int daemonize = 0;
	int usage = 0;

	FILE *pidfile;

	/*
	 * Check whether this process is running
	 */
	if ((pidfile = fopen(IGD_PID_FILE_PATH, "r"))) {
		fprintf(stderr, "%s: UPnP IGD has been started\n", __FILE__);

		fclose(pidfile);
		return -1;
	}

	/* Create pid file */
	if ((pidfile = fopen(IGD_PID_FILE_PATH, "w"))) {
		fprintf(pidfile, "%d\n", getpid());
		fclose(pidfile);
	}
	else {
		perror("pidfile");
		exit(errno);
	}

	/*
	 * Process arguments
	 */
	while (argp < &argv[argc]) {
		if (strcasecmp(*argp, "-D") == 0) {
			daemonize = 1;
		}
		else {
			usage = 1;
		}
		argp++;
	}

	/* Warn if the some arguments are not supported */
	if (usage)
		fprintf(stderr, "usage: %s -D\n", argv[0]);

	/*
	 * We need to have a reaper for child processes we may create.
	 * That happens when we send signals to the dhcp process to
	 * release an renew a lease on the external interface.
	 */
	signal(SIGCHLD, reap);

	/* Handle the TCP -EPIPE error */
	signal(SIGPIPE, reap);

	/*
	 * For some reason that I do not understand, this process gets
	 * a SIGTERM after sending SIGUSR1 to the dhcp process (to
	 * renew a lease).  Ignore SIGTERM to avoid being killed when
	 * this happens.
	 */
	/* signal(SIGTERM, SIG_IGN); */
	signal(SIGUSR1, SIG_IGN);

	signal(SIGINT, igd_stop_handler);
	signal(SIGTERM, igd_stop_handler);

	fflush(stdout);

	/*
	 * Enter mainloop
	 */
	if (daemonize && daemon(1, 1) == -1) {
		/* Destroy pid file */
		unlink(IGD_PID_FILE_PATH);

		perror("daemon");
		exit(errno);
	}

	/* Replace pid file with daemon pid */
	pidfile = fopen(IGD_PID_FILE_PATH, "w");
	fprintf(pidfile, "%d\n", getpid());
	fclose(pidfile);

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
	/* Destroy pid file */
	unlink(IGD_PID_FILE_PATH);

	return 0;
}

/*
 * Miscellaneous services
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
 * $Id: services.c 370928 2012-11-26 07:27:55Z $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#include <bcmnvram.h>
#include <netconf.h>
#include <shutils.h>
#include <rc.h>
#include <pmon.h>

#define assert(a)

#ifdef __CONFIG_NAT__
static char
*make_var(char *prefix, int index, char *name)
{
	static char buf[100];

	assert(prefix);
	assert(name);

	if (index)
		snprintf(buf, sizeof(buf), "%s%d%s", prefix, index, name);
	else
		snprintf(buf, sizeof(buf), "%s%s", prefix, name);
	return buf;
}
int
start_dhcpd(void)
{
	FILE *fp;
	char name[100];
	char word[32], *next;
	char dhcp_conf_file[128];
	char dhcp_lease_file[128];
	char dhcp_ifnames[128] = "";
	int index;
	char tmp[20];
	int i = 0;
	char *lan_ifname = NULL;

	if (nvram_match("router_disable", "1"))
		return 0;

	/* Setup individual dhcp severs for the bridge and the every unbridged interface */
	for (i = 0; i < MAX_NO_BRIDGE; i++) {
		if (!i)
			snprintf(tmp, sizeof(tmp), "lan_ifname");
		else
			snprintf(tmp, sizeof(tmp), "lan%x_ifname", i);

		lan_ifname = nvram_safe_get(tmp);

		if (!strcmp(lan_ifname, ""))
			continue;

		snprintf(dhcp_ifnames, sizeof(dhcp_ifnames), "%s %s", dhcp_ifnames, lan_ifname);
	}

	index = 0;
	foreach(word, dhcp_ifnames, next) {

		if (strstr(word, "br0"))
			index = 0;
		else
			index = 1;

		/* Skip interface if no valid config block is found */
		if (index < 0) continue;

		if (nvram_invmatch(make_var("lan", index, "_proto"), "dhcp"))
			continue;

		dprintf("%s %s %s %s\n",
			nvram_safe_get(make_var("lan", index, "_ifname")),
			nvram_safe_get(make_var("dhcp", index, "_start")),
			nvram_safe_get(make_var("dhcp", index, "_end")),
			nvram_safe_get(make_var("lan", index, "_lease")));

		/* Touch leases file */
		sprintf(dhcp_lease_file, "/tmp/udhcpd%d.leases", index);
		if (!(fp = fopen(dhcp_lease_file, "a"))) {
			perror(dhcp_lease_file);
			return errno;
		}
		fclose(fp);

		/* Write configuration file based on current information */
		sprintf(dhcp_conf_file, "/tmp/udhcpd%d.conf", index);
		if (!(fp = fopen(dhcp_conf_file, "w"))) {
			perror(dhcp_conf_file);
			return errno;
		}
		fprintf(fp, "pidfile /var/run/udhcpd%d.pid\n", index);
		fprintf(fp, "start %s\n", nvram_safe_get(make_var("dhcp", index, "_start")));
		fprintf(fp, "end %s\n", nvram_safe_get(make_var("dhcp", index, "_end")));
		fprintf(fp, "interface %s\n", word);
		fprintf(fp, "remaining yes\n");
		fprintf(fp, "lease_file /tmp/udhcpd%d.leases\n", index);
		fprintf(fp, "option subnet %s\n",
			nvram_safe_get(make_var("lan", index, "_netmask")));
		fprintf(fp, "option router %s\n",
			nvram_safe_get(make_var("lan", index, "_ipaddr")));
		fprintf(fp, "option dns %s\n", nvram_safe_get(make_var("lan", index, "_ipaddr")));
		fprintf(fp, "option lease %s\n", nvram_safe_get(make_var("lan", index, "_lease")));
		snprintf(name, sizeof(name), "%s_wins",
			nvram_safe_get(make_var("dhcp", index, "_wins")));
		if (nvram_invmatch(name, ""))
			fprintf(fp, "option wins %s\n", nvram_get(name));
		snprintf(name, sizeof(name), "%s_domain",
			nvram_safe_get(make_var("dhcp", index, "_domain")));
		if (nvram_invmatch(name, ""))
			fprintf(fp, "option domain %s\n", nvram_get(name));
		fclose(fp);

		eval("udhcpd", dhcp_conf_file);
		index++;
	}
	dprintf("done\n");
	return 0;
}

int
stop_dhcpd(void)
{
	char sigusr1[] = "-XX";
	int ret;

/*
* Process udhcpd handles two signals - SIGTERM and SIGUSR1
*
*  - SIGUSR1 saves all leases in /tmp/udhcpd.leases
*  - SIGTERM causes the process to be killed
*
* The SIGUSR1+SIGTERM behavior is what we like so that all current client
* leases will be honorred when the dhcpd restarts and all clients can extend
* their leases and continue their current IP addresses. Otherwise clients
* would get NAK'd when they try to extend/rebind their leases and they
* would have to release current IP and to request a new one which causes
* a no-IP gap in between.
*/
	sprintf(sigusr1, "-%d", SIGUSR1);
	eval("killall", sigusr1, "udhcpd");
	ret = eval("killall", "udhcpd");

	dprintf("done\n");
	return ret;
}

int
start_dns(void)
{
	int ret;
	FILE *fp;
	char dns_ifnames[64] = "";
	char tmp[20];
	int i = 0;
	char *lan_ifname = NULL;
	char dns_cmd[255];

	if (nvram_match("router_disable", "1"))
		return 0;

	/* Create resolv.conf with empty nameserver list */
	if (!(fp = fopen("/tmp/resolv.conf", "w"))) {
		perror("/tmp/resolv.conf");
		return errno;
	}
	fclose(fp);

	/* Setup interface list for the dns relay */
	for (i = 0; i < MAX_NO_BRIDGE; i++) {
		if (!i)
			snprintf(tmp, sizeof(tmp), "lan_ifname");
		else
			snprintf(tmp, sizeof(tmp), "lan%x_ifname", i);

		lan_ifname = nvram_safe_get(tmp);

		if (!strcmp(lan_ifname, ""))
			continue;

		snprintf(dns_ifnames, sizeof(dns_ifnames), "%s -i %s", dns_ifnames, lan_ifname);
	}

	/* Start the dns relay */
	sprintf(dns_cmd, "/usr/sbin/dnsmasq -h -n %s -r /tmp/resolv.conf&", dns_ifnames);
	ret = system(dns_cmd);

	dprintf("done\n");
	return ret;
}

int
stop_dns(void)
{
	int ret = eval("killall", "dnsmasq");

	/* Remove resolv.conf */
	unlink("/tmp/resolv.conf");

	dprintf("done\n");
	return ret;
}
#endif	/* __CONFIG_NAT__ */

/*
*/
#ifdef __CONFIG_IPV6__
/*
****************************************************************************
*  lanX_ipv6_mode: 0=disable, 1=6to4 Enabled, 2=Native Enabled, 3=6to4+native!
*  lanX_ipv6_6to4id: 0~65535, used to form tunneling address
*     2002:wwxx:yyzz:lanX_ipv6_6to4id::/64
*  lanX_ipv6_dns: v6 DNS IP to be given out by dhcp6s.
*  lanX_ipv6_prefix: prefix for Native IPv6 LAN support
*  wanX_ipv6_prefix: prefix for Native IPv6 WAN support.
*    This will take effect only if at least one LAN has native enabled.
****************************************************************************
*/

#ifdef __CONFIG_DHCPV6S__
static int
stop_dhcp6s(void)
{
	int ret;

	ret = eval("killall", "dhcp6s");

	dprintf("done\n");
	return ret;
}

/*	Start the Stateless DHCPv6 server.
*	Return > 0: number of interfaces configured and supported by this application.
*		==0: the application is not started since no action is required.
*		< 0: Error number
*/
static int
start_dhcp6s(void)
{
	FILE *fp;
	int i, ret, pid;
	int siMode, siCount;
	char *pcDNS, *pcPrefix, *pcNMask, acPrefix[64];
	char *pcIF, acIFName[MAX_NO_BRIDGE][IFNAMSIZ];
	char *dhcp_conf_file = "/tmp/dhcp6s.conf";
	char *dhcp_lease_file = "/tmp/server6.leases";
	char *apCommand[MAX_NO_BRIDGE+4] =
		{ "dhcp6s", "-c", dhcp_conf_file, NULL,	};

	/* Touch leases file */
	if (!(fp = fopen(dhcp_lease_file, "a"))) {
		perror(dhcp_lease_file);
		return errno;
	}
	fclose(fp);

	/* Write configuration file based on current information */
	if (!(fp = fopen(dhcp_conf_file, "w"))) {
		perror(dhcp_conf_file);
		return errno;
	}

	for (i = 0, siCount = 0; i < MAX_NO_BRIDGE; i++) {
		siMode = atoi(nvram_safe_get(make_var("lan", i, "_ipv6_mode")));
		if (siMode == 0)
			continue;

		pcIF = nvram_get(make_var("lan", i, "_ifname"));
		pcDNS = nvram_get(make_var("lan", i, "_ipv6_dns"));
		pcPrefix = nvram_get(make_var("lan", i, "_ipv6_prefix"));
		if (pcIF == NULL || pcPrefix == NULL || pcDNS == NULL)
			continue;

		strncpy(acIFName[siCount], pcIF, sizeof(acIFName[0]));
		apCommand[siCount + 3] = acIFName[siCount];

		strncpy(acPrefix, pcPrefix, sizeof(acPrefix));
		pcNMask = strchr(acPrefix, '/');
		if (pcNMask == NULL)
			pcNMask = "112";
		else
			*pcNMask++ = 0x00;

		fprintf(fp, "interface %s {\n", pcIF);
		fprintf(fp, " server-preference %d;\n", 25);
		fprintf(fp, " option dns_servers %s %s;\n", pcDNS,
			nvram_safe_get(make_var("lan", i, "_domain")));
		fprintf(fp, " send information-only;\n"); /* Stateless DHCP */
		fprintf(fp, " link LAN {\n");
		fprintf(fp, "  pool {\n");
		fprintf(fp, "   range %s%x to %s%x/%s;\n",
			acPrefix, 0x1000, acPrefix, 0x2000, pcNMask);
		fprintf(fp, "   prefix %s;\n", pcPrefix);
		fprintf(fp, "  };\n"); /* pool */
		fprintf(fp, " };\n"); /* link */
		fprintf(fp, "};\n\n"); /* interface */

		siCount++;
	}

	fclose(fp);

	if (siCount == 0)
		return 0;

	apCommand[siCount + 3] = NULL;
	if ((ret = _eval(apCommand, NULL, 0, &pid) < 0))
		return ret;

	return siCount;
}
#endif /* __CONFIG_DHCPV6S__ */

#ifdef __CONFIG_RADVD__
/*	Construct the interface section to the configuration file.
*	Return 0: no configuration is generated for this interface.
*     1: the configuration is written.
*/
static int
radvdconf_add_if(FILE *fp, int siV6Mode, char *pcIFName, char *pcPrefix,
	char *pc6to4WANIF, unsigned short uw6to4ID)
{
	if ((fp == NULL) || (pcIFName == NULL || *pcIFName == 0) ||
		(pcPrefix == NULL || *pcPrefix == 0))
		return 0;

	/* Start of interface section */
	fprintf(fp, "interface %s {\n", pcIFName);
	fprintf(fp, " AdvSendAdvert on;\n");
	fprintf(fp, " MinRtrAdvInterval %d;\n", 4);
	fprintf(fp, " MaxRtrAdvInterval %d;\n", 10);
	fprintf(fp, " AdvDefaultPreference low;\n");
	fprintf(fp, " AdvHomeAgentFlag off;\n");
	fprintf(fp, " AdvOtherConfigFlag on;\n");

	/* Check to advertize Network Prefix */
	if (siV6Mode & IPV6_NATIVE_ENABLED) {
		fprintf(fp, " prefix %s {\n", pcPrefix);
		fprintf(fp, "  AdvOnLink on;\n");
		fprintf(fp, "  AdvAutonomous on;\n");
		fprintf(fp, " };\n");
	}

	/* Check to advertize 6to4 prefix */
	if ((siV6Mode & IPV6_6TO4_ENABLED) && (pc6to4WANIF && *pc6to4WANIF)) {
		fprintf(fp, " prefix 0:0:0:%x::/64 {\n", uw6to4ID);
		fprintf(fp, "  AdvOnLink on;\n");
		fprintf(fp, "  AdvAutonomous on;\n");
		fprintf(fp, "  AdvRouterAddr off;\n");
		fprintf(fp, "  Base6to4Interface %s;\n", pc6to4WANIF);
		fprintf(fp, "  AdvPreferredLifetime %d;\n", 20);
		fprintf(fp, "  AdvValidLifetime %d;\n", 30);
		fprintf(fp, " };\n");
	}

	/* End of interface section */
	fprintf(fp, "};\n\n");
	return 1;
}

static int
stop_radvd(void)
{
	FILE *fp;
	int ret;

	ret = eval("killall", "radvd");
	ret = eval("rm", "-f", "/var/run/radvd.pid");

	if ((fp = fopen("/proc/sys/net/ipv6/conf/all/forwarding", "r+"))) {
		fputc('0', fp);
		fclose(fp);
	}
	return ret;
}

/*	Start the Router Advertizement Daemon.
*	Return > 0: number of interfaces configured and supported by this application.
*		==0: the application is not started since no action is required.
*		< 0: Error number
*/
static int
start_radvd(void)
{
	FILE *fp;
	char *pcWANIF, *pcWANPrefix;
	int i, ret, siMode, siCount, siNative, boolRouterEnable;
	char *radvd_conf_file = "/tmp/radvd.conf";

	if (!(fp = fopen(radvd_conf_file, "w"))) {
		perror(radvd_conf_file);
		return errno;
	}

	boolRouterEnable = nvram_match("router_disable", "0");
	pcWANIF = nvram_match("wan_proto", "pppoe")?
		nvram_safe_get("wan_pppoe_ifname"): nvram_safe_get("wan_ifname");

	/* The Router Advertizement for LAN interfaces */
	for (i = 0, siCount = 0, siNative = 0; i < MAX_NO_BRIDGE; i++) {
		siMode = atoi(nvram_safe_get(make_var("lan", i, "_ipv6_mode")));
		if (siMode == 0 || ((siMode == IPV6_6TO4_ENABLED) && (boolRouterEnable == 0)))
			continue;

		/* The Router Advertizement for LAN interface */
		if (radvdconf_add_if(fp, siMode, nvram_get(make_var("lan", i, "_ifname")),
			nvram_get(make_var("lan", i, "_ipv6_prefix")),
			pcWANIF, /* Use the logical interface for 6to4 */
			atoi(nvram_safe_get(make_var("lan", i, "_ipv6_6to4id"))))) {
			if (siMode & IPV6_NATIVE_ENABLED)
				siNative++;
			siCount++;
		}
	}

	/* The Router Advertizement for WAN interface */
	if (siNative && boolRouterEnable && (pcWANPrefix = nvram_get("wan_ipv6_prefix"))) {
		radvdconf_add_if(fp, IPV6_NATIVE_ENABLED,
			nvram_get("wan_ifname"), /* Use the physical interface */
			pcWANPrefix, NULL, 0);
	}

	fclose(fp);

	if (siCount == 0)
		return 0;

	/*  Enable forwarding as radvd mandates this */
	if ((fp = fopen("/proc/sys/net/ipv6/conf/all/forwarding", "r+"))) {
		fputc('1', fp);
		fclose(fp);
	}

	if ((ret = eval("radvd", "-C", radvd_conf_file)) < 0)
		return ret;

	return siCount;
}
#endif /* __CONFIG_RADVD__ */

/*	Add routes for IPv6 enabled LAN/WAN interfaces.
*	Return > 0: number of interfaces configured and supported by this application.
*		==0: the application is not started since no action is required.
*		< 0: Error number
*/
static int
ipv6_add_routes_del_addrs(void)
{
	int i, ret;
	int siMode, siCount;
	char *pcLANIF;

	/* Setup v6 route and clean up unnecessary autoconfigured addresses for LANs */
	for (i = 0, siCount = 0; i < MAX_NO_BRIDGE; i++) {
		pcLANIF = nvram_get(make_var("lan", i, "_ifname"));
		if (pcLANIF == NULL || *pcLANIF == 0x00)
			continue;
		siMode = atoi(nvram_safe_get(make_var("lan", i, "_ipv6_mode")));
		if (siMode == 0)
			eval("ip", "-6", "addr", "flush", "dev", pcLANIF, "scope", "all");
		else if ((siMode & IPV6_NATIVE_ENABLED)) {
			ret = eval("ip", "-6", "route", "add",
				nvram_safe_get(make_var("lan", i, "_ipv6_prefix")), "dev",
				pcLANIF, "metric", "1");
			siCount++;
		}
	}

	/* Setup v6 route and clean up addresses for WAN */
	{
		char *pcWANPrefix, *pcWANIF;

		/* Native is not enabled, or router is disabled, or the wan prefix is not set */
		if (siCount == 0 || nvram_invmatch("router_disable", "0") ||
			((pcWANPrefix = nvram_get("wan_ipv6_prefix")) == NULL) ||
			(*pcWANPrefix == 0x00)) {
			eval("ip", "-6", "addr", "flush", "dev",
				nvram_safe_get("wan_ifname"), "scope", "all");
		}
		else {
			pcWANIF = nvram_match("wan_proto", "pppoe")?
				nvram_get("wan_pppoe_ifname"): nvram_get("wan_ifname");
			if (pcWANIF && (*pcWANIF != 0x00))
				ret = eval("ip", "-6", "route", "add",
					pcWANPrefix, "dev",	pcWANIF, "metric", "1");
		}
	}

	return siCount;
}

int
stop_ipv6(void)
{
	int ret;

#ifdef __CONFIG_RADVD__
	ret = stop_radvd();
#endif /* __CONFIG_RADVD__ */

#ifdef __CONFIG_DHCPV6S__
	ret = stop_dhcp6s();
#endif /* __CONFIG_DHCPV6S__ */

	/* Flush all v6 routes */
	ret = eval("ip", "-6", "route", "flush");

	return ret;
}

/*
*	Return > 0: number of interfaces configured and supported by this application.
*		==0: the application is not started since no action is required.
*		< 0: Error number
*/
int
start_ipv6(void)
{
	int ret = 0;
	int siSocket;

	/* Check if IPv6 protocol stack is running */
	if ((siSocket = socket(AF_INET6, SOCK_RAW, IPPROTO_RAW)) < 0)
		return 0;
	close(siSocket);

#ifdef __CONFIG_DHCPV6S__
	if ((ret = start_dhcp6s()) < 0)
		return ret;
#endif /* __CONFIG_DHCPV6S__ */

#ifdef __CONFIG_RADVD__
	/* If no interface is configured by radvd than it is done! */
	if ((ret = start_radvd()) < 0) {
		stop_dhcp6s();
		return ret;
	}
#endif /* __CONFIG_RADVD__ */

	/* Note that 6to4 routing is done in wan_up()! */
	return ipv6_add_routes_del_addrs();
}

int
is_ipv6_enabled(void)
{
	int i, siMode;

	for (i = 0; i < MAX_NO_BRIDGE; i++) {
		siMode = atoi(nvram_safe_get(make_var("lan", i, "_ipv6_mode")));
		if (siMode)
			return TRUE;
	}
	return FALSE;
}

#endif /* __CONFIG_IPV6__ */
/*
*/

int
start_httpd(void)
{
	int ret;

	chdir("/www");
	ret = eval("httpd");
	chdir("/");

	dprintf("done\n");
	return ret;
}

int
stop_httpd(void)
{
	int ret = eval("killall", "httpd");

	dprintf("done\n");
	return ret;
}

#ifdef PLC
/* Run time check for if gigled must be started based on nvram keys */
static int
target_uses_gigled(void)
{
	return nvram_match("wl0_plc", "1") || nvram_match("wl1_plc", "1");
}

static int
start_gigled(void)
{
	int ret;

	chdir("/tmp");
	ret = eval("/usr/sbin/gigled");
	chdir("/");

	dprintf("done\n");
	return ret;
}

static int
stop_gigled(void)
{
	int ret = eval("killall", "gigled");

	dprintf("done\n");
	return ret;
}

static int
start_plcnvm(void)
{
	int ret;

	chdir("/tmp");
	ret = eval("/usr/sbin/plcnvm");
	chdir("/");

	dprintf("done\n");
	return ret;
}

static int
stop_plcnvm(void)
{
	int ret = eval("killall", "plcnvm");

	dprintf("done\n");
	return ret;
}

static int
start_plcboot(void)
{
	int ret;

	chdir("/tmp");
	ret = eval("/usr/sbin/plcboot");
	chdir("/");

	dprintf("done\n");
	return ret;
}

static int
stop_plcboot(void)
{
	int ret = eval("killall", "plcboot");

	dprintf("done\n");
	return ret;
}
#endif /* PLC */

#ifdef __CONFIG_NAT__
#ifdef __CONFIG_UPNP__
int
start_upnp(void)
{
	char *wan_ifname;
	int ret;
	char var[100], prefix[] = "wanXXXXXXXXXX_";

	if (!nvram_invmatch("upnp_enable", "0"))
		return 0;

	ret = eval("killall", "-SIGUSR1", "upnp");
	if (ret != 0) {
		snprintf(prefix, sizeof(prefix), "wan%d_", wan_primary_ifunit());
		if (nvram_match(strcat_r(prefix, "proto", var), "pppoe"))
			wan_ifname = nvram_safe_get(strcat_r(prefix, "pppoe_ifname", var));
		else
			wan_ifname = nvram_safe_get(strcat_r(prefix, "ifname", var));

		ret = eval("upnp", "-D", "-W", wan_ifname);

	}
	dprintf("done\n");
	return ret;
}

int
stop_upnp(void)
{
	int ret = 0;

	if (nvram_match("upnp_enable", "0"))
	    ret = eval("killall", "upnp");

	dprintf("done\n");
	return ret;
}
#endif /* __CONFIG_UPNP__ */

#ifdef	__CONFIG_IGD__
int
start_igd(void)
{
	int ret = 0;

	if (nvram_match("upnp_enable", "1")) {
		ret = eval("igd", "-D");
	}

	return ret;
}

int
stop_igd(void)
{
	int ret = 0;

	ret = eval("killall", "igd");

	return ret;
}
#endif	/* __CONFIG_IGD__ */
#endif	/* __CONFIG_NAT__ */



int
start_nas(void)
{
	int ret = eval("nas");

	return ret;
}

int
stop_nas(void)
{
	int ret;

	ret = eval("killall", "nas");
	return ret;
}

#ifdef __CONFIG_WAPI__
int
start_wapid(void)
{
	int ret = eval("wapid");

	return ret;
}

int
stop_wapid(void)
{
	int ret = eval("killall", "wapid");

	return ret;
}
#endif /* __CONFIG_WAPI__ */

#ifdef __CONFIG_WAPI_IAS__
int
start_ias(void)
{
	char *ias_argv[] = {"ias", "-F", "/etc/AS.conf", "-D", NULL};
	pid_t pid;

	if (nvram_match("as_mode", "enabled")) {
		_eval(ias_argv, NULL, 0, &pid);
	}

	return 0;
}

int
stop_ias(void)
{
	int ret = 0;

	if (!nvram_match("as_mode", "enabled")) {
		/* Need add signal handler in as */
		ret = eval("killall", "ias");
	}

	return ret;
}
#endif /* __CONFIG_WAPI_IAS__ */

int
start_ntpc(void)
{
	char *servers = nvram_safe_get("ntp_server");

	if (strlen(servers)) {
		char *nas_argv[] = {"ntpclient", "-h", servers, "-i", "3", "-l", "-s", NULL};
		pid_t pid;

		_eval(nas_argv, NULL, 0, &pid);
	}

	dprintf("done\n");
	return 0;
}

int
stop_ntpc(void)
{
	int ret = eval("killall", "ntpclient");

	dprintf("done\n");
	return ret;
}

int
start_telnet(void)
{
	char tmp[20];
	int i = 0;
	int ret = 0;
	char *lan_ifname = NULL;


	for (i = 0; i < MAX_NO_BRIDGE; i++) {
		if (!i)
			snprintf(tmp, sizeof(tmp), "lan_ifname");
		else
			snprintf(tmp, sizeof(tmp), "lan%x_ifname", i);

		lan_ifname = nvram_safe_get(tmp);

		if (!strcmp(lan_ifname, ""))
			continue;

		ret = eval("utelnetd", "-d", "-i", lan_ifname);
	}

	return ret;
}

int
stop_telnet(void)
{
	int ret;
	ret = eval("killall", "utelnetd");
	return ret;
}


int
start_wps(void)
{
	char *wps_argv[] = {"/bin/wps_monitor", NULL};
	pid_t pid;
	int wait_time = 3;
    if ( nvram_match("wps_status", "2") && !nvram_match("wps_start", "pbc") ) /* added for CGI process */
    {
        return 0;
    }

	nvram_set("wps_status", "0");
	nvram_set("wps_method", "1");
	nvram_set("wps_config_command", "0");
	nvram_set("wps_proc_mac", "");
	nvram_set("wps_proc_status", "0");

	if (nvram_match("wps_restart", "1")) {
		nvram_set("wps_restart", "0");
	}
	else {
		nvram_set("wps_restart", "0");
		nvram_set("wps_proc_status", "0");
	}

	nvram_set("wps_sta_pin", "00000000");
	nvram_set("wps_currentband", "");

	eval("killall", "wps_monitor");
	do {
		if ((pid = get_pid_by_name("/bin/wps_monitor")) <= 0)
			break;
		wait_time--;
		sleep(1);
	} while (wait_time);
	if (wait_time == 0)
		dprintf("Unable to kill wps_monitor!\n");

   	eval("killall","wps_ap");
   	eval("killall","wps_enr");

	_eval(wps_argv, NULL, 0, &pid);

	return 0;

}

int
stop_wps(void)
{
	int ret = 0;

   	ret = eval("killall","wps_monitor");
   	ret = eval("rm", "-f", "/tmp/wps_monitor.pid"); /*  added, zacker */
   	ret = eval("killall","wps_ap");

	return ret;
}
int
start_bcmupnp(void)
{
	int ret;

	if (!nvram_invmatch("upnp_enable", "0"))
	{
	    printf("upnp_enable is 0, needn't trigger bcmupnp.\n");
		return 0;
	}
		
    if ( nvram_match("wl0_wps_mode", "disabled") || nvram_match("wps_mode", "disabled") )
    {
        printf("WPS is disabled, needn't trigger bcmupnp.\n");
        return 0;
    }

    /*  modified start pling 06/29/2010 */
    /* Restart upnp properly */
#if 0
	ret = eval("killall", "-SIGUSR1", "upnp");
	if (ret != 0) 
	{
		ret = eval("upnp", "-D", "-W", nvram_get("wan_ifname"));
	}
#endif
	eval("killall", "upnp");
	eval("upnp", "-D", "-W", nvram_get("wan_ifname"));
    /*  modified end pling 06/29/2010 */

	dprintf("done\n");
	return ret;
}

int
stop_bcmupnp(void)
{
	int ret = 0;

	if (nvram_match("upnp_enable", "0"))
	    ret = eval("killall", "upnp");

	dprintf("done\n");
	return ret;
}

#ifdef __CONFIG_IGMP_PROXY__
void
start_igmp_proxy(void)
{
	/* Start IGMP Proxy in Router mode only */
	if (!nvram_match("router_disable", "1"))
		eval("igmp", nvram_get("wan_ifname"));
	else {
		if (!nvram_match("igmp_enable", "0")) {
			/* Start IGMP proxy in AP mode for media router build */
			eval("igmp", nvram_get("lan_ifname"));
		}
	}

	return;
}

void
stop_igmp_proxy(void)
{
	eval("killall", "igmp");
	return;
}
#endif /* __CONFIG_IGMP_PROXY__ */

#ifdef __CONFIG_LLD2D__
int start_lltd(void)
{
	chdir("/usr/sbin");
	eval("lld2d", "br0");
	chdir("/");

	return 0;
}

int stop_lltd(void)
{
	int ret;

	ret = eval("killall", "lld2d");

	return ret;
}
#endif /* __CONFIG_LLD2D__ */

int
start_eapd(void)
{
	int ret = eval("/bin/eapd");

	return ret;
}

int
stop_eapd(void)
{
	int ret = eval("killall", "eapd");

	return ret;
}

#if defined(BCM_DCS) || defined(EXT_ACS)
int
start_acsd(void)
{
	int ret = eval("/usr/sbin/acsd");

	return ret;
}

int
stop_acsd(void)
{
	int ret = eval("killall", "acsd");

	return ret;
}
#endif /* BCM_DCS || EXT_ACS */


#if defined(PHYMON)
int
start_phymons(void)
{
	int ret = eval("/usr/sbin/phymons");

	return ret;
}

int
stop_phymons(void)
{
	int ret = eval("killall", "phymons");

	return ret;
}
#endif /*  PHYMON */

#ifdef __CONFIG_SAMBA__
void enable_gro(int interval)
{
#ifdef LINUX26
	char *argv[3] = {"echo", "", NULL};
	char lan_ifname[32], *lan_ifnames, *next;
	char path[64] = {0};
	char parm[32] = {0};

	if (nvram_match("inet_gro_disable", "1"))
		return;

	/* enabled gro on vlan interface */
	lan_ifnames = nvram_safe_get("lan_ifnames");
	foreach(lan_ifname, lan_ifnames, next) {
		if (!strncmp(lan_ifname, "vlan", 4)) {
			sprintf(path, ">>/proc/net/vlan/%s", lan_ifname);
			sprintf(parm, "-gro %d", interval);
			argv[1] = parm;
			_eval(argv, path, 0, NULL);
		}
	}
#endif /* LINUX26 */
}

int
start_samba()
{
	char *argv[3] = {"echo", "", NULL};
	char *samba_mode;
	char *samba_passwd;

	enable_gro(2);

	samba_mode = nvram_safe_get("samba_mode");
	samba_passwd = nvram_safe_get("samba_passwd");

	/* Samba is disabled */
	if (strncmp(samba_mode, "1", 1) && strncmp(samba_mode, "2", 1))
		return 0;

	/* Create smb.conf */
	argv[1] = "[global]";
	_eval(argv, ">/tmp/samba/lib/smb.conf", 0, NULL);

	argv[1] = "workgroup = mygroup";
	_eval(argv, ">>/tmp/samba/lib/smb.conf", 0, NULL);

	if (!strncmp(samba_mode, "1", 1))
		argv[1] = "security = user";
	else
		argv[1] = "security = share";
	_eval(argv, ">>/tmp/samba/lib/smb.conf", 0, NULL);

	argv[1] = "use sendfile = yes";
	_eval(argv, ">>/tmp/samba/lib/smb.conf", 0, NULL);

#ifdef LINUX26
	argv[1] = "[media]";
#else
	argv[1] = "[mnt]";
#endif
	_eval(argv, ">>/tmp/samba/lib/smb.conf", 0, NULL);

#ifdef LINUX26
	argv[1] = "path = /media";
#else
	argv[1] = "path = /mnt";
#endif
	_eval(argv, ">>/tmp/samba/lib/smb.conf", 0, NULL);

	argv[1] = "writeable = yes";
	_eval(argv, ">>/tmp/samba/lib/smb.conf", 0, NULL);

	argv[1] = "browseable = yes";
	_eval(argv, ">>/tmp/samba/lib/smb.conf", 0, NULL);

	argv[1] = "guest ok = yes";
	_eval(argv, ">>/tmp/samba/lib/smb.conf", 0, NULL);

	/* Start smbd */
	eval("smbd", "-D");

	/* Set admin password */
	argv[1] = samba_passwd;
	_eval(argv, ">/tmp/spwd", 0, NULL);
	_eval(argv, ">>/tmp/spwd", 0, NULL);
	system("smbpasswd -a admin -s </tmp/spwd");

	return 0;
}

int
stop_samba()
{
	eval("killall", "smbd");
	eval("rm", "-r", "/tmp/samba/var/locks");
	eval("rm", "/tmp/samba/private/passdb.tdb");

	enable_gro(0);

	return 0;
}
#endif /* __CONFIG_SAMBA__ */

#ifdef __CONFIG_DLNA_DMR__
int start_dlna_dmr()
{
	char *dlna_dmr_enable = nvram_safe_get("dlna_dmr_enable");

	if (strcmp(dlna_dmr_enable, "1") == 0) {
		cprintf("Start bcmmrenderer.\n");
		eval("sh", "-c", "bcmmrenderer&");
	}
}

int stop_dlna_dmr()
{
	cprintf("Stop bcmmrenderer.\n");
	eval("killall", "bcmmrenderer");
}
#endif /* __CONFIG_DLNA_DMR__ */


#ifdef __CONFIG_WL_ACI__
int start_wl_aci(void)
{
	int ret = 0;

	/*
	 * If the ACI daemon is enabled, start the ACI daemon.  If not
	 *  simply return
	 */
	if (!nvram_match("acs_daemon", "up"))
		return 0;

	ret = eval("acs");
	return ret;
}

int
stop_wl_aci(void)
{
	int ret;

	ret = eval("killall", "acs");
	return ret;
}
#endif /* __CONFIG_WL_ACI__ */

#ifdef RWL_SOCKET
int
start_server_socket(void)
{
	pid_t pid;
	char *argv[3];
	char *lan_ifname = nvram_safe_get("lan_ifname");

	if (!strcmp(lan_ifname, "")) {
		lan_ifname = "br0";
	}
	argv[0] = "/usr/sbin/wl_server_socket";
	argv[1] = lan_ifname;
	argv[2] = NULL;
	_eval(argv, NULL, 0, &pid);

	return 0;
}

int
stop_server_socket(void)
{
	int ret = eval("killall", "wl_server_socket");

	return ret;
}
#endif /* RWL_SOCKET */

int
start_services(void)
{
/*
*/
#ifdef __CONFIG_IPV6__
	start_ipv6();
#endif /* __CONFIG_IPV6__ */
/*
*/

	start_httpd();
#ifdef __CONFIG_NAT__
	start_dns();
	start_dhcpd();
#ifdef __CONFIG_UPNP__
	start_upnp();
#endif
#ifdef	__CONFIG_IGD__
	start_igd();
#endif
#endif	/* __CONFIG_NAT__ */
	start_eapd();
	start_nas();
#ifdef __CONFIG_WAPI_IAS__
	start_ias();
#endif /* __CONFIG_WAPI_IAS__ */
#ifdef __CONFIG_WAPI__
	start_wapid();
#endif /* __CONFIG_WAPI__ */
	start_wps();
#ifdef __CONFIG_UTELNETD__
	start_telnet();
#endif
#ifdef __CONFIG_IGMP_PROXY__
	start_igmp_proxy();
#endif /* __CONFIG_IGMP_PROXY__ */
#ifdef __CONFIG_WL_ACI__
	start_wl_aci();
#endif /* __CONFIG_WL_ACI__ */
#ifdef __CONFIG_LLD2D__
	start_lltd();
#endif /* __CONFIG_LLD2D__ */
#if defined(PHYMON)
	start_phymons();
#endif /* PHYMON */
#if defined(BCM_DCS) || defined(EXT_ACS)
	start_acsd();
#endif
#ifdef __CONFIG_SAMBA__
	start_samba();
#endif

#ifdef __CONFIG_DLNA_DMR__
	start_dlna_dmr();
#endif

#ifdef RWL_SOCKET
	start_server_socket();
#endif

#ifdef PLC
	if (target_uses_gigled()) {
		start_plcnvm();
		start_plcboot();
		start_gigled();
	}
#endif

	dprintf("done\n");
	return 0;
}

int
stop_services(void)
{
	stop_wps();
#ifdef __CONFIG_WAPI__
	stop_wapid();
#endif /* __CONFIG_WAPI__ */
#ifdef __CONFIG_WAPI_IAS__
	stop_ias();
#endif /* __CONFIG_WAPI_IAS__ */
	stop_nas();
	stop_eapd();
#ifdef __CONFIG_NAT__
#ifdef	__CONFIG_IGD__
	stop_igd();
#endif
#ifdef	__CONFIG_UPNP__
	stop_upnp();
#endif
	stop_dhcpd();
	stop_dns();
#endif	/* __CONFIG_NAT__ */
	stop_httpd();
#ifdef __CONFIG_UTELNETD__
	stop_telnet();
#endif
#ifdef __CONFIG_IGMP_PROXY__
	stop_igmp_proxy();
#endif /* __CONFIG_IGMP_PROXY__ */
#ifdef __CONFIG_WL_ACI__
	stop_wl_aci();
#endif /* __CONFIG_WL_ACI__ */
#ifdef __CONFIG_LLD2D__
	stop_lltd();
#endif 	/* __CONFIG_LLD2D__ */
/*
*/
#ifdef __CONFIG_IPV6__
	stop_ipv6();
#endif /* __CONFIG_IPV6__ */
/*
*/

#if defined(PHYMON)
	stop_phymons();
#endif /* PHYMON */
#if defined(BCM_DCS) || defined(EXT_ACS)
	stop_acsd();
#endif
#ifdef __CONFIG_SAMBA__
	stop_samba();
#endif
#ifdef __CONFIG_DLNA_DMR__
	stop_dlna_dmr();
#endif

#ifdef RWL_SOCKET
	stop_server_socket();
#endif

#ifdef PLC
	if (target_uses_gigled()) {
		stop_gigled();
		stop_plcboot();
		stop_plcnvm();
	}
#endif

	dprintf("done\n");
	return 0;
}

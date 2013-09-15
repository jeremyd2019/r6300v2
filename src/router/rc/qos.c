/*

	Tomato Firmware
	Copyright (C) 2006-2007 Jonathan Zarate
	$Id: qos.c 241182 2011-02-17 21:50:03Z $

*/
#include "rc.h"
#include <sys/stat.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <syslog.h>
#include <fcntl.h>
#include <bcmnvram.h>
#include <shutils.h>


#define vstrsep(buf, sep, args...) _vstrsep(buf, sep, args, NULL)


static const char *qosfn = "/tmp/qos/qos";

static unsigned calc(unsigned bw, unsigned pct)
{
	unsigned n = ((unsigned long)bw * pct) / 100;
	return (n < 2) ? 2 : n;
}

typedef struct sEVAL_CMD {
	int siCount;
	char *apCommand[64];
}	EVAL_CMD;

void printcmd(EVAL_CMD *ptIptCommand)
{	int i;

	printf("\n ### IPTABLE_CMD(%d): \"", ptIptCommand->siCount);
	for (i = 0; i < ptIptCommand->siCount; i++)
		if (ptIptCommand->apCommand[i])
			printf("%s ", ptIptCommand->apCommand[i]);
	printf("\"\n");
}

void del_iQosRules(void)
{
	/* Flush all rules in mangle table */
	eval("iptables", "-t", "mangle", "-F");
}

int add_iQosRules(char *pcWANIF)
{
	char *buf;
	char *g;
	char *p;
	char *addr_type, *addr;
	char *proto;
	char *port_type, *port;
	char *class_prio;
	char *ipp2p, *layer7;
	char *bcount;
	int class_num;
	int proto_num;
	int i;
	char s[256];
	int inuse;
	int qosox_enable;
	unsigned long min;
	int bcount_enable;
	int method;
	int gum;
	int sticky_enable;
	char acClass[8];
	char acByteCounter[32];
	EVAL_CMD stIptCommand =
	{ 3,
		{
		"iptables",
		"-t",
		"mangle",
		NULL,
		}
	};

	if (pcWANIF == NULL || !nvram_match("qos_enable", "1"))
		return -1;
		printf("\n%s: %s", __FUNCTION__, pcWANIF);

	qosox_enable = bcount_enable = inuse = sticky_enable = 0;
	method = atoi(nvram_safe_get("qos_method"));		  // strict rule ordering
	gum = (method == 0) ? 0x100 : 0;
	if (nvram_match("qos_sticky", "0"))
		sticky_enable = 1;

	eval("iptables", "-t", "mangle", "-N", "QOSO");
	eval("iptables", "-t", "mangle", "-A", "QOSO", "-j",
		"CONNMARK", "--restore-mark", "--mask", "0xff");
	eval("iptables", "-t", "mangle", "-A", "QOSO", "-m",
		"connmark", "!", "--mark", "0/0xff00", "-j", "RETURN");

	g = buf = strdup(nvram_safe_get("qos_orules"));
	while (g) {
		/*	addr_type<addr<proto<port_type<port<<<<desc
		addr_type:
			0 = any
			1 = dest ip
			2 = src ip
			3 = src mac
		addr:
			ip/mac if addr_type == 1-3
		proto:
			0-65535 = protocol
			-1 = tcp or udp
			-2 = any protocol
		port_type:
			if proto == -1,tcp,udp:
				d = dest
				s = src
				x = both
				a = any
		port:
			port # if proto == -1,tcp,udp
		class_prio:
			0-4, 0 being highest
		*/
		if ((p = strsep(&g, ">")) == NULL)
			break;
		i = vstrsep(p, "<", &addr_type, &addr, &proto, &port_type,
			&port, &ipp2p, &layer7, &bcount, &class_prio, &p);
		if (i == 9) {
			class_prio = bcount;
			bcount = NULL;
		}
		else if (i != 10)
			continue;

		class_num = atoi(class_prio);
		if ((class_num < 0) || (class_num > 4))
			continue;
		i = 1 << class_num++;
		if (method == 1) class_num |= 0x200;
		if ((inuse & i) == 0) {
			inuse |= i;
			printf("inuse=%d\n", inuse);
		}

		/* Beginning of the Rule */
		{
			stIptCommand.siCount = 3;
			stIptCommand.apCommand[stIptCommand.siCount++] = "-A";
			if (qosox_enable)
				stIptCommand.apCommand[stIptCommand.siCount++] = "QOSOX"; //Offset 4
			else
			stIptCommand.apCommand[stIptCommand.siCount++] = "QOSO";
		}

		/*
		*	protocol & ports:
		*/

	{
			proto_num = atoi(proto);
		if (proto_num > -2) {
			stIptCommand.apCommand[stIptCommand.siCount++] = "-p";
		if (proto_num == 17)
			stIptCommand.apCommand[stIptCommand.siCount++] = "udp"; //Offset 6
		else if (proto_num != -1)
			stIptCommand.apCommand[stIptCommand.siCount++] = proto;
		else
			stIptCommand.apCommand[stIptCommand.siCount++] = "tcp"; //Offset 6

		if ((proto_num == 6) || (proto_num == 17) || (proto_num == -1)) {
				if (*port_type != 'a') {
				if ((*port_type == 'x') || (strchr(port, ','))) {
		// dst-or-src port matches, and anything with multiple lists "," use mport
			stIptCommand.apCommand[stIptCommand.siCount++] = "-m";
			stIptCommand.apCommand[stIptCommand.siCount++] = "mport";
				if (*port_type == 's')
			stIptCommand.apCommand[stIptCommand.siCount++] = "--sports";
				else
			stIptCommand.apCommand[stIptCommand.siCount++] = "--dports";
					}
					else {
				if (*port_type == 's')
			stIptCommand.apCommand[stIptCommand.siCount++] = "--sport";
				else if (*port_type == 'd')
			stIptCommand.apCommand[stIptCommand.siCount++] = "--dport";
				else
			stIptCommand.apCommand[stIptCommand.siCount++] = "--port";
					}
				if (port && *port)
			stIptCommand.apCommand[stIptCommand.siCount++] = port;
					}
				}
			}
		}

		/*
		*	MAC or IP address match:
		*/
	{
		if ((*addr_type == '1') || (*addr_type == '2')) {	// match ip
			if (strchr(addr, '-') != NULL) {
				stIptCommand.apCommand[stIptCommand.siCount++] = "-m";
				stIptCommand.apCommand[stIptCommand.siCount++] = "iprange";
			if (*addr_type == '1')
				stIptCommand.apCommand[stIptCommand.siCount++] = "--dst-range";
			else
				stIptCommand.apCommand[stIptCommand.siCount++] = "--src-range";
			}
			else {
				if (*addr_type == '1')
				stIptCommand.apCommand[stIptCommand.siCount++] = "-d";
			else
				stIptCommand.apCommand[stIptCommand.siCount++] = "-s";
			}
	}
			else if (*addr_type == '3') {	// match mac
				stIptCommand.apCommand[stIptCommand.siCount++] = "-m";
				stIptCommand.apCommand[stIptCommand.siCount++] = "mac";
				stIptCommand.apCommand[stIptCommand.siCount++] = "--mac-source";
			}
			if (*addr_type != '0')
				stIptCommand.apCommand[stIptCommand.siCount++] = addr;
		}

		/* End of the rule */
		{
			class_num |= gum;
			if (sticky_enable)
				class_num &= 0xFF;
			sprintf(acClass, "0x%x/0xFF", class_num);
			stIptCommand.apCommand[stIptCommand.siCount++] = "-j";
			stIptCommand.apCommand[stIptCommand.siCount++] = "CONNMARK";
			stIptCommand.apCommand[stIptCommand.siCount++] = "--set-return";
			stIptCommand.apCommand[stIptCommand.siCount++] = acClass;
			stIptCommand.apCommand[stIptCommand.siCount++] = NULL;
		}
		printcmd(&stIptCommand);
		_eval(stIptCommand.apCommand, NULL, 4, NULL);
		if (proto_num == -1) {
			stIptCommand.apCommand[6] = "udp";
			printcmd(&stIptCommand);
			_eval(stIptCommand.apCommand, NULL, 4, NULL);
		}
	}
	free(buf);


	/*
	*	The default class:
	*/

	{	char acClass[4];

		i = atoi(nvram_safe_get("qos_default"));
		if ((i < 0) || (i > 9)) i = 3;	// "low"
		class_num = i + 1;
		if (method == 1) class_num |= 0x200;
		sprintf(acClass, "0x%x", class_num);
		eval("iptables", "-t", "mangle", "-A", "QOSO",
		"-j", "CONNMARK", "--set-return", acClass);
	}

		eval("iptables", "-t", "mangle", "-A", "FORWARD", "-o", pcWANIF, "-j", "QOSO");

	/*
	*	Ingress rules:
	*/
	{	i = atoi(nvram_safe_get("qos_default"));
		inuse |= (1 << i) | 1;	// default and highest are always built
		sprintf(s, "%d", inuse);
		nvram_set("qos_inuse", s);	  // create the inuse NVRAM here

		g = buf = strdup(nvram_safe_get("qos_irates"));
		for (i = 0; i < 5; i++) {
			if ((!g) || ((p = strsep(&g, ",")) == NULL)) continue;
			if ((inuse & (1 << i)) == 0) continue;
			if (atoi(p) > 0) {// if ibound rules are set, use the PRE-ROUTE
				eval("iptables", "-t", "mangle", "-A", "PREROUTING",
				"-i", pcWANIF, "-j", "CONNMARK",
				"--restore-mark", "--mask", "0xff");
				break;
			}
		}
		free(buf);
	}

}

/* Tc */
int start_iQos(void)
{
	int i;
	char *buf, *g, *p;
	unsigned int rate;
	unsigned int ceil;
	unsigned int bw;
	unsigned int mtu;
	FILE *f;
	int x;
	int inuse;
	char s[256];
	int first;
	char burst_root[32];
	char burst_leaf[32];

	if (!nvram_match("qos_enable", "1")) return;
	if ((f = fopen(qosfn, "w")) == NULL) return;

	i = atoi(nvram_safe_get("qos_burst0"));
	if (i > 0) sprintf(burst_root, "burst %dk", i);
		else burst_root[0] = 0;
	i = atoi(nvram_safe_get("qos_burst1"));

	if (i > 0) sprintf(burst_leaf, "burst %dk", i);
		else burst_leaf[0] = 0;
	/* Egress OBW  -- set the HTB shaper (Classful Qdisc)  
	* the BW is set here for each class 
	*/

	mtu = strtoul(nvram_safe_get("wan_mtu"), NULL, 10);
	bw = strtoul(nvram_safe_get("qos_obw"), NULL, 10);

	fprintf(f,
		"#!/bin/sh\n"
		"I=%s\n"
		"SFQ=\"sfq perturb 10\"\n"
		"TQA=\"tc qdisc add dev $I\"\n"
		"TCA=\"tc class add dev $I\"\n"
		"TFA=\"tc filter add dev $I\"\n"
		"\n"
		"case \"$1\" in\n"
		"start)\n"
		"\ttc qdisc del dev $I root 2>/dev/null\n"
		"\t$TQA root handle 1: htb default %u\n"
		"\t$TCA parent 1: classid 1:1 htb rate %ukbit ceil %ukbit %s\n",
		nvram_safe_get("wan_ifname"),
		(atoi(nvram_safe_get("qos_default")) + 1) * 10,	bw, bw, burst_root);
		inuse = atoi(nvram_safe_get("qos_inuse"));

	g = buf = strdup(nvram_safe_get("qos_orates"));
	for (i = 0; i < 10; ++i) {
	if ((!g) || ((p = strsep(&g, ",")) == NULL)) break;
		if ((inuse & (1 << i)) == 0) continue;
		if ((sscanf(p, "%u-%u", &rate, &ceil) != 2) || (rate < 1)) continue;
		if (ceil > 0) sprintf(s, "ceil %ukbit ", calc(bw, ceil));
			else s[0] = 0;
		x = (i + 1) * 10;
		fprintf(f,
			"# egress %d: %u-%u%%\n"
			"\t$TCA parent 1:1 classid 1:%d htb rate %ukbit %s %s prio %d quantum %u\n"
			"\t$TQA parent 1:%d handle %d: $SFQ\n"
			"\t$TFA parent 1: prio %d protocol ip handle %d fw flowid 1:%d\n",
				i, rate, ceil,
				x, calc(bw, rate), s, burst_leaf, (i >= 6) ? 7 : (i + 1), mtu,
				x, x,
				x, i + 1, x);
	}
	free(buf);


	if (nvram_match("qos_ack", "1")) {

		fprintf(f,
			"\n"
			"\t$TFA parent 1: prio 15 protocol ip u32 "
			"match ip protocol 6 0xff "		// TCP
			"match u8 0x05 0x0f at 0 "		// IP header length
			"match u16 0x0000 0xffc0 at 2 "	// total length(0-63)
			"match u8 0x10 0xff at 33 "		// ACK only
			"flowid 1:10\n");

	}
	if (nvram_match("qos_icmp", "1")) {

		fputs("\n\t$TFA parent 1: prio 14 protocol ip u32 match"
			"ip protocol 1 0xff flowid 1:10\n", f);
		fputs("\n\t$TFA parent 1: prio 14 protocol ip u32 match"
			"ip protocol 1 0xff flowid 1:10\n", stderr);
	}

	// ingress
	first = 1;
	bw = strtoul(nvram_safe_get("qos_ibw"), NULL, 10);
	if (bw > 0) {
	g = buf = strdup(nvram_safe_get("qos_irates"));
	for (i = 0; i < 10; ++i) {
		if ((!g) || ((p = strsep(&g, ",")) == NULL)) break;
		if ((inuse & (1 << i)) == 0) continue;
		if ((rate = atoi(p)) < 1) continue;	// 0 = off

		if (first) {
			first = 0;
			fprintf(f,
				"\n"
				"\ttc qdisc del dev $I ingress 2>/dev/null\n"
				"\t$TQA handle ffff: ingress\n");
		}

		// rate in kb/s
		unsigned int u = calc(bw, rate);

		// burst rate
		unsigned int v = u / 25;
		if (v < 50) v = 50;

		x = i + 1;
		fprintf(f,
			"# ingress %d: %u%%\n"
			"\t$TFA parent ffff: prio %d protocol ip handle %d"
				" fw police rate %ukbit burst %ukbit drop flowid ffff:%d\n",
				i, rate, x, x, u, v, x);
	}

}

	free(buf);

	fputs(
		"\t;;\n"
		"stop)\n"
		"\ttc qdisc del dev $I root 2>/dev/null\n"
		"\ttc qdisc del dev $I ingress 2>/dev/null\n"
		"\t;;\n"
		"*)\n"
		"\ttc -s -d qdisc ls dev $I\n"
		"\techo\n"
		"\ttc -s -d class ls dev $I\n"
		"esac\n",
		f);

	fclose(f);
	chmod(qosfn, 0700);
	eval((char *)qosfn, "start");

}


void stop_iQos(void)
{
		eval((char *)qosfn, "stop");
}

/* va_list is part of stdarg.h */						   
int _vstrsep(char *buf, const char *sep, ...)
{
	va_list ap;
	char **p;
	int n;
	n = 0;
	va_start(ap, sep);
	while ((p = va_arg(ap, char **)) != NULL) {
		if ((*p = strsep(&buf, sep)) == NULL) break;
		++n;
	}
	va_end(ap);
	return n;
}

/*
 * Broadcom IGD module, InternetGatewayDevice.h
 *
 * Copyright (C) 2011, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: InternetGatewayDevice.h 241182 2011-02-17 21:50:03Z $
 */
#ifndef __INTERNETGATEWAYDEVICE_H__
#define __INTERNETGATEWAYDEVICE_H__

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

#define SOAP_INACTIVE_CONNECTION_STATE_REQUIRED		703
#define SOAP_CONNECTION_SETUP_FAILED			704
#define SOAP_CONNECTION_SETUP_IN_PROGRESS		705
#define SOAP_CONNECTION_NOT_CONFIGURED			706
#define SOAP_DISCONNECT_IN_PROGRESS			707
#define SOAP_INVALID_LAYER2_ADDRESS			708
#define SOAP_INTERNETACCESS_DISABLED			709
#define SOAP_INVALID_CONNECTION_TYPE			710
#define SOAP_CONNECTION_ALREADY_TERMINATED		711
#define	SOAP_SPECIFIED_ARRAY_INDEX_INVALID		713
#define	SOAP_NO_SUCH_ENTRY_IN_ARRAY			714
#define	SOAP_WILD_CARD_NOT_PERMITTED_IN_SRC_IP		715
#define	SOAP_WILD_CARD_NOT_PERMITTED_IN_EXT_PORT	716
#define SOAP_CONFLICT_IN_MAPPING_ENTRY			718
#define SOAP_SAME_PORT_VALUES_REQUIRED			724
#define SOAP_ONLY_PERMANENT_LEASES_SUPPORTED		725
#define SOAP_REMOTE_HOST_ONLY_SUPPORTS_WILDCARD		726
#define SOAP_EXTERNAL_PORT_ONLY_SUPPORTS_WILDCARD	727

/* Control structure */
#define IGD_PM_SIZE         32
#define IGD_PM_DESC_SIZE   256

typedef struct igd_portmap {
	char remote_host[sizeof("255.255.255.255")];
	unsigned short external_port;
	char protocol[8];
	unsigned short internal_port;
	char internal_client[sizeof("255.255.255.255")];
	unsigned int enable;
	char description[IGD_PM_DESC_SIZE];
	unsigned long duration;
	unsigned long book_time;
} IGD_PORTMAP;


#define IGD_WAN_LINK_LOCKS		5

typedef	struct igd_net_s {
	char *lan_ifname;
	char *wan_ifname;
	char *wan_devname;
} IGD_NET;

typedef	struct igd_ctrl_s {
	unsigned int igd_seconds;
	char wan_ifname[IFNAMSIZ];
	char wan_devname[IFNAMSIZ];
	struct in_addr wan_ip;
	int wan_up;
	unsigned int wan_up_time;
	int pm_num;
	int pm_limit;
	IGD_PORTMAP pmlist[1];
} IGD_CTRL;
#define IGD_CTRL_SIZE (sizeof(IGD_CTRL) - sizeof(IGD_PORTMAP))

/* Functions */
int igd_portmap_add
(
	UPNP_CONTEXT *context,
	char *remote_host,
	unsigned short external_port,
	char *protocol,
	unsigned short internal_port,
	char *internal_client,
	unsigned int enable,
	char *description,
	unsigned long duration
);

int igd_portmap_del
(
	UPNP_CONTEXT *context,
	char *remote_host,
	unsigned short external_port,
	char *protocol
);

IGD_PORTMAP *igd_portmap_find
(
	UPNP_CONTEXT *context,
	char *remote_host,
	unsigned short external_port,
	char *protocol
);

unsigned short igd_portmap_num(UPNP_CONTEXT *context);
IGD_PORTMAP *igd_portmap_with_index(UPNP_CONTEXT *context, int index);

/* OSL dependent function */
typedef struct igd_stats {
	unsigned long rx_packets;	/* total packets received       */
	unsigned long tx_packets;	/* total packets transmitted    */
	unsigned long rx_bytes;		/* total bytes received         */
	unsigned long tx_bytes;		/* total bytes transmitted      */
} igd_stats_t;

extern int igd_osl_wan_ifstats(IGD_CTRL *igd_ctrl, igd_stats_t *pstats);
extern unsigned int igd_osl_wan_max_bitrates(char *wan_devname, unsigned long *rx,
	unsigned long *tx);
extern int igd_osl_wan_ip(char *wan_ifname, struct in_addr *inaddr);
extern int igd_osl_wan_isup(char *wan_ifname);
extern void igd_osl_nat_config(char *wan_ifname, IGD_PORTMAP *map);

/* << TABLE BEGIN */
/*
 * WARNNING: DON'T MODIFY THE FOLLOWING TABLES
 * AND DON'T REMOVE TAG :
 *          "<< TABLE BEGIN"
 *          ">> TABLE END"
 */

extern UPNP_DEVICE InternetGatewayDevice;

int InternetGatewayDevice_open(UPNP_CONTEXT *context);
int InternetGatewayDevice_close(UPNP_CONTEXT *context);
int InternetGatewayDevice_timeout(UPNP_CONTEXT *context, time_t now);
int InternetGatewayDevice_notify(UPNP_CONTEXT *context, UPNP_SERVICE *service,
	UPNP_SUBSCRIBER *subscriber, int notify);
/* >> TABLE END */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INTERNETGATEWAYDEVICE_H__ */

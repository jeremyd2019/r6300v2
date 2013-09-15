/*
 * Broadcom IGD module, InternetGatewayDevice_table.c
 *
 * Copyright (C) 2011, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: InternetGatewayDevice_table.c 241182 2011-02-17 21:50:03Z $
 */
#include <upnp.h>
#include <InternetGatewayDevice.h>

/* << TABLE BEGIN */
/*
 * WARNNING: DON'T MODIFY THE FOLLOWING TABLES
 * AND DON'T REMOVE TAG :
 *          "<< TABLE BEGIN"
 *          ">> TABLE END"
 */

extern UPNP_ACTION action_x_layer3forwarding[];
extern UPNP_ACTION action_x_wancommoninterfaceconfig[];
extern UPNP_ACTION action_x_wanipconnection[];

extern UPNP_STATE_VAR statevar_x_layer3forwarding[];
extern UPNP_STATE_VAR statevar_x_wancommoninterfaceconfig[];
extern UPNP_STATE_VAR statevar_x_wanipconnection[];

static UPNP_SERVICE InternetGatewayDevice_service [] =
{
	{"/control?Layer3Forwarding",		"/event?Layer3Forwarding",		"urn:schemas-upnp-org:service:Layer3Forwarding",	"L3Forwarding1",	action_x_layer3forwarding,		statevar_x_layer3forwarding},
	{"/control?WANCommonInterfaceConfig",	"/event?WANCommonInterfaceConfig",	"urn:schemas-upnp-org:service:WANCommonInterfaceConfig","WANCommonIFC1",	action_x_wancommoninterfaceconfig,	statevar_x_wancommoninterfaceconfig},
	{"/control?WANIPConnection",		"/event?WANIPConnection",		"urn:schemas-upnp-org:service:WANIPConnection",		"WANIPConn1",		action_x_wanipconnection,		statevar_x_wanipconnection},
	{0,					0,					0,							0,			0,					0}
};


static UPNP_ADVERTISE InternetGatewayDevice_advertise [] =
{
	{"urn:schemas-upnp-org:device:InternetGatewayDevice",		"eb9ab5b2-981c-4401-a20e-b7bcde359dbb", ADVERTISE_ROOTDEVICE},
	{"urn:schemas-upnp-org:service:Layer3Forwarding",		"eb9ab5b2-981c-4401-a20e-b7bcde359dbb",	ADVERTISE_SERVICE},
	{"urn:schemas-upnp-org:device:WANDevice",			"e1f05c9d-3034-4e4c-af82-17cdfbdcc077", ADVERTISE_DEVICE},
	{"urn:schemas-upnp-org:service:WANCommonInterfaceConfig",	"e1f05c9d-3034-4e4c-af82-17cdfbdcc077",	ADVERTISE_SERVICE},
	{"urn:schemas-upnp-org:device:WANConnectionDevice",		"1995cf2d-d4b1-4fdb-bf84-8e59d2066198", ADVERTISE_DEVICE},
	{"urn:schemas-upnp-org:service:WANIPConnection",		"1995cf2d-d4b1-4fdb-bf84-8e59d2066198",	ADVERTISE_SERVICE},
	{0,															""}
};


extern char xml_InternetGatewayDevice[];
extern char xml_x_layer3forwarding[];
extern char xml_x_wancommoninterfaceconfig[];
extern char xml_x_wanipconnection[];

static UPNP_DESCRIPTION InternetGatewayDevice_description [] =
{
	{"/InternetGatewayDevice.xml",          "text/xml",     0,      xml_InternetGatewayDevice},
	{"/x_layer3forwarding.xml",             "text/xml",     0,      xml_x_layer3forwarding},
	{"/x_wancommoninterfaceconfig.xml",     "text/xml",     0,      xml_x_wancommoninterfaceconfig},
	{"/x_wanipconnection.xml",              "text/xml",     0,      xml_x_wanipconnection},
	{0,                                     0,              0,      0}
};

UPNP_DEVICE InternetGatewayDevice =
{
	"InternetGatewayDevice.xml",
	InternetGatewayDevice_service,
	InternetGatewayDevice_advertise,
	InternetGatewayDevice_description,
	InternetGatewayDevice_open,
	InternetGatewayDevice_close,
	InternetGatewayDevice_timeout,
	InternetGatewayDevice_notify
};
/* >> TABLE END */

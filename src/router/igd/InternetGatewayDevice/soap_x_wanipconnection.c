/*
 * Broadcom IGD module, soap_x_wanipconnection.c
 *
 * Copyright (C) 2011, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: soap_x_wanipconnection.c 241182 2011-02-17 21:50:03Z $
 */
#include <upnp.h>
#include <InternetGatewayDevice.h>

/*
 * WARNNING: PLEASE IMPLEMENT YOUR CODES AFTER 
 *          "<< USER CODE START >>"
 * AND DON'T REMOVE TAG :
 *          "<< AUTO GENERATED FUNCTION: "
 *          ">> AUTO GENERATED FUNCTION"
 *          "<< USER CODE START >>"
 */

/* << AUTO GENERATED FUNCTION: statevar_ConnectionType() */
static int
statevar_ConnectionType
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE * service,
	UPNP_TLV * tlv
)
{
	HINT(STR::tlv)

	/* << USER CODE START >> */
	upnp_tlv_set(tlv, (int)"IP_Routed");
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: statevar_PossibleConnectionTypes() */
static int
statevar_PossibleConnectionTypes
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE * service,
	UPNP_TLV * tlv
)
{
	HINT(STR::tlv)

	HINT(char *pUnconfigured = "Unconfigured";)
	HINT(char *pIP_Routed = "IP_Routed";)
	HINT(char *pIP_Bridged = "IP_Bridged";)

	/* << USER CODE START >> */
	upnp_tlv_set(tlv, (int)"IP_Routed");
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: statevar_ConnectionStatus() */
static int
statevar_ConnectionStatus
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE * service,
	UPNP_TLV * tlv
)
{
	HINT(STR::tlv)

	HINT( char *pUnconfigured = "Unconfigured"; )
	HINT( char *pConnected = "Connected"; )
	HINT( char *pDisconnected = "Disconnected"; )

	/* << USER CODE START >> */
	IGD_CTRL *igd_ctrl = (IGD_CTRL *)(context->focus_ifp->devctrl);
	char *connect = igd_ctrl->wan_up ? "Connected" : "Disconnected";

	upnp_tlv_set(tlv, (int)connect);
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: statevar_Uptime() */
static int
statevar_Uptime
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE * service,
	UPNP_TLV * tlv
)
{
	HINT(UI4::tlv)

	/* << USER CODE START >> */
	IGD_CTRL *igd_ctrl = (IGD_CTRL *)(context->focus_ifp->devctrl);

	upnp_tlv_set(tlv, igd_ctrl->wan_up_time);
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: statevar_LastConnectionError() */
static int
statevar_LastConnectionError
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE * service,
	UPNP_TLV * tlv
)
{
	HINT(STR::tlv)

	HINT( char *pERROR_NONE = "ERROR_NONE"; )
	HINT( char *pERROR_UNKNOWN = "ERROR_UNKNOWN"; )

	/* << USER CODE START >> */
	upnp_tlv_set(tlv, (int)"ERROR_NONE");
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: statevar_RSIPAvailable() */
static int
statevar_RSIPAvailable
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE * service,
	UPNP_TLV * tlv
)
{
	HINT(BOOL::tlv)

	/* << USER CODE START >> */
	upnp_tlv_set(tlv, 0);
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: statevar_NATEnabled() */
static int
statevar_NATEnabled
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE * service,
	UPNP_TLV * tlv
)
{
	HINT(BOOL::tlv)

	/* << USER CODE START >> */
	/* We have to check router mode settings */
	upnp_tlv_set(tlv, 1);
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: statevar_PortMappingNumberOfEntries() */
static int
statevar_PortMappingNumberOfEntries
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE * service,
	UPNP_TLV * tlv
)
{
	HINT(UI2::tlv)

	/* << USER CODE START >> */
	upnp_tlv_set(tlv, igd_portmap_num(context));
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: statevar_ExternalIPAddress() */
static int
statevar_ExternalIPAddress
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE * service,
	UPNP_TLV * tlv
)
{
	HINT(STR::tlv)

	/* << USER CODE START >> */
	IGD_CTRL *igd_ctrl = (IGD_CTRL *)(context->focus_ifp->devctrl);

	upnp_tlv_set(tlv, (int)inet_ntoa(igd_ctrl->wan_ip));
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: action_SetConnectionType() */
static int
action_SetConnectionType
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument
)
{
	HINT( STR::UPNP_TLV *in_NewConnectionType = UPNP_IN_TLV("NewConnectionType"); )
	
	/* << USER CODE START >> */
	UPNP_TLV *in_NewConnectionType = UPNP_IN_TLV("NewConnectionType");
	
	if (strcmp(in_NewConnectionType->val.str, "IP_Routed") != 0)
		return SOAP_ARGUMENT_VALUE_INVALID;
	
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: action_GetConnectionTypeInfo() */
static int
action_GetConnectionTypeInfo
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument
)
{
	HINT( STR::UPNP_TLV *out_NewConnectionType = UPNP_OUT_TLV("NewConnectionType"); )
	HINT( STR::UPNP_TLV *out_NewPossibleConnectionTypes = UPNP_OUT_TLV("NewPossibleConnectionTypes"); )

	/* << USER CODE START >> */
	UPNP_TLV *out_NewConnectionType = UPNP_OUT_TLV("NewConnectionType");
	UPNP_TLV *out_NewPossibleConnectionTypes = UPNP_OUT_TLV("NewPossibleConnectionTypes");

	upnp_tlv_set(out_NewConnectionType, (int)"IP_Routed");
	upnp_tlv_set(out_NewPossibleConnectionTypes, (int)"IP_Routed");
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: action_RequestConnection() */
static int
action_RequestConnection
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument
)
{
	/* << USER CODE START >> */
	/* 
	 * For security consideration,
	 * we don't implement it.
	 */
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: action_ForceTermination() */
static int
action_ForceTermination
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument
)
{
	/* << USER CODE START >> */
	/* 
	 * For security consideration,
	 * we don't implement it.
	 */
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: action_GetStatusInfo() */
static int
action_GetStatusInfo
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument
)
{
	HINT( STR::UPNP_TLV *out_NewConnectionStatus = UPNP_OUT_TLV("NewConnectionStatus"); )
	HINT( STR::UPNP_TLV *out_NewLastConnectionError = UPNP_OUT_TLV("NewLastConnectionError"); )
	HINT( UI4::UPNP_TLV *out_NewUptime = UPNP_OUT_TLV("NewUptime"); )
	
	/* << USER CODE START >> */
	UPNP_TLV *out_NewConnectionStatus = UPNP_OUT_TLV("NewConnectionStatus");
	UPNP_TLV *out_NewLastConnectionError = UPNP_OUT_TLV("NewLastConnectionError");
	UPNP_TLV *out_NewUptime = UPNP_OUT_TLV("NewUptime");
	int ret;

	ret = statevar_ConnectionStatus(context, service, out_NewConnectionStatus);
	if (ret != OK)
		return ret;

	ret = statevar_LastConnectionError(context, service, out_NewLastConnectionError);
	if (ret != OK)
		return ret;
    
	return statevar_Uptime(context, service, out_NewUptime);
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: action_GetNATRSIPStatus() */
static int
action_GetNATRSIPStatus
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument
)
{
	HINT( BOOL::UPNP_TLV *out_NewRSIPAvailable = UPNP_OUT_TLV("NewRSIPAvailable"); )
	HINT( BOOL::UPNP_TLV *out_NewNATEnabled = UPNP_OUT_TLV("NewNATEnabled"); )
	
	/* << USER CODE START >> */
	UPNP_TLV *out_NewRSIPAvailable = UPNP_OUT_TLV("NewRSIPAvailable");
	UPNP_TLV *out_NewNATEnabled = UPNP_OUT_TLV("NewNATEnabled");

	upnp_tlv_set(out_NewRSIPAvailable, 0);
	upnp_tlv_set(out_NewNATEnabled, 1);
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: action_GetGenericPortMappingEntry() */
static int
action_GetGenericPortMappingEntry
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument
)
{
	HINT( UI2::UPNP_TLV *in_NewPortMappingIndex = UPNP_IN_TLV("NewPortMappingIndex"); )
	HINT( STR::UPNP_TLV *out_NewRemoteHost = UPNP_OUT_TLV("NewRemoteHost"); )
	HINT( UI2::UPNP_TLV *out_NewExternalPort = UPNP_OUT_TLV("NewExternalPort"); )
	HINT( STR::UPNP_TLV *out_NewProtocol = UPNP_OUT_TLV("NewProtocol"); )
	HINT( UI2::UPNP_TLV *out_NewInternalPort = UPNP_OUT_TLV("NewInternalPort"); )
	HINT( STR::UPNP_TLV *out_NewInternalClient = UPNP_OUT_TLV("NewInternalClient"); )
	HINT( BOOL:UPNP_TLV *out_NewEnabled = UPNP_OUT_TLV("NewEnabled"); )
	HINT( STR::UPNP_TLV *out_NewPortMappingDescription = UPNP_OUT_TLV("NewPortMappingDescription"); )
	HINT( UI4::UPNP_TLV *out_NewLeaseDuration = UPNP_OUT_TLV("NewLeaseDuration"); )
	
	/* << USER CODE START >> */
	UPNP_TLV *in_NewPortMappingIndex = UPNP_IN_TLV("NewPortMappingIndex");
	UPNP_TLV *out_NewRemoteHost = UPNP_OUT_TLV("NewRemoteHost");
	UPNP_TLV *out_NewExternalPort = UPNP_OUT_TLV("NewExternalPort");
	UPNP_TLV *out_NewProtocol = UPNP_OUT_TLV("NewProtocol");
	UPNP_TLV *out_NewInternalPort = UPNP_OUT_TLV("NewInternalPort");
	UPNP_TLV *out_NewInternalClient = UPNP_OUT_TLV("NewInternalClient");
	UPNP_TLV *out_NewEnabled = UPNP_OUT_TLV("NewEnabled");
	UPNP_TLV *out_NewPortMappingDescription = UPNP_OUT_TLV("NewPortMappingDescription");
	UPNP_TLV *out_NewLeaseDuration = UPNP_OUT_TLV("NewLeaseDuration");

	IGD_PORTMAP *map;

	map = igd_portmap_with_index(context, in_NewPortMappingIndex->val.uval);
	if (!map)
		return SOAP_SPECIFIED_ARRAY_INDEX_INVALID;

	upnp_tlv_set(out_NewRemoteHost, (int)map->remote_host);
	upnp_tlv_set(out_NewExternalPort, map->external_port);
	upnp_tlv_set(out_NewProtocol, (int)map->protocol);
	upnp_tlv_set(out_NewInternalPort, map->internal_port);
	upnp_tlv_set(out_NewInternalClient, (int)map->internal_client);
	upnp_tlv_set(out_NewEnabled, map->enable);
	upnp_tlv_set(out_NewPortMappingDescription, (int)map->description);
	upnp_tlv_set(out_NewLeaseDuration, map->duration);
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: action_GetSpecificPortMappingEntry() */
static int
action_GetSpecificPortMappingEntry
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument
)
{
	HINT( STR::UPNP_TLV *in_NewRemoteHost = UPNP_IN_TLV("NewRemoteHost"); )
	HINT( UI2::UPNP_TLV *in_NewExternalPort = UPNP_IN_TLV("NewExternalPort"); )
	HINT( STR::UPNP_TLV *in_NewProtocol = UPNP_IN_TLV("NewProtocol"); )
	HINT( UI2::UPNP_TLV *out_NewInternalPort = UPNP_OUT_TLV("NewInternalPort"); )
	HINT( STR::UPNP_TLV *out_NewInternalClient = UPNP_OUT_TLV("NewInternalClient"); )
	HINT( BOOL::UPNP_TLV *out_NewEnabled = UPNP_OUT_TLV("NewEnabled"); )
	HINT( STR::UPNP_TLV *out_NewPortMappingDescription = UPNP_OUT_TLV("NewPortMappingDescription"); )
	HINT( UI4::UPNP_TLV *out_NewLeaseDuration = UPNP_OUT_TLV("NewLeaseDuration"); )
	
	/* << USER CODE START >> */
	UPNP_TLV *in_NewRemoteHost = UPNP_IN_TLV("NewRemoteHost");
	UPNP_TLV *in_NewExternalPort = UPNP_IN_TLV("NewExternalPort");
	UPNP_TLV *in_NewProtocol = UPNP_IN_TLV("NewProtocol");
	UPNP_TLV *out_NewInternalPort = UPNP_OUT_TLV("NewInternalPort");
	UPNP_TLV *out_NewInternalClient = UPNP_OUT_TLV("NewInternalClient");
	UPNP_TLV *out_NewEnabled = UPNP_OUT_TLV("NewEnabled");
	UPNP_TLV *out_NewPortMappingDescription = UPNP_OUT_TLV("NewPortMappingDescription");
	UPNP_TLV *out_NewLeaseDuration = UPNP_OUT_TLV("NewLeaseDuration");
	
	IGD_PORTMAP *map;

	map = igd_portmap_find(context,
		in_NewRemoteHost->val.str, 
		in_NewExternalPort->val.uval,
		in_NewProtocol->val.str);
	if (!map)
		return SOAP_NO_SUCH_ENTRY_IN_ARRAY;

	upnp_tlv_set(out_NewInternalClient, (int)map->internal_client);
	upnp_tlv_set(out_NewInternalPort, map->internal_port);
	upnp_tlv_set(out_NewEnabled, map->enable);
	upnp_tlv_set(out_NewPortMappingDescription, (int)map->description);
	upnp_tlv_set(out_NewLeaseDuration, map->duration);
	return OK;
}
/* >> AUTO GENERATED FUNCTION */


/* << AUTO GENERATED FUNCTION: action_AddPortMapping() */
static int
action_AddPortMapping
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument
)
{
	HINT( STR::UPNP_TLV *in_NewRemoteHost = UPNP_IN_TLV("NewRemoteHost"); )
	HINT( UI2::UPNP_TLV *in_NewExternalPort = UPNP_IN_TLV("NewExternalPort"); )
	HINT( STR::UPNP_TLV *in_NewProtocol = UPNP_IN_TLV("NewProtocol"); )
	HINT( UI2::UPNP_TLV *in_NewInternalPort = UPNP_IN_TLV("NewInternalPort"); )
	HINT( STR::UPNP_TLV *in_NewInternalClient = UPNP_IN_TLV("NewInternalClient"); )
	HINT( BOOL::UPNP_TLV *in_NewEnabled = UPNP_IN_TLV("NewEnabled"); )
	HINT( STR::UPNP_TLV *in_NewPortMappingDescription = UPNP_IN_TLV("NewPortMappingDescription"); )
	HINT( UI4::UPNP_TLV *in_NewLeaseDuration = UPNP_IN_TLV("NewLeaseDuration"); )

	/* << USER CODE START >> */
	UPNP_TLV *in_NewRemoteHost = UPNP_IN_TLV("NewRemoteHost");
	UPNP_TLV *in_NewExternalPort = UPNP_IN_TLV("NewExternalPort");
	UPNP_TLV *in_NewProtocol = UPNP_IN_TLV("NewProtocol");
	UPNP_TLV *in_NewInternalPort = UPNP_IN_TLV("NewInternalPort");
	UPNP_TLV *in_NewInternalClient = UPNP_IN_TLV("NewInternalClient");
	UPNP_TLV *in_NewEnabled = UPNP_IN_TLV("NewEnabled");
	UPNP_TLV *in_NewPortMappingDescription = UPNP_IN_TLV("NewPortMappingDescription");
	UPNP_TLV *in_NewLeaseDuration = UPNP_IN_TLV("NewLeaseDuration");
	char buf[64];
	char *headers[1] = {buf};
	int error;

	error = igd_portmap_add(context,
		in_NewRemoteHost->val.str,
		in_NewExternalPort->val.uval,
		in_NewProtocol->val.str,
		in_NewInternalPort->val.uval,
		in_NewInternalClient->val.str,
		in_NewEnabled->val.ival,
		in_NewPortMappingDescription->val.str,
		in_NewLeaseDuration->val.uval);
	if (error)
		return error;

	/* Update "PortMappingNumberOfEntries" */
	sprintf(buf, "%s=%d", "PortMappingNumberOfEntries", igd_portmap_num(context));
	gena_event_alarm(context, service->name, 1, headers, 0);
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: action_DeletePortMapping() */
static int
action_DeletePortMapping
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument
)
{
	HINT( STR::UPNP_TLV *in_NewRemoteHost = UPNP_IN_TLV("NewRemoteHost"); )
	HINT( UI2::UPNP_TLV *in_NewExternalPort = UPNP_IN_TLV("NewExternalPort"); )
	HINT( STR::UPNP_TLV *in_NewProtocol = UPNP_IN_TLV("NewProtocol"); )
	
	/* << USER CODE START >> */
	UPNP_TLV *in_NewRemoteHost = UPNP_IN_TLV("NewRemoteHost");
	UPNP_TLV *in_NewExternalPort = UPNP_IN_TLV("NewExternalPort");
	UPNP_TLV *in_NewProtocol = UPNP_IN_TLV("NewProtocol");
	char buf[64];
	char *headers[1] = {buf};
	int error;

	error = igd_portmap_del(context,
		in_NewRemoteHost->val.str,
		in_NewExternalPort->val.uval,
		in_NewProtocol->val.str);
	if (error)
		return error;

	/* Update "PortMappingNumberOfEntries" */
	sprintf(buf, "%s=%d", "PortMappingNumberOfEntries", igd_portmap_num(context));
	gena_event_alarm(context, service->name, 1, headers, 0);
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: action_GetExternalIPAddress() */
static int
action_GetExternalIPAddress
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument
)
{
	HINT( STR::UPNP_TLV *out_NewExternalIPAddress = UPNP_OUT_TLV("NewExternalIPAddress"); )
	
	/* << USER CODE START >> */
	UPNP_TLV *out_NewExternalIPAddress = UPNP_OUT_TLV("NewExternalIPAddress");
	
	return statevar_ExternalIPAddress(context, service, out_NewExternalIPAddress);
}
/* >> AUTO GENERATED FUNCTION */


/* << TABLE BEGIN */
/*
 * WARNNING: DON'T MODIFY THE FOLLOWING TABLES
 * AND DON'T REMOVE TAG :
 *          "<< TABLE BEGIN"
 *          ">> TABLE END"
 */

#define	STATEVAR_CONNECTIONSTATUS           0
#define	STATEVAR_CONNECTIONTYPE             1
#define	STATEVAR_EXTERNALIPADDRESS          2
#define	STATEVAR_EXTERNALPORT               3
#define	STATEVAR_INTERNALCLIENT             4
#define	STATEVAR_INTERNALPORT               5
#define	STATEVAR_LASTCONNECTIONERROR        6
#define	STATEVAR_NATENABLED                 7
#define	STATEVAR_PORTMAPPINGDESCRIPTION     8
#define	STATEVAR_PORTMAPPINGENABLED         9
#define	STATEVAR_PORTMAPPINGLEASEDURATION   10
#define	STATEVAR_PORTMAPPINGNUMBEROFENTRIES 11
#define	STATEVAR_PORTMAPPINGPROTOCOL        12
#define	STATEVAR_POSSIBLECONNECTIONTYPES    13
#define	STATEVAR_RSIPAVAILABLE              14
#define	STATEVAR_REMOTEHOST                 15
#define	STATEVAR_UPTIME                     16

/* State Variable Table */
UPNP_STATE_VAR statevar_x_wanipconnection[] =
{
	{0, "ConnectionStatus",           UPNP_TYPE_STR,    &statevar_ConnectionStatus,           1},
	{0, "ConnectionType",             UPNP_TYPE_STR,    &statevar_ConnectionType,             0},
	{0, "ExternalIPAddress",          UPNP_TYPE_STR,    &statevar_ExternalIPAddress,          1},
	{0, "ExternalPort",               UPNP_TYPE_UI2,    0,                                    0},
	{0, "InternalClient",             UPNP_TYPE_STR,    0,                                    0},
	{0, "InternalPort",               UPNP_TYPE_UI2,    0,                                    0},
	{0, "LastConnectionError",        UPNP_TYPE_STR,    &statevar_LastConnectionError,        0},
	{0, "NATEnabled",                 UPNP_TYPE_BOOL,   &statevar_NATEnabled,                 0},
	{0, "PortMappingDescription",     UPNP_TYPE_STR,    0,                                    0},
	{0, "PortMappingEnabled",         UPNP_TYPE_BOOL,   0,                                    0},
	{0, "PortMappingLeaseDuration",   UPNP_TYPE_UI4,    0,                                    0},
	{0, "PortMappingNumberOfEntries", UPNP_TYPE_UI2,    &statevar_PortMappingNumberOfEntries, 1},
	{0, "PortMappingProtocol",        UPNP_TYPE_STR,    0,                                    0},
	{0, "PossibleConnectionTypes",    UPNP_TYPE_STR,    &statevar_PossibleConnectionTypes,    1},
	{0, "RSIPAvailable",              UPNP_TYPE_BOOL,   &statevar_RSIPAvailable,              0},
	{0, "RemoteHost",                 UPNP_TYPE_STR,    0,                                    0},
	{0, "Uptime",                     UPNP_TYPE_UI4,    &statevar_Uptime,                     0},
	{0, 0,                            0,                0,                                    0}
};

/* Action Table */
static ACTION_ARGUMENT arg_in_SetConnectionType [] =
{
	{"NewConnectionType",             UPNP_TYPE_STR,    STATEVAR_CONNECTIONTYPE}
};

static ACTION_ARGUMENT arg_out_GetConnectionTypeInfo [] =
{
	{"NewConnectionType",             UPNP_TYPE_STR,    STATEVAR_CONNECTIONTYPE},
	{"NewPossibleConnectionTypes",    UPNP_TYPE_STR,    STATEVAR_POSSIBLECONNECTIONTYPES}
};

static ACTION_ARGUMENT arg_out_GetStatusInfo [] =
{
	{"NewConnectionStatus",           UPNP_TYPE_STR,    STATEVAR_CONNECTIONSTATUS},
	{"NewLastConnectionError",        UPNP_TYPE_STR,    STATEVAR_LASTCONNECTIONERROR},
	{"NewUptime",                     UPNP_TYPE_UI4,    STATEVAR_UPTIME}
};

static ACTION_ARGUMENT arg_out_GetNATRSIPStatus [] =
{
	{"NewRSIPAvailable",              UPNP_TYPE_BOOL,   STATEVAR_RSIPAVAILABLE},
	{"NewNATEnabled",                 UPNP_TYPE_BOOL,   STATEVAR_NATENABLED}
};

static ACTION_ARGUMENT arg_in_GetGenericPortMappingEntry [] =
{
	{"NewPortMappingIndex",           UPNP_TYPE_UI2,    STATEVAR_PORTMAPPINGNUMBEROFENTRIES}
};

static ACTION_ARGUMENT arg_out_GetGenericPortMappingEntry [] =
{
	{"NewRemoteHost",                 UPNP_TYPE_STR,    STATEVAR_REMOTEHOST},
	{"NewExternalPort",               UPNP_TYPE_UI2,    STATEVAR_EXTERNALPORT},
	{"NewProtocol",                   UPNP_TYPE_STR,    STATEVAR_PORTMAPPINGPROTOCOL},
	{"NewInternalPort",               UPNP_TYPE_UI2,    STATEVAR_INTERNALPORT},
	{"NewInternalClient",             UPNP_TYPE_STR,    STATEVAR_INTERNALCLIENT},
	{"NewEnabled",                    UPNP_TYPE_BOOL,   STATEVAR_PORTMAPPINGENABLED},
	{"NewPortMappingDescription",     UPNP_TYPE_STR,    STATEVAR_PORTMAPPINGDESCRIPTION},
	{"NewLeaseDuration",              UPNP_TYPE_UI4,    STATEVAR_PORTMAPPINGLEASEDURATION}
};

static ACTION_ARGUMENT arg_in_GetSpecificPortMappingEntry [] =
{
	{"NewRemoteHost",                 UPNP_TYPE_STR,    STATEVAR_REMOTEHOST},
	{"NewExternalPort",               UPNP_TYPE_UI2,    STATEVAR_EXTERNALPORT},
	{"NewProtocol",                   UPNP_TYPE_STR,    STATEVAR_PORTMAPPINGPROTOCOL}
};

static ACTION_ARGUMENT arg_out_GetSpecificPortMappingEntry [] =
{
	{"NewInternalPort",               UPNP_TYPE_UI2,    STATEVAR_INTERNALPORT},
	{"NewInternalClient",             UPNP_TYPE_STR,    STATEVAR_INTERNALCLIENT},
	{"NewEnabled",                    UPNP_TYPE_BOOL,   STATEVAR_PORTMAPPINGENABLED},
	{"NewPortMappingDescription",     UPNP_TYPE_STR,    STATEVAR_PORTMAPPINGDESCRIPTION},
	{"NewLeaseDuration",              UPNP_TYPE_UI4,    STATEVAR_PORTMAPPINGLEASEDURATION}
};

static ACTION_ARGUMENT arg_in_AddPortMapping [] =
{
	{"NewRemoteHost",                 UPNP_TYPE_STR,    STATEVAR_REMOTEHOST},
	{"NewExternalPort",               UPNP_TYPE_UI2,    STATEVAR_EXTERNALPORT},
	{"NewProtocol",                   UPNP_TYPE_STR,    STATEVAR_PORTMAPPINGPROTOCOL},
	{"NewInternalPort",               UPNP_TYPE_UI2,    STATEVAR_INTERNALPORT},
	{"NewInternalClient",             UPNP_TYPE_STR,    STATEVAR_INTERNALCLIENT},
	{"NewEnabled",                    UPNP_TYPE_BOOL,   STATEVAR_PORTMAPPINGENABLED},
	{"NewPortMappingDescription",     UPNP_TYPE_STR,    STATEVAR_PORTMAPPINGDESCRIPTION},
	{"NewLeaseDuration",              UPNP_TYPE_UI4,    STATEVAR_PORTMAPPINGLEASEDURATION}
};

static ACTION_ARGUMENT arg_in_DeletePortMapping [] =
{
	{"NewRemoteHost",                 UPNP_TYPE_STR,    STATEVAR_REMOTEHOST},
	{"NewExternalPort",               UPNP_TYPE_UI2,    STATEVAR_EXTERNALPORT},
	{"NewProtocol",                   UPNP_TYPE_STR,    STATEVAR_PORTMAPPINGPROTOCOL}
};

static ACTION_ARGUMENT arg_out_GetExternalIPAddress [] =
{
	{"NewExternalIPAddress",          UPNP_TYPE_STR,    STATEVAR_EXTERNALIPADDRESS}
};

UPNP_ACTION action_x_wanipconnection[] =
{
	{"AddPortMapping",              8,  arg_in_AddPortMapping,              0,  0,                                      &action_AddPortMapping},
	{"DeletePortMapping",           3,  arg_in_DeletePortMapping,           0,  0,                                      &action_DeletePortMapping},
	{"ForceTermination",            0,  0,                                  0,  0,                                      &action_ForceTermination},
	{"GetConnectionTypeInfo",       0,  0,                                  2,  arg_out_GetConnectionTypeInfo,          &action_GetConnectionTypeInfo},
	{"GetExternalIPAddress",        0,  0,                                  1,  arg_out_GetExternalIPAddress,           &action_GetExternalIPAddress},
	{"GetGenericPortMappingEntry",  1,  arg_in_GetGenericPortMappingEntry,  8,  arg_out_GetGenericPortMappingEntry,     &action_GetGenericPortMappingEntry},
	{"GetNATRSIPStatus",            0,  0,                                  2,  arg_out_GetNATRSIPStatus,               &action_GetNATRSIPStatus},
	{"GetSpecificPortMappingEntry", 3,  arg_in_GetSpecificPortMappingEntry, 5,  arg_out_GetSpecificPortMappingEntry,    &action_GetSpecificPortMappingEntry},
	{"GetStatusInfo",               0,  0,                                  3,  arg_out_GetStatusInfo,                  &action_GetStatusInfo},
	{"RequestConnection",           0,  0,                                  0,  0,                                      &action_RequestConnection},
	{"SetConnectionType",           1,  arg_in_SetConnectionType,           0,  0,                                      &action_SetConnectionType},
	{0,                             0,  0,                                  0,  0,                                      0}
};
/* >> TABLE END */

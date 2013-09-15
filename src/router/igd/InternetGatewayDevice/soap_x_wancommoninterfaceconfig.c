/*
 * Broadcom IGD module, soap_x_wancommoniniterfaceconfig.c
 *
 * Copyright (C) 2011, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: soap_x_wancommoninterfaceconfig.c 241182 2011-02-17 21:50:03Z $
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

/* << AUTO GENERATED FUNCTION: statevar_WANAccessType() */
static int
statevar_WANAccessType
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE * service,
	UPNP_TLV * tlv
)
{
	HINT(STR::tlv)

	HINT(char *pDSL = "DSL";)
	HINT(char *pPOTS = "POTS";)
	HINT(char *pCable = "Cable";)
	HINT(char *pEthernet = "Ethernet";)
	HINT(char *pOther = "Other";)

	/* << USER CODE START >> */
	upnp_tlv_set(tlv, (int)"Ethernet");
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: statevar_Layer1UpstreamMaxBitRate() */
static int
statevar_Layer1UpstreamMaxBitRate
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE * service,
	UPNP_TLV * tlv
)
{
	HINT(UI4::tlv)

	/* << USER CODE START >> */
	unsigned long tx, rx;
	IGD_CTRL *igd_ctrl = (IGD_CTRL *)(context->focus_ifp->devctrl);

	igd_osl_wan_max_bitrates(igd_ctrl->wan_devname, &rx, &tx);

	upnp_tlv_set(tlv, tx);
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: statevar_Layer1DownstreamMaxBitRate() */
static int
statevar_Layer1DownstreamMaxBitRate
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE * service,
	UPNP_TLV * tlv
)
{
	HINT(UI4::tlv)

	/* << USER CODE START >> */
	unsigned long rx, tx;
	IGD_CTRL *igd_ctrl = (IGD_CTRL *)(context->focus_ifp->devctrl);

	igd_osl_wan_max_bitrates(igd_ctrl->wan_devname, &rx, &tx);

	upnp_tlv_set(tlv, rx);
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: statevar_PhysicalLinkStatus() */
static int
statevar_PhysicalLinkStatus
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE * service,
	UPNP_TLV * tlv
)
{
	HINT(STR::tlv)

	HINT(char *pUp = "Up";)
	HINT(char *pDown = "Down";)
	HINT(char *pInitializing = "Initializing";)
	HINT(char *pUnavailable = "Unavailable";)

	/* << USER CODE START >> */
	IGD_CTRL *igd_ctrl = (IGD_CTRL *)(context->focus_ifp->devctrl);
	char *link_status = igd_ctrl->wan_up ? "Up" : "Down";

	upnp_tlv_set(tlv, (int)link_status);
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: statevar_EnabledForInternet() */
static int
statevar_EnabledForInternet
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE * service,
	UPNP_TLV * tlv
)
{
	HINT(BOOL::tlv)

	/* << USER CODE START >> */
	/*
	 * Always return TRUE ??
	 * Maybe we make VISTA happy, and we
	 * have to block the action from guest zone, too.
	 */
	upnp_tlv_set(tlv, 1);
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: statevar_MaximumActiveConnections() */
static int
statevar_MaximumActiveConnections
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE * service,
	UPNP_TLV * tlv
)
{
	HINT(UI2::tlv)

	/* << USER CODE START >> */
	upnp_tlv_set(tlv, 1);
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: statevar_NumberOfActiveConnections() */
static int
statevar_NumberOfActiveConnections
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE * service,
	UPNP_TLV * tlv
)
{
	HINT(UI2::tlv)

	/* << USER CODE START >> */
	upnp_tlv_set(tlv, 1);
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: statevar_TotalBytesSent() */
static int
statevar_TotalBytesSent
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE * service,
	UPNP_TLV * tlv
)
{
	HINT(UI4::tlv)

	/* << USER CODE START >> */
	igd_stats_t stats;
	IGD_CTRL *igd_ctrl = (IGD_CTRL *)(context->focus_ifp->devctrl);

	igd_osl_wan_ifstats(igd_ctrl, &stats);

	upnp_tlv_set(tlv, stats.tx_bytes);
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: statevar_TotalBytesReceived() */
static int
statevar_TotalBytesReceived
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE * service,
	UPNP_TLV * tlv
)
{
	HINT(UI4::tlv)

	/* << USER CODE START >> */
	igd_stats_t stats;
	IGD_CTRL *igd_ctrl = (IGD_CTRL *)(context->focus_ifp->devctrl);

	igd_osl_wan_ifstats(igd_ctrl, &stats);

	upnp_tlv_set(tlv, stats.rx_bytes);
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: statevar_TotalPacketsSent() */
static int
statevar_TotalPacketsSent
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE * service,
	UPNP_TLV * tlv
)
{
	HINT(UI4::tlv)

	/* << USER CODE START >> */
	igd_stats_t stats;
	IGD_CTRL *igd_ctrl = (IGD_CTRL *)(context->focus_ifp->devctrl);

	igd_osl_wan_ifstats(igd_ctrl, &stats);

	upnp_tlv_set(tlv, stats.tx_packets);
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: statevar_TotalPacketsReceived() */
static int
statevar_TotalPacketsReceived
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE * service,
	UPNP_TLV * tlv
)
{
	HINT(UI4::tlv)

	/* << USER CODE START >> */
	igd_stats_t stats;
	IGD_CTRL *igd_ctrl = (IGD_CTRL *)(context->focus_ifp->devctrl);

	igd_osl_wan_ifstats(igd_ctrl, &stats);

	upnp_tlv_set(tlv, stats.rx_packets);
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: statevar_ActiveConnectionDeviceContainer() */
static int
statevar_ActiveConnectionDeviceContainer
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE * service,
	UPNP_TLV * tlv
)
{
	HINT(STR::tlv)

	/* << USER CODE START >> */
	UPNP_SERVICE *wan_service = 0;
	UPNP_ADVERTISE *wan_advertise = 0;
	char *buf = upnp_buffer(context);
	
	wan_service = upnp_get_service_by_control_url(context, "/control?WANIPConnection");
	if (!wan_service)
		return SOAP_ACTION_FAILED;

	wan_advertise = upnp_get_advertise_by_name(context, wan_service->name);
	if (!wan_advertise)
		return SOAP_ACTION_FAILED;
    
	sprintf(buf, "uuid:%s:WANConnectionDevice:1", wan_advertise->uuid);

	/* Update tlv */
	upnp_tlv_set(tlv, (int)buf);
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: statevar_ActiveConnectionServiceID() */
static int
statevar_ActiveConnectionServiceID
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE * service,
	UPNP_TLV * tlv
)
{
	HINT(STR::tlv)

	/* << USER CODE START >> */
	UPNP_SERVICE *wan_service = 0;
	char *buf = upnp_buffer(context);
	
	wan_service = upnp_get_service_by_control_url(context, "/control?WANIPConnection");
	if (!wan_service)
		return SOAP_ACTION_FAILED;
	
	sprintf(buf, "urn:upnp-org:serviceId:%s", wan_service->service_id);
	
	/* Update tlv */
	upnp_tlv_set(tlv, (int)buf);
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: action_SetEnabledForInternet() */
static int
action_SetEnabledForInternet
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument
)
{
	HINT( BOOL::UPNP_TLV *in_NewEnabledForInternet = UPNP_IN_TLV("NewEnabledForInternet"); )
	
	/* << USER CODE START >> */
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: action_GetEnabledForInternet() */
static int
action_GetEnabledForInternet
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument
)
{
	HINT( BOOL::UPNP_TLV *out_NewEnabledForInternet = UPNP_OUT_TLV("NewEnabledForInternet"); )
	
	/* << USER CODE START >> */
	UPNP_TLV *out_NewEnabledForInternet = UPNP_OUT_TLV("NewEnabledForInternet");
	
	return statevar_EnabledForInternet(context, service, out_NewEnabledForInternet);
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: action_GetCommonLinkProperties() */
static int
action_GetCommonLinkProperties
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument
)
{
	HINT( STR::UPNP_TLV *out_NewWANAccessType = UPNP_OUT_TLV("NewWANAccessType"); )
	HINT( UI4::UPNP_TLV *out_NewLayer1UpstreamMaxBitRate = UPNP_OUT_TLV("NewLayer1UpstreamMaxBitRate"); )
	HINT( UI4::UPNP_TLV *out_NewLayer1DownstreamMaxBitRate = UPNP_OUT_TLV("NewLayer1DownstreamMaxBitRate"); )
	HINT( STR::UPNP_TLV *out_NewPhysicalLinkStatus = UPNP_OUT_TLV("NewPhysicalLinkStatus"); )

	/* << USER CODE START >> */
	UPNP_TLV *out_NewWANAccessType = UPNP_OUT_TLV("NewWANAccessType");
	UPNP_TLV *out_NewLayer1UpstreamMaxBitRate = UPNP_OUT_TLV("NewLayer1UpstreamMaxBitRate");
	UPNP_TLV *out_NewLayer1DownstreamMaxBitRate = UPNP_OUT_TLV("NewLayer1DownstreamMaxBitRate");
	UPNP_TLV *out_NewPhysicalLinkStatus = UPNP_OUT_TLV("NewPhysicalLinkStatus");

	IGD_CTRL *igd_ctrl = (IGD_CTRL *)(context->focus_ifp->devctrl);
	unsigned long rx, tx;
	char *link_status = igd_ctrl->wan_up ? "Up" : "Down";

	igd_osl_wan_max_bitrates(igd_ctrl->wan_devname, &rx, &tx);

	upnp_tlv_set(out_NewWANAccessType, (int)"Ethernet");
	upnp_tlv_set(out_NewLayer1UpstreamMaxBitRate, tx);
	upnp_tlv_set(out_NewLayer1DownstreamMaxBitRate, rx);
	upnp_tlv_set(out_NewPhysicalLinkStatus, (int)link_status);
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: action_GetMaximumActiveConnections() */
static int
action_GetMaximumActiveConnections
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument
)
{
	HINT( UI2:UPNP_TLV *out_NewMaximumActiveConnections = UPNP_OUT_TLV("NewMaximumActiveConnections"); )
	
	/* << USER CODE START >> */
	UPNP_TLV *out_NewMaximumActiveConnections = UPNP_OUT_TLV("NewMaximumActiveConnections");
	
	return statevar_MaximumActiveConnections(context, service, out_NewMaximumActiveConnections);
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: action_GetTotalBytesSent() */
static int
action_GetTotalBytesSent
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument
)
{
	HINT( UI4::UPNP_TLV *out_NewTotalBytesSent = UPNP_OUT_TLV("NewTotalBytesSent"); )

	/* << USER CODE START >> */
	UPNP_TLV *out_NewTotalBytesSent = UPNP_OUT_TLV("NewTotalBytesSent");
	
	return statevar_TotalBytesSent(context, service, out_NewTotalBytesSent);
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: action_GetTotalPacketsSent() */
static int
action_GetTotalPacketsSent
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument
)
{
	HINT( UI4::UPNP_TLV *out_NewTotalPacketsSent = UPNP_OUT_TLV("NewTotalPacketsSent"); )
	
	/* << USER CODE START >> */
	UPNP_TLV *out_NewTotalPacketsSent = UPNP_OUT_TLV("NewTotalPacketsSent");
	
	return statevar_TotalPacketsSent(context, service, out_NewTotalPacketsSent);
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: action_GetTotalBytesReceived() */
static int
action_GetTotalBytesReceived
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument
)
{
	HINT( UI4::UPNP_TLV *out_NewTotalBytesReceived = UPNP_OUT_TLV("NewTotalBytesReceived"); )
	
	/* << USER CODE START >> */
	UPNP_TLV *out_NewTotalBytesReceived = UPNP_OUT_TLV("NewTotalBytesReceived");
	
	return statevar_TotalBytesReceived(context, service, out_NewTotalBytesReceived);
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: action_GetTotalPacketsReceived() */
static int
action_GetTotalPacketsReceived
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument
)
{
	HINT( UI4::UPNP_TLV *out_NewTotalPacketsReceived = UPNP_OUT_TLV("NewTotalPacketsReceived"); )
	
	/* << USER CODE START >> */
	UPNP_TLV *out_NewTotalPacketsReceived = UPNP_OUT_TLV("NewTotalPacketsReceived");
	
	return statevar_TotalPacketsReceived(context, service, out_NewTotalPacketsReceived);
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: action_GetActiveConnections() */
static int
action_GetActiveConnections
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument
)
{
	HINT( UI2::UPNP_TLV *in_NewActiveConnectionIndex = UPNP_IN_TLV("NewActiveConnectionIndex"); )
	HINT( STR::UPNP_TLV *out_NewActiveConnDeviceContainer = UPNP_OUT_TLV("NewActiveConnDeviceContainer"); )
	HINT( STR::UPNP_TLV *out_NewActiveConnectionServiceID = UPNP_OUT_TLV("NewActiveConnectionServiceID"); )

	/* << USER CODE START >> */
	UPNP_TLV *out_NewActiveConnDeviceContainer = UPNP_OUT_TLV("NewActiveConnDeviceContainer");
	UPNP_TLV *out_NewActiveConnectionServiceID = UPNP_OUT_TLV("NewActiveConnectionServiceID");
	
	UPNP_SERVICE *wan_service = 0;
	UPNP_ADVERTISE *wan_advertise = 0;
	char *buf = upnp_buffer(context);
	
	wan_service = upnp_get_service_by_control_url(context, "/control?WANIPConnection");
	if (!wan_service)
		return SOAP_ACTION_FAILED;

	wan_advertise = upnp_get_advertise_by_name(context, wan_service->name);
	if (!wan_advertise)
		return SOAP_ACTION_FAILED;
    
	sprintf(buf, "uuid:%s:WANConnectionDevice:1", wan_advertise->uuid);
	upnp_tlv_set(out_NewActiveConnDeviceContainer, (int)buf);

	sprintf(buf, "urn:upnp-org:serviceId:%s", wan_service->service_id);
	upnp_tlv_set(out_NewActiveConnectionServiceID, (int)buf);
	return OK;
}
/* >> AUTO GENERATED FUNCTION */


/* << TABLE BEGIN */
/*
 * WARNNING: DON'T MODIFY THE FOLLOWING TABLES
 * AND DON'T REMOVE TAG :
 *          "<< TABLE BEGIN"
 *          ">> TABLE END"
 */

#define	STATEVAR_ACTIVECONNECTIONDEVICECONTAINER    0
#define	STATEVAR_ACTIVECONNECTIONSERVICEID          1
#define	STATEVAR_ENABLEDFORINTERNET                 2
#define	STATEVAR_LAYER1DOWNSTREAMMAXBITRATE         3
#define	STATEVAR_LAYER1UPSTREAMMAXBITRATE           4
#define	STATEVAR_MAXIMUMACTIVECONNECTIONS           5
#define	STATEVAR_NUMBEROFACTIVECONNECTIONS          6
#define	STATEVAR_PHYSICALLINKSTATUS                 7
#define	STATEVAR_TOTALBYTESRECEIVED                 8
#define	STATEVAR_TOTALBYTESSENT                     9
#define	STATEVAR_TOTALPACKETSRECEIVED               10
#define	STATEVAR_TOTALPACKETSSENT                   11
#define	STATEVAR_WANACCESSTYPE                      12

/* State Variable Table */
UPNP_STATE_VAR statevar_x_wancommoninterfaceconfig[] =
{
	{0, "ActiveConnectionDeviceContainer",    UPNP_TYPE_STR,   &statevar_ActiveConnectionDeviceContainer,    0},
	{0, "ActiveConnectionServiceID",          UPNP_TYPE_STR,   &statevar_ActiveConnectionServiceID,          0},
	{0, "EnabledForInternet",                 UPNP_TYPE_BOOL,  &statevar_EnabledForInternet,                 1},
	{0, "Layer1DownstreamMaxBitRate",         UPNP_TYPE_UI4,   &statevar_Layer1DownstreamMaxBitRate,         0},
	{0, "Layer1UpstreamMaxBitRate",           UPNP_TYPE_UI4,   &statevar_Layer1UpstreamMaxBitRate,           0},
	{0, "MaximumActiveConnections",           UPNP_TYPE_UI2,   &statevar_MaximumActiveConnections,           0},
	{0, "NumberOfActiveConnections",          UPNP_TYPE_UI2,   &statevar_NumberOfActiveConnections,          0},
	{0, "PhysicalLinkStatus",                 UPNP_TYPE_STR,   &statevar_PhysicalLinkStatus,                 1},
	{0, "TotalBytesReceived",                 UPNP_TYPE_UI4,   &statevar_TotalBytesReceived,                 0},
	{0, "TotalBytesSent",                     UPNP_TYPE_UI4,   &statevar_TotalBytesSent,                     0},
	{0, "TotalPacketsReceived",               UPNP_TYPE_UI4,   &statevar_TotalPacketsReceived,               0},
	{0, "TotalPacketsSent",                   UPNP_TYPE_UI4,   &statevar_TotalPacketsSent,                   0},
	{0, "WANAccessType",                      UPNP_TYPE_STR,   &statevar_WANAccessType,                      0},
	{0, 0,                                    0,               0,                                            0}
};

/* Action Table */
static ACTION_ARGUMENT arg_in_SetEnabledForInternet [] =
{
	{"NewEnabledForInternet",         UPNP_TYPE_BOOL,   STATEVAR_ENABLEDFORINTERNET}
};

static ACTION_ARGUMENT arg_out_GetEnabledForInternet [] =
{
	{"NewEnabledForInternet",         UPNP_TYPE_BOOL,   STATEVAR_ENABLEDFORINTERNET}
};

static ACTION_ARGUMENT arg_out_GetCommonLinkProperties [] =
{
	{"NewWANAccessType",              UPNP_TYPE_STR,    STATEVAR_WANACCESSTYPE},
	{"NewLayer1UpstreamMaxBitRate",   UPNP_TYPE_UI4,    STATEVAR_LAYER1UPSTREAMMAXBITRATE},
	{"NewLayer1DownstreamMaxBitRate", UPNP_TYPE_UI4,    STATEVAR_LAYER1DOWNSTREAMMAXBITRATE},
	{"NewPhysicalLinkStatus",         UPNP_TYPE_STR,    STATEVAR_PHYSICALLINKSTATUS}
};

static ACTION_ARGUMENT arg_out_GetMaximumActiveConnections [] =
{
	{"NewMaximumActiveConnections",   UPNP_TYPE_UI2,    STATEVAR_MAXIMUMACTIVECONNECTIONS}
};

static ACTION_ARGUMENT arg_out_GetTotalBytesSent [] =
{
	{"NewTotalBytesSent",             UPNP_TYPE_UI4,    STATEVAR_TOTALBYTESSENT}
};

static ACTION_ARGUMENT arg_out_GetTotalPacketsSent [] =
{
	{"NewTotalPacketsSent",           UPNP_TYPE_UI4,    STATEVAR_TOTALPACKETSSENT}
};

static ACTION_ARGUMENT arg_out_GetTotalBytesReceived [] =
{
	{"NewTotalBytesReceived",         UPNP_TYPE_UI4,   STATEVAR_TOTALBYTESRECEIVED}
};

static ACTION_ARGUMENT arg_out_GetTotalPacketsReceived [] =
{
	{"NewTotalPacketsReceived",       UPNP_TYPE_UI4,   STATEVAR_TOTALPACKETSRECEIVED}
};

static ACTION_ARGUMENT arg_in_GetActiveConnections [] =
{
	{"NewActiveConnectionIndex",      UPNP_TYPE_UI2,   STATEVAR_NUMBEROFACTIVECONNECTIONS}
};

static ACTION_ARGUMENT arg_out_GetActiveConnections [] =
{
	{"NewActiveConnDeviceContainer",  UPNP_TYPE_STR,   STATEVAR_ACTIVECONNECTIONDEVICECONTAINER},
	{"NewActiveConnectionServiceID",  UPNP_TYPE_STR,   STATEVAR_ACTIVECONNECTIONSERVICEID}
};

UPNP_ACTION action_x_wancommoninterfaceconfig[] =
{
	{"GetActiveConnections",        1,  arg_in_GetActiveConnections,    2,  arg_out_GetActiveConnections,           &action_GetActiveConnections},
	{"GetCommonLinkProperties",     0,  0,                              4,  arg_out_GetCommonLinkProperties,        &action_GetCommonLinkProperties},
	{"GetEnabledForInternet",       0,  0,                              1,  arg_out_GetEnabledForInternet,          &action_GetEnabledForInternet},
	{"GetMaximumActiveConnections", 0,  0,                              1,  arg_out_GetMaximumActiveConnections,    &action_GetMaximumActiveConnections},
	{"GetTotalBytesReceived",       0,  0,                              1,  arg_out_GetTotalBytesReceived,          &action_GetTotalBytesReceived},
	{"GetTotalBytesSent",           0,  0,                              1,  arg_out_GetTotalBytesSent,              &action_GetTotalBytesSent},
	{"GetTotalPacketsReceived",     0,  0,                              1,  arg_out_GetTotalPacketsReceived,        &action_GetTotalPacketsReceived},
	{"GetTotalPacketsSent",         0,  0,                              1,  arg_out_GetTotalPacketsSent,            &action_GetTotalPacketsSent},
	{"SetEnabledForInternet",       1,  arg_in_SetEnabledForInternet,   0,  0,                                      &action_SetEnabledForInternet},
	{0,                             0,  0,                              0,  0,                                      0}
};
/* >> TABLE END */
